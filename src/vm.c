#include "vm.h"

#include <stdlib.h>
#include <stdint.h>

#include "stack.h"

typedef struct VM {
    StrPool str_pool;
    Object* object;
    size_t sp;
    Value stack[STACK_SIZE];
} VM;

static void vm_free(VM* self) {
    Object* object = self->object;

    printf("Freeing runtime objects...\n");
    while(object != NULL) {
        printf("Freeing object %p of type %d: ", object, object->type);
        object_println(object);
        Object* next = object->next;
        object_free(object);
        object = next;
    }
    printf("\n");

    free(self);
}

static void vm_push(VM* self, Value value) {
    self->stack[self->sp++] = value;
}

static Value vm_pop(VM* self) {
    return self->stack[--self->sp];
}

static void vm_pop_n(VM* self, size_t n) {
    self->sp -= n;
}

static Value vm_top(VM* self) {
    return self->stack[self->sp - 1];
}

static ObjectStr* vm_object_str_register(VM* self, ObjectStr* str) {
    ObjectStr* old_str = str;
    str = str_pool_insert(&self->str_pool, str);

    // If string wasn't found
    if(str == old_str) {
        str->base.next = self->object;
        self->object = (Object*) str;
    }

    return str;
}

static ObjectArray* vm_object_array_malloc(VM* self, size_t element_count) {
    ObjectArray* out = malloc(sizeof(ObjectArray));
    *out = object_array_of_length(element_count);

    out->base.next = self->object;
    self->object = (Object*) out;

    return out;
}

