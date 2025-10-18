#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef enum {
	VAL_BOOL,
	VAL_NIL,
	VAL_NUMBER,
} ValueType;

typedef struct {
	ValueType type;
	union {
		bool boolean;
		double number;
	} as;
} Value;

typedef struct {
	int capacity;
	int count;
	Value *values;
} ValueArray;

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL(value) ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

// We pass an uninitialized ValueArray and fill its values
void initValueArray(ValueArray *array);
// What are we trying to fill in with constants that are added.
// This will be used within the op code for constant array loading
// i.e. where we store something when adding an opcode.
// Do we need to return where the memory is written? Or just update the array
// internally to the function.
void writeValueArray(ValueArray *array, Value value);
// TODO: add a free array method
void freeValueArray(ValueArray *array);
void printValue(Value value);

#endif
