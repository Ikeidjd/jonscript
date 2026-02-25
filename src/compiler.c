#include "compiler.h"

#include <stdlib.h>
#include <stdint.h>

static void comp(Chunk* self, NodeArray* nodes, NodeIndex node_index);

static void comp_program(Chunk* self, NodeArray* nodes, NodeProgram* node) {
    for(size_t i = 0; i < node->length; i++) comp(self, nodes, node->data[i]);
}

static void comp_var_decl(Chunk* self, NodeArray* nodes, NodeVarDecl* node) {
    comp(self, nodes, node->value);
}

static void comp_assign_stat(Chunk* self, NodeArray* nodes, NodeAssignStat* node) {
    comp(self, nodes, node->rvalue);
    comp(self, nodes, node->lvalue);
}

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

static void comp_index_op(Chunk* self, NodeArray* nodes, NodeIndexOp* node) {
    comp(self, nodes, node->left);
    comp(self, nodes, node->right);
    code_push(&self->code, node->should_set ? OP_INDEX_SET : OP_INDEX_GET);
}

static void comp_var(Chunk* self, NodeArray* nodes, NodeVar* node) {
    code_push_args(&self->code, node->should_set ? OP_LOCAL_SET : OP_LOCAL_GET, 2, TO_LE_2_BYTES(node->stack_index));
}

static void comp_array_list_init(Chunk* self, NodeArray* nodes, NodeArrayListInit* node) {
    for(size_t i = 0; i < node->length; i++) comp(self, nodes, node->data[i]);
    code_push_args(&self->code, OP_ARRAYIFY_LIST, 2, TO_LE_2_BYTES(node->length));
}

static void comp_array_length_init(Chunk* self, NodeArray* nodes, NodeArrayLengthInit* node) {
    comp(self, nodes, node->expr);
    comp(self, nodes, node->length);
    code_push(&self->code, OP_ARRAYIFY_LENGTH);
}

static void comp_int(Chunk* self, NodeArray* nodes, NodeInt* node) {
    char temp = node->value.text[node->value.text_len];
    node->value.text[node->value.text_len] = '\0';
    int64_t number = atoi(node->value.text);
    node->value.text[node->value.text_len] = temp;

    if(number <= 0xFF) code_push_args(&self->code, OP_LOAD_BYTE, 1, (byte[]) {(byte) number});
    else chunk_emit_load_value_op(self, value_new_int(number));
}

static void comp(Chunk* self, NodeArray* nodes, NodeIndex node_index) {
    Node* node = &nodes->data[node_index];
    switch(node->type) {
        case NODE_PROGRAM:
            comp_program(self, nodes, &node->as.program);
            break;
        case NODE_VAR_DECL:
            comp_var_decl(self, nodes, &node->as.var_decl);
            break;
        case NODE_ASSIGN_STAT:
            comp_assign_stat(self, nodes, &node->as.assign_stat);
            break;
        case NODE_BIN_OP:
            comp_bin_op(self, nodes, &node->as.bin_op);
            break;
        case NODE_INDEX_OP:
            comp_index_op(self, nodes, &node->as.index_op);
            break;
        case NODE_VAR:
            comp_var(self, nodes, &node->as.var);
            break;
        case NODE_ARRAY_LIST_INIT:
            comp_array_list_init(self, nodes, &node->as.array_list_init);
            break;
        case NODE_ARRAY_LENGTH_INIT:
            comp_array_length_init(self, nodes, &node->as.array_length_init);
            break;
        case NODE_INT:
            comp_int(self, nodes, &node->as.int_literal);
            break;
    }
}

Chunk compile(NodeArray* nodes, bool* had_error) {
    Chunk chunk = chunk_new();
    comp(&chunk, nodes, nodes->root);
    *had_error = false;
    return chunk;
}