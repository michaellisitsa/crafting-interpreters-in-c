#include "chunk.h"
#include "common.h"
#include "debug.h"
#include <stdio.h>

int main(int argc, const char *argv[]) {
  Chunk chunk;
  initChunk(&chunk);

  int constant = addConstant(&chunk, 1.2);
  writeChunk(&chunk, OP_CONSTANT);
  writeChunk(&chunk, constant);

  writeChunk(&chunk, OP_RETURN);
  disassembleChunk(&chunk, "Test");
  freeChunk(&chunk);
  return 0;
}
