// This is really part of jon_parser.c, but it would have gotten too long, so I separated it for convenience

#include "jon_parser.h"

typedef struct Parser {
    const TokenArray* tokens;
    size_t cur;
    NodeArray nodes;
    bool had_error;
} Parser;

static Parser parser_new(const TokenArray* tokens) {
    return (Parser) {
        .tokens = tokens,
        .cur = 0,
        .nodes = node_array_new(),
        .had_error = false
    };
}

static Token parser_prev(Parser* self) {
    return self->tokens->data[self->cur - 1];
}

static Token parser_peek(Parser* self) {
    return self->tokens->data[self->cur];
}

static Token parser_advance(Parser* self) {
    return self->tokens->data[self->cur++];
}

static bool parser_matches(Parser* self, TokenType expected) {
    return expected == parser_advance(self).type;
}

static void parser_error_string(Parser* self, const char* expected) {
    self->had_error = true;

    Token got = parser_prev(self);

    fprintf(stderr, "Expected %s, but got ", expected);
    token_type_fprint(stderr, got.type);
    fprintf(stderr, " at line %d, pos %d.\n", got.line, got.pos);
}

static void parser_error_token_type(Parser* self, TokenType expected) {
    self->had_error = true;

    Token got = parser_prev(self);

    fprintf(stderr, "Expected ");
    token_type_fprint(stderr, expected);
    fprintf(stderr, ", but got ");
    token_type_fprint(stderr, got.type);
    fprintf(stderr, " at line %d, pos %d.\n", got.line, got.pos);
}

static bool parser_consume(Parser* self, TokenType expected, const char* name_of_expected) {
    if(!parser_matches(self, expected)) {
        if(name_of_expected) parser_error_string(self, name_of_expected);
        else parser_error_token_type(self, expected);
        return false;
    }
    return true;
}