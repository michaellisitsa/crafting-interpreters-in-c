#include "chunk.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>

void initChunk(Chunk *chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  // Start off completely empty
  chunk->code = NULL;
}

void writeChunk(Chunk *chunk, uint8_t byte) {
  // Does the array have capacity for this byte;
  if (chunk->capacity < chunk->count + 1) {
    // Does this copy the capacity?
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code =
        GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
  }
  // If not grow the array to make room
  chunk->code[chunk->count] = byte;
  chunk->count++;
}

void freeChunk(Chunk *chunk) {
  // I guess we're freeing the entire memory. Why does it care to know the whole
  // capacity?
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  initChunk(chunk);
  // Use the same reallocate function with a different macro
}
