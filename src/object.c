#include "object.h"

#include <stdlib.h>

#include "dynamic_array.h"

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
    }
}

bool object_equals(Object* left, Object* right) {
    if(left->type != right->type) {
        fprintf(stderr, "Types of object don't match on equality. This should never happen.\n");
        return false;
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
    PUSH(self, value, 16);
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