#include "compiler.h"

#include <stdlib.h>
#include <stdint.h>

static void comp(Chunk* self, NodeArray* nodes, size_t index);

static void comp_bin_op(Chunk* self, NodeArray* nodes, NodeBinOp* node) {
    comp(self, nodes, node->left);
    comp(self, nodes, node->right);

    switch(node->op.type) {
        case TOKEN_PLUS:
            code_push(&self->code, OP_ADD);
            break;
        case TOKEN_MINUS:
            code_push(&self->code, OP_SUB);
            break;
        case TOKEN_MULT:
            code_push(&self->code, OP_MUL);
            break;
        case TOKEN_DIV:
            code_push(&self->code, OP_DIV);
            break;
        case TOKEN_MOD:
            code_push(&self->code, OP_MOD);
            break;
        default:
            fprintf(stderr, "Invalid binary operator ");
            token_fprint(stderr, node->op);
            fprintf(stderr, ". This should never happen");
            break;
    }
}

static void comp_int(Chunk* self, NodeArray* nodes, NodeInt* node) {
    char temp = node->value.text[node->value.text_len];
    node->value.text[node->value.text_len] = '\0';
    uint64_t number = atoi(node->value.text);
    node->value.text[node->value.text_len] = temp;

    if(number <= 0xFF) code_push_args(&self->code, OP_LOAD_BYTE, 1, (byte[]) {(byte) number});
    else chunk_emit_load_value_op(self, value_new_int(number));
}

static void comp(Chunk* self, NodeArray* nodes, size_t index) {
    Node* node = &nodes->data[index];
    switch(node->type) {
        case NODE_BIN_OP:
            comp_bin_op(self, nodes, &node->as.bin_op);
            break;
        case NODE_INT:
            comp_int(self, nodes, &node->as.int_literal);
            break;
        default:
            fprintf(stderr, "Unknown NodeType %d\n", node->type);
            break;
    }
}

Chunk compile(NodeArray* nodes) {
    Chunk chunk = chunk_new();
    comp(&chunk, nodes, nodes->root);
    return chunk;
}