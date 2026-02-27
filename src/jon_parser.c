#include "jon_parser.h"

#include <stdio.h>
#include <stdbool.h>

#include "type.h"
#include "node.h"
#include "dynamic_array.h"

#include "parser_helper.c"

#define PROPAGATE_ERROR(statement) \
    statement; \
    if(self->panic_mode) return 0;

#define GET(node) self->nodes.data[node]

typedef enum Precedence {
    PREC_NONE,
    PREC_OR,
    PREC_AND,
    PREC_COMP,
    PREC_BITWISE_OR,
    PREC_BITWISE_AND,
    PREC_ADD,
    PREC_MUL
} Precedence;

static Precedence parser_get_cur_prec(Parser* self) {
    switch(parser_peek(self).type) {
        case TOKEN_OR:
            return PREC_OR;
        case TOKEN_AND:
            return PREC_AND;
        case TOKEN_LT:
        case TOKEN_LE:
        case TOKEN_GT:
        case TOKEN_GE:
        case TOKEN_EQEQ:
            return PREC_COMP;
        case TOKEN_BITWISE_OR:
            return PREC_BITWISE_OR;
        case TOKEN_BITWISE_AND:
            return PREC_BITWISE_AND;
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
        case TOKEN_COMMA:
        case TOKEN_COLON:
        case TOKEN_SEMICOLON:
        case TOKEN_PAREN_LEFT:
        case TOKEN_PAREN_RIGHT:
        case TOKEN_BRACKET_LEFT:
        case TOKEN_BRACKET_RIGHT:
        case TOKEN_BRACE_LEFT:
        case TOKEN_BRACE_RIGHT:
        case TOKEN_IDENTIFIER:
        case TOKEN_KEYWORD_LET:
        case TOKEN_KEYWORD_MUT:
        case TOKEN_KEYWORD_INT:
        case TOKEN_KEYWORD_BOOL:
        case TOKEN_KEYWORD_TRUE:
        case TOKEN_KEYWORD_FALSE:
            return PREC_NONE;
    }
}

static NodeIndex parse_expr(Parser* self);

static NodeIndex parse_prefix_expr(Parser* self) {
    Token token = parser_advance(self);
    NodeIndex node;
    switch(token.type) {
        case TOKEN_INT:
            node = node_array_push(&self->nodes, NODE_INT);
            GET(node).as.literal.value = token;
            break;
        case TOKEN_PAREN_LEFT:
            node = PROPAGATE_ERROR(parse_expr(self));
            parser_consume(self, TOKEN_PAREN_RIGHT, "')'");
            break;
        case TOKEN_BRACKET_LEFT:
            if(!parser_peek_matches(self, TOKEN_BRACKET_RIGHT)) {
                NodeIndex element = PROPAGATE_ERROR(parse_expr(self));

                if(parser_matches(self, TOKEN_COLON)) {
                    NodeIndex length = PROPAGATE_ERROR(parse_expr(self));
                    node = node_array_push(&self->nodes, NODE_ARRAY_LENGTH_INIT);

                    GET(node).as.array_length_init = (NodeArrayLengthInit) {
                        .bracket_left = token,
                        .expr = element,
                        .length = length
                    };
                } else {
                    node = node_array_push(&self->nodes, NODE_ARRAY_LIST_INIT);

                    GET(node).as.array_list_init = (NodeArrayListInit) {
                        .bracket_left = token,
                        DYNAMIC_ARRAY_NEW_PARTIAL()
                    };

                    PUSH(&GET(node).as.array_list_init, element, 16);

                    while(parser_matches(self, TOKEN_COMMA)) {
                        element = PROPAGATE_ERROR(parse_expr(self));
                        PUSH(&GET(node).as.array_list_init, element, 16);
                    }
                }
            }
            parser_consume(self, TOKEN_BRACKET_RIGHT, "']'");
            break;
        case TOKEN_IDENTIFIER:
            node = node_array_push(&self->nodes, NODE_VAR);
            GET(node).as.var.name = token;
            GET(node).as.var.should_set = false;
            break;
        case TOKEN_KEYWORD_TRUE:
        case TOKEN_KEYWORD_FALSE:
            node = node_array_push(&self->nodes, NODE_BOOL);
            GET(node).as.literal.value = token;
            break;
        default:
            parser_error_string(self, "expression");
            return 0;
    }

    while(parser_matches(self, TOKEN_BRACKET_LEFT)) {
        NodeIndex node_prev = node;
        node = node_array_push(&self->nodes, NODE_INDEX_OP);

        GET(node).as.index_op.bracket_left = parser_prev(self);
        GET(node).as.index_op.left = node_prev;
        GET(node).as.index_op.right = PROPAGATE_ERROR(parse_expr(self));

        parser_consume(self, TOKEN_BRACKET_RIGHT, "']'");
    }

    return node;
}

