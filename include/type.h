#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

typedef unsigned char byte;

typedef enum TypeType {
    TTYPE_PRIMITIVE,
    TTYPE_ARRAY,
    // TTYPE_DICT,
    // TTYPE_USER
} TypeType;

typedef struct Type {
    TypeType type;
    uint64_t hash;
} Type;

void type_fprint(FILE* file, Type* self);
void type_fprintln(FILE* file, Type* self);
void type_print(Type* self);
void type_println(Type* self);

typedef enum {
    TYPE_INT,
    TYPE_BOOL
} PrimitiveType;

typedef struct {
    Type base;
    PrimitiveType type;
} PrimitiveTypeObj;

void primitive_type_fprint(FILE* file, PrimitiveType self);
void primitive_type_fprintln(FILE* file, PrimitiveType self);
void primitive_type_print(PrimitiveType self);
void primitive_type_println(PrimitiveType self);

typedef struct ArrayType {
    Type base;
    Type* type;
} ArrayType;

void array_type_fprint(FILE* file, ArrayType* self);
void array_type_fprintln(FILE* file, ArrayType* self);
void array_type_print(ArrayType* self);
void array_type_println(ArrayType* self);

// typedef struct DictType {
//     Type base;
//     Type* key;
//     Type* value;
// } DictType;

// typedef struct UserType {
//     Type base;
//     char* name;
//     size_t name_length;
// } UserType;

struct TypeArena;

typedef struct TypeHashSet {
    struct TypeArena* arena;
    Type** data;
    size_t size;
    size_t occupied;
} TypeHashSet;

TypeHashSet type_hash_set_new();
void type_hash_set_destruct(TypeHashSet self);

PrimitiveTypeObj* primitive_type_new(TypeHashSet* set, PrimitiveType type);
ArrayType* array_type_new(TypeHashSet* set, Type* type);

bool is_primitive(Type* self, PrimitiveType type);
bool is_array(Type* self);