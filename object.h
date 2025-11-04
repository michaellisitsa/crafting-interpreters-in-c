#ifndef clox_object_h
#define clox_object_h

#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

// We use this with a real Value and see if it is an object and
// And has the type that's passed in i.e. string
#define IS_STRING(value) isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)

#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

typedef enum {
	OBJ_STRING,
	OBJ_FUNCTION,
} ObjType;

struct Obj {
	ObjType type;
	// pointer to next Obj in the chain
	struct Obj *next;
};

typedef struct {
	Obj obj;
	int arity;
	Chunk chunk;
	ObjString *name;
} ObjFunction;

struct ObjString {
	// This is an enum of the types, that uses structural inheritance from the Obj above
	Obj obj;
	int length;
	char *chars;
	uint32_t hash;
};

ObjFunction *newFunction();

ObjString *takeString(char *chars, int length);
ObjString *copyString(const char *chars, int length);

void printObject(Value value);
static inline bool isObjType(Value value, ObjType type) {
	// Since we use value twice, a macro would evaluate it twice. So we need a normal function
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
