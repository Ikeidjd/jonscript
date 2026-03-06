#include "compiler.h"

#include <stdlib.h>
#include <stdint.h>

#define COMP(node_index) comp(self, chunk, nodes, node_index)

typedef struct Compiler {
    StrPool str_pool;
} Compiler;

static void compiler_emit_load_str_op(Compiler* self, Chunk* chunk, ObjectStr* str) {
    str = str_pool_insert(&self->str_pool, str);
    chunk_emit_load_value_op(chunk, value_new_object((Object*) str));
}

static void comp(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeIndex node_index);

static void comp_program(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeProgram* node) {
    for(size_t i = 0; i < node->length; i++) COMP(node->data[i]);
    if(node->pop_amount == 1) code_emit(&chunk->code, OP_POP);
    else if(node->pop_amount > 1) code_emit_args(&chunk->code, OP_POP_N, 2, TO_LE_2_BYTES(node->pop_amount));
}

static void comp_var_decl(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeVarDecl* node) {
    COMP(node->value);
}

static void comp_assign_stat(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeAssignStat* node) {
    COMP(node->rvalue);
    COMP(node->lvalue);
}

static void comp_print_stat(Compiler* self, Chunk* chunk, NodeArray* nodes, NodePrintStat* node) {
    COMP(node->expr);
    code_emit(&chunk->code, node->add_line ? OP_PRINTLN : OP_PRINT);
}

static void comp_if_stat(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeIfStat* node) {
    COMP(node->cond);
    size_t if_jump = code_emit_jump(&chunk->code, OP_JUMP_IF_FALSE_POP);
    COMP(node->body);

    if(node->has_else_body) {
        size_t else_jump = code_emit_jump(&chunk->code, OP_JUMP);
        code_patch_jump(&chunk->code, if_jump);
        COMP(node->else_body);
        code_patch_jump(&chunk->code, else_jump);
    } else {
        code_patch_jump(&chunk->code, if_jump);
    }
}

static void comp_while_stat(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeWhileStat* node) {
    size_t loop_start = chunk->code.length;
    COMP(node->cond);
    size_t cond_jump = code_emit_jump(&chunk->code, OP_JUMP_IF_FALSE_POP);
    COMP(node->body);
    code_emit_loop(&chunk->code, loop_start);
    code_patch_jump(&chunk->code, cond_jump);
}

static void comp_bin_op(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeBinOp* node) {
    COMP(node->left);
    COMP(node->right);

    switch(node->op.type) {
        case TOKEN_PLUS:
            code_emit(&chunk->code, OP_ADD);
            break;
        case TOKEN_MINUS:
            code_emit(&chunk->code, OP_SUB);
            break;
        case TOKEN_MULT:
            code_emit(&chunk->code, OP_MUL);
            break;
        case TOKEN_DIV:
            code_emit(&chunk->code, OP_DIV);
            break;
        case TOKEN_MOD:
            code_emit(&chunk->code, OP_MOD);
            break;
        case TOKEN_BITWISE_AND:
            code_emit(&chunk->code, OP_BITWISE_AND);
            break;
        case TOKEN_BITWISE_OR:
            code_emit(&chunk->code, OP_BITWISE_OR);
            break;
        case TOKEN_LT:
            code_emit(&chunk->code, OP_LT);
            break;
        case TOKEN_LE:
            code_emit(&chunk->code, OP_LE);
            break;
        case TOKEN_GT:
            code_emit(&chunk->code, OP_GT);
            break;
        case TOKEN_GE:
            code_emit(&chunk->code, OP_GE);
            break;
        case TOKEN_EQEQ:
            code_emit(&chunk->code, OP_EQUALS);
            break;
        case TOKEN_DOTDOT:
            code_emit(&chunk->code, OP_CONCAT);
            break;
        default:
            fprintf(stderr, "Invalid binary operator ");
            token_fprint(stderr, node->op);
            fprintf(stderr, ". This should never happen.\n");
            break;
    }
}

static void comp_logical_op(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeBinOp* node) {
    COMP(node->left);

    size_t jump = code_emit_jump(&chunk->code, node->op.type == TOKEN_AND ? OP_JUMP_IF_FALSE : OP_JUMP_IF_TRUE);

    code_emit(&chunk->code, OP_POP);
    COMP(node->right);

    code_patch_jump(&chunk->code, jump);
}

