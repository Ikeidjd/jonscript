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
    code_emit_args(&self->code, OP_LOAD_VALUE, 2, TO_LE_2_BYTES(index));
}

static size_t chunk_display_monoarg_op(Chunk* self, Opcode op, size_t byte_count, size_t offset) {
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
    printf("%04d ", offset);
    Opcode op = self->code.data[offset];
    switch(op) {
        case OP_LOAD_TRUE:
        case OP_LOAD_FALSE:
        case OP_INDEX_GET:
        case OP_INDEX_SET:
        case OP_POP:
        case OP_ARRAYIFY_LENGTH:
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
        case OP_BITWISE_AND:
        case OP_BITWISE_OR:
        case OP_LT:
        case OP_LE:
        case OP_GT:
        case OP_GE:
        case OP_EQUALS:
            return chunk_display_simple_op(op, offset);
        case OP_LOAD_BYTE:
            return chunk_display_monoarg_op(self, op, 1, offset);
        case OP_LOAD_VALUE:
        case OP_LOCAL_GET:
        case OP_LOCAL_SET:
        case OP_ARRAYIFY_LIST:
        case OP_JUMP:
        case OP_JUMP_IF_TRUE:
        case OP_JUMP_IF_FALSE:
            return chunk_display_monoarg_op(self, op, 2, offset);
    }
}

void chunk_disassemble(Chunk* self) {
    for(size_t i = 0; i < self->values.length; i++) {
        printf("[");
        value_print(self->values.data[i]);
        printf("]");
    }
    if(self->values.length == 0) printf("[]");
    printf("\n");
    size_t offset = 0;
    while(offset < self->code.length) offset = chunk_disassemble_op(self, offset);
}