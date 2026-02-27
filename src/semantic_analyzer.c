#include "semantic_analyzer.h"

#include <stdlib.h>
#include <string.h>

#include "stack.h"

#define PROPAGATE_ERROR(statement) \
    statement; \
    if(self->panic_mode) return NULL

#define ANAL(node_index) PROPAGATE_ERROR(anal(self, nodes, node_index))

typedef struct LocalVar {
    Token name;
    Type* type;
    size_t depth;
} LocalVar;

typedef struct Semen {
    LocalVar* locals;
    size_t locals_top;
    size_t locals_depth;
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
        .locals_depth = 0,
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

static void semen_non_int_length_error(Semen* self, Token pos, Type* type) {
    semen_signal_error(self);
    fprintf(stderr, "Length should be ");
    primitive_type_fprint(stderr, TYPE_INT);
    fprintf(stderr, ", but was ");
    type_fprint(stderr, type);
    fprintf(stderr, " on line %d, pos %d.\n", pos.line, pos.pos);
}

static void semen_not_found_error(Semen* self, Token name) {
    semen_signal_error(self);
    fprintf(stderr, "Variable %.*s not found on line %d, pos %d.\n", name.text_len, name.text, name.line, name.pos);
}

static void semen_add_local(Semen* self, Type* type, Token name) {
    self->locals[self->locals_top++] = (LocalVar) {
        .name = name,
        .type = type,
        .depth = self->locals_depth
    };
}

static Type* semen_get_type_and_index_of_local(Semen* self, Token name, size_t* out_index) {
    // i can't be unsigned (and therefore can't be size_t) because it would mess with the i >= 0
    for(long long i = self->locals_top - 1; i >= 0; i--) {
        LocalVar* local = &self->locals[i];
        if(name.text_len == local->name.text_len && strncmp(name.text, local->name.text, name.text_len) == 0) {
            *out_index = (size_t) i;
            return local->type;
        }
    }

    semen_not_found_error(self, name);
    return NULL;
}

static Type* anal(Semen* self, NodeArray* nodes, NodeIndex node_index);

static Type* anal_program(Semen* self, NodeArray* nodes, NodeProgram* node) {
    for(size_t i = 0; i < node->length; i++) {
        anal(self, nodes, node->data[i]);
        self->panic_mode = false;
    }
    return NULL;
}

static Type* anal_var_decl(Semen* self, NodeArray* nodes, NodeVarDecl* node) {
    Type* value_type = ANAL(node->value);

    semen_add_local(self, node->var_type, node->name);
    if(node->var_type != value_type) semen_error(self, SEMEN_MISMATCH, node->equals, node->var_type, value_type);

    return NULL;
}

static Type* anal_assign_stat(Semen* self, NodeArray* nodes, NodeAssignStat* node) {
    Type* var_type = ANAL(node->lvalue);
    Type* value_type = ANAL(node->rvalue);

    if(var_type != value_type) semen_error(self, SEMEN_MISMATCH, node->equals, var_type, value_type);

    return NULL;
}

static Type* anal_bin_op(Semen* self, NodeArray* nodes, NodeBinOp* node) {
    Type* left = ANAL(node->left);
    Type* right = ANAL(node->right);

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
            is_correct_type_for_op = true;
            out = (Type*) primitive_type_new(&nodes->type_hash_set, TYPE_BOOL);
            break;
        default:
            fprintf(stderr, "Invalid binary operator ");
            token_fprint(stderr, node->op);
            fprintf(stderr, ". This should never happen.\n");
            break;
    }

    if(!is_correct_type_for_op || left != right) semen_error(self, SEMEN_OPERATION, node->op, left, right);

    return out;
}

static Type* anal_logical_op(Semen* self, NodeArray* nodes, NodeBinOp* node) {
    Type* left = ANAL(node->left);
    Type* right = ANAL(node->right);

    if(!is_primitive(left, TYPE_BOOL) || left != right) semen_error(self, SEMEN_OPERATION, node->op, left, right);

    return left;
}

static Type* anal_index_op(Semen* self, NodeArray* nodes, NodeIndexOp* node) {
    Type* left = ANAL(node->left);
    Type* right = ANAL(node->right);

    if(!is_array(left) || !is_primitive(right, TYPE_INT)) semen_error(self, SEMEN_INDEX, node->bracket_left, left, right);

    return ((ArrayType*) left)->type;
}

static Type* anal_var(Semen* self, NodeArray* nodes, NodeVar* node) {
    return semen_get_type_and_index_of_local(self, node->name, &node->stack_index);
}

static Type* anal_array_list_init(Semen* self, NodeArray* nodes, NodeArrayListInit* node) {
    Type* type = ANAL(node->data[0]);

    for(int i = 1; i < node->length; i++) {
        Type* type2 = ANAL(node->data[i]);
        if(type != type2) {
            semen_error(self, SEMEN_MISMATCH, node->bracket_left, type, type2);
            return NULL;
        }
    }

    return (Type*) array_type_new(&nodes->type_hash_set, type);
}

static Type* anal_array_length_init(Semen* self, NodeArray* nodes, NodeArrayLengthInit* node) {
    Type* type = ANAL(node->expr);
    Type* length_type = ANAL(node->length);

    if(!is_primitive(length_type, TYPE_INT)) {
        semen_non_int_length_error(self, node->bracket_left, length_type);
        return NULL;
    }

    return (Type*) array_type_new(&nodes->type_hash_set, type);
}

static Type* anal_int(Semen* self, NodeArray* nodes, NodeLiteral* node) {
    return (Type*) primitive_type_new(&nodes->type_hash_set, TYPE_INT);
}

static Type* anal_bool(Semen* self, NodeArray* nodes, NodeLiteral* node) {
    return (Type*) primitive_type_new(&nodes->type_hash_set, TYPE_BOOL);
}

static Type* anal(Semen* self, NodeArray* nodes, NodeIndex node_index) {
    Node* node = &nodes->data[node_index];
    switch(node->type) {
        case NODE_PROGRAM: return anal_program(self, nodes, &node->as.program);
        case NODE_ASSIGN_STAT: return anal_assign_stat(self, nodes, &node->as.assign_stat);
        case NODE_VAR_DECL: return anal_var_decl(self, nodes, &node->as.var_decl);
        case NODE_BIN_OP: return anal_bin_op(self, nodes, &node->as.bin_op);
        case NODE_LOGICAL_OP: return anal_logical_op(self, nodes, &node->as.bin_op);
        case NODE_INDEX_OP: return anal_index_op(self, nodes, &node->as.index_op);
        case NODE_VAR: return anal_var(self, nodes, &node->as.var);
        case NODE_ARRAY_LIST_INIT: return anal_array_list_init(self, nodes, &node->as.array_list_init);
        case NODE_ARRAY_LENGTH_INIT: return anal_array_length_init(self, nodes, &node->as.array_length_init);
        case NODE_INT: return anal_int(self, nodes, &node->as.literal);
        case NODE_BOOL: return anal_bool(self, nodes, &node->as.literal);
    }
}

void analyze(NodeArray* nodes, bool* had_error) {
    Semen self = semen_new();

    anal(&self, nodes, nodes->root);
    *had_error = self.had_error;

    semen_destruct(self);
}