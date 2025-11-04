#include "object.h"
#include "memory.h"
#include "table.h"
#include "value.h"
#include "vm.h"
#include <stdio.h>
#include <string.h>

#define ALLOCATE_OBJ(type, objectType) (type *)allocateObject(sizeof(type), objectType)

// This will be used for allocating all types of objects
// This ensures that whatever type is passed
// in there will be enough room for that type
static Obj *allocateObject(size_t size, ObjType type) {
	Obj *object = (Obj *)reallocate(NULL, 0, size);
	object->type = type;

	// Keep a link to the next object allocated.
	// This tracks all objects to be freed later
	object->next = vm.objects;
	vm.objects = object;
	return object;
}

ObjFunction *newFunction() {
	ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
	function->arity = 0;
	function->name = NULL;
	initChunk(&function->chunk);
	return function;
}

static ObjString *allocateString(char *chars, int length, uint32_t hash) {
	ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	string->length = length;
	string->chars = chars;
	string->hash = hash;
	// Intern each string into a table of strings
	// We have no Value, so its more like a set
	tableSet(&vm.strings, string, NIL_VAL);
	return string;
}

static uint32_t hashString(const char *key, int length) {
	// FNV-1a hash function
	uint32_t hash = 2166136261u;
	for (int i = 0; i < length; i++) {
		hash ^= (uint8_t)key[i];
		hash *= 16777619;
	}
	return hash;
}

ObjString *takeString(char *chars, int length) {
	// Used for cases where the string is not re-allocated
	// Instead its just inserted into ObjString->chars
	uint32_t hash = hashString(chars, length);
	ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
	if (interned != NULL) {
		// Sinced ownership is being passed to this function, we need to free the string
		// if it already exists in the interned table. Then we just return that interned value.
		FREE_ARRAY(char, chars, length + 1);
		return interned;
	}
	return allocateString(chars, length, hash);
}

ObjString *copyString(const char *chars, int length) {
	// Used for cases where the string needs to be copied into a new memory block
	// This ensures that when the created ObjString is eventually freed,
	// it doesn't free the source string.
	uint32_t hash = hashString(chars, length);
	ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
	if (interned != NULL)
		return interned;
	char *heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';
	return allocateString(heapChars, length, hash);
}

static void printFunction(ObjFunction *function) {
	if (function->name == NULL) {
		printf("<script>");
		return;
	}
	printf("<fn %s>", function->name->chars);
}

void printObject(Value value) {
	switch (OBJ_TYPE(value)) {
	case OBJ_FUNCTION: {
		printFunction(AS_FUNCTION(value));
		break;
	}
	case OBJ_STRING:
		printf("%s", AS_CSTRING(value));
		break;
	}
}
