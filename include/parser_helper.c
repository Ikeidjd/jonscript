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

static bool parser_peek_matches(Parser* self, TokenType expected) {
    return expected == parser_peek(self).type;
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

    fprintf(stderr, "Expected %s, but got %s ('%.*s') on line %d, pos %d.\n", expected, token_type_to_string(got.type), got.text_len, got.text, got.line, got.pos);
}

static void parser_error_too_many_params(Parser* self, Token function_name) {
    parser_signal_error(self);

    fprintf(stderr, "Too many parameters for function %.*s on line %d, pos %d. Maximum is %d\n",
        function_name.text_len, function_name.text, function_name.line, function_name.pos, MAX_PARAM_LENGTH);
}

static void parser_error_too_many_params_for_type(Parser* self, Token keyword_fn) {
    parser_signal_error(self);

    fprintf(stderr, "Too many parameters for function type on line %d, pos %d. Maximum is %d\n", keyword_fn.line, keyword_fn.pos, MAX_PARAM_LENGTH);
}

static void parser_error_too_many_args(Parser* self, Token paren_left) {
    parser_signal_error(self);

    fprintf(stderr, "Too many arguments for function call on line %d, pos %d. Maximum is %d\n", paren_left.line, paren_left.pos, MAX_PARAM_LENGTH);
}

static void parser_error_expr_stat(Parser* self) {
    parser_signal_error(self);

    Token pos = parser_prev(self);
    fprintf(stderr, "Expression used as a statement on line %d, pos %d.\n", pos.line, pos.pos);
}

static Token parser_peek_consume(Parser* self, TokenType expected, const char* name_of_expected) {
    if(!parser_peek_matches(self, expected)) {
        parser_advance(self);
        parser_error_string(self, name_of_expected);
    }
    return parser_peek(self);
}

static Token parser_consume(Parser* self, TokenType expected, const char* name_of_expected) {
    if(!parser_matches(self, expected)) {
        parser_advance(self);
        parser_error_string(self, name_of_expected);
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