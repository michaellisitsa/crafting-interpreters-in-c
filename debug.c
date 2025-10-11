#include "debug.h"
#include "value.h"
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
static int constantInstruction(const char *name, Chunk *chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

int disassembleInstruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);
  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    // don't print the line number again
    printf("    | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }
  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_RETURN:
    // How do we print the next chunk if its a constant?
    return simpleInstruction("OP_RETURN", offset);
  case OP_CONSTANT:
    // Not sure what we do yet, maybe just print the value from the memory
    // address.
    return constantInstruction("OP_CONSTANT", chunk, offset);
  default:
    printf("Unknown OpCode %d\n", instruction);
    return offset + 1;
  }
}
