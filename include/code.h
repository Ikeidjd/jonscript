#pragma once

#include <stdio.h>

typedef unsigned char byte;

#define TO_LE_2_BYTES(n) (byte[]) {(n) & 0xFF, ((n) >> 8) & 0xFF}

typedef enum Opcode {
    OP_LOAD_BYTE,
    OP_LOAD_VALUE,
    OP_LOAD_TRUE,
    OP_LOAD_FALSE,
    OP_LOAD_CLOSURE,

    OP_LOCAL_GET,
    OP_LOCAL_SET,
    OP_CAPTURE_GET,
    OP_CAPTURE_SET,
    OP_INDEX_GET,
    OP_INDEX_SET,

    OP_CALL,

    OP_POP,
    OP_POP_N,

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

    OP_CONCAT,

    OP_PRINT,
    OP_PRINTLN,

    OP_JUMP,
    OP_JUMP_IF_TRUE,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_FALSE_POP,

    OP_LOOP,

    OP_RETURN,
    OP_RETURN_VALUE
} Opcode;

char* opcode_to_string(Opcode self);

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
void code_patch_jump(Code* self, size_t jump_index);

void code_emit_loop(Code* self, size_t loop_start);