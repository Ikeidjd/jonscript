#pragma once

#include <stdio.h>

typedef enum TokenType {
    TOKEN_INT,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULT,
    TOKEN_DIV,
    TOKEN_MOD,
    TOKEN_EOF
} TokenType;

typedef struct Token {
    char* text;
    size_t text_len;
    TokenType type;
    size_t line;
    size_t pos;
} Token;

typedef struct TokenArray {
    char* const string_data;
    Token* data;
    size_t length;
    size_t capacity;
} TokenArray;

void token_fprint(FILE* file, Token token);
void token_fprintln(FILE* file, Token token);
void token_print(Token token);
void token_println(Token token);

void token_type_fprint(FILE* file, TokenType type);
void token_type_fprintln(FILE* file, TokenType type);
void token_type_print(TokenType type);
void token_type_println(TokenType type);

TokenArray token_array_new(char* string_data); // This takes ownership of string_data
void token_array_push(TokenArray* self, Token token);
void token_array_destruct(TokenArray self);