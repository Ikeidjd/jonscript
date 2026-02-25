#pragma once

#include <stdio.h>

typedef unsigned char byte;

#define TO_LE_2_BYTES(n) (byte[]) {(n) & 0xFF, ((n) >> 8) & 0xFF}

typedef enum Opcode {
    OP_LOAD_BYTE,
    OP_LOAD_VALUE,

    OP_LOCAL_GET,
    OP_LOCAL_SET,
    OP_INDEX_GET,
    OP_INDEX_SET,

    OP_ARRAYIFY_LIST,
    OP_ARRAYIFY_LENGTH,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD
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

void code_push(Code* self, Opcode opcode);
void code_push_args(Code* self, Opcode opcode, size_t args_length, byte args[]);