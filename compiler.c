#include "compiler.h"
#include "chunk.h"
#include "common.h"
#include "scanner.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	Token current;
	Token previous;
	bool hadError;
	bool panicMode;
} Parser;

Parser parser;
Chunk *compilingChunk;

static Chunk *currentChunk() { return compilingChunk; }

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
static void emitReturn() {
	// Why a separate function for just return?
	emitByte(OP_RETURN);
}

static void endCompiler() { emitReturn(); }
static void expression() {
	// something will go here
}
bool compile(const char *source, Chunk *chunk) {
	initScanner(source);
	// I guess this is useful for future when we compile multiple chunks / functions
	compilingChunk = chunk;
	parser.hadError = false;
	parser.panicMode = false;
	advance();
	expression();
	consume(TOKEN_EOF, "Expect end of expression.");
	endCompiler();
	return !parser.hadError;
};
