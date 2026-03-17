#include "semantic_analyzer.h"

#include <stdlib.h>
#include <string.h>

#include "dynamic_array.h"
#include "stack.h"

#define PROPAGATE_ERROR(statement) \
    statement; \
    if(self->panic_mode) return FAKE_NULL

#define ANAL_GET_TYPE(node_index) PROPAGATE_ERROR(anal(self, nodes, node_index).type)
#define ANAL(node_index) PROPAGATE_ERROR(anal(self, nodes, node_index))

#define TYPE_MUT_OF(type) (TypeMut) { type, false }
#define TYPE_MUT_OF_MUTABILITY(type, is_mutable) (TypeMut) { type, is_mutable }

#define FAKE_NULL (TypeMut) {}

#define GET(node_index) nodes->data[node_index]

typedef struct TypeMut {
    Type* type;
    bool is_mutable;
} TypeMut;

typedef struct LocalVar {
    Token name;
    TypeMut type_mut;
    size_t scope;
} LocalVar;

typedef struct Semen {
    LocalVar* locals;
    size_t locals_top;
    size_t locals_scope;
    size_t stack_frame_start_index;
    NodeFunDecl* cur_fun_decl;
    bool had_error;
    bool panic_mode;
} Semen;

typedef enum SemenError {
    SEMEN_OPERATION,
    SEMEN_INDEX,
    SEMEN_MISMATCH
} SemenError;

static Semen semen_new() {
    return (Semen) {
        .locals = malloc(STACK_SIZE),
        .locals_top = 0,
        .locals_scope = 0,
        .stack_frame_start_index = 0,
        .cur_fun_decl = NULL,
        .had_error = false,
        .panic_mode = false
    };
}

static void semen_destruct(Semen self) {
    free(self.locals);
}

static void semen_signal_error(Semen* self) {
    self->had_error = true;
    self->panic_mode = true;
}

static void semen_error(Semen* self, SemenError error, Token op, Type* left, Type* right) {
    semen_signal_error(self);

    switch(error) {
        case SEMEN_OPERATION:
            fprintf(stderr, "Can't perform operation %.*s on ", op.text_len, op.text);
            break;
        case SEMEN_INDEX:
            fprintf(stderr, "Can't perform indexing operation on ");
            break;
        case SEMEN_MISMATCH:
            fprintf(stderr, "Mismatched types ");
            break;
    }

    type_fprint(stderr, left);
    fprintf(stderr, " and ");
    type_fprint(stderr, right);
    fprintf(stderr, " on line %d, pos %d.\n", op.line, op.pos);
}

static void semen_error_non_int_length(Semen* self, Token pos, Type* type) {
    semen_signal_error(self);
    fprintf(stderr, "Length should be %s, but was ", primitive_type_to_string(TYPE_INT));
    type_fprint(stderr, type);
    fprintf(stderr, " on line %d, pos %d.\n", pos.line, pos.pos);
}

static void semen_error_non_bool_condition(Semen* self, Token pos, Type* type) {
    semen_signal_error(self);
    fprintf(stderr, "Condition should be %s, but was ", primitive_type_to_string(TYPE_BOOL));
    type_fprint(stderr, type);
    fprintf(stderr, " on line %d, pos %d.\n", pos.line, pos.pos);
}

static void semen_error_not_an_lvalue(Semen* self, Token pos) {
    semen_signal_error(self);
    fprintf(stderr, "Can't assign to rvalue on line %d, pos %d.\n", pos.line, pos.pos);
}

static void semen_error_mutability(Semen* self, Token pos) {
    semen_signal_error(self);
    fprintf(stderr, "Can't assign to immutable variable on line %d, pos %d.\n", pos.line, pos.pos);
}

static void semen_error_not_found(Semen* self, Token name) {
    semen_signal_error(self);
    fprintf(stderr, "Variable %.*s not found on line %d, pos %d.\n", name.text_len, name.text, name.line, name.pos);
}

