#include "object.h"

#include <stdlib.h>

#include "dynamic_array.h"
#include "chunk.h"

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

bool object_equals(Object* left, Object* right) {
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
            for(size_t i = 0; i < a->length; i++) if(!value_equals(a->data[i], b->data[i])) return false;

            return true;
        }
        case OBJECT_FUNCTION:
        case OBJECT_CLOSURE:
            return left == right;
        case OBJECT_CAPTURE:
            fprintf(stderr, "Attempt to perform equality on capture. This should never happen.\n");
            exit(-1);
    }
}

char* object_to_string(Object* self, size_t* length, bool* should_free) {
    switch(self->type) {
        case OBJECT_STR: {
            ObjectStr* str = (ObjectStr*) self;
            *length = str->length;
            *should_free = false;
            return str->data;
        }
        case OBJECT_ARRAY: {
            ObjectArray* array = (ObjectArray*) self;

            if(array->length == 0) {
                *length = 2;
                *should_free = false;
                return "[]";
            }

            char* elements_as_strings[array->length];
            size_t lengths[array->length];
            bool should_frees[array->length];

            *length = 0;
            for(size_t i = 0; i < array->length; i++) {
                elements_as_strings[i] = value_to_string(array->data[i], &lengths[i], &should_frees[i]);
                *length += lengths[i] + 2; // + 2 because of the ", " between elements. No need to subtract on the last element because the extra + 2 accounts for the "[" and "]".
            }

            char* out = malloc(*length);
            size_t out_i = 0;

            out[out_i++] = '[';

            for(size_t i = 0; i < array->length; i++) {
                for(size_t j = 0; j < lengths[i]; j++) out[out_i++] = elements_as_strings[i][j];

                if(should_frees[i]) free(elements_as_strings[i]);

                if(i < array->length - 1) {
                    out[out_i++] = ',';
                    out[out_i++] = ' ';
                }
            }

            out[out_i] = ']';

            *should_free = true;
            return out;
        }
        case OBJECT_FUNCTION: {
            *length = sizeof("<fn 00000000>");
            *should_free = true;
            char* out = malloc(*length);
            sprintf(out, "<fn %p>", self);
            return out;
        }
        case OBJECT_CLOSURE: {
            *length = sizeof("<cl 00000000>");
            *should_free = true;
            char* out = malloc(*length);
            sprintf(out, "<cl %p>", self);
            return out;
        }
        case OBJECT_CAPTURE:
            fprintf(stderr, "Attempt to stringify capture. This should never happen.\n");
            exit(-1);
    }
}

void object_fprint(FILE* file, Object* self) {
    switch(self->type) {
        case OBJECT_STR: {
            ObjectStr* str = (ObjectStr*) self;
            fprintf(file, "%.*s", str->length, str->data);
            break;
        }
        case OBJECT_ARRAY: {
            ObjectArray* array = (ObjectArray*) self;
            fprintf(file, "[");
            for(size_t i = 0; i < array->length; i++) {
                value_fprint(file, array->data[i]);
                if(i + 1 < array->length) fprintf(file, ", ");
            }
            fprintf(file, "]");
            break;
        }
        case OBJECT_FUNCTION:
            fprintf(file, "<fn %p>", self);
            break;
        case OBJECT_CLOSURE:
            fprintf(file, "<cl %p>", self);
            break;
        case OBJECT_CAPTURE:
            fprintf(file, "<capture: ");
            value_fprint(file, ((ObjectCapture*) self)->captured_value);
            fprintf(file, ">");
            break;
    }
}

void object_fprintln(FILE* file, Object* self) {
    object_fprint(file, self);
    fprintf(file, "\n");
}

void object_print(Object* self) {
    object_fprint(stdout, self);
}

void object_println(Object* self) {
    object_fprintln(stdout, self);
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