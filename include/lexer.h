#pragma once

#include <stdbool.h>

#include "token.h"

TokenArray lex(const char* filepath, bool* had_error); // Remember to destruct the returned TokenArray