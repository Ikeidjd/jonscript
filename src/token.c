#include "token.h"

#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"

void token_fprint(FILE* file, Token self) {
    fprintf(file, "Token('%.*s', ", self.text_len, self.text);
    token_type_fprint(file, self.type);
    fprintf(file, ")");
}

void token_fprintln(FILE* file, Token self) {
    token_fprint(file, self);
    fprintf(file, "\n");
}

void token_print(Token self) {
    token_fprint(stdout, self);
}

void token_println(Token self) {
    token_fprintln(stdout, self);
}

void token_type_fprint(FILE* file, TokenType self) {
    switch(self) {
        case TOKEN_EOF:
            fprintf(file, "TOKEN_EOF");
            break;
        case TOKEN_INT:
            fprintf(file, "TOKEN_INT");
            break;
        case TOKEN_PLUS:
            fprintf(file, "TOKEN_PLUS");
            break;
        case TOKEN_MINUS:
            fprintf(file, "TOKEN_MINUS");
            break;
        case TOKEN_MULT:
            fprintf(file, "TOKEN_MULT");
            break;
        case TOKEN_DIV:
            fprintf(file, "TOKEN_DIV");
            break;
        case TOKEN_MOD:
            fprintf(file, "TOKEN_MOD");
            break;
        case TOKEN_EQUAL:
            fprintf(file, "TOKEN_EQUAL");
            break;
        case TOKEN_SEMICOLON:
            fprintf(file, "TOKEN_SEMICOLON");
            break;
        case TOKEN_PAREN_LEFT:
            fprintf(file, "TOKEN_PAREN_LEFT");
            break;
        case TOKEN_PAREN_RIGHT:
            fprintf(file, "TOKEN_PAREN_RIGHT");
            break;
        case TOKEN_BRACKET_LEFT:
            fprintf(file, "TOKEN_BRACKET_LEFT");
            break;
        case TOKEN_BRACKET_RIGHT:
            fprintf(file, "TOKEN_BRACKET_RIGHT");
            break;
        case TOKEN_BRACE_LEFT:
            fprintf(file, "TOKEN_BRACE_LEFT");
            break;
        case TOKEN_BRACE_RIGHT:
            fprintf(file, "TOKEN_BRACE_RIGHT");
            break;
        case TOKEN_IDENTIFIER:
            fprintf(file, "TOKEN_IDENTIFIER");
            break;
        case TOKEN_KEYWORD_INT:
            fprintf(file, "TOKEN_KEYWORD_INT");
            break;
    }
}

void token_type_fprintln(FILE* file, TokenType self) {
    token_type_fprint(file, self);
    fprintf(file, "\n");
}

void token_type_print(TokenType self) {
    token_type_fprint(stdout, self);
}

void token_type_println(TokenType self) {
    token_type_fprintln(stdout, self);
}

TokenArray token_array_new(char* string_data) {
    return (TokenArray) {
        .string_data = string_data,
        .data = NULL,
        .length = 0,
        .capacity = 0
    };
}

void token_array_push(TokenArray* self, Token token) {
    PUSH(self, token, 16);
}

void token_array_destruct(TokenArray self) {
    free(self.string_data);
    free(self.data);
}