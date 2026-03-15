#pragma once

#include <stdint.h>

#include "value.h"

typedef enum ObjectType {
    OBJECT_STR,
    OBJECT_ARRAY,
    OBJECT_FUNCTION,
    OBJECT_CAPTURE,
    OBJECT_CLOSURE
} ObjectType;

char* object_type_to_string(ObjectType self);

struct Object {
    ObjectType type;
    struct Object* next;
};

Object object_new(ObjectType type);
void object_free(Object* object);

bool object_equals(Object* a, Object* b);

char* object_to_string(Object* self, size_t* length, bool* should_free);

void object_fprint(FILE* file, Object* self);
void object_fprintln(FILE* file, Object* self);
void object_print(Object* self);
void object_println(Object* self);

typedef struct ObjectStr {
    Object base;
    uint64_t hash;
    char* data;
    size_t length;
} ObjectStr;

ObjectStr object_str_new(char* data, size_t length);

void object_str_destruct(ObjectStr* const self);
void object_str_free(ObjectStr* self);

typedef struct ObjectArray {
    Object base;
    Value* data;
    size_t length;
    size_t capacity;
} ObjectArray;

ObjectArray object_array_new();
ObjectArray object_array_of_length(size_t length);

void object_array_destruct(ObjectArray* const self);
void object_array_free(ObjectArray* self);

void object_array_push(ObjectArray* self, Value value);

typedef struct Chunk Chunk;

typedef struct ObjectFunction {
    Object base;
    Chunk* chunk;
    size_t* captured_locals;
    size_t captured_locals_length;
} ObjectFunction;

ObjectFunction object_function_new(size_t* captured_locals, size_t captured_locals_length);

void object_function_destruct(ObjectFunction* const self);
void object_function_free(ObjectFunction* self);

typedef struct ObjectCapture {
    Object base;
    Value captured_value;
} ObjectCapture;

ObjectCapture object_capture_new(Value value);

typedef struct ObjectClosure {
    Object base;
    Chunk* chunk;
    ObjectCapture** captured_locals;
    size_t captured_locals_length;
} ObjectClosure;

ObjectClosure object_closure_new(ObjectFunction* function);

void object_closure_destruct(ObjectClosure* const self);
void object_closure_free(ObjectClosure* self);