static void semen_error_repeated_parameter_names(Semen* self, Token left, Token right) {
    semen_signal_error(self);
    fprintf(stderr, "Repeated parameter name %.*s on line %d, pos %d and line %d, pos %d.\n", left.text_len, left.text, left.line, left.pos, right.line, right.pos);
}

static void semen_error_return_out_of_function(Semen* self, Token return_token) {
    semen_signal_error(self);
    fprintf(stderr, "'return' should not appear outside of a function on line %d, pos %d.\n", return_token.line, return_token.pos);
}

static void semen_error_wrong_return(Semen* self, Token return_token, Type* got) {
    semen_signal_error(self);
    fprintf(stderr, "Should have returned ");
    type_fprint(stderr, self->cur_fun_decl->type->return_type);
    fprintf(stderr, ", but returned ");
    type_fprint(stderr, got);
    fprintf(stderr, " on line %d, pos %d.\n", return_token.line, return_token.pos);
}

static void semen_error_tried_to_call_non_function(Semen* self, Token paren_left, Type* non_function) {
    semen_signal_error(self);
    fprintf(stderr, "Tried to call non-function ");
    type_fprint(stderr, non_function);
    fprintf(stderr, " on line %d, pos %d.\n", paren_left.line, paren_left.pos);
}

static void semen_error_wrong_args_length(Semen* self, Token paren_left, size_t params_length, size_t args_length) {
    semen_signal_error(self);
    fprintf(stderr, "Expected %zu %s for function call, but got %zu on line %d, pos %d.\n",
        params_length, params_length == 1 ? "argument" : "arguments", args_length, paren_left.line, paren_left.pos);
}

static void semen_error_void(Semen* self, Token pos) {
    semen_signal_error(self);
    fprintf(stderr, "Can't treat void as a value on line %d, pos %d.\n", pos.line, pos.pos);
}

static void semen_error_member_access(Semen* self, Token dot, Type* type) {
    semen_signal_error(self);
    fprintf(stderr, "Invalid member access for type ");
    type_fprint(stderr, type);
    fprintf(stderr, " on line %d, pos %d.\n", dot.line, dot.pos);
}

static void semen_error_tuple_out_of_bounds(Semen* self, Token dot, Type* type, int64_t index) {
    semen_signal_error(self);
    fprintf(stderr, "Index %lld out of bounds for type ", index);
    type_fprint(stderr, type);
    fprintf(stderr, " on line %d, pos %d.\n", dot.line, dot.pos);
}

static void semen_add_local(Semen* self, TypeMut type_mut, Token name) {
    if(is_tuple(type_mut.type)) {
        TupleType* type = (TupleType*) type_mut.type;
        for(size_t i = 0; i < type->types_length; i++) {
            self->locals[self->locals_top++] = (LocalVar) {
                .name = "",
                .type_mut = FAKE_NULL,
                .scope = self->locals_scope
            };
        }
    }

    self->locals[self->locals_top++] = (LocalVar) {
        .name = name,
        .type_mut = type_mut,
        .scope = self->locals_scope
    };
}

static LocalVar* semen_get_local(Semen* self, Token name, size_t* out_index) {
    // i can't be unsigned (and therefore can't be size_t) because it would mess with the i >= 0
    for(long long i = (long long) self->locals_top - 1; i >= 0; i--) {
        LocalVar* local = &self->locals[i];

        if(name.text_len == local->name.text_len && strncmp(name.text, local->name.text, name.text_len) == 0) {
            if(i < self->stack_frame_start_index) *out_index = self->stack_frame_start_index - ((size_t) i) - 1;
            else *out_index = ((size_t) i) - self->stack_frame_start_index;

            return local;
        }
    }

    semen_error_not_found(self, name);
    return NULL;
}

static TypeMut anal(Semen* self, NodeArray* nodes, NodeIndex node_index);

