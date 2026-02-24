#pragma once

#include <stdbool.h>

#include "token.h"
#include "node.h"

NodeArray parse(const TokenArray* tokens, bool* had_error);