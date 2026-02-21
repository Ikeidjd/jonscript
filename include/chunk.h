#pragma once

#include "value.h"
#include "code.h"

typedef struct Chunk {
    ValueArray values;
    Code code;
} Chunk;

Chunk chunk_new();
void chunk_destruct(Chunk chunk);

void chunk_emit_load_value_op(Chunk* self, Value value);

size_t chunk_disassemble_op(Chunk* self, size_t offset);
void chunk_disassemble(Chunk* self);