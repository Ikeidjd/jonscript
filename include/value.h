#pragma once

typedef enum ValueType {
    VALUE_INT
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        int integer;
    } as;
} Value;

typedef struct ValueArray {
    Value* data;
    size_t length;
    size_t capacity;
} ValueArray;

ValueArray value_array_new();
void value_array_push(ValueArray* self, Value value);