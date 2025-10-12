#include "vm.h"
#include "common.h"
#include "debug.h"
#include "value.h"
#include <stdio.h>

VM vm;

void initVM() {}

void freeVM() {}

InterpretResult interpret(Chunk *chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
  // This will actually run the bytecode
  return run();
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    // Since disassemble expects an offset, and vm.ip is a pointer into the code
    // array directly
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {

    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      printValue(constant);
      printf("\n");
      break;
    }
    case OP_RETURN: {
      return INTERPRET_OK;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
}
