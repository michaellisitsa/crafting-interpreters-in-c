#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "value.h"
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

#include "chunk.h"
#include "table.h"

typedef struct {
	ObjFunction *function;
	uint8_t *ip;
	Value *slots;
} CallFrame;

typedef struct {
	CallFrame frames[FRAMES_MAX];
	int frameCount;
	Chunk *chunk;
	// Points into a location in the chunk array
	uint8_t *ip;
	// This is the data structure where value stack is kept.
	// Python stores this in PyFrameObject->f_valuestack
	// https://stackoverflow.com/questions/44346433/in-c-python-accessing-the-bytecode-evaluation-stack
	// https://github.com/python/cpython/blob/2.7/Include/frameobject.h#L23
	// PyFrameObject https://shanechang.com/p/python-frames-systems-programming-connection/
	Value stack[STACK_MAX];
	// Pointer to the latest value in `stack` Value above.
	// Python stores in stack_pointer local variable
	// https://github.com/python/cpython/blob/v3.8.2/Python/ceval.c#L1153
	Value *stackTop;
	Table globals;
	Table strings;
	// Linked List head pointer for garbage collector to mark and sweep all dynamically allocated
	// https://craftinginterpreters.com/strings.html#freeing-objects.
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
