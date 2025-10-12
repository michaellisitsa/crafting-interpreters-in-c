#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"
#include <stdio.h>

int main(int argc, const char *argv[]) {
  initVM();
  Chunk chunk;
  initChunk(&chunk);

  // left operand
  int constant = addConstant(&chunk, 1.2);
  writeChunk(&chunk, OP_CONSTANT, 123);
  writeChunk(&chunk, constant, 123);

  // right operand
  constant = addConstant(&chunk, 3.4);
  writeChunk(&chunk, OP_CONSTANT, 123);
  writeChunk(&chunk, constant, 123);

  // add together
  writeChunk(&chunk, OP_ADD, 123);

  // new right operand
  constant = addConstant(&chunk, 5.6);
  writeChunk(&chunk, OP_CONSTANT, 123);
  writeChunk(&chunk, constant, 123);

  // division
  writeChunk(&chunk, OP_DIVIDE, 123);
  writeChunk(&chunk, OP_NEGATE, 123);

  // finish
  writeChunk(&chunk, OP_RETURN, 123);
  interpret(&chunk);
  freeVM();
  freeChunk(&chunk);
  return 0;
}
