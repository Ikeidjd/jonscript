#include "semantic_analyzer.h"

#include <stdlib.h>

typedef struct Semen {
    
} Semen;

static int anal(Semen* self, NodeArray* nodes, size_t index);

static int anal_bin_op(Semen* self, NodeArray* nodes, NodeBinOp* node) {
    int left = anal(self, nodes, node->left);
    int right = anal(self, nodes, node->right);

    // token_print(node->op);
    // printf(" %d %d\n", left, right);

    switch(node->op.type) {
        case TOKEN_PLUS: return left + right;
        case TOKEN_MINUS: return left - right;
        case TOKEN_MULT: return left * right;
        case TOKEN_DIV: return left / right;
        case TOKEN_MOD: return left % right;
        default:
            fprintf(stderr, "Invalid binary operator ");
            token_fprint(stderr, node->op);
            fprintf(stderr, ". This should never happen");
            return 0;
    }
}

static int anal_int(Semen* self, NodeArray* nodes, NodeInt* node) {
    char temp = node->value.text[node->value.text_len];
    node->value.text[node->value.text_len] = '\0';
    int out = atoi(node->value.text);
    node->value.text[node->value.text_len] = temp;
    // token_print(node->value);
    // printf(" %d\n", out);
    return out;
}

static int anal(Semen* self, NodeArray* nodes, size_t index) {
    Node* node = &nodes->data[index];
    switch(node->type) {
        case NODE_BIN_OP: return anal_bin_op(self, nodes, &node->as.bin_op);
        case NODE_INT: return anal_int(self, nodes, &node->as.int_literal);
        default:
            fprintf(stderr, "Unknown NodeType %d\n", node->type);
            return 0;
    }
}

int analyze(NodeArray* nodes) {
    Semen self = (Semen) {};
    return anal(&self, nodes, nodes->root);
}