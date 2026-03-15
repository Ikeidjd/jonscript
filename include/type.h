#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "token.h"

#define MAX_PARAM_LENGTH 255

typedef unsigned char byte;

typedef enum TypeType {
    TTYPE_PRIMITIVE,
    TTYPE_ARRAY,
    TTYPE_FUNCTION,
    TTYPE_VOID,
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
    TYPE_BOOL,
    TYPE_STR // "Buh-but strings aren't primitive" Eh, close enough
} PrimitiveType;

typedef struct {
    Type base;
    PrimitiveType type;
} PrimitiveTypeObj;

char* primitive_type_to_string(PrimitiveType self);

typedef struct ArrayType {
    Type base;
    Type* type;
} ArrayType;

void array_type_fprint(FILE* file, ArrayType* self);
void array_type_fprintln(FILE* file, ArrayType* self);
void array_type_print(ArrayType* self);
void array_type_println(ArrayType* self);

typedef struct FunctionType {
    Type base;
    Type* param_types[MAX_PARAM_LENGTH];
    size_t params_length;
    Type* return_type;
} FunctionType;

void function_type_fprint(FILE* file, FunctionType* self);
void function_type_fprintln(FILE* file, FunctionType* self);
void function_type_print(FunctionType* self);
void function_type_println(FunctionType* self);

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
FunctionType* function_type_new(TypeHashSet* set, Type* param_types[MAX_PARAM_LENGTH], size_t param_length, Type* return_type);
Type* void_type_new(TypeHashSet* set);

bool is_void(Type* self);
bool is_primitive(Type* self, PrimitiveType type);
bool is_array(Type* self);
bool is_function(Type* self);