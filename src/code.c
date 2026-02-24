#include "code.h"

#include "dynamic_array.h"

void opcode_fprint(FILE* file, Opcode self) {
    switch(self) {
        case OP_LOAD_BYTE:
            fprintf(file, "OP_LOAD_BYTE");
            break;
        case OP_LOAD_VALUE:
            fprintf(file, "OP_LOAD_VALUE");
            break;
        case OP_LOAD_STACK:
            fprintf(file, "OP_LOAD_STACK");
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
        default:
            fprintf(file, "Unknown Opcode %d", self);
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
    return (Code) {
        .data = NULL,
        .length = 0,
        .capacity = 0
    };
}

void code_destruct(Code self) {
    free(self.data);
}

void code_push(Code* self, Opcode opcode) {
    PUSH(self, opcode, 16);
}

void code_push_args(Code* self, Opcode opcode, size_t args_length, byte args[]) {
    code_push(self, opcode);
    for(size_t i = 0; i < args_length; i++) code_push(self, args[i]);
}