static void vm_run(VM* self, Chunk* chunk) {
#define NUMERICAL_BIN_OP(operator) \
do { \
    int64_t b = vm_pop(self).as.integer; \
    int64_t a = vm_pop(self).as.integer; \
    int64_t result = a operator b; \
    vm_push(self, value_new_int(result)); \
} while(0)

#define COMPARISON_OP(operator) \
    do { \
        int64_t b = vm_pop(self).as.integer; \
        int64_t a = vm_pop(self).as.integer; \
        bool result = a operator b; \
        vm_push(self, value_new_bool(result)); \
    } while(0)

// Each READ() increments ip by one, so substracting 3 balances it out (the READ() that's missing here is the one that reads the opcode)
#define JUMP(condition) \
do { \
    size_t jump_length = READ(); \
    jump_length |= READ() << 8; \
    if(condition) ip += jump_length - 3; \
} while(0)

#define READ() chunk->code.data[ip++]

    size_t ip = 0;
    while(ip < chunk->code.length) {
        for(size_t i = 0; i < self->sp; i++) {
            printf("[");
            value_print(self->stack[i]);
            printf("]");
        }

        if(self->sp == 0) printf("[]");
        printf("\n");

        chunk_disassemble_op(chunk, ip);

        Opcode op = READ();
        switch(op) {
            case OP_LOAD_BYTE:
                vm_push(self, value_new_int((int64_t) READ()));
                break;
            case OP_LOAD_VALUE: {
                size_t index = READ();
                index |= (READ() << 8);
                vm_push(self, chunk->constants.data[index]);
                break;
            }
            case OP_LOAD_TRUE: {
                vm_push(self, value_new_bool(true));
                break;
            }
            case OP_LOAD_FALSE: {
                vm_push(self, value_new_bool(false));
                break;
            }
            case OP_LOCAL_GET: {
                size_t index = READ();
                index |= (READ() << 8);
                vm_push(self, self->stack[index]);
                break;
            }
            case OP_LOCAL_SET: {
                size_t index = READ();
                index |= (READ() << 8);
                self->stack[index] = vm_pop(self);
                break;
            }
            case OP_INDEX_GET: {
                size_t index = vm_pop(self).as.integer;
                ObjectArray* array = AS_ARRAY(vm_pop(self));
                vm_push(self, array->data[index]);
                break;
            }
            case OP_INDEX_SET: {
                // Yeah, the order of arguments is a little weird, but it was easier to compile that way
                size_t index = vm_pop(self).as.integer;
                ObjectArray* array = AS_ARRAY(vm_pop(self));
                Value value = vm_pop(self);
                array->data[index] = value;
                break;
            }
            case OP_POP:
                vm_pop(self);
                break;
            case OP_POP_N: {
                size_t n = READ();
                n |= (READ() << 8);
                vm_pop_n(self, n);
                break;
            }
            case OP_ARRAYIFY_LIST: {
                size_t length = READ();
                length |= (READ() << 8);
                ObjectArray* array = vm_object_array_malloc(self, length);
                for(size_t i = 0; i < length; i++) array->data[i] = self->stack[i + self->sp - length];
                vm_pop_n(self, length);
                vm_push(self, value_new_object((Object*) array));
                break;
            }
            case OP_ARRAYIFY_LENGTH: {
                size_t length = vm_pop(self).as.integer;
                ObjectArray* array = vm_object_array_malloc(self, length);
                Value value = vm_pop(self);
                for(size_t i = 0; i < length; i++) array->data[i] = value;
                vm_push(self, value_new_object((Object*) array));
                break;
            }
            case OP_ADD:
                NUMERICAL_BIN_OP(+);
                break;
            case OP_SUB:
                NUMERICAL_BIN_OP(-);
                break;
            case OP_MUL:
                NUMERICAL_BIN_OP(*);
                break;
            case OP_DIV:
                NUMERICAL_BIN_OP(/);
                break;
            case OP_MOD:
                NUMERICAL_BIN_OP(%);
                break;
            case OP_BITWISE_AND:
                NUMERICAL_BIN_OP(&);
                break;
            case OP_BITWISE_OR:
                NUMERICAL_BIN_OP(|);
                break;
            case OP_LT:
                COMPARISON_OP(<);
                break;
            case OP_LE:
                COMPARISON_OP(<=);
                break;
            case OP_GT:
                COMPARISON_OP(>);
                break;
            case OP_GE:
                COMPARISON_OP(>=);
                break;
            case OP_CONCAT: {
                Value b = vm_pop(self);
                Value a = vm_pop(self);

                size_t a_length;
                bool a_should_free;
                char* a_str = value_to_string(a, &a_length, &a_should_free);

                size_t b_length;
                bool b_should_free;
                char* b_str = value_to_string(b, &b_length, &b_should_free);

                char* data = malloc(a_length + b_length);
                for(size_t i = 0; i < a_length; i++) data[i] = a_str[i];
                for(size_t i = 0; i < b_length; i++) data[i + a_length] = b_str[i];

                if(a_should_free) free(a_str);
                if(b_should_free) free(b_str);

                ObjectStr* result = malloc(sizeof(ObjectStr));
                *result = object_str_new(data, a_length + b_length);
                result = vm_object_str_register(self, result);

                vm_push(self, value_new_object((Object*) result));

                break;
            }
            case OP_PRINT:
                value_print(vm_pop(self));
                break;
            case OP_PRINTLN:
                value_println(vm_pop(self));
                break;
            case OP_EQUALS: {
                Value b = vm_pop(self);
                Value a = vm_pop(self);
                bool result = value_equals(a, b);
                vm_push(self, value_new_bool(result));
                break;
            }
            case OP_JUMP:
                JUMP(true);
                break;
            case OP_JUMP_IF_TRUE:
                JUMP(vm_top(self).as.boolean);
                break;
            case OP_JUMP_IF_FALSE:
                JUMP(!vm_top(self).as.boolean);
                break;
            case OP_JUMP_IF_FALSE_POP:
                JUMP(!vm_top(self).as.boolean);
                vm_pop(self);
                break;
            case OP_LOOP: {
                size_t loop_length = READ();
                loop_length |= READ() << 8;
                ip -= loop_length + 3; // Each READ() increments ip by one, so adding 3 balances it out (the READ() that's missing here is the one that reads the opcode)
            }
        }
    }
}

void run(Chunk* chunk, StrPool str_pool) {
    VM* self = (VM*) malloc(sizeof(VM));
    self->str_pool = str_pool;
    self->object = NULL;
    self->sp = 0;
    vm_run(self, chunk);

    for(size_t i = 0; i < self->sp; i++) {
        printf("[");
        value_print(self->stack[i]);
        printf("]");
    }
    printf("\n\n");

    vm_free(self);
}