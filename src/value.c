#include "value.h"

#include <stdlib.h>

#include "object.h"
#include "vm.h"

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

Value value_new_tuple(ValueTuple tuple) {
    return (Value) {
        .type = VALUE_TUPLE,
        .as.tuple = tuple
    };
}

Value value_new_object(Object* object) {
    return (Value) {
        .type = VALUE_OBJECT,
        .as.object = object
    };
}

bool value_equals(Value a, Value b, Value stack[]) {
    if(a.type != b.type) {
        fprintf(stderr, "Types of value don't match on equality. This should never happen.\n");
        return false;
    }

    switch(a.type) {
        case VALUE_INT: return a.as.integer == b.as.integer;
        case VALUE_BOOL: return a.as.boolean == b.as.boolean;
        case VALUE_TUPLE: {
            ValueTuple left = a.as.tuple;
            ValueTuple right = b.as.tuple;

            if(left.length != right.length) return false;

            for(size_t i = 0; i < left.length; i++) {
                if(!value_equals(stack[left.value_stack_index + i], stack[right.value_stack_index + i], stack)) return false;
            }

            return true;
        }
        case VALUE_OBJECT: return object_equals(a.as.object, b.as.object, stack);
    }
}

Value value_deep_copy(Value self, VM* vm) {
    switch(self.type) {
        case VALUE_INT:
        case VALUE_BOOL:
            return self;
        case VALUE_TUPLE: {
            ValueTuple tuple = self.as.tuple;
            Value out = value_new_tuple((ValueTuple) {
                .value_stack_index = vm->sp,
                .length = tuple.length
            });
            for(size_t i = 0; i < tuple.length; i++) vm_push(vm, vm->stack[tuple.value_stack_index + i]);
            return out;
        }
        case VALUE_OBJECT:
            if(self.as.object->type == OBJECT_CAPTURE) return value_deep_copy(((ObjectCapture*) self.as.object)->captured_value, vm); // This is kind of a hack, but it works well enough
            return value_new_object(object_deep_copy(self.as.object, vm));
    }
}

void temp_string_destruct(TempString self) {
    if(self.should_free) free(self.data);
}

TempString value_to_str(Value self, Value stack[]) {
    switch(self.type) {
        case VALUE_INT: {
            uint64_t integer = self.as.integer;

            if(integer == 0) {
                return (TempString) {
                    .data = "0",
                    .length = 1,
                    .should_free = false
                };
            }

            TempString str = {
                .data = malloc(32),
                .length = 0,
                .should_free = true
            };

            while(integer > 0) {
                str.data[str.length++] = '0' + integer % 10;
                integer /= 10;
            }

            for(size_t i = 0; i < str.length / 2; i++) {
                char temp = str.data[i];
                str.data[i] = str.data[str.length - i - 1];
                str.data[str.length - i - 1] = temp;
            }

            return str;
        }
        case VALUE_BOOL: {
            bool b = self.as.boolean;
            return (TempString) {
                .data = b ? "true" : "false",
                .length = b ? 4 : 5,
                .should_free = false
            };
        }
        case VALUE_TUPLE: {
            ValueTuple tuple = self.as.tuple;

            if(tuple.length == 0) {
                return (TempString) {
                    .data = "()",
                    .length = 2,
                    .should_free = false
                };
            }

            TempString* elements = malloc(tuple.length * sizeof(TempString));

            size_t length = 0;
            for(size_t i = 0; i < tuple.length; i++) {
                elements[i] = value_to_repr(stack[tuple.value_stack_index + i], stack);
                length += elements[i].length + 2; // + 2 because of the ", " between elements. No need to subtract on the last element because the extra + 2 accounts for the "[" and "]".
            }

            TempString str = (TempString) {
                .data = malloc(length),
                .length = length,
                .should_free = true
            };

            size_t index = 0;
            str.data[index++] = '(';

            for(size_t i = 0; i < tuple.length; i++) {
                for(size_t j = 0; j < elements[i].length; j++) str.data[index++] = elements[i].data[j];
                temp_string_destruct(elements[i]);

                if(i < tuple.length - 1) {
                    str.data[index++] = ',';
                    str.data[index++] = ' ';
                }
            }

            str.data[index] = ')';
            free(elements);

            return str;
        }
        case VALUE_OBJECT:
            return object_to_str(self.as.object, stack);
    }
}

TempString value_to_repr(Value self, Value stack[]) {
    switch(self.type) {
        case VALUE_INT:
        case VALUE_BOOL:
        case VALUE_TUPLE:
            return value_to_str(self, stack);
        case VALUE_OBJECT:
            return object_to_repr(self.as.object, stack);
    }
}

void value_fprint(FILE* file, Value self, Value stack[]) {
    TempString str = value_to_str(self, stack);
    fprintf(file, "%.*s", str.length, str.data);
    temp_string_destruct(str);
}

void value_fprintln(FILE* file, Value self, Value stack[]) {
    value_fprint(file, self, stack);
    fprintf(file, "\n");
}

void value_print(Value self, Value stack[]) {
    value_fprint(stdout, self, stack);
}

void value_println(Value self, Value stack[]) {
    value_fprintln(stdout, self, stack);
}