#pragma once

#include "token.h"

TokenArray lex(const char* filepath); // Remember to destruct the returned TokenArray