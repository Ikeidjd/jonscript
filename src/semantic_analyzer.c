#include "semantic_analyzer.h"

#include <stdlib.h>
#include <string.h>

#include "stack.h"

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
    SEMEN_ARITHMETIC,
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

static void semen_add_local(Semen* self, Type* type, Token name) {
    self->locals[self->locals_top++] = (LocalVar) {
        .name = name,
        .type = type,
        .depth = self->locals_depth
    };
}

static void semen_signal_error(Semen* self) {
    self->had_error = true;
    self->panic_mode = true;
}

static void semen_error(Semen* self, SemenError error, Token op, Type* left, Type* right) {
    semen_signal_error(self);

    switch(error) {
        case SEMEN_ARITHMETIC:
            fprintf(stderr, "Can't perform arithmetic operation %c on ", op.text[0]);
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

static void semen_not_found_error(Semen* self, Token name) {
    semen_signal_error(self);
    fprintf(stderr, "Variable %.*s not found on line %d, pos %d.\n", name.text_len, name.text, name.line, name.pos);
}

static Type* anal(Semen* self, NodeArray* nodes, size_t index);

static Type* anal_program(Semen* self, NodeArray* nodes, NodeProgram* node) {
    for(size_t i = 0; i < node->length; i++) {
        anal(self, nodes, node->data[i]);
        self->panic_mode = false;
    }
    return NULL;
}

static Type* anal_var_decl(Semen* self, NodeArray* nodes, NodeVarDecl* node) {
    Type* type = anal(self, nodes, node->value_index);
    if(self->panic_mode) return NULL;

    semen_add_local(self, node->type, node->name);
    if(node->type != type) semen_error(self, SEMEN_MISMATCH, node->equals, node->type, type);

    return NULL;
}

static Type* anal_bin_op(Semen* self, NodeArray* nodes, NodeBinOp* node) {
    Type* left = anal(self, nodes, node->left_index);
    if(self->panic_mode) return NULL;

    Type* right = anal(self, nodes, node->right_index);
    if(self->panic_mode) return NULL;

    if(!is_primitive(left, TYPE_INT) || left != right) semen_error(self, SEMEN_ARITHMETIC, node->op, left, right);

    return left;
}

static Type* anal_var(Semen* self, NodeArray* nodes, NodeVar* node) {
    // i can't be unsigned (and therefore can't be ssize_t) because it would mess with the i >= 0
    for(long long i = self->locals_top - 1; i >= 0; i--) {
        LocalVar* local = &self->locals[i];
        if(node->name.text_len == local->name.text_len && strncmp(node->name.text, local->name.text, node->name.text_len) == 0) {
            node->stack_index = (size_t) i;
            return local->type;
        }
    }

    semen_not_found_error(self, node->name);
    return NULL;
}

static Type* anal_int(Semen* self, NodeArray* nodes, NodeInt* node) {
    return (Type*) primitive_type_new(&nodes->type_hash_set, TYPE_INT);
}

static Type* anal(Semen* self, NodeArray* nodes, size_t index) {
    Node* node = &nodes->data[index];
    switch(node->type) {
        case NODE_PROGRAM: return anal_program(self, nodes, &node->as.program);
        case NODE_VAR_DECL: return anal_var_decl(self, nodes, &node->as.var_decl);
        case NODE_BIN_OP: return anal_bin_op(self, nodes, &node->as.bin_op);
        case NODE_VAR: return anal_var(self, nodes, &node->as.var);
        case NODE_INT: return anal_int(self, nodes, &node->as.int_literal);
    }
}

void analyze(NodeArray* nodes, bool* had_error) {
    Semen self = semen_new();

    anal(&self, nodes, nodes->root);
    *had_error = self.had_error;

    semen_destruct(self);
}