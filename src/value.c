#include "value.h"

#include "dynamic_array.h"

Value value_new_int(int64_t n) {
    return (Value) {
        .type = VALUE_INT,
        .as.integer = n
    };
}

Value value_new_array(ValueArray* array) {
    return (Value) {
        .type = VALUE_ARRAY,
        .as.array = array
    };
}

void value_fprint(FILE* file, Value self) {
    switch(self.type) {
        case VALUE_INT:
            fprintf(file, "%lld", self.as.integer);
            break;
        case VALUE_ARRAY:
            fprintf(file, "[");
            for(size_t i = 0; i < self.as.array->length; i++) {
                value_fprint(file, self.as.array->data[i]);
                if(i + 1 < self.as.array->length) fprintf(file, ", ");
            }
            fprintf(file, "]");
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

ValueArray value_array_new() {
    return DYNAMIC_ARRAY_NEW(ValueArray);
}

void value_array_destruct(ValueArray self) {
    free(self.data);
}

void value_array_push(ValueArray* self, Value value) {
    PUSH(self, value, 16);
}