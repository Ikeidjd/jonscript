#pragma once

#include <stdio.h>
#include <stdint.h>

typedef enum ValueType {
    VALUE_INT
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        int64_t integer;
    } as;
} Value;

Value value_new_int(int64_t n);

void value_fprint(FILE* file, Value self);
void value_fprintln(FILE* file, Value self);
void value_print(Value self);
void value_println(Value self);

typedef struct ValueArray {
    Value* data;
    size_t length;
    size_t capacity;
} ValueArray;

ValueArray value_array_new();
void value_array_destruct(ValueArray self);

void value_array_push(ValueArray* self, Value value);