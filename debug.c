#include "debug.h"
#include "chunk.h"
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
	// What's the next bytecode to show.
	return offset + 1;
}
static int byteInstruction(const char *name, Chunk *chunk, int offset) {
	// We want to get the values in the locals array, i.e. the name and scope depth
	// But we're debugging the vm. So we don't have that info
	// So we just get the value of the arg to the op code which will point us to the slot in the
	// locals array
	uint8_t slot = chunk->code[offset + 1];
	printf("%-16s %4d\n", name, slot);
	return offset + 1;
}

static int constantInstruction(const char *name, Chunk *chunk, int offset) {
	// Same as simple instruction but also print the item at the location
	// of the constants array at the offset
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
	case OP_PRINT:
		return simpleInstruction("OP_PRINT", offset);
	case OP_RETURN:
		// How do we print the next chunk if its a constant?
		return simpleInstruction("OP_RETURN", offset);
	case OP_CONSTANT:
		// Not sure what we do yet, maybe just print the value from the memory
		// address.
		return constantInstruction("OP_CONSTANT", chunk, offset);
	case OP_NIL:
		return simpleInstruction("OP_NIL", offset);
	case OP_TRUE:
		return simpleInstruction("OP_TRUE", offset);
	case OP_FALSE:
		return simpleInstruction("OP_FALSE", offset);
	case OP_POP:
		return simpleInstruction("OP_POP", offset);
	case OP_GET_LOCAL:
		return byteInstruction("OP_GET_LOCAL", chunk, offset);
	case OP_SET_LOCAL:
		return byteInstruction("OP_SET_LOCAL", chunk, offset);
	case OP_GET_GLOBAL:
		return constantInstruction("OP_GET_GLOBAL", chunk, offset);
	case OP_DEFINE_GLOBAL:
		return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
	case OP_SET_GLOBAL:
		return constantInstruction("OP_SET_GLOBAL", chunk, offset);
	case OP_EQUAL:
		return simpleInstruction("OP_EQUAL", offset);
	case OP_GREATER:
		return simpleInstruction("OP_GREATER", offset);
	case OP_LESS:
		return simpleInstruction("OP_LESS", offset);
	case OP_NEGATE:
		return simpleInstruction("OP_NEGATE", offset);
	case OP_ADD:
		return simpleInstruction("OP_ADD", offset);
	case OP_SUBTRACT:
		return simpleInstruction("OP_SUBTRACT", offset);
	case OP_MULTIPLY:
		return simpleInstruction("OP_MULTIPLY", offset);
	case OP_DIVIDE:
		return simpleInstruction("OP_DIVIDE", offset);
	case OP_NOT:
		return simpleInstruction("OP_NOT", offset);
	default:
		printf("Unknown OpCode %d\n", instruction);
		return offset + 1;
	}
}
