#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

// Op codes we will use
typedef enum {
  OP_RETURN,
  LOAD_CONST,
} OpCode;

// Wrapper around an array of bytes
// We need dynamic arrays
typedef struct {
  int count;
  int capacity;
  uint8_t *code;
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

void writeChunk(Chunk *chunk, uint8_t byte);
int addConstant(Chunk *chunk, double constant);
#endif