static TypeMut anal_program(Semen* self, NodeArray* nodes, NodeProgram* node) {
    if(node->create_scope) self->locals_scope++;

    for(size_t i = 0; i < node->length; i++) {
        anal(self, nodes, node->data[i]);
        self->panic_mode = false;
    }

    while(self->locals_top > 0 && self->locals[self->locals_top - 1].scope == self->locals_scope) {
        self->locals_top--;
        node->pop_amount++;
    }

    if(node->create_scope) self->locals_scope--;
    return FAKE_NULL;
}

static TypeMut anal_var_decl(Semen* self, NodeArray* nodes, NodeVarDecl* node) {
    Type* value_type = anal(self, nodes, node->value).type;
    if(!self->panic_mode && node->var_type != value_type) semen_error(self, SEMEN_MISMATCH, node->equals, node->var_type, value_type);

    if(!is_any_primitive(value_type) && GET(node->value).type == NODE_VAR) GET(node->value).as.var.should_deep_copy = true;

    semen_add_local(self, TYPE_MUT_OF_MUTABILITY(node->var_type, node->is_mutable), node->name);

    return FAKE_NULL;
}

static TypeMut anal_fun_decl(Semen* self, NodeArray* nodes, NodeFunDecl* node) {
    semen_add_local(self, TYPE_MUT_OF((Type*) node->type), node->name);

    self->locals_scope++;

    size_t old_stack_frame_start_index = self->stack_frame_start_index;
    self->stack_frame_start_index = self->locals_top;

    semen_add_local(self, TYPE_MUT_OF((Type*) node->type), node->name);

    for(size_t i = 0; i < node->type->params_length; i++) {
        Token left = node->param_names[i];

        for(size_t j = i + 1; j < node->type->params_length; j++) {
            Token right = node->param_names[j];

            if(left.text_len == right.text_len && strncmp(left.text, right.text, left.text_len) == 0) {
                semen_error_repeated_parameter_names(self, left, right);
                break;
            }
        }

        semen_add_local(self, TYPE_MUT_OF(node->type->param_types[i]), left);
    }

    self->locals_scope--;

    bool panic_mode = self->panic_mode;
    self->panic_mode = false;

    NodeFunDecl* old_fun_decl = self->cur_fun_decl;
    self->cur_fun_decl = node;

    ANAL(node->body);

    self->cur_fun_decl = old_fun_decl;
    self->stack_frame_start_index = old_stack_frame_start_index;

    self->panic_mode = panic_mode;
    return FAKE_NULL;
}

static TypeMut anal_assign_stat(Semen* self, NodeArray* nodes, NodeAssignStat* node) {
    Node* lvalue = &GET(node->lvalue);
    switch(lvalue->type) {
        case NODE_VAR:
            lvalue->as.var.should_set = true;
            break;
        case NODE_INDEX_OP:
        case NODE_MEMBER_ACCESS_OP:
            lvalue->as.index_op.should_set = true;
            break;
        default:
            semen_error_not_an_lvalue(self, node->equals);
            break;
    }

    TypeMut var = ANAL(node->lvalue);
    Type* value_type = ANAL_GET_TYPE(node->rvalue);

    if(!var.is_mutable) semen_error_mutability(self, node->equals);
    else if(var.type != value_type) semen_error(self, SEMEN_MISMATCH, node->equals, var.type, value_type);

    if(!is_any_primitive(value_type) && GET(node->rvalue).type == NODE_VAR) GET(node->rvalue).as.var.should_deep_copy = true;

    return FAKE_NULL;
}

static TypeMut anal_print_stat(Semen* self, NodeArray* nodes, NodePrintStat* node) {
    Type* type = ANAL_GET_TYPE(node->expr);
    if(is_void(type)) semen_error_void(self, node->print_token);
    return FAKE_NULL;
}

