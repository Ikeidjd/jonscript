#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "token.h"

typedef struct Lexer {
    TokenArray tokens;
    size_t start_cur;
    size_t start_line;
    size_t start_pos;
    size_t cur;
    size_t line;
    size_t pos;
    bool had_error;
} Lexer;

typedef enum LexerError {
    LEXER_INVALID_CHARACTER
} LexerError;

// Remember to free() returned buffer
static char* read_file(const char* filepath) {
    FILE* file = fopen(filepath, "r");

    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*) malloc(length + 1);
    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    return buffer;
}

static Lexer lexer_new(const char* filepath) {
    return (Lexer) {
        .tokens = token_array_new(read_file(filepath)),
        .start_cur = 0,
        .start_line = 1,
        .start_pos = 1,
        .cur = 0,
        .line = 1,
        .pos = 1,
        .had_error = false
    };
}

static char lexer_start_cur_char(Lexer* self) {
    return self->tokens.string_data[self->start_cur];
}

static char lexer_peek(Lexer* self) {
    return self->tokens.string_data[self->cur];
}

static void lexer_error(Lexer* self, LexerError error) {
    self->had_error = true;
    switch(error) {
        case LEXER_INVALID_CHARACTER:
            fprintf(stderr, "Invalid character '%c'", lexer_start_cur_char(self));
            break;
        default:
            fprintf(stderr, "Unknown LexerError %d", (int) error);
            break;
    }
    fprintf(stderr, " at line %d, pos %d.\n", self->start_line, self->start_pos);
}

static char lexer_advance(Lexer* self) {
    if(lexer_peek(self) == '\0') return '\0';

    if(lexer_peek(self) == '\n') {
        self->line++;
        self->pos = 1;
    } else {
        self->pos++;
    }

    return self->tokens.string_data[self->cur++];
}

static void lexer_skip_whitespace(Lexer* self) {
    while(isspace(lexer_peek(self))) lexer_advance(self);
}

static void lexer_update_start_indices(Lexer* self) {
    self->start_cur = self->cur;
    self->start_line = self->line;
    self->start_pos = self->pos;
}

static void lexer_add_token(Lexer* self, TokenType type) {
    token_array_push(
        &self->tokens,
        (Token) {
            .text = &self->tokens.string_data[self->start_cur],
            .text_len = self->cur - self->start_cur,
            .type = type,
            .line = self->start_line,
            .pos = self->start_pos
        }
    );
}

static void lexer_add_int_token(Lexer* self) {
    while(isdigit(lexer_peek(self))) lexer_advance(self);
    lexer_add_token(self, TOKEN_INT);
}

static void lexer_add_next_token(Lexer* self) {
    lexer_skip_whitespace(self);
    lexer_update_start_indices(self);
    char c = lexer_advance(self);
    switch(c) {
        case '+':
            lexer_add_token(self, TOKEN_PLUS);
            break;
        case '-':
            lexer_add_token(self, TOKEN_MINUS);
            break;
        case '*':
            lexer_add_token(self, TOKEN_MULT);
            break;
        case '/':
            lexer_add_token(self, TOKEN_DIV);
            break;
        case '%':
            lexer_add_token(self, TOKEN_MOD);
            break;
        default:
            if(isdigit(c)) lexer_add_int_token(self);
            else lexer_error(self, LEXER_INVALID_CHARACTER);
            break;
    }
}

TokenArray lex(const char* filepath) {
    Lexer self = lexer_new(filepath);

    while(lexer_peek(&self) != '\0') lexer_add_next_token(&self);
    lexer_update_start_indices(&self);
    lexer_add_token(&self, TOKEN_EOF);

    return self.had_error ? token_array_new(NULL) : self.tokens;
}