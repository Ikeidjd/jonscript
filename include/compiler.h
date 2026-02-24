#pragma once

#include <stdbool.h>

#include "node.h"
#include "chunk.h"

Chunk compile(NodeArray* nodes, bool* had_error);