static TypeMut anal_if_stat(Semen* self, NodeArray* nodes, NodeIfStat* node) {
    Type* cond_type = anal(self, nodes, node->cond).type;
    if(!self->panic_mode && !is_primitive(cond_type, TYPE_BOOL)) semen_error_non_bool_condition(self, node->if_token, cond_type);

    bool panic_mode = self->panic_mode;
    self->panic_mode = false;

    anal(self, nodes, node->body);

    if(node->has_else_body) {
        panic_mode |= self->panic_mode;
        self->panic_mode = false;

        ANAL(node->else_body);

        self->panic_mode = panic_mode;
    }

    return FAKE_NULL;
}

static TypeMut anal_while_stat(Semen* self, NodeArray* nodes, NodeWhileStat* node) {
    Type* cond_type = anal(self, nodes, node->cond).type;
    if(!self->panic_mode && !is_primitive(cond_type, TYPE_BOOL)) semen_error_non_bool_condition(self, node->while_token, cond_type);

    bool panic_mode = self->panic_mode;
    self->panic_mode = false;

    ANAL(node->body);

    self->panic_mode = panic_mode;
    return FAKE_NULL;
}

static TypeMut anal_return_stat(Semen* self, NodeArray* nodes, NodeReturnStat* node) {
    Type* type;
    if(node->has_expr) {
        type = ANAL_GET_TYPE(node->expr);
        if(is_void(type)) {
            semen_error_void(self, node->return_token);
            return FAKE_NULL;
        }
    }
    else {
        type = void_type_new(&nodes->type_hash_set);
    }

    if(self->cur_fun_decl == NULL) semen_error_return_out_of_function(self, node->return_token);
    else if(self->cur_fun_decl->type->return_type != type) semen_error_wrong_return(self, node->return_token, type);

    return FAKE_NULL;
}

static TypeMut anal_bin_op(Semen* self, NodeArray* nodes, NodeBinOp* node) {
    Type* left = ANAL_GET_TYPE(node->left);
    Type* right = ANAL_GET_TYPE(node->right);

    bool is_correct_type_for_op;
    Type* out;
    switch(node->op.type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_MULT:
        case TOKEN_DIV:
        case TOKEN_MOD:
        case TOKEN_BITWISE_AND:
        case TOKEN_BITWISE_OR:
            is_correct_type_for_op = is_primitive(left, TYPE_INT);
            out = left;
            break;
        case TOKEN_LT:
        case TOKEN_LE:
        case TOKEN_GT:
        case TOKEN_GE:
            is_correct_type_for_op = is_primitive(left, TYPE_INT);
            out = (Type*) primitive_type_new(&nodes->type_hash_set, TYPE_BOOL);
            break;
        case TOKEN_EQEQ:
            is_correct_type_for_op = !is_void(left);
            out = (Type*) primitive_type_new(&nodes->type_hash_set, TYPE_BOOL);
            break;
        case TOKEN_DOTDOT:
            is_correct_type_for_op = !is_void(left) && !is_void(right);
            if(!is_correct_type_for_op) break;
            return TYPE_MUT_OF((Type*) primitive_type_new(&nodes->type_hash_set, TYPE_STR));
        default:
            fprintf(stderr, "Invalid binary operator ");
            token_fprint(stderr, node->op);
            fprintf(stderr, ". This should never happen.\n");
            exit(-1);
            break;
    }

    if(!is_correct_type_for_op || left != right) semen_error(self, SEMEN_OPERATION, node->op, left, right);

    return TYPE_MUT_OF(out);
}

static TypeMut anal_logical_op(Semen* self, NodeArray* nodes, NodeBinOp* node) {
    Type* left = ANAL_GET_TYPE(node->left);
    Type* right = ANAL_GET_TYPE(node->right);

    if(!is_primitive(left, TYPE_BOOL) || left != right) semen_error(self, SEMEN_OPERATION, node->op, left, right);

    return TYPE_MUT_OF(left);
}

