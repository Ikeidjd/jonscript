#include "code.h"

#include <stdint.h>

#include "dynamic_array.h"

char* opcode_to_string(Opcode self) {
    switch(self) {
        case OP_LOAD_BYTE: return "OP_LOAD_BYTE";
        case OP_LOAD_VALUE: return "OP_LOAD_VALUE";
        case OP_LOAD_TRUE: return "OP_LOAD_TRUE";
        case OP_LOAD_FALSE: return "OP_LOAD_FALSE";
        case OP_LOAD_CLOSURE: return "OP_LOAD_CLOSURE";
        case OP_LOCAL_GET: return "OP_LOCAL_GET";
        case OP_LOCAL_SET: return "OP_LOCAL_SET";
        case OP_CAPTURE_GET: return "OP_CAPTURE_GET";
        case OP_CAPTURE_SET: return "OP_CAPTURE_SET";
        case OP_INDEX_GET: return "OP_INDEX_GET";
        case OP_INDEX_SET: return "OP_INDEX_SET";
        case OP_CALL: return "OP_CALL";
        case OP_POP: return "OP_POP";
        case OP_POP_N: return "OP_POP_N";
        case OP_ARRAYIFY_LIST: return "OP_ARRAYIFY_LIST";
        case OP_ARRAYIFY_LENGTH: return "OP_ARRAYIFY_LENGTH";
        case OP_ADD: return "OP_ADD";
        case OP_SUB: return "OP_SUB";
        case OP_MUL: return "OP_MUL";
        case OP_DIV: return "OP_DIV";
        case OP_MOD: return "OP_MOD";
        case OP_BITWISE_AND: return "OP_BITWISE_AND";
        case OP_BITWISE_OR: return "OP_BITWISE_OR";
        case OP_LT: return "OP_LT";
        case OP_LE: return "OP_LE";
        case OP_GT: return "OP_GT";
        case OP_GE: return "OP_GE";
        case OP_EQUALS: return "OP_EQUALS";
        case OP_CONCAT: return "OP_CONCAT";
        case OP_PRINT: return "OP_PRINT";
        case OP_PRINTLN: return "OP_PRINTLN";
        case OP_JUMP: return "OP_JUMP";
        case OP_JUMP_IF_TRUE: return "OP_JUMP_IF_TRUE";
        case OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
        case OP_JUMP_IF_FALSE_POP: return "OP_JUMP_IF_FALSE_POP";
        case OP_LOOP: return "OP_LOOP";
        case OP_RETURN: return "OP_RETURN";
        case OP_RETURN_VALUE: return "OP_RETURN_VALUE";
    }
}

Code code_new() {
    return DYNAMIC_ARRAY_NEW(Code);
}

void code_destruct(Code self) {
    free(self.data);
}

void code_emit(Code* self, Opcode opcode) {
    DYNARRAY_PTR_PUSH(self, opcode, 16);
}

void code_emit_args(Code* self, Opcode opcode, size_t args_length, byte args[]) {
    code_emit(self, opcode);
    for(size_t i = 0; i < args_length; i++) code_emit(self, args[i]);
}

size_t code_emit_jump(Code* self, Opcode opcode) {
    size_t jump_index = self->length;
    code_emit_args(self, opcode, 2, (byte[]) {0, 0});
    return jump_index;
}

void code_emit_loop(Code* self, size_t loop_start) {
    uint16_t loop_length = self->length - loop_start;
    code_emit_args(self, OP_LOOP, 2, TO_LE_2_BYTES(loop_length));
}

void code_patch_jump(Code* self, size_t jump_index) {
    uint16_t jump_arg = self->length - jump_index;
    self->data[jump_index + 1] = jump_arg & 0xFF;
    self->data[jump_index + 2] = jump_arg >> 8;
}