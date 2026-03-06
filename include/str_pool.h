#pragma once

#include "object.h"

typedef struct StrPool {
    ObjectStr** data;
    size_t size;
    size_t occupied;
} StrPool;

StrPool str_pool_new();
void str_pool_destruct(StrPool self);

// Frees str and its data if it's already found
ObjectStr* str_pool_insert(StrPool* self, ObjectStr* str);