static TypeMut anal_index_op(Semen* self, NodeArray* nodes, NodeIndexOp* node) {
    TypeMut left = ANAL(node->left);
    Type* right = ANAL_GET_TYPE(node->right);

    if(!is_array(left.type) || !is_primitive(right, TYPE_INT)) semen_error(self, SEMEN_INDEX, node->op, left.type, right);

    return TYPE_MUT_OF_MUTABILITY(((ArrayType*) left.type)->type, left.is_mutable);
}

static TypeMut anal_member_access_op(Semen* self, NodeArray* nodes, NodeIndexOp* node) {
    TypeMut left = ANAL(node->left);
    NodeType right = GET(node->right).type;

    if(!is_tuple(left.type) || right != NODE_INT) {
        semen_error_member_access(self, node->op, left.type);
        return FAKE_NULL;
    }

    TupleType* tuple = (TupleType*) left.type;
    NodeLiteral* integer_node = &GET(node->right).as.literal;
    char temp = integer_node->value.text[integer_node->value.text_len];
    integer_node->value.text[integer_node->value.text_len] = '\0';
    int64_t index = atoi(integer_node->value.text);
    integer_node->value.text[integer_node->value.text_len] = temp;

    if(index >= tuple->types_length) semen_error_tuple_out_of_bounds(self, node->op, left.type, index);

    return TYPE_MUT_OF_MUTABILITY(tuple->types[index], left.is_mutable);
}

static TypeMut anal_fun_call(Semen* self, NodeArray* nodes, NodeFunCall* node) {
    Type* func_type_base = ANAL_GET_TYPE(node->func_expr);

    if(!is_function(func_type_base)) {
        semen_error_tried_to_call_non_function(self, node->paren_left, func_type_base);
        return FAKE_NULL;
    }

    FunctionType* func_type = (FunctionType*) func_type_base;

    if(func_type->params_length != node->args_length) {
        semen_error_wrong_args_length(self, node->paren_left, func_type->params_length, node->args_length);
        return FAKE_NULL;
    }

    bool panic_mode = self->panic_mode;
    self->panic_mode = false;

    for(size_t i = 0; i < func_type->params_length; i++) {
        Type* param_type = func_type->param_types[i];
        Type* arg_type = ANAL_GET_TYPE(node->args[i]);
        if(param_type != arg_type) semen_error(self, SEMEN_MISMATCH, node->paren_left, param_type, arg_type);

        panic_mode |= self->panic_mode;
        self->panic_mode = false;
    }

    self->panic_mode = panic_mode;
    return TYPE_MUT_OF(func_type->return_type);
}

static TypeMut anal_var(Semen* self, NodeArray* nodes, NodeVar* node) {
    LocalVar* local = PROPAGATE_ERROR(semen_get_local(self, node->name, &node->stack_index));

    if(local->scope < self->locals[self->stack_frame_start_index].scope) {
        node->captured = true;

        for(size_t i = 0; i <= self->cur_fun_decl->captured_locals.length; i++) {
            if(i == self->cur_fun_decl->captured_locals.length) DYNARRAY_NON_PTR_PUSH(self->cur_fun_decl->captured_locals, node->stack_index, 16);

            if(self->cur_fun_decl->captured_locals.data[i] == node->stack_index) {
                node->stack_index = i;
                break;
            }
        }
    }

    return local->type_mut;
}

static TypeMut anal_array_list_init(Semen* self, NodeArray* nodes, NodeArrayListInit* node) {
    Type* type = ANAL_GET_TYPE(node->data[0]);

    for(int i = 0; i < node->length; i++) {
        Type* type2 = ANAL_GET_TYPE(node->data[i]);
        if(type != type2) {
            semen_error(self, SEMEN_MISMATCH, node->bracket_left, type, type2);
            return FAKE_NULL;
        }
        if(!is_any_primitive(type2) && GET(node->data[i]).type == NODE_VAR) GET(node->data[i]).as.var.should_deep_copy = true;
    }

    return TYPE_MUT_OF((Type*) array_type_new(&nodes->type_hash_set, type));
}

