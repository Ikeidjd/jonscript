#include "value.h"

#include "dynamic_array.h"

Value value_new_int(int64_t n) {
    return (Value) {
        .type = VALUE_INT,
        .as.integer = n
    };
}

void value_fprint(FILE* file, Value self) {
    switch(self.type) {
        case VALUE_INT:
            fprintf(file, "%lld", self.as.integer);
            break;
        default:
            fprintf(file, "Unknown ValueType %d", self.type);
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
    return (ValueArray) {
        .data = NULL,
        .length = 0,
        .capacity = 0
    };
}

void value_array_destruct(ValueArray self) {
    free(self.data);
}

void value_array_push(ValueArray* self, Value value) {
    PUSH(self, value, 16);
}