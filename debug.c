#include "debug.h"
#include <stdio.h>

void disassembleChunk(Chunk *chunk, const char *name) {
  printf("== %s ==\n", name);
  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(chunk, offset);
  }
}

static int simpleInstruction(const char *name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

int disassembleInstruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);
  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_RETURN:
    // How do we print the next chunk if its a constant?
    return simpleInstruction("OP_RETURN", offset);
  case LOAD_CONST:
    // Not sure what we do yet, maybe just print the value from the memory
    // address.
    return simpleInstruction("LOAD_CONST", offset + 1);
  default:
    printf("Unknown OpCode %d\n", instruction);
    return offset + 1;
  }
}
