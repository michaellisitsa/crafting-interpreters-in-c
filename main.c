#include "chunk.h"
#include "common.h"
#include "debug.h"
#include <stdio.h>

int main(int argc, const char *argv[]) {
  Chunk chunk;
  initChunk(&chunk);
  writeChunk(&chunk, OP_RETURN);
  disassembleChunk(&chunk, "Test");
  freeChunk(&chunk);
  return 0;
}
