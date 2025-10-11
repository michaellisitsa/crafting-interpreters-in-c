#include "chunk.h"
#include "common.h"
#include "debug.h"
#include <stdio.h>

int main(int argc, const char *argv[]) {
  Chunk chunk;
  initChunk(&chunk);
  writeChunk(&chunk, OP_RETURN);
  writeChunk(&chunk, LOAD_CONST);
  // We want to write the constant directly after the opcode.
  // But we don't want to write the double as its size is bigger than a byte,
  // and we could be writing all sorts of things of different sizes
  // Let's first write a function to store the value and return its position in
  // the ValueArray, so we can pull it out later.
  int constant = addConstant(&chunk, 1.2);
  writeChunk(&chunk, constant);
  disassembleChunk(&chunk, "Test");
  freeChunk(&chunk);
  return 0;
}
