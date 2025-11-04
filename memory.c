#include "memory.h"
#include "object.h"
#include "vm.h"
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

static void freeObject(Obj *object) {
	switch (object->type) {
	case OBJ_FUNCTION: {
		ObjFunction *function = (ObjFunction *)object;
		freeChunk(&function->chunk);
		FREE(ObjFunction, object);
		break;
	}
	case OBJ_STRING: {
		ObjString *string = (ObjString *)object;
		FREE_ARRAY(char, string->chars, string->length + 1);
		FREE(ObjString, object);
		break;
	}
	}
}

void freeObjects() {
	// We need to go through the list of objects
	// Get the head from the vm.
	Obj *object = vm.objects;
	while (object != NULL) {
		Obj *next = object->next;
		freeObject(object);
		object = next;
	}
}
