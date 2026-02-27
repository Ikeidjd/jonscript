#include "code.h"

#include <stdint.h>

#include "dynamic_array.h"

void opcode_fprint(FILE* file, Opcode self) {
    switch(self) {
        case OP_LOAD_BYTE:
            fprintf(file, "OP_LOAD_BYTE");
            break;
        case OP_LOAD_VALUE:
            fprintf(file, "OP_LOAD_VALUE");
            break;
        case OP_LOAD_TRUE:
            fprintf(file, "OP_LOAD_TRUE");
            break;
        case OP_LOAD_FALSE:
            fprintf(file, "OP_LOAD_FALSE");
            break;
        case OP_LOCAL_GET:
            fprintf(file, "OP_LOCAL_GET");
            break;
        case OP_LOCAL_SET:
            fprintf(file, "OP_LOCAL_SET");
            break;
        case OP_INDEX_GET:
            fprintf(file, "OP_INDEX_GET");
            break;
        case OP_INDEX_SET:
            fprintf(file, "OP_INDEX_SET");
            break;
        case OP_POP:
            fprintf(file, "OP_POP");
            break;
        case OP_ARRAYIFY_LIST:
            fprintf(file, "OP_ARRAYIFY_LIST");
            break;
        case OP_ARRAYIFY_LENGTH:
            fprintf(file, "OP_ARRAYIFY_LENGTH");
            break;
        case OP_ADD:
            fprintf(file, "OP_ADD");
            break;
        case OP_SUB:
            fprintf(file, "OP_SUB");
            break;
        case OP_MUL:
            fprintf(file, "OP_MUL");
            break;
        case OP_DIV:
            fprintf(file, "OP_DIV");
            break;
        case OP_MOD:
            fprintf(file, "OP_MOD");
            break;
        case OP_BITWISE_AND:
            fprintf(file, "OP_BITWISE_AND");
            break;
        case OP_BITWISE_OR:
            fprintf(file, "OP_BITWISE_OR");
            break;
        case OP_LT:
            fprintf(file, "OP_LT");
            break;
        case OP_LE:
            fprintf(file, "OP_LE");
            break;
        case OP_GT:
            fprintf(file, "OP_GT");
            break;
        case OP_GE:
            fprintf(file, "OP_GE");
            break;
        case OP_EQUALS:
            fprintf(file, "OP_EQUALS");
            break;
        case OP_JUMP:
            fprintf(file, "OP_JUMP");
            break;
        case OP_JUMP_IF_TRUE:
            fprintf(file, "OP_JUMP_IF_TRUE");
            break;
        case OP_JUMP_IF_FALSE:
            fprintf(file, "OP_JUMP_IF_FALSE");
            break;
    }
}

void opcode_fprintln(FILE* file, Opcode self) {
    opcode_fprint(file, self);
    fprintf(file, "\n");
}

void opcode_print(Opcode self) {
    opcode_fprint(stdout, self);
}

void opcode_println(Opcode self) {
    opcode_fprintln(stdout, self);
}

Code code_new() {
    return DYNAMIC_ARRAY_NEW(Code);
}

void code_destruct(Code self) {
    free(self.data);
}

void code_emit(Code* self, Opcode opcode) {
    PUSH(self, opcode, 16);
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

void code_patch_jump(Code* self, size_t jump) {
    uint16_t jump_arg = self->length - jump;
    self->data[jump + 1] = jump_arg & 0xFF;
    self->data[jump + 2] = jump_arg >> 8;
}