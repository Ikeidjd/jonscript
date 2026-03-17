#include "object.h"

#include <stdlib.h>
#include <string.h>

#include "dynamic_array.h"
#include "chunk.h"
#include "vm.h"

char* object_type_to_string(ObjectType self) {
    switch(self) {
        case OBJECT_STR: return "OBJECT_STR";
        case OBJECT_ARRAY: return "OBJECT_ARRAY";
        case OBJECT_FUNCTION: return "OBJECT_FUNCTION";
        case OBJECT_CAPTURE: return "OBJECT_CAPTURE";
        case OBJECT_CLOSURE: return "OBJECT_CLOSURE";
    }
}

Object object_new(ObjectType type) {
    return (Object) {
        .type = type,
        .next = NULL
    };
}

void object_free(Object* self) {
    switch(self->type) {
        case OBJECT_STR:
            object_str_free((ObjectStr*) self);
            break;
        case OBJECT_ARRAY:
            object_array_free((ObjectArray*) self);
            break;
        case OBJECT_FUNCTION:
            object_function_free((ObjectFunction*) self);
            break;
        case OBJECT_CAPTURE:
            free(self);
            break;
        case OBJECT_CLOSURE:
            object_closure_free((ObjectClosure*) self);
            break;
    }
}

bool object_equals(Object* left, Object* right, Value stack[]) {
    if(left->type != right->type) {
        fprintf(stderr, "Types of object don't match on equality. This should never happen.\n");
        exit(-1);
    }

    switch(left->type) {
        case OBJECT_STR:
            return left == right;
        case OBJECT_ARRAY: {
            ObjectArray* a = (ObjectArray*) left;
            ObjectArray* b = (ObjectArray*) right;

            if(a->length != b->length) return false;
            for(size_t i = 0; i < a->length; i++) if(!value_equals(a->data[i], b->data[i], stack)) return false;

            return true;
        }
        case OBJECT_FUNCTION:
        case OBJECT_CLOSURE:
            return left == right;
        case OBJECT_CAPTURE:
            fprintf(stderr, "Attempt to perform equality on capture. This should never happen.\n");
            exit(-1);
            break;
    }
}

Object* object_deep_copy(Object* self, VM* vm) {
    switch(self->type) {
        case OBJECT_STR:
        case OBJECT_FUNCTION:
        case OBJECT_CLOSURE:
            return self;
        case OBJECT_ARRAY: {
            ObjectArray* array = (ObjectArray*) self;
            ObjectArray* out = vm_object_array_malloc(vm, array->length);
            for(size_t i = 0; i < array->length; i++) out->data[i] = value_deep_copy(array->data[i], vm);
            return (Object*) out;
        }
        case OBJECT_CAPTURE:
            fprintf(stderr, "This should have already been handled by value_deep_copy. This should never happen.\n");
            exit(-1);
            break;
    }
}

static TempString fill_in_with_char_ptr(const char* format, char* str, size_t str_length) {
    size_t format_length = strlen(format);
    size_t length = str_length + format_length;

    TempString out = (TempString) {
        .data = malloc(length),
        .length = length,
        .should_free = true
    };

    size_t position = 0;
    for(; format[position] != '%'; position++) out.data[position] = format[position];
    for(size_t i = 0; i < str_length; i++) out.data[position + i] = str[i];
    for(size_t i = position + 1; i < format_length; i++) out.data[str_length + i] = format[i];

    return out;
}

// Destructs str, so be careful
static TempString fill_in_with_temp_string(const char* format, TempString str) {
    TempString out = fill_in_with_char_ptr(format, str.data, str.length);
    temp_string_destruct(str);
    return out;
}

static TempString fill_in_with_object_str(const char* format, ObjectStr* str) {
    return fill_in_with_char_ptr(format, str->data, str->length);
}

static TempString fill_in_with_address(const char* format, void* address) {
    size_t length = sizeof("00000000");
    char str[length--];
    sprintf(str, "%p", address);
    return fill_in_with_char_ptr(format, str, length);
}

