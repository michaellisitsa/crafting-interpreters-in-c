#include "value.h"
#include "memory.h"
#include "object.h"
#include <stdio.h>
#include <string.h>

void initValueArray(ValueArray *array) {
	// We will dynamically resize the array when we add item to it.
	array->capacity = 0;
	array->count = 0;
	// We will write chunks to the values later. Can it be a null?
	array->values = NULL;
}

void writeValueArray(ValueArray *array, Value value) {
	// Let's try to grow the array if its not big enough yet.
	if (array->capacity <= array->count) {
		// We store this in a variable
		int oldCapacity = array->capacity;
		array->capacity = GROW_CAPACITY(oldCapacity);
		// We get the result of realloc and save the new pointer into code,
		// The old one is now gone.
		array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
	}

	// I guess count points to a free element. How will we discover this?
	array->values[array->count] = value;
	// Increment the count
	array->count++;
}

void freeValueArray(ValueArray *array) {
	FREE_ARRAY(Value, array->values, array->capacity);

	// Leave the value struct in a clean empty state with no pointers to freed
	// memory
	initValueArray(array);
}
void printValue(Value value) {
	switch (value.type) {
	case VAL_BOOL:
		printf(AS_BOOL(value) ? "true" : "false");
		break;
	case VAL_NIL:
		printf("nil");
		break;
	case VAL_NUMBER:
		printf("%g", AS_NUMBER(value));
		break;
	case VAL_OBJ:
		printObject(value);
		break;
	}
}

bool valuesEqual(Value a, Value b) {
	if (a.type != b.type)
		return false;
	switch (a.type) {
	case VAL_BOOL:
		return AS_BOOL(a) == AS_BOOL(b);
	case VAL_NIL:
		return true;
	case VAL_NUMBER:
		return AS_NUMBER(a) == AS_NUMBER(b);
	case VAL_OBJ: {
		// Since all strings are interned, we can
		// be sure if its the same characters they will be in the same memory location
		return AS_OBJ(a) == AS_OBJ(b);
	}
	default:
		return false;
	}
}
