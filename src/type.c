#include "type.h"

#include "hash_set.h"

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
        case TTYPE_TUPLE: {
            TupleType* a = (TupleType*) left;
            TupleType* b = (TupleType*) right;

            if(a->types_length != b->types_length) return false;

            for(size_t i = 0; i < a->types_length; i++) {
                if(a->types[i] != b->types[i]) return false;
            }

            return true;
        }
        case TTYPE_FUNCTION: {
            FunctionType* a = (FunctionType*) left;
            FunctionType* b = (FunctionType*) right;

            if(a->params_length != b->params_length || a->return_type != b->return_type) return false;

            for(size_t i = 0; i < a->params_length; i++) {
                if(a->param_types[i] != b->param_types[i]) return false;
            }

            return true;
        }
        case TTYPE_VOID:
            return true;
    }
}

HASH_SET_FIND_FUNCTION(TypeHashSet, Type*, type_equals, type_hash_set)
HASH_SET_INSERT_NO_RESIZE_AND_NO_CHECK_DUPLICATES_FUNCTION(TypeHashSet, Type*, type_hash_set)
HASH_SET_RESIZE_FUNCTION(TypeHashSet, Type*, type_hash_set)

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

TupleType* tuple_type_new(TypeHashSet* set, Type* types[MAX_PARAM_LENGTH], size_t types_length) {
    TupleType* self = (TupleType*) type_arena_alloc(set->arena, sizeof(TupleType));

    *self = (TupleType) {
        .base = {
            .type = TTYPE_TUPLE,
            .hash = 0
        },
        .types_length = types_length
    };

    for(size_t i = 0; i < types_length; i++) {
        self->types[i] = types[i];
        self->base.hash ^= (uint64_t) types[i];
    }

    return (TupleType*) type_hash_set_insert(set, (Type*) self, sizeof(TupleType));
}

FunctionType* function_type_new(TypeHashSet* set, Type* param_types[MAX_PARAM_LENGTH], size_t params_length, Type* return_type) {
    FunctionType* self = (FunctionType*) type_arena_alloc(set->arena, sizeof(FunctionType));

    *self = (FunctionType) {
        .base = {
            .type = TTYPE_FUNCTION,
            .hash = (uint64_t) return_type
        },
        .params_length = params_length,
        .return_type = return_type
    };

    for(size_t i = 0; i < params_length; i++) {
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

bool is_void(Type* self) {
    return self->type == TTYPE_VOID;
}

bool is_any_primitive(Type* self) {
    return self->type == TTYPE_PRIMITIVE;
}

bool is_primitive(Type* self, PrimitiveType type) {
    return is_any_primitive(self) && ((PrimitiveTypeObj*) self)->type == type;
}

bool is_array(Type* self) {
    return self->type == TTYPE_ARRAY;
}

bool is_tuple(Type* self) {
    return self->type == TTYPE_TUPLE;
}

bool is_function(Type* self) {
    return self->type == TTYPE_FUNCTION;
}