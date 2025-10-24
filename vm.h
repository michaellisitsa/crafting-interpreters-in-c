#ifndef clox_vm_h
#define clox_vm_h

#include "value.h"
#define STACK_MAX 256

#include "chunk.h"
#include "table.h"

typedef struct {
	Chunk *chunk;
	// pointer because it points into a location in the chunk array
	uint8_t *ip;
	// Interesting stackoverflow about Python's approach to store this
	// https://stackoverflow.com/questions/44346433/in-c-python-accessing-the-bytecode-evaluation-stack
	// Python stores this in PyFrameObject->f_valuestack
	// https://github.com/python/cpython/blob/2.7/Include/frameobject.h#L23
	// Also excellent explanation of the PyFrameObject
	// https://shanechang.com/p/python-frames-systems-programming-connection/
	Value stack[STACK_MAX];
	// Python stores this in stack_pointer local variable
	//  Note this is a pointer to the top of the stack
	// https://github.com/python/cpython/blob/v3.8.2/Python/ceval.c#L1153
	Value *stackTop;
	Table strings;
	// This will allow us to see the head of the list
	// so we can navigate along all objects. We need a global variable to store the head to start
	// navigating through.
	Obj *objects;
} VM;

typedef enum { INTERPRET_OK, INTERPRET_COMPILE_ERROR, INTERPRET_RUNTIME_ERROR } InterpretResult;

// We want to tell translation units that import this header that the vm exists.
// Since this isn't a struct or a function, its a variable, we can use extern
extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char *source);
void push(Value value);
Value pop();
static InterpretResult run();

#endif
