#include "jon_parser.h"

#include <stdio.h>
#include <stdbool.h>

#include "node.h"

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
        default:
            parser_error_string(self, "expression");
            return -1;
    }
}

static size_t parser_expr(Parser* self, Precedence prev_prec) {
    size_t node_index = parser_prefix_expr(self);
    Precedence cur_prec = parser_get_cur_prec(self);

    while(cur_prec > prev_prec) {
        Node* node = node_array_push(&self->nodes, NODE_BIN_OP);

        node->as.bin_op.left = node_index;
        node->as.bin_op.op = parser_advance(self);
        node->as.bin_op.right = parser_expr(self, cur_prec);

        node_index = node_array_get_index(&self->nodes, node);
        cur_prec = parser_get_cur_prec(self);
    }

    return node_index;
}

NodeArray parse(const TokenArray* tokens) {
    Parser self = parser_new(tokens);

    self.nodes.root = parser_expr(&self, PREC_NONE);
    parser_consume(&self, TOKEN_EOF, NULL);

    return self.nodes;
}