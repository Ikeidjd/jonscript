#include "type.h"

#include "dynamic_array.h"

#define TYPE_ARENA_SIZE 0x10000

typedef struct TypeArena {
    byte data[TYPE_ARENA_SIZE];
    size_t top;
} TypeArena;

static TypeArena type_arena_new() {
    return (TypeArena) { .top = 0 };
}

static Type* type_arena_alloc(TypeArena* self, size_t size_in_bytes) {
    // We'll just hope and dream that the end of self->data won't ever be reached. I mean, how many types can you have anyway?
    self->top += size_in_bytes;
    return (Type*) &self->data[self->top - size_in_bytes];
}

static void type_arena_dealloc(TypeArena* self, size_t size_in_bytes) {
    self->top -= size_in_bytes;
}

TypeHashSet type_hash_set_new() {
    TypeArena* arena = (TypeArena*) malloc(sizeof(TypeArena));
    *arena = type_arena_new();
    return (TypeHashSet) {
        .arena = arena,
        .data = NULL,
        .size = 0,
        .occupied = 0
    };
}

void type_hash_set_destruct(TypeHashSet self) {
    free(self.arena);
    free(self.data);
}

static bool type_equals(Type* left, Type* right) {
    if(left->type != right->type) return false;

    switch(left->type) {
        case TTYPE_PRIMITIVE:
            return ((PrimitiveTypeObj*) left)->type == ((PrimitiveTypeObj*) right)->type;
        case TTYPE_ARRAY:
            return ((ArrayType*) left)->type == ((ArrayType*) right)->type;
        case TTYPE_FUNCTION: {
            FunctionType* a = (FunctionType*) left;
            FunctionType* b = (FunctionType*) right;

            if(a->param_count != b->param_count || a->return_type != b->return_type) return false;

            for(size_t i = 0; i < a->param_count; i++) {
                if(a->param_types[i] != b->param_types[i]) return false;
            }

            return true;
        }
        case TTYPE_VOID:
            return true;
    }
}

Type* type_hash_set_find(TypeHashSet* self, Type* type) {
    size_t index = (type->hash & (self->size - 1)); // Like type->hash % self->capacity but more optimized. Only works with powers of two, though
    for(size_t counter = 0; counter < self->size && self->data[index] != NULL; counter++) {
        if(type_equals(self->data[index], type)) return self->data[index];
        index = ((index + 1) & (self->size - 1));
    }
    return NULL;
}

// A self-insert without resizing, huh... what would be a self-insert with resizing, an inflation fantasy? Really makes you think. The no_check_duplicates could be selfcest
static void type_hash_set_insert_no_resize_and_no_check_duplicates(TypeHashSet* self, Type* type) {
    size_t index = (type->hash & (self->size - 1)); // Like type->hash % self->capacity but more optimized. Only works with powers of two, though
    for(size_t counter = 0; counter < self->size && self->data[index] != NULL; counter++) index = ((index + 1) & (self->size - 1));
    self->data[index] = type;
    self->occupied++;
}

static void type_hash_set_resize(TypeHashSet* self) {
    size_t old_size = self->size;
    Type** old_data = self->data;

    if(self->size == 0) self->size = 64;
    else self->size *= 2;

    self->data = malloc(self->size * sizeof(Type*));

    for(size_t i = 0; i < self->size; i++) self->data[i] = NULL;

    for(size_t i = 0; i < old_size; i++) {
        if(old_data[i] != NULL) type_hash_set_insert_no_resize_and_no_check_duplicates(self, old_data[i]);
    }

    free(old_data);
}

// Haha, self-insert reference
static Type* type_hash_set_insert(TypeHashSet* self, Type* type, size_t size_in_bytes_to_deallocate_in_case_of_duplicate) {
    if(self->size <= self->occupied * 2) type_hash_set_resize(self);

    Type* found = type_hash_set_find(self, type);
    if(found != NULL) {
        type_arena_dealloc(self->arena, size_in_bytes_to_deallocate_in_case_of_duplicate);
        return found;
    }

    type_hash_set_insert_no_resize_and_no_check_duplicates(self, type);
    return type;
}

PrimitiveTypeObj* primitive_type_new(TypeHashSet* set, PrimitiveType type) {
    PrimitiveTypeObj* self = (PrimitiveTypeObj*) type_arena_alloc(set->arena, sizeof(PrimitiveTypeObj));

    *self = (PrimitiveTypeObj) {
        .base = {
            .type = TTYPE_PRIMITIVE,
            .hash = type + 1 // .hash = 0 corresponds to void
        },
        .type = type
    };

    return (PrimitiveTypeObj*) type_hash_set_insert(set, (Type*) self, sizeof(PrimitiveTypeObj));
}

ArrayType* array_type_new(TypeHashSet* set, Type* type) {
    ArrayType* self = (ArrayType*) type_arena_alloc(set->arena, sizeof(ArrayType));

    *self = (ArrayType) {
        .base = {
            .type = TTYPE_ARRAY,
            .hash = (((uint64_t) 'a' << 24) | ((uint64_t) type & 0xFFFFFF))
        },
        .type = type
    };

    return (ArrayType*) type_hash_set_insert(set, (Type*) self, sizeof(ArrayType));
}

FunctionType* function_type_new(TypeHashSet* set, Type* param_types[MAX_PARAM_COUNT], size_t param_count, Type* return_type) {
    FunctionType* self = (FunctionType*) type_arena_alloc(set->arena, sizeof(FunctionType));

    *self = (FunctionType) {
        .base = {
            .type = TTYPE_FUNCTION,
            .hash = 0
        },
        .param_count = param_count,
        .return_type = return_type
    };

    printf("HERE: %zu\n", self->param_count);

    for(size_t i = 0; i < param_count; i++) {
        self->param_types[i] = param_types[i];
        self->base.hash ^= (uint64_t) param_types[i]; // Didn't bother to come up with anything better
    }

    return (FunctionType*) type_hash_set_insert(set, (Type*) self, sizeof(FunctionType));
}

Type* void_type_new(TypeHashSet* set) {
    Type* self = type_arena_alloc(set->arena, sizeof(Type));

    *self = (Type) {
        .type = TTYPE_VOID,
        .hash = 0
    };

    return type_hash_set_insert(set, self, sizeof(Type));
}

bool is_primitive(Type* self, PrimitiveType type) {
    return self->hash == type;
}

bool is_array(Type* self) {
    return self->type == TTYPE_ARRAY;
}