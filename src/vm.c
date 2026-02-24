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
            case OP_LOAD_STACK: {
                size_t index = READ();
                index |= (READ() << 8);
                vm_push(self, self->stack[index]);
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