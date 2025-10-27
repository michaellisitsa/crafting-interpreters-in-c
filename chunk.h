#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

// Op codes we will use
typedef enum {
	OP_CONSTANT,
	OP_NIL,
	OP_TRUE,
	OP_FALSE,
	OP_POP,
	OP_GET_GLOBAL,
	OP_DEFINE_GLOBAL,
	OP_SET_GLOBAL,
	OP_EQUAL,
	OP_GREATER,
	OP_LESS,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_NOT,
	OP_NEGATE,
	OP_PRINT,
	OP_RETURN,
} OpCode;

// Wrapper around an array of bytes
// We need dynamic arrays
typedef struct {
	int count;
	int capacity;
	uint8_t *code;
	int *lines;
	// This is a sub dynamic array inside this main one.
	// Here we can create arbitrarily nested data structures.
	// We store all the constants we need.
	// Note that ValueArray is a struct, not a pointer to a struct.
	ValueArray constants;
} Chunk;

// This will be a function to construct a chunk
// C doesn't have constructors
void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);

void writeChunk(Chunk *chunk, uint8_t byte, int line);
// We define a shortcut for double in value, that we use here.
// Later the type of a constant will be exp[andedfhhh
int addConstant(Chunk *chunk, Value value);
#endif
