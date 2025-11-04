#include "compiler.h"
#include "chunk.h"
#include "common.h"
#include "object.h"
#include "scanner.h"
#include "value.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
	Token current;
	Token previous;
	bool hadError;
	bool panicMode;
} Parser;
typedef enum {
	PREC_NONE,
	PREC_ASSIGNMENT, // =
	PREC_OR,		 // or
	PREC_AND,		 // and
	PREC_EQUALITY,	 // == !=
	PREC_COMPARISON, // < > <= >=
	PREC_TERM,		 // + -
	PREC_FACTOR,	 // * /
	PREC_UNARY,		 // ! -
	PREC_CALL,		 // . ()
	PREC_PRIMARY
} Precedence;
typedef void (*ParseFn)(bool canAssign);
typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;
typedef struct {
	Token name;
	int depth;
} Local;

typedef enum { TYPE_FUNCTION, TYPE_SCRIPT } FunctionType;

typedef struct Compiler {
	struct Compiler *enclosing;
	ObjFunction *function;
	FunctionType type;
	Local locals[UINT8_COUNT];
	int localCount;
	int scopeDepth;
} Compiler;

Parser parser;
Compiler *current = NULL;

static Chunk *currentChunk() { return &current->function->chunk; }

// This just prints the error and set the panic mode
static void errorAt(Token *token, const char *message) {
	if (parser.panicMode)
		return;
	parser.panicMode = true;
	fprintf(stderr, "[line %d] Error", token->line);
	if (token->type == TOKEN_EOF) {
		fprintf(stderr, " at end");
	} else if (token->type == TOKEN_ERROR) {
		// Nothing
	} else {
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}
	fprintf(stderr, ": %s\n", message);
	parser.hadError = true;
}

static void error(const char *message) { errorAt(&parser.previous, message); }

static void errorAtCurrent(const char *message) { errorAt(&parser.current, message); }

static void advance() {
	parser.previous = parser.current;
	for (;;) {
		parser.current = scanToken();
		if (parser.current.type != TOKEN_ERROR)
			break;
		errorAtCurrent(parser.current.start);
	}
}

// We're consuming the token from scanner
// and then if its an error we set the error message
static void consume(TokenType type, const char *message) {
	if (parser.current.type == type) {
		advance();
		return;
	}
	errorAtCurrent(message);
}

static bool check(TokenType type) { return parser.current.type == type; }

static bool match(TokenType type) {
	// We want to check if the given token matches by type
	if (!check(type))
		return false;
	advance();
	return true;
}

static void emitByte(uint8_t byte) {
	// Basically instead of hand compiling, we use the currentChunk() to point to which we want to
	// write
	writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
	// Basically instead of hand compiling, we use the currentChunk() to point to which we want to
	// write
	emitByte(byte1);
	emitByte(byte2);
}

static void emitLoop(int loopStart) {
	// Very similar to emitJump and patchJump together. This is likely becuase
	// we're jumping backwards so we use normal C variables to track the the loop start
	// and by the time this op code comes around, we can write it and its operand at once without
	// patching.
	emitByte(OP_LOOP);

	int offset = currentChunk()->count - loopStart + 2;
	if (offset > UINT16_MAX)
		error("Loop body too large.");
	emitByte((offset >> 8) & 0xff);
	emitByte(offset & 0xff);
}

static int emitJump(uint8_t instruction) {
	emitByte(instruction);
	// Jump offset operand. We will write this after compiling the statement or block
	emitByte(0xff);
	emitByte(0xff);
	return currentChunk()->count - 2;
}

static void emitReturn() {
	// Why a separate function for just return?
	emitByte(OP_RETURN);
}
static uint8_t makeConstant(Value value) {
	int constant = addConstant(currentChunk(), value);
	if (constant > UINT8_MAX) {
		error("Too many constants in one chunk");
		return 0;
	}
	return (uint8_t)constant;
}

