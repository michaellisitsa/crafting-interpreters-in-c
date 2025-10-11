#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef double Value;

typedef struct {
  int capacity;
  int count;
  Value *values;
} ValueArray;

// We pass an uninitialized ValueArray and fill its values
void initValueArray(ValueArray *array);
// What are we trying to fill in with constants that are added.
// This will be used within the op code for constant array loading
// i.e. where we store something when adding an opcode.
// Do we need to return where the memory is written? Or just update the array
// internally to the function.
void writeValueArray(ValueArray *array, Value value);
// TODO: add a free array method
// void freeValueArray(ValueArray *array);
//
void freeValueArray(ValueArray *array);

#endif
