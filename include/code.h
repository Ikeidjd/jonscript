#pragma once

#include <stdio.h>

typedef unsigned char byte;

typedef enum Opcode {
    OP_LOAD_1B,
    OP_LOAD_2B,
    OP_LOAD_4B,
    OP_LOAD_8B,
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
void code_push(Code* self, Opcode opcode);
void code_push_args(Code* self, Opcode opcode, size_t args_length, byte args[]);