static void emitConstant(Value value) { emitBytes(OP_CONSTANT, makeConstant(value)); }
static void patchJump(int offset) {
	// The offset will be from before compiling the statement.
	int jump = currentChunk()->count - offset - 2;
	if (jump > UINT16_MAX) {
		error("Too much code to jump over.");
	}
	// We now want to write the position to jump to into that memory address.
	// We have two bytes that are the high-order and low-order bytes of a two-byte
	// (16-bit) unsigned value. We need to construct that number
	// The and will apply a mask with all 1s, and extract the last 8 bits
	//   0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1
	// & 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1
	//   -------------------------------
	// = 0 0 0 0 0 0 0 0 0 1 0 1 0 1 0 1
	currentChunk()->code[offset] = (jump >> 8) & 0xff;
	currentChunk()->code[offset + 1] = jump & 0xff;
}
static void initCompiler(Compiler *compiler, FunctionType type) {
	compiler->enclosing = current;
	compiler->function = NULL;
	compiler->type = type;
	compiler->localCount = 0;
	compiler->scopeDepth = 0;
	compiler->function = newFunction();
	current = compiler;
	if (type != TYPE_SCRIPT) {
		current->function->name = copyString(parser.previous.start, parser.previous.length);
	}
	Local *local = &current->locals[current->localCount++];
	local->depth = 0;
	local->name.start = "";
	local->name.length = 0;
}
static ObjFunction *endCompiler() {
	emitReturn();
	ObjFunction *function = current->function;

#ifdef DEBUG_PRINT_CODE
	if (!parser.hadError) {
		disassembleChunk(currentChunk(),
						 function->name != NULL ? function->name->chars : "<script>");
	}
#endif
	current = current->enclosing;
	return function;
}

static void beginScope() { current->scopeDepth++; }

static void endScope() {
	current->scopeDepth--;
	// We want to remove all local positions we were storing
	// Sinced the compiler is mirroring the vm locals indices,
	// it now needs to go back to the outer scope and only keep the variables still in scope
	while (current->localCount > 0 &&
		   current->locals[current->localCount - 1].depth > current->scopeDepth) {
		emitByte(OP_POP);
		current->localCount--;
	}
}
// Global variable forward declaration
// The order of function declarations means some functions have to be forward declared
static void expression();
static void statement();
static void declaration();

static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void expression() { parsePrecedence(PREC_ASSIGNMENT); }
static void block() {
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
		declaration();
	}
	consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}
static uint8_t identifierConstant(Token *name) {
	return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}
static bool identifiersEqual(Token *a, Token *b) {
	if (a->length != b->length)
		return false;
	return memcmp(a->start, b->start, a->length) == 0;
}
static int resolveLocal(Compiler *compiler, Token *name) {
	for (int i = compiler->localCount - 1; i >= 0; i--) {
		// &compiler->locals would give an array pointer
		// a special pointer type that points at the array as a whole
		// but without '&' it decays to a pointer to the first element
		// https://stackoverflow.com/a/44524152
		// We want to point into a specific location in the locals array
		Local *local = &compiler->locals[i];
		if (identifiersEqual(&local->name, name)) {
			if (local->depth == -1) {
				error("Can't read local variable in its own initializer");
			}
			return i;
		}
	}
	return -1;
}
// We get the next value from the locals array and fill it in.
static void addLocal(Token name) {
	if (current->localCount == UINT8_COUNT) {
		error("Too many local variables in function.");
		return;
	}
	Local *local = &current->locals[current->localCount++];
	local->name = name;
	local->depth = -1;
}

static void declareVariable() {
	if (current->scopeDepth == 0)
		return;

	Token *name = &parser.previous;
	// We don't want 2 variables declared in the same scope
	// Different scopes is fine, as that's shadowing
	for (int i = current->localCount - 1; i >= 0; i--) {
		Local *local = &current->locals[i];
		if (local->depth != -1 && local->depth < current->scopeDepth) {
			break;
		}

		if (identifiersEqual(name, &local->name)) {
			error("Already a variable with this name in this scope.");
		}
	}
	addLocal(*name);
}
static uint8_t parseVariable(const char *errorMessage) {
	consume(TOKEN_IDENTIFIER, errorMessage);
	declareVariable();
	// We are not interested in local variables here
	// Locals are looked up by index rather than name at runtime
	if (current->scopeDepth > 0)
		return 0;
	// Global variable names will get written to the constants table here
	// The value of the global won't be written as they are lazily evaluated.
	// It then returns the index of that constant in the constant table.
	return identifierConstant(&parser.previous);
}

