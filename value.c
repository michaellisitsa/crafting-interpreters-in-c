#include "value.h"
#include "memory.h"

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
    array->capacity = GROW_CAPACITY(array->capacity);
    // We get the result of realloc and save the new pointer into code,
    // The old one is now gone.
    array->values =
        GROW_ARRAY(uint8_t, array->values, oldCapacity, array->capacity);
  }

  // I guess count points to a free element. How will we discover this?
  array->values[array->count] = value;
  // Increment the count
  array->count++;
}
