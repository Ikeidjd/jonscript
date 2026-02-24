// This is really part of jon_parser.c, but it would have gotten too long, so I separated it for convenience

#include "jon_parser.h"

typedef struct Parser {
    const TokenArray* tokens;
    size_t cur;
    NodeArray nodes;
    bool had_error;
    bool panic_mode;
} Parser;

static Parser parser_new(const TokenArray* tokens) {
    return (Parser) {
        .tokens = tokens,
        .cur = 0,
        .nodes = node_array_new(),
        .had_error = false,
        .panic_mode = false
    };
}

static Token parser_prev(Parser* self) {
    return self->tokens->data[self->cur - 1];
}

static Token parser_peek(Parser* self) {
    return self->cur < self->tokens->length ? self->tokens->data[self->cur] : self->tokens->data[self->tokens->length - 1];
}

static Token parser_advance(Parser* self) {
    Token token = parser_peek(self);
    if(self->cur < self->tokens->length) self->cur++;
    return token;
}

static bool parser_matches(Parser* self, TokenType expected) {
    if(expected == parser_peek(self).type) {
        parser_advance(self);
        return true;
    }
    return false;
}

static void parser_signal_error(Parser* self) {
    self->had_error = true;
    self->panic_mode = true;
}

static void parser_error_string(Parser* self, const char* expected) {
    parser_signal_error(self);

    Token got = parser_prev(self);

    fprintf(stderr, "Expected %s, but got ", expected);
    token_type_fprint(stderr, got.type);
    fprintf(stderr, " at line %d, pos %d.\n", got.line, got.pos);
}

static void parser_error_token_type(Parser* self, TokenType expected) {
    parser_signal_error(self);

    Token got = parser_prev(self);

    fprintf(stderr, "Expected ");
    token_type_fprint(stderr, expected);
    fprintf(stderr, ", but got ");
    token_type_fprint(stderr, got.type);
    fprintf(stderr, " at line %d, pos %d.\n", got.line, got.pos);
}

static Token parser_consume(Parser* self, TokenType expected, const char* name_of_expected) {
    if(!parser_matches(self, expected)) {
        parser_advance(self);
        if(name_of_expected) parser_error_string(self, name_of_expected);
        else parser_error_token_type(self, expected);
    }
    return parser_prev(self);
}

static void parser_sync(Parser* self) {
    self->panic_mode = false;
    while(true) {
        switch(parser_advance(self).type) {
            case TOKEN_EOF:
            case TOKEN_SEMICOLON:
                return;
            default:
                break;
        }
    }
}