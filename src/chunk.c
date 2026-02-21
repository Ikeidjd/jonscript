#include "chunk.h"

Chunk chunk_new() {
    return (Chunk) {
        .values = value_array_new(),
        .code = code_new()
    };
}

void chunk_destruct(Chunk chunk) {
    code_destruct(chunk.code);
    value_array_destruct(chunk.values);
}

void chunk_emit_load_value_op(Chunk* self, Value value) {
    value_array_push(&self->values, value);
    size_t index = self->values.length - 1;
    code_push_args(&self->code, OP_LOAD_VALUE, 2, (byte[]) {index & 0xFF, (index >> 8) & 0xFF});
}

static size_t chunk_display_load_op(Chunk* self, Opcode op, size_t byte_count, size_t offset) {
    opcode_print(op);
    offset++;

    uint64_t value = 0;
    for(size_t i = 0; i < byte_count; i++) {
        value |= (self->code.data[offset] << (i * 8));
        offset++;
    }

    printf(" %llu\n", value);

    return offset;
}

static size_t chunk_display_simple_op(Opcode op, size_t offset) {
    opcode_println(op);
    return offset + 1;
}

size_t chunk_disassemble_op(Chunk* self, size_t offset) {
    Opcode op = self->code.data[offset];
    switch(op) {
        case OP_LOAD_BYTE: return chunk_display_load_op(self, op, 1, offset);
        case OP_LOAD_VALUE: return chunk_display_load_op(self, op, 2, offset);
        case OP_ADD: return chunk_display_simple_op(op, offset);
        case OP_SUB: return chunk_display_simple_op(op, offset);
        case OP_MUL: return chunk_display_simple_op(op, offset);
        case OP_DIV: return chunk_display_simple_op(op, offset);
        case OP_MOD: return chunk_display_simple_op(op, offset);
        default: return chunk_display_simple_op(op, offset);
    }
}

void chunk_disassemble(Chunk* self) {
    for(size_t i = 0; i < self->values.length; i++) {
        printf("[");
        value_print(self->values.data[i]);
        printf("]");
    }
    printf("\n");
    size_t offset = 0;
    while(offset < self->code.length) offset = chunk_disassemble_op(self, offset);
}