static NodeIndex parse_precedence(Parser* self, Precedence prev_prec) {
    NodeIndex node = PROPAGATE_ERROR(parse_prefix_expr(self));
    Precedence cur_prec = parser_get_cur_prec(self);

    while(cur_prec > prev_prec) {
        NodeIndex node_next = node_array_push(&self->nodes, NODE_BIN_OP);

        GET(node_next).as.bin_op.left = node;
        GET(node_next).as.bin_op.op = parser_advance(self);
        GET(node_next).as.bin_op.right = PROPAGATE_ERROR(parse_precedence(self, cur_prec));

        if(GET(node_next).as.bin_op.op.type == TOKEN_AND || GET(node_next).as.bin_op.op.type == TOKEN_OR) GET(node_next).type = NODE_LOGICAL_OP;

        node = node_next;
        cur_prec = parser_get_cur_prec(self);
    }

    return node;
}

static NodeIndex parse_expr(Parser* self) {
    return parse_precedence(self, PREC_NONE);
}

static Type* parse_type(Parser* self) {
    Type* type;
    switch(parser_advance(self).type) {
        case TOKEN_KEYWORD_INT:
            type = (Type*) primitive_type_new(&self->nodes.type_hash_set, TYPE_INT);
            break;
        case TOKEN_KEYWORD_BOOL:
            type = (Type*) primitive_type_new(&self->nodes.type_hash_set, TYPE_BOOL);
            break;
        default:
            parser_error_string(self, "type");
            return NULL;
    }

    while(parser_matches(self, TOKEN_BRACKET_LEFT)) {
        PROPAGATE_ERROR(parser_consume(self, TOKEN_BRACKET_RIGHT, "']'"));
        type = (Type*) array_type_new(&self->nodes.type_hash_set, type);
    }

    return type;
}

// Doesn't include the 'let' or 'mut' keyword, since we advance past that one before calling this function
static NodeIndex parse_var_decl(Parser* self) {
    // Oh God... I have been operating under the assumption that self->nodes wasn't gonna get resized in the middle of the function and therefore invalidate Node* node, but that's wrong ._.
    // I'll have to modify node_array_push to return an index now and I'll have to always work with indices (during parsing, anyway)
    NodeIndex node = node_array_push(&self->nodes, NODE_VAR_DECL);

    GET(node).as.var_decl.name = PROPAGATE_ERROR(parser_consume(self, TOKEN_IDENTIFIER, "identifier"));
    PROPAGATE_ERROR(parser_consume(self, TOKEN_COLON, "':'"));
    GET(node).as.var_decl.var_type = PROPAGATE_ERROR(parse_type(self));
    GET(node).as.var_decl.equals = PROPAGATE_ERROR(parser_consume(self, TOKEN_EQUAL, "="));
    GET(node).as.var_decl.value = PROPAGATE_ERROR(parse_expr(self));

    parser_consume(self, TOKEN_SEMICOLON, "';'");
    return node;
}

static NodeIndex parse_assign_stat(Parser* self) {
    NodeIndex node = node_array_push(&self->nodes, NODE_ASSIGN_STAT);

    NodeIndex lvalue = PROPAGATE_ERROR(parse_expr(self));
    switch(GET(lvalue).type) {
        case NODE_VAR:
            GET(lvalue).as.var.should_set = true;
            break;
        case NODE_INDEX_OP:
            GET(lvalue).as.index_op.should_set = true;
            break;
        default:
            parser_error_string(self, "lvalue");
            return 0;
    }

    GET(node).as.assign_stat.lvalue = lvalue;
    GET(node).as.assign_stat.equals = PROPAGATE_ERROR(parser_consume(self, TOKEN_EQUAL, "="));
    GET(node).as.assign_stat.rvalue = PROPAGATE_ERROR(parse_expr(self));

    parser_consume(self, TOKEN_SEMICOLON, "';'");
    return node;
}

static NodeIndex parse_program(Parser* self) {
    NodeIndex node = node_array_push(&self->nodes, NODE_PROGRAM);
    GET(node).as.program = DYNAMIC_ARRAY_NEW(NodeProgram);

    while(!parser_matches(self, TOKEN_EOF)) {
        NodeIndex decl;

        if(parser_matches(self, TOKEN_KEYWORD_LET) || parser_matches(self, TOKEN_KEYWORD_MUT)) decl = parse_var_decl(self);
        else if(parser_peek_matches(self, TOKEN_IDENTIFIER)) decl = parse_assign_stat(self);
        else {
            parser_advance(self);
            parser_error_string(self, "statement or declaration");
        }

        if(self->panic_mode) parser_sync(self);
        else PUSH(&GET(node).as.program, decl, 16);
    }

    return node;
}

NodeArray parse(const TokenArray* tokens, bool* had_error) {
    Parser self = parser_new(tokens);

    self.nodes.root = parse_program(&self);
    *had_error = self.had_error;

    return self.nodes;
}