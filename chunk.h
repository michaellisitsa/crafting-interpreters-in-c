#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"

// Op codes we will use
typedef enum {
  OP_RETURN,
} OpCode;

// Wrapper around an array of bytes
// We need dynamic arrays
typedef struct {
  int count;
  int capacity;
  uint8_t *code;
} Chunk;

// This will be a function to construct a chunk
// C doesn't have constructors
void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);

void writeChunk(Chunk *chunk, uint8_t byte);

#endif