static void markInitialized() {
	// In global scope, there is no local variable to mark initialized
	if (current->scopeDepth == 0)
		return;
	// We want to set the local depth to the current depth
	// Local should live at the localsCount index of the array - 1
	current->locals[current->localCount - 1].depth = current->scopeDepth;
}
static void defineVariable(uint8_t global) {
	// In the VM, the stack top will have the value on the RHS of the assignment
	// So we don't need to do anything as the variable will be right where it needs to be
	// The compiler will have pointed the bytecode here
	if (current->scopeDepth > 0) {
		// We mark the variable as initialized which sets the scope depth to the current
		// which we previously did in addLocal which leads to problems with referencing a variable
		// in its declaration
		markInitialized();
		return;
	}
	emitBytes(OP_DEFINE_GLOBAL, global);
}

static void function(FunctionType type) {
	Compiler compiler;
	initCompiler(&compiler, type);
	beginScope();

	consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			current->function->arity++;
			if (current->function->arity > 255) {
				errorAtCurrent("Can't have more than 255 parameters.");
			}
			uint8_t constant = parseVariable("Expect parameter name");
			defineVariable(constant);
		} while (match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
	consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
	block();

	ObjFunction *function = endCompiler();
	emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));
}

static void funDeclaration() {
	uint8_t global = parseVariable("Expect function name.");
	markInitialized();
	function(TYPE_FUNCTION);
	defineVariable(global);
}
static void and_(bool canAssign) {
	// Logical operators are just if else statements
	//
	int endJump = emitJump(OP_JUMP_IF_FALSE);
	emitByte(OP_POP);
	parsePrecedence(PREC_AND);
	patchJump(endJump);
}

static void varDeclaration() {
	uint8_t global = parseVariable("Expect variable name.");

	if (match(TOKEN_EQUAL)) {
		expression();
	} else {
		emitByte(OP_NIL);
	}
	consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
	defineVariable(global);
}

static void expressionStatement() {
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
	emitByte(OP_POP);
}

static void forStatement() {
	beginScope();
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
	if (match(TOKEN_SEMICOLON)) {
		// for (;  ...)
		// No initializer.
	} else if (match(TOKEN_VAR)) {
		// for (THIS statement; ...)
		varDeclaration();
	} else {
		expressionStatement();
	}

	int loopStart = currentChunk()->count;
	int exitJump = -1;
	if (!match(TOKEN_SEMICOLON)) {
		expression();
		consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

		// Jump out of the loop if the condition is false.
		exitJump = emitJump(OP_JUMP_IF_FALSE);
		emitByte(OP_POP); // Condition.
	}

	// Increment clause
	// We jump back to the increment after the loop to perform the action
	// We advance past the ')', if we match it, we also emit the bytecode for the expression
	if (!match(TOKEN_RIGHT_PAREN)) {
		// We have to jump over the increment expression unconditionally.
		// It is executed after the body only
		// We therefore need to record where the jump should go, i.e. after the expression()
		// I guess at the end of the loop, we have to jump back to this point to actually call this.
		int bodyJump = emitJump(OP_JUMP);
		int incrementStart = currentChunk()->count;
		expression();
		emitByte(OP_POP);
		consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

		// Unconditionally jump backwards to the start of the loop,
		// To evaluate the condition.
		emitLoop(loopStart);
		loopStart = incrementStart;
		patchJump(bodyJump);
	}
	statement();
	emitLoop(loopStart);

	// If there's a condition clause, we patch the jump.
	if (exitJump != -1) {
		patchJump(exitJump);
		emitByte(OP_POP); // Condition.
	}
	endScope();
}

static void ifStatement() {
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
	int thenJump = emitJump(OP_JUMP_IF_FALSE);
	// The net stack effect should be zero
	// and the condition expression won't be used anymore even though its returned a value.
	emitByte(OP_POP);
	statement();
	// Now that we know how far we've progressed,
	// we can write the location back to the thenJump bytecode argument.
	int elseJump = emitJump(OP_JUMP);
	patchJump(thenJump);
	emitByte(OP_POP);
	if (match(TOKEN_ELSE))
		statement();
	patchJump(elseJump);
}

