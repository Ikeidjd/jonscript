#include "token.h"

#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"

void token_fprint(FILE* file, Token self) {
    fprintf(file, "Token('%.*s', %s)", self.text_len, self.text, token_type_to_string(self.type));
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

char* token_type_to_string(TokenType self) {
    switch(self) {
        case TOKEN_EOF: return "TOKEN_EOF";
        case TOKEN_INT: return "TOKEN_INT";
        case TOKEN_STR: return "TOKEN_STR";
        case TOKEN_PLUS: return "TOKEN_PLUS";
        case TOKEN_MINUS: return "TOKEN_MINUS";
        case TOKEN_MULT: return "TOKEN_MULT";
        case TOKEN_DIV: return "TOKEN_DIV";
        case TOKEN_MOD: return "TOKEN_MOD";
        case TOKEN_BITWISE_AND: return "TOKEN_BITWISE_AND";
        case TOKEN_BITWISE_OR: return "TOKEN_BITWISE_OR";
        case TOKEN_LT: return "TOKEN_LT";
        case TOKEN_LE: return "TOKEN_LE";
        case TOKEN_GT: return "TOKEN_GT";
        case TOKEN_GE: return "TOKEN_GE";
        case TOKEN_EQEQ: return "TOKEN_EQEQ";
        case TOKEN_AND: return "TOKEN_AND";
        case TOKEN_OR: return "TOKEN_OR";
        case TOKEN_DOTDOT: return "TOKEN_DOTDOT";
        case TOKEN_DOT: return "TOKEN_DOT";
        case TOKEN_EQUAL: return "TOKEN_EQUAL";
        case TOKEN_COMMA: return "TOKEN_COMMA";
        case TOKEN_COLON: return "TOKEN_COLON";
        case TOKEN_ARROW: return "TOKEN_ARROW";
        case TOKEN_SEMICOLON: return "TOKEN_SEMICOLON";
        case TOKEN_PAREN_LEFT: return "TOKEN_PAREN_LEFT";
        case TOKEN_PAREN_RIGHT: return "TOKEN_PAREN_RIGHT";
        case TOKEN_BRACKET_LEFT: return "TOKEN_BRACKET_LEFT";
        case TOKEN_BRACKET_RIGHT: return "TOKEN_BRACKET_RIGHT";
        case TOKEN_BRACE_LEFT: return "TOKEN_BRACE_LEFT";
        case TOKEN_BRACE_RIGHT: return "TOKEN_BRACE_RIGHT";
        case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
        case TOKEN_KEYWORD_LET: return "TOKEN_KEYWORD_LET";
        case TOKEN_KEYWORD_MUT: return "TOKEN_KEYWORD_MUT";
        case TOKEN_KEYWORD_PRINT: return "TOKEN_KEYWORD_PRINT";
        case TOKEN_KEYWORD_PRINTLN: return "TOKEN_KEYWORD_PRINTLN";
        case TOKEN_KEYWORD_IF: return "TOKEN_KEYWORD_IF";
        case TOKEN_KEYWORD_ELIF: return "TOKEN_KEYWORD_ELIF";
        case TOKEN_KEYWORD_ELSE: return "TOKEN_KEYWORD_IF";
        case TOKEN_KEYWORD_WHILE: return "TOKEN_KEYWORD_WHILE";
        case TOKEN_KEYWORD_DO: return "TOKEN_KEYWORD_DO";
        case TOKEN_KEYWORD_INT: return "TOKEN_KEYWORD_INT";
        case TOKEN_KEYWORD_BOOL: return "TOKEN_KEYWORD_BOOL";
        case TOKEN_KEYWORD_STR: return "TOKEN_KEYWORD_STR";
        case TOKEN_KEYWORD_FN: return "TOKEN_KEYWORD_FN";
        case TOKEN_KEYWORD_RETURN: return "TOKEN_KEYWORD_RETURN";
        case TOKEN_KEYWORD_TRUE: return "TOKEN_KEYWORD_TRUE";
        case TOKEN_KEYWORD_FALSE: return "TOKEN_KEYWORD_FALSE";
    }
}

TokenArray token_array_new(char* string_data, size_t string_data_length) {
    return (TokenArray) {
        .string_data = string_data,
        .string_data_length = string_data_length,
        DYNAMIC_ARRAY_NEW_PARTIAL()
    };
}

void token_array_push(TokenArray* self, Token token) {
    DYNARRAY_PTR_PUSH(self, token, 16);
}

void token_array_destruct(TokenArray self) {
    free(self.string_data);
    free(self.data);
}