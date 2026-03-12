#ifndef clox_common_h
#define clox_common_h

// Overrides in build process
// https://stackoverflow.com/a/41868450
#ifdef BUILD_B
#define DEBUG_TRACE_EXECUTION
#endif

#ifdef BUILD_C
#define DEBUG_PRINT_CODE
#endif

#define UINT8_COUNT (UINT8_MAX + 1)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#endif
