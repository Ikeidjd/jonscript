#pragma once

#include "chunk.h"
#include "str_pool.h"

#include "stack.h"

typedef struct VM {
    StrPool str_pool;
    Object* object;
    size_t sp;
    Value stack[STACK_SIZE];
} VM;

void vm_push(VM* self, Value value);
void vm_object_non_str_register(VM* self, Object* object);
ObjectArray* vm_object_array_malloc(VM* self, size_t element_count);

void run(Chunk* chunk, StrPool str_pool);