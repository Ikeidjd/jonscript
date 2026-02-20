#include "value.h"

#include "dynamic_array.h"

ValueArray value_array_new() {
    return (ValueArray) {
        .data = NULL,
        .length = 0,
        .capacity = 0
    };
}

void value_array_push(ValueArray* self, Value value) {
    PUSH(self, value, 16);
}