static void comp_index_op(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeIndexOp* node) {
    COMP(node->left);
    COMP(node->right);
    code_emit(&chunk->code, node->should_set ? OP_INDEX_SET : OP_INDEX_GET);
}

static void comp_var(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeVar* node) {
    code_emit_args(&chunk->code, node->should_set ? OP_LOCAL_SET : OP_LOCAL_GET, 2, TO_LE_2_BYTES(node->stack_index));
}

static void comp_array_list_init(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeArrayListInit* node) {
    for(size_t i = 0; i < node->length; i++) COMP(node->data[i]);
    code_emit_args(&chunk->code, OP_ARRAYIFY_LIST, 2, TO_LE_2_BYTES(node->length));
}

static void comp_array_length_init(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeArrayLengthInit* node) {
    COMP(node->expr);
    COMP(node->length);
    code_emit(&chunk->code, OP_ARRAYIFY_LENGTH);
}

static void comp_int(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeLiteral* node) {
    char temp = node->value.text[node->value.text_len];
    node->value.text[node->value.text_len] = '\0';
    int64_t number = atoi(node->value.text);
    node->value.text[node->value.text_len] = temp;

    if(number <= 0xFF) code_emit_args(&chunk->code, OP_LOAD_BYTE, 1, (byte[]) {(byte) number});
    else chunk_emit_load_value_op(chunk, value_new_int(number));
}

static void comp_bool(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeLiteral* node) {
    code_emit(&chunk->code, node->value.type == TOKEN_KEYWORD_TRUE ? OP_LOAD_TRUE : OP_LOAD_FALSE);
}

static void comp_str(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeLiteral* node) {
    ObjectStr* str = malloc(sizeof(ObjectStr));
    char* data = malloc(node->value.text_len);
    for(size_t i = 0; i < node->value.text_len; i++) data[i] = node->value.text[i];
    *str = object_str_new(data, node->value.text_len);
    compiler_emit_load_str_op(self, chunk, str);
}

static void comp(Compiler* self, Chunk* chunk, NodeArray* nodes, NodeIndex node_index) {
    Node* node = &nodes->data[node_index];
    switch(node->type) {
        case NODE_PROGRAM:
            comp_program(self, chunk, nodes, &node->as.program);
            break;
        case NODE_VAR_DECL:
            comp_var_decl(self, chunk, nodes, &node->as.var_decl);
            break;
        case NODE_ASSIGN_STAT:
            comp_assign_stat(self, chunk, nodes, &node->as.assign_stat);
            break;
        case NODE_PRINT_STAT:
            comp_print_stat(self, chunk, nodes, &node->as.print_stat);
            break;
        case NODE_IF_STAT:
            comp_if_stat(self, chunk, nodes, &node->as.if_stat);
            break;
        case NODE_WHILE_STAT:
            comp_while_stat(self, chunk, nodes, &node->as.while_stat);
            break;
        case NODE_BIN_OP:
            comp_bin_op(self, chunk, nodes, &node->as.bin_op);
            break;
        case NODE_LOGICAL_OP:
            comp_logical_op(self, chunk, nodes, &node->as.bin_op);
            break;
        case NODE_INDEX_OP:
            comp_index_op(self, chunk, nodes, &node->as.index_op);
            break;
        case NODE_VAR:
            comp_var(self, chunk, nodes, &node->as.var);
            break;
        case NODE_ARRAY_LIST_INIT:
            comp_array_list_init(self, chunk, nodes, &node->as.array_list_init);
            break;
        case NODE_ARRAY_LENGTH_INIT:
            comp_array_length_init(self, chunk, nodes, &node->as.array_length_init);
            break;
        case NODE_INT:
            comp_int(self, chunk, nodes, &node->as.literal);
            break;
        case NODE_BOOL:
            comp_bool(self, chunk, nodes, &node->as.literal);
            break;
        case NODE_STR:
            comp_str(self, chunk, nodes, &node->as.literal);
            break;
    }
}

void compile(NodeArray* nodes, Chunk* chunk, StrPool* str_pool, bool* had_error) {
    Compiler self = (Compiler) { .str_pool = *str_pool };
    comp(&self, chunk, nodes, nodes->root);

    *str_pool = self.str_pool;
    *had_error = false;
}