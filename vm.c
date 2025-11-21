#include "vm.h"
#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

VM vm;

static void resetStack() {
	// since stack is a pointer this will be its zeroth value
	vm.stackTop = vm.stack;
	vm.frameCount = 0;
}

static void runtimeError(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);
	CallFrame *frame = &vm.frames[vm.frameCount - 1];
	size_t instruction = frame->ip - frame->function->chunk.code - 1;
	int line = frame->function->chunk.lines[instruction];
	fprintf(stderr, "[line %d] in script\n", line);
	resetStack();
}
void initVM() {
	resetStack();
	vm.objects = NULL;
	// We pass a pointer to the vm strings table,
	initTable(&vm.globals);
	initTable(&vm.strings);
}

void freeVM() {
	freeTable(&vm.strings);
	freeTable(&vm.globals);
	freeObjects();
}

void push(Value value) {
	*vm.stackTop = value;
	vm.stackTop++;
}

Value pop() {
	vm.stackTop--;
	return *vm.stackTop;
}
static Value peek(int distance) { return vm.stackTop[-1 - distance]; }
static bool isFalsey(Value value) { return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value)); }

static void concatenate() {
	// We want to get the 2 strings off the value stack.
	// Then we allocate a new piece of memory with the length of both strings
	ObjString *b = AS_STRING(pop());
	ObjString *a = AS_STRING(pop());
	int length = a->length + b->length;
	char *chars = ALLOCATE(char, length + 1);
	memcpy(chars, a->chars, a->length);
	memcpy(chars + a->length, b->chars, b->length);
	chars[length] = '\0';

	ObjString *result = takeString(chars, length);
	push(OBJ_VAL(result));
}

InterpretResult interpret(const char *source) {
	ObjFunction *function = compile(source);
	if (function == NULL)
		return INTERPRET_RUNTIME_ERROR;
	push(OBJ_VAL(function));
	CallFrame *frame = &vm.frames[vm.frameCount++];
	frame->function = function;
	frame->ip = function->chunk.code;
	frame->slots = vm.stack;

	return run();
}

static InterpretResult run() {
	// Get the current frame
	CallFrame *frame = &vm.frames[vm.frameCount - 1];
#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op)                                                                   \
	do {                                                                                           \
		if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                                          \
			runtimeError("Operands must be numbers.");                                             \
			return INTERPRET_RUNTIME_ERROR;                                                        \
		}                                                                                          \
		double b = AS_NUMBER(pop());                                                               \
		double a = AS_NUMBER(pop());                                                               \
		push(valueType(a op b));                                                                   \
	} while (false)
	for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
		// Get visibility into the value stack
		for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
			printf("[ ");
			printValue(*slot);
			printf(" ]");
		}
		printf("\n");
		// Since disassemble expects an offset, and vm.ip is a pointer into the code
		// array directly
		disassembleInstruction(&frame->function->chunk,
							   (int)(frame->ip - frame->function->chunk.code));