static void printStatement() {
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emitByte(OP_PRINT);
}

static void whileStatement() {
	int loopStart = currentChunk()->count;
	// Same as if statement
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	int exitJump = emitJump(OP_JUMP_IF_FALSE);
	emitByte(OP_POP);
	statement();
	// This instruction needs to know how far back to fly
	emitLoop(loopStart);
	// We know this is the end of the loop now.
	patchJump(exitJump);
	emitByte(OP_POP);
}

static void synchronize() {
	parser.panicMode = false;
	while (parser.current.type != TOKEN_EOF) {
		if (parser.previous.type == TOKEN_SEMICOLON)
			return;
		switch (parser.current.type) {
		case TOKEN_CLASS:
		case TOKEN_FUN:
		case TOKEN_VAR:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_WHILE:
		case TOKEN_PRINT:
		case TOKEN_RETURN:
			return;

		default:; // Do nothing.
		}

		advance();
	}
}
static void declaration() {
	if (match(TOKEN_FUN)) {
		funDeclaration();
	} else if (match(TOKEN_VAR)) {
		varDeclaration();
	} else {
		statement();
	}
	if (parser.panicMode)
		synchronize();
}

static void statement() {
	if (match(TOKEN_PRINT)) {
		printStatement();
	} else if (match(TOKEN_FOR)) {
		forStatement();
	} else if (match(TOKEN_IF)) {
		ifStatement();
	} else if (match(TOKEN_WHILE)) {
		whileStatement();
	} else if (match(TOKEN_LEFT_BRACE)) {
		beginScope();
		block();
		endScope();
	} else {
		expressionStatement();
	}
}

static void binary(bool canAssign) {
	TokenType operatorType = parser.previous.type;
	ParseRule *rule = getRule(operatorType);
	parsePrecedence((Precedence)(rule->precedence + 1));
	switch (operatorType) {
	case TOKEN_BANG_EQUAL:
		emitBytes(OP_EQUAL, OP_NOT);
		break;
	case TOKEN_EQUAL_EQUAL:
		emitByte(OP_EQUAL);
		break;
	case TOKEN_GREATER:
		emitByte(OP_GREATER);
		break;
	case TOKEN_GREATER_EQUAL:
		emitBytes(OP_LESS, OP_NOT);
		break;
	case TOKEN_LESS:
		emitByte(OP_LESS);
		break;
	case TOKEN_LESS_EQUAL:
		emitBytes(OP_GREATER, OP_NOT);
		break;
	case TOKEN_PLUS:
		emitByte(OP_ADD);
		break;
	case TOKEN_MINUS:
		emitByte(OP_SUBTRACT);
		break;
	case TOKEN_STAR:
		emitByte(OP_MULTIPLY);
		break;
	case TOKEN_SLASH:
		emitByte(OP_DIVIDE);
		break;
	default:
		return; // Unreachable.
	}
}
static void literal(bool canAssign) {
	switch (parser.previous.type) {
	case TOKEN_FALSE:
		emitByte(OP_FALSE);
		break;
	case TOKEN_NIL:
		emitByte(OP_NIL);
		break;
	case TOKEN_TRUE:
		emitByte(OP_TRUE);
		break;
	default:
		return; // Unreachable.
	}
}
static void grouping(bool canAssign) {
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}
static void number(bool canAssign) {
	double value = strtod(parser.previous.start, NULL);
	emitConstant(NUMBER_VAL(value));
}

static void or_(bool canAssign) {
	int elseJump = emitJump(OP_JUMP_IF_FALSE);
	int endJump = emitJump(OP_JUMP);
	// Before we parse the expression, we want to
	patchJump(elseJump);
	emitByte(OP_POP);
	// Instead of parsing the parentheses and a whole expression,
	// we can just proceed and parse everything with a lower precedence to OR
	// including the infix expression on both sides.
	parsePrecedence(PREC_OR);
	// Then we know we're at the end of the logical condition expression
	patchJump(endJump);
}

