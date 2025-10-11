#ifndef clox_memory_h
#define clox_memory_h

// We also handle when the current capacity is zero. In that case, we jump
// straight to eight elements instead of starting at one. That avoids a little
// extra memory churn when the array is very small
#include "common.h"
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

// Why don't we use a return statement to return the value
// Because it's just text substitution
#define GROW_ARRAY(type, pointer, oldCount, newCount)                          \
  (type *)reallocate(pointer, sizeof(type) * (oldCount),                       \
                     sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount)                                    \
  (type *)reallocate(pointer, sizeof(type) * (oldCount), 0)

void *reallocate(void *pointer, size_t oldSize, size_t newSize);

#endif
