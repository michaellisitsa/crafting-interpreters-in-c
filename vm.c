#include "vm.h"
#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "value.h"
#include <stdio.h>

VM vm;

static void resetStack() {
  // since stack is a pointer this will be its zeroth value
  vm.stackTop = vm.stack;
}

void initVM() { resetStack(); }

void freeVM() {}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

InterpretResult interpret(Chunk *chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
  // This will actually run the bytecode
  return run();
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op)                                                          \
  do {                                                                         \
    double b = pop();                                                          \
    double a = pop();                                                          \
    push(a op b);                                                              \
  } while (false)
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    // Get visibility into the value stack
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    // Since disassemble expects an offset, and vm.ip is a pointer into the code
    // array directly
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {

    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }
    case OP_ADD:
      BINARY_OP(+);
      break;
    case OP_SUBTRACT:
      BINARY_OP(-);
      break;
    case OP_MULTIPLY:
      BINARY_OP(*);
      break;
    case OP_DIVIDE:
      BINARY_OP(/);
      break;
    case OP_NEGATE:
      push(-pop());
      break;
    case OP_RETURN: {
      printValue(pop());
      printf("\n");
      return INTERPRET_OK;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}