static TypeMut anal_array_length_init(Semen* self, NodeArray* nodes, NodeArrayLengthInit* node) {
    Type* type = ANAL_GET_TYPE(node->expr);
    Type* length_type = ANAL_GET_TYPE(node->length);

    if(!is_primitive(length_type, TYPE_INT)) {
        semen_error_non_int_length(self, node->bracket_left, length_type);
        return FAKE_NULL;
    }

    return TYPE_MUT_OF((Type*) array_type_new(&nodes->type_hash_set, type));
}

static TypeMut anal_tuple(Semen* self, NodeArray* nodes, NodeTuple* node) {
    Type* types[node->length];
    for(size_t i = 0; i < node->length; i++) {
        types[i] = ANAL_GET_TYPE(node->data[i]);
    }
    return TYPE_MUT_OF((Type*) tuple_type_new(&nodes->type_hash_set, types, node->length));
}

static TypeMut anal_int(Semen* self, NodeArray* nodes, NodeLiteral* node) {
    return TYPE_MUT_OF((Type*) primitive_type_new(&nodes->type_hash_set, TYPE_INT));
}

static TypeMut anal_bool(Semen* self, NodeArray* nodes, NodeLiteral* node) {
    return TYPE_MUT_OF((Type*) primitive_type_new(&nodes->type_hash_set, TYPE_BOOL));
}

static TypeMut anal_str(Semen* self, NodeArray* nodes, NodeLiteral* node) {
    return TYPE_MUT_OF((Type*) primitive_type_new(&nodes->type_hash_set, TYPE_STR));
}

static TypeMut anal(Semen* self, NodeArray* nodes, NodeIndex node_index) {
    Node* node = &GET(node_index);
    switch(node->type) {
        case NODE_PROGRAM: return anal_program(self, nodes, &node->as.program);
        case NODE_VAR_DECL: return anal_var_decl(self, nodes, &node->as.var_decl);
        case NODE_FUN_DECL: return anal_fun_decl(self, nodes, &node->as.fun_decl);
        case NODE_ASSIGN_STAT: return anal_assign_stat(self, nodes, &node->as.assign_stat);
        case NODE_PRINT_STAT: return anal_print_stat(self, nodes, &node->as.print_stat);
        case NODE_IF_STAT: return anal_if_stat(self, nodes, &node->as.if_stat);
        case NODE_WHILE_STAT: return anal_while_stat(self, nodes, &node->as.while_stat);
        case NODE_RETURN_STAT: return anal_return_stat(self, nodes, &node->as.return_stat);
        case NODE_BIN_OP: return anal_bin_op(self, nodes, &node->as.bin_op);
        case NODE_LOGICAL_OP: return anal_logical_op(self, nodes, &node->as.bin_op);
        case NODE_INDEX_OP: return anal_index_op(self, nodes, &node->as.index_op);
        case NODE_MEMBER_ACCESS_OP: return anal_member_access_op(self, nodes, &node->as.index_op);
        case NODE_FUN_CALL: return anal_fun_call(self, nodes, &node->as.fun_call);
        case NODE_VAR: return anal_var(self, nodes, &node->as.var);
        case NODE_ARRAY_LIST_INIT: return anal_array_list_init(self, nodes, &node->as.array_list_init);
        case NODE_ARRAY_LENGTH_INIT: return anal_array_length_init(self, nodes, &node->as.array_length_init);
        case NODE_TUPLE: return anal_tuple(self, nodes, &node->as.tuple);
        case NODE_INT: return anal_int(self, nodes, &node->as.literal);
        case NODE_BOOL: return anal_bool(self, nodes, &node->as.literal);
        case NODE_STR: return anal_str(self, nodes, &node->as.literal);
    }
}

void analyze(NodeArray* nodes, bool* had_error) {
    Semen self = semen_new();

    anal(&self, nodes, nodes->root);
    *had_error = self.had_error;

    semen_destruct(self);
}