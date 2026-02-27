#pragma once

#include <stdio.h>

typedef enum TokenType {
    // End of file
    TOKEN_EOF,

    // Literals
    TOKEN_INT,

    // Operators
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULT,
    TOKEN_DIV,
    TOKEN_MOD,

    TOKEN_BITWISE_AND,
    TOKEN_BITWISE_OR,

    TOKEN_LT,
    TOKEN_LE,
    TOKEN_GT,
    TOKEN_GE,
    TOKEN_EQEQ,

    TOKEN_AND,
    TOKEN_OR,

    TOKEN_EQUAL,

    // Symbols
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_PAREN_LEFT,
    TOKEN_PAREN_RIGHT,
    TOKEN_BRACKET_LEFT,
    TOKEN_BRACKET_RIGHT,
    TOKEN_BRACE_LEFT,
    TOKEN_BRACE_RIGHT,

    // Identifiers and keywords
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD_LET,
    TOKEN_KEYWORD_MUT,
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_BOOL,

    TOKEN_KEYWORD_TRUE,
    TOKEN_KEYWORD_FALSE
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

void token_fprint(FILE* file, Token self);
void token_fprintln(FILE* file, Token self);
void token_print(Token self);
void token_println(Token self);

void token_type_fprint(FILE* file, TokenType self);
void token_type_fprintln(FILE* file, TokenType self);
void token_type_print(TokenType self);
void token_type_println(TokenType self);

TokenArray token_array_new(char* string_data); // This takes ownership of string_data
void token_array_push(TokenArray* self, Token token);
void token_array_destruct(TokenArray self);