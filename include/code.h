#pragma once

#include <stdio.h>

typedef unsigned char byte;

#define TO_LE_2_BYTES(n) (byte[]) {(n) & 0xFF, ((n) >> 8) & 0xFF}

typedef enum Opcode {
    OP_LOAD_BYTE,
    OP_LOAD_VALUE,
    OP_LOAD_TRUE,
    OP_LOAD_FALSE,

    OP_LOCAL_GET,
    OP_LOCAL_SET,
    OP_INDEX_GET,
    OP_INDEX_SET,

    OP_POP,

    OP_ARRAYIFY_LIST,
    OP_ARRAYIFY_LENGTH,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,

    OP_BITWISE_AND,
    OP_BITWISE_OR,

    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    OP_EQUALS,

    OP_JUMP,
    OP_JUMP_IF_TRUE,
    OP_JUMP_IF_FALSE
} Opcode;

void opcode_fprint(FILE* file, Opcode self);
void opcode_fprintln(FILE* file, Opcode self);
void opcode_print(Opcode self);
void opcode_println(Opcode self);

typedef struct Code {
    byte* data;
    size_t length;
    size_t capacity;
} Code;

Code code_new();
void code_destruct(Code self);

void code_emit(Code* self, Opcode opcode);
void code_emit_args(Code* self, Opcode opcode, size_t args_length, byte args[]);

size_t code_emit_jump(Code* self, Opcode opcode);
void code_patch_jump(Code* self, size_t jump);