TempString object_to_str(Object* self, Value stack[]) {
    switch(self->type) {
        case OBJECT_STR: {
            ObjectStr* str = (ObjectStr*) self;
            return (TempString) {
                .data = str->data,
                .length = str->length,
                .should_free = false
            };
        }
        case OBJECT_ARRAY: {
            ObjectArray* array = (ObjectArray*) self;

            if(array->length == 0) {
                return (TempString) {
                    .data = "[]",
                    .length = 2,
                    .should_free = false
                };
            }

            TempString* elements = malloc(array->length * sizeof(TempString));

            size_t length = 0;
            for(size_t i = 0; i < array->length; i++) {
                elements[i] = value_to_repr(array->data[i], stack);
                length += elements[i].length + 2; // + 2 because of the ", " between elements. No need to subtract on the last element because the extra + 2 accounts for the "[" and "]".
            }

            TempString str = (TempString) {
                .data = malloc(length),
                .length = length,
                .should_free = true
            };

            size_t index = 0;
            str.data[index++] = '[';

            for(size_t i = 0; i < array->length; i++) {
                for(size_t j = 0; j < elements[i].length; j++) str.data[index++] = elements[i].data[j];
                temp_string_destruct(elements[i]);

                if(i < array->length - 1) {
                    str.data[index++] = ',';
                    str.data[index++] = ' ';
                }
            }

            str.data[index] = ']';
            free(elements);

            return str;
        }
        case OBJECT_FUNCTION:
            return fill_in_with_address("<fn %>", self);
        case OBJECT_CLOSURE:
            return fill_in_with_address("<cl %>", self);
        case OBJECT_CAPTURE:
            return fill_in_with_temp_string("<capture: %>", value_to_str(((ObjectCapture*) self)->captured_value, stack));
    }
}

TempString object_to_repr(Object* self, Value stack[]) {
    switch(self->type) {
        case OBJECT_ARRAY:
        case OBJECT_FUNCTION:
        case OBJECT_CLOSURE:
        case OBJECT_CAPTURE:
            return object_to_str(self, stack);
        case OBJECT_STR:
            return fill_in_with_object_str("\"%\"", (ObjectStr*) self);
    }
}

void object_fprint(FILE* file, Object* self, Value stack[]) {
    TempString str = object_to_str(self, stack);
    fprintf(file, "%.*s", str.length, str.data);
    temp_string_destruct(str);
}

void object_fprintln(FILE* file, Object* self, Value stack[]) {
    object_fprint(file, self, stack);
    fprintf(file, "\n");
}

void object_print(Object* self, Value stack[]) {
    object_fprint(stdout, self, stack);
}

void object_println(Object* self, Value stack[]) {
    object_fprintln(stdout, self, stack);
}

// https://stackoverflow.com/questions/7666509/hash-function-for-string
// http://www.cse.yorku.ca/~oz/hash.html
static uint64_t djb2(unsigned char *str, size_t length) {
    uint64_t hash = 5381;

    for(size_t i = 0; i < length; i++) hash = ((hash << 5) + hash) + str[i]; /* hash * 33 + str[i] */

    return hash;
}

ObjectStr object_str_new(char* data, size_t length) {
    return (ObjectStr) {
        .base = object_new(OBJECT_STR),
        .hash = djb2((unsigned char*) data, length),
        .data = data,
        .length = length
    };
}

void object_str_destruct(ObjectStr* const self) {
    free(self->data);
}

void object_str_free(ObjectStr* self) {
    object_str_destruct(self);
    free(self);
}

ObjectArray object_array_new() {
    return (ObjectArray) {
        .base = object_new(OBJECT_ARRAY),
        DYNAMIC_ARRAY_NEW_PARTIAL()
    };
}

ObjectArray object_array_of_length(size_t length) {
    return (ObjectArray) {
        .base = object_new(OBJECT_ARRAY),
        .data = malloc(length * sizeof(Value)),
        .length = length,
        .capacity = length
    };
}

void object_array_destruct(ObjectArray* const self) {
    free(self->data);
}

void object_array_free(ObjectArray* self) {
    object_array_destruct(self);
    free(self);
}

void object_array_push(ObjectArray* self, Value value) {
    DYNARRAY_PTR_PUSH(self, value, 16);
}

ObjectFunction object_function_new(size_t* captured_locals, size_t captured_locals_length) {
    Chunk* chunk = malloc(sizeof(Chunk));
    *chunk = chunk_new();

    return (ObjectFunction) {
        .base = object_new(OBJECT_FUNCTION),
        .chunk = chunk,
        .captured_locals = captured_locals,
        .captured_locals_length = captured_locals_length
    };
}

void object_function_destruct(ObjectFunction* const self) {
    chunk_free(self->chunk);
}

void object_function_free(ObjectFunction* self) {
    object_function_destruct(self);
    free(self);
}

ObjectCapture object_capture_new(Value value) {
    return (ObjectCapture) {
        .base = object_new(OBJECT_CAPTURE),
        .captured_value = value
    };
}

ObjectClosure object_closure_new(ObjectFunction* function) {
    return (ObjectClosure) {
        .base = object_new(OBJECT_CLOSURE),
        .chunk = function->chunk,
        .captured_locals = malloc(function->captured_locals_length * sizeof(ObjectCapture*)),
        .captured_locals_length = function->captured_locals_length
    };
}

void object_closure_destruct(ObjectClosure* const self) {
    free(self->captured_locals);
}

void object_closure_free(ObjectClosure* self) {
    object_closure_destruct(self);
    free(self);
}