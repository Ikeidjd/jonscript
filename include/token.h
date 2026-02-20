#pragma once

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
    char* string_data;
    Token* tokens;
    size_t length;
    size_t capacity;
} TokenArray;

void token_print(Token token);
void token_println(Token token);
void token_type_print(TokenType token);
void token_type_println(TokenType token);

TokenArray token_array_new(char* string_data); // This takes ownership of string_data
void token_array_push(TokenArray* self, Token token);
void token_array_destruct(TokenArray self);