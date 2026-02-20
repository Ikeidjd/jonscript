#include "chunk.h"

Chunk chunk_new() {
    return (Chunk) {
        .values = value_array_new(),
        .code = code_new()
    };
}

void chunk_display_load_op(Chunk* self, Opcode op, int byte_count, size_t* offset) {
    opcode_print(op);
    (*offset)++;

    int value = 0;
    for(size_t i = 0; i < byte_count; i++) {
        value |= (self->code.data[*offset] << (i * 8));
        (*offset)++;
    }

    printf(" %d\n", value);
}

void chunk_display_simple_op(Opcode op, size_t* offset) {
    opcode_println(op);
    (*offset)++;
}

void chunk_disassemble(Chunk* self) {
    size_t offset = 0;
    while(offset < self->code.length) {
        Opcode op = self->code.data[offset];
        switch(op) {
            case OP_LOAD_1B:
                chunk_display_load_op(self, op, 1, &offset);
                break;
            case OP_LOAD_2B:
                chunk_display_load_op(self, op, 2, &offset);
                break;
            case OP_LOAD_4B:
                chunk_display_load_op(self, op, 4, &offset);
                break;
            case OP_LOAD_8B:
                chunk_display_load_op(self, op, 8, &offset);
                break;
            case OP_ADD:
                chunk_display_simple_op(op, &offset);
                break;
            case OP_SUB:
                chunk_display_simple_op(op, &offset);
                break;
            case OP_MUL:
                chunk_display_simple_op(op, &offset);
                break;
            case OP_DIV:
                chunk_display_simple_op(op, &offset);
                break;
            case OP_MOD:
                chunk_display_simple_op(op, &offset);
                break;
            default:
                chunk_display_simple_op(op, &offset);
                break;
        }
    }
}