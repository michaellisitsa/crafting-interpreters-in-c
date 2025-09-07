#include "memory.h"
#include <stdlib.h>
void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }
  void *result = realloc(pointer, newSize);

  // We want to confirm the system was able to re-allocate new memory.
  if (result == NULL) {
    exit(1);
  }
  return result;
}