#endif
		uint8_t instruction;
		switch (instruction = READ_BYTE()) {

		case OP_CONSTANT: {
			Value constant = READ_CONSTANT();
			push(constant);
			break;
		}
		case OP_NIL:
			push(NIL_VAL);
			break;
		case OP_TRUE:
			push(BOOL_VAL(true));
			break;
		case OP_FALSE:
			push(BOOL_VAL(false));
			break;
		case OP_POP:
			pop();
			break;
		case OP_GET_LOCAL: {
			uint8_t slot = READ_BYTE();
			push(frame->slots[slot]);
			break;
		}
		case OP_SET_LOCAL: {
			uint8_t slot = READ_BYTE();
			// We peek at the current value on the value stack, and set the value in the
			// local stack slot to that value
			frame->slots[slot] = peek(0);
			break;
		}
		case OP_GET_GLOBAL: {
			ObjString *name = READ_STRING();
			Value value;
			if (!tableGet(&vm.globals, name, &value)) {
				runtimeError("Undefined variable: '%s'.", name->chars);
				return INTERPRET_RUNTIME_ERROR;
			}
			push(value);
			break;
		}
		case OP_DEFINE_GLOBAL: {
			ObjString *name = READ_STRING();
			tableSet(&vm.globals, name, peek(0));
			pop();
			break;
		}
		case OP_SET_GLOBAL: {
			ObjString *name = READ_STRING();
			if (tableSet(&vm.globals, name, peek(0))) {
				// It must already exist if its being set.
				tableDelete(&vm.globals, name);
				runtimeError("Undefined variable '%s'.", name->chars);
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_EQUAL: {
			// We need to do get the last 2 values from the stack
			// And then we compare them
			// The values must be of any type?
			Value b = pop();
			Value a = pop();
			push(BOOL_VAL(valuesEqual(a, b)));
			break;
		}
		case OP_GREATER:
			// We don't pass in a value so we initialize an empty Value
			BINARY_OP(BOOL_VAL, >);
			break;
		case OP_LESS:
			BINARY_OP(BOOL_VAL, <);
			break;
		case OP_ADD:
			if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
				concatenate();
			} else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
				double b = AS_NUMBER(pop());
				double a = AS_NUMBER(pop());
				push(NUMBER_VAL(a + b));
			} else {
				runtimeError("Operands must be two numbers or two strings.");
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		case OP_SUBTRACT:
			BINARY_OP(NUMBER_VAL, -);
			break;
		case OP_MULTIPLY:
			BINARY_OP(NUMBER_VAL, *);
			break;
		case OP_DIVIDE:
			BINARY_OP(NUMBER_VAL, /);
			break;
		case OP_NOT:
			push(BOOL_VAL(isFalsey(pop())));
			break;
		case OP_NEGATE:
			if (!IS_NUMBER(peek(0))) {
				runtimeError("Operand must be a number");
				return INTERPRET_RUNTIME_ERROR;
			}

			// The expansion of these macros equals
			// push(
			// 	// Create a copy of the original Value but negate the value inside
			//     (Value){VAL_NUMBER, {.number = -pop().as.number}}
			// )
			push(NUMBER_VAL(-AS_NUMBER(pop())));
			break;
		case OP_PRINT: {
			printValue(pop());
			printf("\n");
			break;
		}
		case OP_JUMP: {
			uint16_t offset = READ_SHORT();
			// We don't need to check the condition, likely
			// if then condition is false, it will jump right past this instruction
			// like a game of snakes and ladders where you've landed one past the ladder.
			frame->ip += offset;
			break;
		}
		case OP_JUMP_IF_FALSE: {
			// We will jump to the location only if the result is falsey on the top of the value
			// stack
			uint16_t offset = READ_SHORT();
			if (isFalsey(peek(0)))
				frame->ip += offset;
			break;
		}
		case OP_LOOP: {
			uint16_t offset = READ_SHORT();
			frame->ip -= offset;
			break;
		}
		case OP_CALL: {
			int count = READ_BYTE();
			// TODO: We need to check if this matches the function arity
			CallFrame *newFrame = &vm.frames[vm.frameCount++];
			// The frame's instruction pointer points into the
			// bytecode chunks which is different.
			Value functionPointer = peek(count);
			if (IS_OBJ(functionPointer)) {
				switch (OBJ_TYPE(functionPointer)) {
				case OBJ_FUNCTION: {
					ObjFunction *function = AS_FUNCTION(functionPointer);
					// We reach this instruction and we know the stack has the
					// arguments before it. We need to create a new stack frame and enter the new
					// function. This stack
					newFrame->ip = function->chunk.code;
					newFrame->slots = vm.stackTop - count - 1;
					// I guess I need to extract the function to save to the call frame.
					newFrame->function = function;

					// The moment of truth, my new frame is ready and filled. Now we can activate it
					// by pointing frame to it.
					frame = &vm.frames[vm.frameCount - 1];
					printf("incremented the frame");
				}
				default:
					break;
				}
			}
			break;
		}
		case OP_RETURN: {
			return INTERPRET_OK;
		}
		}
	}

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}
