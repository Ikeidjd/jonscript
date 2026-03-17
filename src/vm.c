#include "vm.h"

#include <stdlib.h>
#include <stdint.h>

static void vm_free(VM* self) {
    Object* object = self->object;

#ifdef DEBUG_PRINT_FREED_OBJECTS
    printf("Freeing runtime objects...\n");
#endif
    while(object != NULL) {
    #ifdef DEBUG_PRINT_FREED_OBJECTS
        printf("Freeing object %p of type %s: ", object, object_type_to_string(object->type));
        object_println(object, NULL);
    #endif
        Object* next = object->next;
        object_free(object);
        object = next;
    }
#ifdef DEBUG_PRINT_FREED_OBJECTS
    printf("\n");
#endif

    free(self);
}

void vm_push(VM* self, Value value) {
    self->stack[self->sp++] = value;
}

static void vm_pop_n(VM* self, size_t n) {
    self->sp -= n;
}

static Value vm_pop(VM* self) {
    Value value = self->stack[--self->sp];
    if(value.type == VALUE_TUPLE) {
        ValueTuple tuple = value.as.tuple;
        // If this is the original tuple (i.e., we're dealing with an rvalue)
        if(tuple.value_stack_index == self->sp - tuple.length) vm_pop_n(self, tuple.length);
    }
    return value;
}

static Value vm_top(VM* self) {
    return self->stack[self->sp - 1];
}

void vm_object_non_str_register(VM* self, Object* object) {
    object->next = self->object;
    self->object = object;
}

static ObjectStr* vm_object_str_register(VM* self, ObjectStr* str) {
    ObjectStr* old_str = str;
    str = str_pool_insert(&self->str_pool, str);

    // If string wasn't found, i.e., it's not a duplicate
    if(str == old_str) vm_object_non_str_register(self, (Object*) str);

    return str;
}

ObjectArray* vm_object_array_malloc(VM* self, size_t element_count) {
    ObjectArray* array = malloc(sizeof(ObjectArray));
    *array = object_array_of_length(element_count);
    vm_object_non_str_register(self, (Object*) array);
    return array;
}

static ObjectCapture* vm_capture(VM* self, size_t index) {
    if(IS_CAPTURE(self->stack[index])) return AS_CAPTURE(self->stack[index]);

    ObjectCapture* capture = malloc(sizeof(ObjectCapture));
    *capture = object_capture_new(self->stack[index]);
    self->stack[index] = value_new_object((Object*) capture);

    vm_object_non_str_register(self, (Object*) capture);
    return capture;
}

static ObjectClosure* vm_object_closure_malloc(VM* self, ObjectFunction* function) {
    ObjectClosure* closure = malloc(sizeof(ObjectClosure));
    *closure = object_closure_new(function);

    for(size_t i = 0; i < function->captured_locals_length; i++) {
        closure->captured_locals[i] = vm_capture(self, self->sp - function->captured_locals[i]);
    }

    vm_object_non_str_register(self, (Object*) closure);
    return closure;
}

