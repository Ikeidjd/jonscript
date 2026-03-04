#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum ValueType {
    VALUE_INT,
    VALUE_BOOL,
    VALUE_OBJECT
} ValueType;

typedef struct Object Object;

typedef struct Value {
    ValueType type;
    union {
        int64_t integer;
        bool boolean;
        Object* object;
    } as;
} Value;

Value value_new_int(int64_t n);
Value value_new_bool(bool b);
Value value_new_object(Object* object);

bool value_equals(Value a, Value b);

char* value_to_string(Value self, size_t* length, bool* should_free);

void value_fprint(FILE* file, Value self);
void value_fprintln(FILE* file, Value self);
void value_print(Value self);
void value_println(Value self);

#define AS_OBJECT(value, type) ((type*) ((value).as.object))
#define AS_STR(value) AS_OBJECT(value, ObjectStr)
#define AS_ARRAY(value) AS_OBJECT(value, ObjectArray)