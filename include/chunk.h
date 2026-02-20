#pragma once

#include "value.h"
#include "code.h"

typedef struct Chunk {
    ValueArray values;
    Code code;
} Chunk;

Chunk chunk_new();
void chunk_disassemble(Chunk* self);