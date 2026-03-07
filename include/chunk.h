#pragma once

#include "token.h"
#include "value.h"
#include "object.h"
#include "code.h"

typedef struct Chunk {
    ObjectArray constants;
    Code code;
} Chunk;

Chunk chunk_new();

void chunk_destruct(Chunk self);
void chunk_free(Chunk* self);

void chunk_emit_load_value_op(Chunk* self, Value value);
void chunk_emit_load_str_op(Chunk* self, Token token);

size_t chunk_disassemble_op(Chunk* self, size_t offset);
void chunk_disassemble(Chunk* self);