static void string(bool canAssign) {
	emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}
static void namedVariable(Token name, bool canAssign) {
	uint8_t getOp, setOp;
	int arg = resolveLocal(current, &name);
	if (arg != -1) {
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
	} else {
		arg = identifierConstant(&name);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}
	if (canAssign && match(TOKEN_EQUAL)) {
		expression();
		emitBytes(setOp, arg);
	} else {
		emitBytes(getOp, arg);
	}
}
static void variable(bool canAssign) { namedVariable(parser.previous, canAssign); }

static void unary(bool canAssign) {
	TokenType operatorType = parser.previous.type;
	parsePrecedence(PREC_UNARY);
	switch (operatorType) {
	case TOKEN_MINUS:
		emitByte(OP_NEGATE);
		break;
	default:
		return;
	}
}
ParseRule rules[] = {
	[TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
	[TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
	[TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
	[TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
	[TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
	[TOKEN_DOT] = {NULL, NULL, PREC_NONE},
	[TOKEN_MINUS] = {unary, binary, PREC_TERM},
	[TOKEN_PLUS] = {NULL, binary, PREC_TERM},
	[TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
	[TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
	[TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
	[TOKEN_BANG] = {unary, NULL, PREC_NONE},
	[TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
	[TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
	[TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
	[TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
	[TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
	[TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
	[TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
	[TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
	[TOKEN_STRING] = {string, NULL, PREC_NONE},
	[TOKEN_NUMBER] = {number, NULL, PREC_NONE},
	[TOKEN_AND] = {NULL, and_, PREC_AND},
	[TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
	[TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
	[TOKEN_FALSE] = {literal, NULL, PREC_NONE},
	[TOKEN_FOR] = {NULL, NULL, PREC_NONE},
	[TOKEN_FUN] = {NULL, NULL, PREC_NONE},
	[TOKEN_IF] = {NULL, NULL, PREC_NONE},
	[TOKEN_NIL] = {literal, NULL, PREC_NONE},
	[TOKEN_OR] = {NULL, or_, PREC_OR},
	[TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
	[TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
	[TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
	[TOKEN_THIS] = {NULL, NULL, PREC_NONE},
	[TOKEN_TRUE] = {literal, NULL, PREC_NONE},
	[TOKEN_VAR] = {NULL, NULL, PREC_NONE},
	[TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
	[TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
	[TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

// This is a recursive function
// When it calls the prefix rule function, that function
// e.g. unary, binary may call this again
// There is a diagram about how this works on
// https://craftinginterpreters.com/compiling-expressions.html#parsing-with-precedence
static void parsePrecedence(Precedence precedence) {
	advance();
	// We get the rule for the token we're on,
	// i.e. what its ParseFn for infix, prefix and infix precedence
	// For Now we just need the prefix rule
	ParseFn prefixRule = getRule(parser.previous.type)->prefix;
	if (prefixRule == NULL) {
		error("Expect expression.");
		return;
	}
	bool canAssign = precedence <= PREC_ASSIGNMENT;
	prefixRule(canAssign);

	// Here we're going to call the infix function so it can generate some bytecode recursively
	// inside 'expression'
	Precedence precendenceCompare = getRule(parser.current.type)->precedence;
	while (precedence <= getRule(parser.current.type)->precedence) {
		advance();
		ParseFn infixRule = getRule(parser.previous.type)->infix;
		infixRule(canAssign);
	}
	// What token is parser.previous. If = has not been consumed, this is an error
	// This means that we have reached to end of the LHS of the declaration, and nothing has
	// consumed it
	if (canAssign && match(TOKEN_EQUAL)) {
		error("Invalid assignment target.");
	}
}

static ParseRule *getRule(TokenType type) { return &rules[type]; }

ObjFunction *compile(const char *source) {
	initScanner(source);
	Compiler compiler;
	initCompiler(&compiler, TYPE_SCRIPT);
	parser.hadError = false;
	parser.panicMode = false;
	advance();
	while (!match(TOKEN_EOF)) {
		declaration();
	}
	ObjFunction *function = endCompiler();
	return parser.hadError ? NULL : function;
};
