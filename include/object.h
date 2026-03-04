#pragma once

#include <stdint.h>

#include "value.h"

typedef enum ObjectType {
    OBJECT_STR,
    OBJECT_ARRAY
} ObjectType;

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

typedef struct ObjectStr {
    Object base;
    char* data;
    size_t length;
} ObjectStr;

ObjectStr object_str_new(char* const data, size_t length);

void object_str_destruct(ObjectStr* const self);
void object_str_free(ObjectStr* self);