static void vm_run(VM* self, ObjectClosure* closure, size_t stack_frame_start_index) {
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

#define READ() closure->chunk->code.data[ip++]

    size_t ip = 0;
    while(ip < closure->chunk->code.length) {
    #ifdef DEBUG_TRACE_EXECUTION
        for(size_t i = 0; i < self->sp; i++) {
            if(i > 0 && i == stack_frame_start_index) printf("\n");
            TempString str = value_to_repr(self->stack[i], self->stack);
            printf("[%.*s]", str.length, str.data);
            temp_string_destruct(str);
        }

        if(self->sp == 0) printf("[]");
        printf("\n");

        chunk_disassemble_op(closure->chunk, ip);
    #endif

        Opcode op = READ();
        switch(op) {
            case OP_LOAD_BYTE:
                vm_push(self, value_new_int((int64_t) READ()));
                break;
            case OP_LOAD_VALUE: {
                size_t index = READ();
                index |= (READ() << 8);
                vm_push(self, closure->chunk->constants.data[index]);
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
            case OP_LOAD_CLOSURE: {
                size_t index = READ();
                index |= (READ() << 8);
                ObjectFunction* function = AS_FUNCTION(closure->chunk->constants.data[index]);
                ObjectClosure* closure = vm_object_closure_malloc(self, function);
                vm_push(self, value_new_object((Object*) closure));
                break;
            }
            case OP_LOCAL_GET: {
                size_t index = READ();
                index |= (READ() << 8);
                Value value = self->stack[index + stack_frame_start_index];
                if(IS_CAPTURE(value)) value = AS_CAPTURE(value)->captured_value;
                vm_push(self, value);
                break;
            }
            case OP_LOCAL_SET: {
                size_t index = READ();
                index |= (READ() << 8);
                Value value = self->stack[index + stack_frame_start_index];
                if(IS_CAPTURE(value)) AS_CAPTURE(value)->captured_value = vm_pop(self);
                else self->stack[index + stack_frame_start_index] = vm_pop(self);
                break;
            }
            case OP_CAPTURE_GET: {
                size_t index = READ();
                index |= (READ() << 8);
                vm_push(self, closure->captured_locals[index]->captured_value);
                break;
            }
            case OP_CAPTURE_SET: {
                size_t index = READ();
                index |= (READ() << 8);
                closure->captured_locals[index]->captured_value = vm_pop(self);
                break;
            }
            case OP_INDEX_GET: {
                int64_t index = vm_pop(self).as.integer;
                ObjectArray* array = AS_ARRAY(vm_pop(self));
                vm_push(self, array->data[index]);
                break;
            }
            case OP_INDEX_SET: {
                // Yeah, the order of arguments is a little weird, but it was easier to compile that way
                int64_t index = vm_pop(self).as.integer;
                ObjectArray* array = AS_ARRAY(vm_pop(self));
                Value value = vm_pop(self);
                array->data[index] = value;
                break;
            }
            case OP_TUPLE_MEMBER_GET: {
                int64_t index = vm_pop(self).as.integer;
                ValueTuple tuple = vm_pop(self).as.tuple;
                vm_push(self, self->stack[tuple.value_stack_index + index]);
                break;
            }
            case OP_TUPLE_MEMBER_SET: {
                // Yeah, the order of arguments is a little weird, but it was easier to compile that way
                int64_t index = vm_pop(self).as.integer;
                ValueTuple tuple = vm_pop(self).as.tuple;
                Value value = vm_pop(self);
                self->stack[tuple.value_stack_index + index] = value;
                break;
            }
            case OP_CALL: {
                size_t args_length = READ();
                size_t new_stack_frame_start_index = self->sp - args_length - 1;
                vm_run(self, AS_CLOSURE(self->stack[new_stack_frame_start_index]), new_stack_frame_start_index);
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
                for(size_t i = 0; i < length; i++) array->data[i] = value_deep_copy(value, self);
                vm_push(self, value_new_object((Object*) array));
                break;
            }
            case OP_TUPLIFY: {
                size_t length = READ();
                length |= (READ() << 8);
                vm_push(self, value_new_tuple((ValueTuple) {
                    .value_stack_index = self->sp - length,
                    .length = length
                }));
                break;
            }
            case OP_DEEP_COPY: {
                vm_push(self, value_deep_copy(vm_pop(self), self));
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

                TempString a_str = value_to_str(a, self->stack);
                TempString b_str = value_to_str(b, self->stack);

                char* data = malloc(a_str.length + b_str.length);
                for(size_t i = 0; i < a_str.length; i++) data[i] = a_str.data[i];
                for(size_t i = 0; i < b_str.length; i++) data[i + a_str.length] = b_str.data[i];

                ObjectStr* result = malloc(sizeof(ObjectStr));
                *result = object_str_new(data, a_str.length + b_str.length);
                result = vm_object_str_register(self, result);
                vm_push(self, value_new_object((Object*) result));

                temp_string_destruct(a_str);
                temp_string_destruct(b_str);
                break;
            }
            case OP_PRINT:
                value_print(vm_pop(self), self->stack);
                break;
            case OP_PRINTLN:
                value_println(vm_pop(self), self->stack);
                break;
            case OP_EQUALS: {
                Value b = vm_pop(self);
                Value a = vm_pop(self);
                bool result = value_equals(a, b, self->stack);
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
                break;
            }
            case OP_RETURN:
                self->sp = stack_frame_start_index;
                return;
            case OP_RETURN_VALUE: {
                Value value = vm_top(self);
                self->sp = stack_frame_start_index;
                vm_push(self, value);
                return;
            }
        }
    }
}

void run(Chunk* chunk, StrPool str_pool) {
    VM* self = (VM*) malloc(sizeof(VM));
    self->str_pool = str_pool;
    self->object = NULL;
    self->sp = 0;

    ObjectClosure start = {
        .base = object_new(OBJECT_CLOSURE),
        .chunk = chunk,
        .captured_locals = NULL,
        .captured_locals_length = 0
    };

    vm_run(self, &start, 0);

    object_closure_destruct(&start);
    vm_free(self);
}