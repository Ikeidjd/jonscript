#include "vm.h"

#include <stdlib.h>
#include <stdint.h>

#include "stack.h"

typedef struct VM {
    size_t sp;
    Value stack[STACK_SIZE];
} VM;

static void vm_push(VM* self, Value value) {
    self->stack[self->sp++] = value;
}

static Value vm_pop(VM* self) {
    return self->stack[--self->sp];
}

static void vm_pop_n(VM* self, size_t n) {
    self->sp -= n;
}

static void vm_run(VM* self, Chunk* chunk) {
#define BIN_OP(operator) \
do { \
    int64_t b = vm_pop(self).as.integer; \
    int64_t a = vm_pop(self).as.integer; \
    int64_t result = a operator b; \
    vm_push(self, value_new_int(result)); \
} while(0)

#define READ() chunk->code.data[ip++]

    size_t ip = 0;
    while(ip < chunk->code.length) {
        for(size_t i = 0; i < self->sp; i++) {
            printf("[");
            value_print(self->stack[i]);
            printf("]");
        }
        if(self->sp == 0) printf("[]");
        printf("\n");
        chunk_disassemble_op(chunk, ip);
        Opcode op = READ();
        switch(op) {
            case OP_LOAD_BYTE:
                vm_push(self, value_new_int((int64_t) READ()));
                break;
            case OP_LOAD_VALUE: {
                size_t index = READ();
                index |= (READ() << 8);
                vm_push(self, chunk->values.data[index]);
                break;
            }
            case OP_LOCAL_GET: {
                size_t index = READ();
                index |= (READ() << 8);
                vm_push(self, self->stack[index]);
                break;
            }
            case OP_LOCAL_SET: {
                size_t index = READ();
                index |= (READ() << 8);
                self->stack[index] = vm_pop(self);
                break;
            }
            case OP_INDEX_GET: {
                size_t index = vm_pop(self).as.integer;
                ValueArray* array = vm_pop(self).as.array;
                vm_push(self, array->data[index]);
                break;
            }
            case OP_INDEX_SET: {
                // Yeah, the order of arguments is a little weird, but it was easier to compile that way
                size_t index = vm_pop(self).as.integer;
                ValueArray* array = vm_pop(self).as.array;
                Value value = vm_pop(self);
                array->data[index] = value;
                break;
            }
            case OP_ARRAYIFY_LIST: {
                size_t length = READ();
                length |= (READ() << 8);
                ValueArray* array = malloc(sizeof(ValueArray)); // TODO: remember to free() this when you implement garbage collection
                *array = (ValueArray) {
                    .data = malloc(length * sizeof(Value)), // TODO: remember to free() this when you implement garbage collection
                    .length = length,
                    .capacity = length
                };
                for(size_t i = 0; i < length; i++) array->data[i] = self->stack[i + self->sp - length];
                vm_pop_n(self, length);
                vm_push(self, value_new_array(array));
                break;
            }
            case OP_ARRAYIFY_LENGTH: {
                size_t length = vm_pop(self).as.integer;
                ValueArray* array = malloc(sizeof(ValueArray)); // TODO: remember to free() this when you implement garbage collection
                *array = (ValueArray) {
                    .data = malloc(length * sizeof(Value)), // TODO: remember to free() this when you implement garbage collection
                    .length = length,
                    .capacity = length
                };
                Value value = vm_pop(self);
                for(size_t i = 0; i < length; i++) array->data[i] = value;
                vm_push(self, value_new_array(array));
                break;
            }
            case OP_ADD:
                BIN_OP(+);
                break;
            case OP_SUB:
                BIN_OP(-);
                break;
            case OP_MUL:
                BIN_OP(*);
                break;
            case OP_DIV:
                BIN_OP(/);
                break;
            case OP_MOD:
                BIN_OP(%);
                break;
        }
    }
}

void run(Chunk* chunk) {
    VM* self = (VM*) malloc(sizeof(VM));
    self->sp = 0;
    vm_run(self, chunk);
    for(size_t i = 0; i < self->sp; i++) {
        printf("[");
        value_print(self->stack[i]);
        printf("]");
    }
}