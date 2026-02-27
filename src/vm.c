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

static Value vm_top(VM* self) {
    return self->stack[self->sp - 1];
}

static void vm_run(VM* self, Chunk* chunk) {
#define NUMERICAL_BIN_OP(operator) \
do { \
    int64_t b = vm_pop(self).as.integer; \
    int64_t a = vm_pop(self).as.integer; \
    int64_t result = a operator b; \
    vm_push(self, value_new_int(result)); \
} while(0)

#define COMPARISON_OP(operator) \
    do { \
        int64_t b = vm_pop(self).as.integer; \
        int64_t a = vm_pop(self).as.integer; \
        bool result = a operator b; \
        vm_push(self, value_new_bool(result)); \
    } while(0)

// Each READ() increments ip by one, so substracting 3 balances it out (the READ() that's missing here is the one that reads the opcode)
#define JUMP(condition) \
do { \
    size_t jump_length = READ(); \
    jump_length |= READ() << 8; \
    if(condition) ip += jump_length - 3; \
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
            case OP_LOAD_TRUE: {
                vm_push(self, value_new_bool(true));
                break;
            }
            case OP_LOAD_FALSE: {
                vm_push(self, value_new_bool(false));
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
            case OP_POP:
                vm_pop(self);
                break;
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
                NUMERICAL_BIN_OP(+);
                break;
            case OP_SUB:
                NUMERICAL_BIN_OP(-);
                break;
            case OP_MUL:
                NUMERICAL_BIN_OP(*);
                break;
            case OP_DIV:
                NUMERICAL_BIN_OP(/);
                break;
            case OP_MOD:
                NUMERICAL_BIN_OP(%);
                break;
            case OP_BITWISE_AND:
                NUMERICAL_BIN_OP(&);
                break;
            case OP_BITWISE_OR:
                NUMERICAL_BIN_OP(|);
                break;
            case OP_LT:
                COMPARISON_OP(<);
                break;
            case OP_LE:
                COMPARISON_OP(<=);
                break;
            case OP_GT:
                COMPARISON_OP(>);
                break;
            case OP_GE:
                COMPARISON_OP(>=);
                break;
            case OP_EQUALS: {
                Value b = vm_pop(self);
                Value a = vm_pop(self);
                bool result = value_equals(a, b);
                vm_push(self, value_new_bool(result));
                break;
            }
            case OP_JUMP:
                JUMP(true);
                break;
            case OP_JUMP_IF_TRUE:
                JUMP(vm_top(self).as.boolean);
                break;
            case OP_JUMP_IF_FALSE:
                JUMP(!vm_top(self).as.boolean);
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