#include "jon_parser.h"

#include <stdio.h>
#include <stdbool.h>

#include "type.h"
#include "node.h"
#include "dynamic_array.h"

#include "parser_helper.c"

typedef enum Precedence {
    PREC_NONE,
    PREC_ADD,
    PREC_MUL
} Precedence;

static Precedence precs[] = {
    [TOKEN_INT] = PREC_NONE,
    [TOKEN_PLUS] = PREC_ADD,
    [TOKEN_MINUS] = PREC_ADD,
    [TOKEN_MULT] = PREC_MUL,
    [TOKEN_DIV] = PREC_MUL,
    [TOKEN_MOD] = PREC_MUL,
    [TOKEN_EOF] = PREC_NONE
};

static Precedence parser_get_cur_prec(Parser* self) {
    return precs[parser_peek(self).type];
}

static size_t parser_prefix_expr(Parser* self) {
    Token token = parser_advance(self);
    switch(token.type) {
        case TOKEN_INT: {
            Node* node = node_array_push(&self->nodes, NODE_INT);
            node->as.int_literal.value = token;
            return node_array_get_index(&self->nodes, node);
        }
        case TOKEN_IDENTIFIER: {
            Node* node = node_array_push(&self->nodes, NODE_VAR);
            node->as.var.name = token;
            return node_array_get_index(&self->nodes, node);
        }
        default:
            parser_error_string(self, "expression");
            return 0;
    }
}

static size_t parser_expr(Parser* self, Precedence prev_prec) {
    size_t node_index = parser_prefix_expr(self);
    if(self->panic_mode) return 0;
    Precedence cur_prec = parser_get_cur_prec(self);

    while(cur_prec > prev_prec) {
        Node* node = node_array_push(&self->nodes, NODE_BIN_OP);

        node->as.bin_op.left_index = node_index;
        node->as.bin_op.op = parser_advance(self);
        node->as.bin_op.right_index = parser_expr(self, cur_prec);

        node_index = node_array_get_index(&self->nodes, node);
        cur_prec = parser_get_cur_prec(self);
    }

    return node_index;
}

static Type* parser_type(Parser* self) {
    parser_consume(self, TOKEN_KEYWORD_INT, "type");
    if(self->panic_mode) return NULL;

    Type* type = (Type*) primitive_type_new(&self->nodes.type_hash_set, TYPE_INT);

    while(parser_matches(self, TOKEN_BRACKET_LEFT)) {
        parser_consume(self, TOKEN_BRACKET_RIGHT, "']'");
        if(self->panic_mode) return NULL;
        type = (Type*) array_type_new(&self->nodes.type_hash_set, type);
    }

    return type;
}

static size_t parser_decl(Parser* self) {
    Type* type = parser_type(self);
    if(self->panic_mode) return 0;

    Node* node = node_array_push(&self->nodes, NODE_VAR_DECL);
    node->as.var_decl.type = type;

    node->as.var_decl.name = parser_consume(self, TOKEN_IDENTIFIER, "identifier");
    if(self->panic_mode) return 0;

    node->as.var_decl.equals = parser_consume(self, TOKEN_EQUAL, "=");
    if(self->panic_mode) return 0;

    node->as.var_decl.value_index = parser_expr(self, PREC_NONE);
    if(self->panic_mode) return 0;

    parser_consume(self, TOKEN_SEMICOLON, "';'");
    if(self->panic_mode) return 0;

    return node_array_get_index(&self->nodes, node);
}

static size_t parser_program(Parser* self) {
    Node* node = node_array_push(&self->nodes, NODE_PROGRAM);
    node->as.program = (NodeProgram) {
        .data = NULL,
        .length = 0,
        .capacity = 0
    };

    while(!parser_matches(self, TOKEN_EOF)) {
        size_t index = parser_decl(self);
        if(self->panic_mode) parser_sync(self);
        else PUSH(&node->as.program, index, 16);
    }

    return node_array_get_index(&self->nodes, node);
}

NodeArray parse(const TokenArray* tokens, bool* had_error) {
    Parser self = parser_new(tokens);

    self.nodes.root = parser_program(&self);
    *had_error = self.had_error;

    return self.nodes;
}