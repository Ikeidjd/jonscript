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
#define AS_FUNCTION(value) AS_OBJECT(value, ObjectFunction)
#define AS_CAPTURE(value) AS_OBJECT(value, ObjectCapture)
#define AS_CLOSURE(value) AS_OBJECT(value, ObjectClosure)

#define IS_OBJECT(value, of_type) ((value).type == VALUE_OBJECT && value.as.object->type == (of_type))
#define IS_STR(value) IS_OBJECT(value, OBJECT_STR)
#define IS_ARRAY(value) IS_OBJECT(value, OBJECT_ARRAY)
#define IS_FUNCTION(value) IS_OBJECT(value, OBJECT_FUNCTION)
#define IS_CAPTURE(value) IS_OBJECT(value, OBJECT_CAPTURE)
#define IS_CLOSURE(value) IS_OBJECT(value, OBJECT_CLOSURE)