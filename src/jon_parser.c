#include "jon_parser.h"

#include <stdio.h>
#include <stdbool.h>

#include "type.h"
#include "node.h"
#include "dynamic_array.h"

#include "parser_helper.c"

#define GET(node) self->nodes.data[node]

typedef enum Precedence {
    PREC_NONE,
    PREC_ADD,
    PREC_MUL
} Precedence;

static Precedence parser_get_cur_prec(Parser* self) {
    switch(parser_peek(self).type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return PREC_ADD;
        case TOKEN_MULT:
        case TOKEN_DIV:
        case TOKEN_MOD:
            return PREC_MUL;
        case TOKEN_EOF:
        case TOKEN_INT:
        case TOKEN_EQUAL:
        case TOKEN_SEMICOLON:
        case TOKEN_PAREN_LEFT:
        case TOKEN_PAREN_RIGHT:
        case TOKEN_BRACKET_LEFT:
        case TOKEN_BRACKET_RIGHT:
        case TOKEN_BRACE_LEFT:
        case TOKEN_BRACE_RIGHT:
        case TOKEN_IDENTIFIER:
        case TOKEN_KEYWORD_INT:
            return PREC_NONE;
    }
}

static size_t parse_expr(Parser* self);

static size_t parse_prefix_expr(Parser* self) {
    Token token = parser_advance(self);
    switch(token.type) {
        case TOKEN_INT: {
            size_t node = node_array_push(&self->nodes, NODE_INT);
            GET(node).as.int_literal.value = token;
            return node;
        }
        case TOKEN_PAREN_LEFT: {
            size_t expr = parse_expr(self);
            parser_consume(self, TOKEN_PAREN_RIGHT, "')'");
            return expr;
        }
        case TOKEN_IDENTIFIER: {
            size_t node = node_array_push(&self->nodes, NODE_VAR);
            GET(node).as.var.name = token;
            return node;
        }
        default:
            parser_error_string(self, "expression");
            return 0;
    }
}

static size_t parse_precedence(Parser* self, Precedence prev_prec) {
    size_t node = parse_prefix_expr(self);
    if(self->panic_mode) return 0;
    Precedence cur_prec = parser_get_cur_prec(self);

    while(cur_prec > prev_prec) {
        size_t node_next = node_array_push(&self->nodes, NODE_BIN_OP);

        GET(node_next).as.bin_op.left_index = node;
        GET(node_next).as.bin_op.op = parser_advance(self);
        GET(node_next).as.bin_op.right_index = parse_precedence(self, cur_prec);
        if(self->panic_mode) return 0;

        node = node_next;
        cur_prec = parser_get_cur_prec(self);
    }

    return node;
}

static size_t parse_expr(Parser* self) {
    return parse_precedence(self, PREC_NONE);
}

static Type* parse_type(Parser* self) {
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

static size_t parse_decl(Parser* self) {
    // Oh God... I have been operating under the assumption that self->nodes wasn't gonna get resized in the middle of the function and therefore invalidate Node* node, but that's wrong ._.
    // I'll have to modify node_array_push to return an index now and I'll have to always work with indices (during parsing, anyway)
    Type* type = parse_type(self);
    if(self->panic_mode) return 0;

    size_t node = node_array_push(&self->nodes, NODE_VAR_DECL);
    GET(node).as.var_decl.type = type;

    GET(node).as.var_decl.name = parser_consume(self, TOKEN_IDENTIFIER, "identifier");
    if(self->panic_mode) return 0;

    GET(node).as.var_decl.equals = parser_consume(self, TOKEN_EQUAL, "=");
    if(self->panic_mode) return 0;

    GET(node).as.var_decl.value_index = parse_expr(self);
    if(self->panic_mode) return 0;

    parser_consume(self, TOKEN_SEMICOLON, "';'");
    if(self->panic_mode) return 0;

    return node;
}

static size_t parse_program(Parser* self) {
    size_t node = node_array_push(&self->nodes, NODE_PROGRAM);
    GET(node).as.program = (NodeProgram) {
        .data = NULL,
        .length = 0,
        .capacity = 0
    };

    while(!parser_matches(self, TOKEN_EOF)) {
        size_t index = parse_decl(self);
        if(self->panic_mode) parser_sync(self);
        else PUSH(&GET(node).as.program, index, 16);
    }

    return node;
}

NodeArray parse(const TokenArray* tokens, bool* had_error) {
    Parser self = parser_new(tokens);

    self.nodes.root = parse_program(&self);
    *had_error = self.had_error;

    return self.nodes;
}