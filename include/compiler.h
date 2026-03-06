#pragma once

#include <stdbool.h>

#include "node.h"
#include "chunk.h"
#include "str_pool.h"

void compile(NodeArray* nodes, Chunk* chunk, StrPool* str_pool, bool* had_error);