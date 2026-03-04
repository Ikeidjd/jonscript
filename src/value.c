#include "value.h"

#include <stdlib.h>

#include "object.h"

Value value_new_int(int64_t n) {
    return (Value) {
        .type = VALUE_INT,
        .as.integer = n
    };
}

Value value_new_bool(bool b) {
    return (Value) {
        .type = VALUE_BOOL,
        .as.boolean = b
    };
}

Value value_new_object(Object* object) {
    return (Value) {
        .type = VALUE_OBJECT,
        .as.object = object
    };
}

bool value_equals(Value a, Value b) {
    if(a.type != b.type) {
        fprintf(stderr, "Types of value don't match on equality. This should never happen.\n");
        return false;
    }

    switch(a.type) {
        case VALUE_INT: return a.as.integer == b.as.integer;
        case VALUE_BOOL: return a.as.boolean == b.as.boolean;
        case VALUE_OBJECT: return object_equals(a.as.object, b.as.object);
    }
}

char* value_to_string(Value self, size_t* length, bool* should_free) {
    switch(self.type) {
        case VALUE_INT: {
            uint64_t integer = self.as.integer;

            if(integer == 0) {
                *length = 1;
                *should_free = false;
                return "0";
            }

            char* out = malloc(32);

            *length = 0;
            while(integer > 0) {
                out[(*length)++] = '0' + integer % 10;
                integer /= 10;
            }

            for(size_t i = 0; i < *length / 2; i++) {
                char temp = out[i];
                out[i] = out[*length - i - 1];
                out[*length - i - 1] = temp;
            }

            *should_free = true;
            return out;
        }
        case VALUE_BOOL:
            *should_free = false;

            if(self.as.boolean) {
                *length = 4;
                return "true";
            }

            *length = 5;
            return "false";
        case VALUE_OBJECT:
            return object_to_string(self.as.object, length, should_free);
    }
}

void value_fprint(FILE* file, Value self) {
    switch(self.type) {
        case VALUE_INT:
            fprintf(file, "%lld", self.as.integer);
            break;
        case VALUE_BOOL:
            fprintf(file, "%s", self.as.boolean ? "true" : "false");
            break;
        case VALUE_OBJECT:
            object_fprint(file, self.as.object);
            break;
    }
}

void value_fprintln(FILE* file, Value self) {
    value_fprint(file, self);
    fprintf(file, "\n");
}

void value_print(Value self) {
    value_fprint(stdout, self);
}

void value_println(Value self) {
    value_fprintln(stdout, self);
}