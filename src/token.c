#include "token.h"

#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"

void token_fprint(FILE* file, Token token) {
    fprintf(file, "Token('%.*s', ", token.text_len, token.text);
    token_type_fprint(file, token.type);
    fprintf(file, ")");
}

void token_fprintln(FILE* file, Token token) {
    token_fprint(file, token);
    fprintf(file, "\n");
}

void token_print(Token token) {
    token_fprint(stdout, token);
}

void token_println(Token token) {
    token_fprintln(stdout, token);
}

void token_type_fprint(FILE* file, TokenType type) {
    switch(type) {
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
        case TOKEN_EOF:
            fprintf(file, "TOKEN_EOF");
            break;
        default:
            fprintf(file, "Unknown TokenType %d", (int) type);
            break;
    }
}

void token_type_fprintln(FILE* file, TokenType type) {
    token_type_fprint(file, type);
    fprintf(file, "\n");
}

void token_type_print(TokenType type) {
    token_type_fprint(stdout, type);
}

void token_type_println(TokenType type) {
    token_type_fprintln(stdout, type);
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
}