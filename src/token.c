#include "token.h"

#include <stdio.h>
#include <stdlib.h>

#define INITIAL_TOKEN_ARRAY_CAPACITY 16

void token_print(Token token) {
    printf("Token('%.*s', ", token.text_len, token.text);
    token_type_print(token.type);
    printf(")");
}

void token_println(Token token) {
    token_print(token);
    printf("\n");
}

void token_type_print(TokenType type) {
    switch(type) {
        case TOKEN_INT:
            printf("TOKEN_INT");
            break;
        case TOKEN_PLUS:
            printf("TOKEN_PLUS");
            break;
        case TOKEN_MINUS:
            printf("TOKEN_MINUS");
            break;
        case TOKEN_MULT:
            printf("TOKEN_MULT");
            break;
        case TOKEN_DIV:
            printf("TOKEN_DIV");
            break;
        case TOKEN_MOD:
            printf("TOKEN_MOD");
            break;
        case TOKEN_EOF:
            printf("TOKEN_EOF");
            break;
        default:
            fprintf(stderr, "Unknown TokenType %d", (int) type);
            break;
    }
}

void token_type_println(TokenType type) {
    token_type_print(type);
    printf("\n");
}

TokenArray token_array_new(char* string_data) {
    return (TokenArray) {
        .string_data = string_data,
        .tokens = NULL,
        .length = 0,
        .capacity = 0
    };
}

void token_array_push(TokenArray* self, Token token) {
    if(self->capacity <= self->length) {
        if(self->capacity == 0) self->capacity = INITIAL_TOKEN_ARRAY_CAPACITY;
        else self->capacity *= 2;
        self->tokens = realloc(self->tokens, self->capacity * sizeof(Token));
    }
    self->tokens[self->length++] = token;
}

void token_array_destruct(TokenArray self) {
    free(self.string_data);
}