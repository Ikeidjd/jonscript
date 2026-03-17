#include "chunk.h"

#include <stdlib.h>

Chunk chunk_new() {
    return (Chunk) {
        .constants = object_array_new(),
        .code = code_new()
    };
}

void chunk_destruct(Chunk self) {
    code_destruct(self.code);

#ifdef DEBUG_PRINT_FREED_OBJECTS
    printf("Freeing compile-time objects...\n");
#endif
    for(size_t i = 0; i < self.constants.length; i++) {
        if(self.constants.data[i].type != VALUE_OBJECT) continue;

        Object* object = self.constants.data[i].as.object;
    #ifdef DEBUG_PRINT_FREED_OBJECTS
        printf("Freeing object %p of type %s: ", object, object_type_to_string(object->type));
        object_println(object, NULL);
    #endif
        Object* next = object->next;
        object_free(object);
        object = next;
    }

    object_array_destruct(&self.constants);
}

void chunk_free(Chunk* self) {
    chunk_destruct(*self);
    free(self);
}

void chunk_emit_load_value_op(Chunk* self, Value value) {
    object_array_push(&self->constants, value);
    size_t index = self->constants.length - 1;
    code_emit_args(&self->code, OP_LOAD_VALUE, 2, TO_LE_2_BYTES(index));
}

void chunk_emit_load_closure_op(Chunk* self, ObjectFunction* function) {
    object_array_push(&self->constants, value_new_object((Object*) function));
    size_t index = self->constants.length - 1;
    code_emit_args(&self->code, OP_LOAD_CLOSURE, 2, TO_LE_2_BYTES(index));
}

static size_t chunk_display_monoarg_op(Chunk* self, Opcode op, size_t byte_count, size_t offset) {
    printf("%s", opcode_to_string(op));
    offset++;

    uint64_t value = 0;
    for(size_t i = 0; i < byte_count; i++) {
        value |= (self->code.data[offset] << (i * 8));
        offset++;
    }

    printf(" %llu\n", value);

    return offset;
}

static size_t chunk_display_simple_op(Opcode op, size_t offset) {
    printf("%s\n", opcode_to_string(op));
    return offset + 1;
}

size_t chunk_disassemble_op(Chunk* self, size_t offset) {
    printf("%04d ", offset);
    Opcode op = self->code.data[offset];
    switch(op) {
        case OP_LOAD_TRUE:
        case OP_LOAD_FALSE:
        case OP_INDEX_GET:
        case OP_INDEX_SET:
        case OP_TUPLE_MEMBER_GET:
        case OP_TUPLE_MEMBER_SET:
        case OP_POP:
        case OP_ARRAYIFY_LENGTH:
        case OP_DEEP_COPY:
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
        case OP_BITWISE_AND:
        case OP_BITWISE_OR:
        case OP_LT:
        case OP_LE:
        case OP_GT:
        case OP_GE:
        case OP_EQUALS:
        case OP_CONCAT:
        case OP_PRINT:
        case OP_PRINTLN:
        case OP_RETURN:
        case OP_RETURN_VALUE:
            return chunk_display_simple_op(op, offset);
        case OP_LOAD_BYTE:
        case OP_CALL:
            return chunk_display_monoarg_op(self, op, 1, offset);
        case OP_LOAD_VALUE:
        case OP_LOAD_CLOSURE:
        case OP_LOCAL_GET:
        case OP_LOCAL_SET:
        case OP_CAPTURE_GET:
        case OP_CAPTURE_SET:
        case OP_POP_N:
        case OP_ARRAYIFY_LIST:
        case OP_TUPLIFY:
        case OP_JUMP:
        case OP_JUMP_IF_TRUE:
        case OP_JUMP_IF_FALSE:
        case OP_JUMP_IF_FALSE_POP:
        case OP_LOOP:
            return chunk_display_monoarg_op(self, op, 2, offset);
    }
}

void chunk_disassemble(Chunk* self) {
    for(size_t i = 0; i < self->constants.length; i++) {
        TempString str = value_to_repr(self->constants.data[i], NULL);
        printf("[%.*s]", str.length, str.data);
        temp_string_destruct(str);
    }
    if(self->constants.length == 0) printf("[]");
    printf("\n");

    size_t offset = 0;
    while(offset < self->code.length) offset = chunk_disassemble_op(self, offset);

    for(size_t i = 0; i < self->constants.length; i++) {
        if(self->constants.data[i].type == VALUE_OBJECT && self->constants.data[i].as.object->type == OBJECT_FUNCTION) {
            printf("\n");
            object_println(self->constants.data[i].as.object, NULL);
            ObjectFunction* function = AS_FUNCTION(self->constants.data[i]);
            chunk_disassemble(function->chunk);
        }
    }
}