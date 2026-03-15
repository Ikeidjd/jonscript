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
    LEXER_INVALID_CHARACTER,
    LEXER_UNCLOSED_STRING_LITERAL
} LexerError;

// Remember to free() returned buffer
static char* read_file(const char* filepath, size_t* out_length) {
    /*
    I HATE CRLF. So, an error happened when I tried to read a file with more than one line. The final char was -51. WHAT?
    So, what I think happened is it was giving me the length while counting \r as a character, but then when reading, it was ignoring \r, therefore reading an extra char. WHY?
    I'm not entirely sure if it was that, but I don't think it could be anything else.
    Anyway, I remember having had some trouble with stuff like this in C++ a while back, so that's how I got the idea.
    Changing the file mode to rb seems to have fixed it. I guess it's actually reading the \r now, AS IT SHOULD.
    Or if you're gonna ignore it, AT LEAST BE CONSISTENT AND ALWAYS IGNORE IT.
    Anyway, I'm actually a genius for figuring it out. Albert Einstein's got nothing on my huge brain.
    No debugger btw, just brains and printf("HERE\n");, as God intended (I actually just couldn't figure out how to configure a debugger :/).

    Actually, I just checked and it was totally what I thought, 100%. When printing the read characters one by one (as integers, to check properly), there is no \r. I'm truly goated.
    */
    FILE* file = fopen(filepath, "rb");
    if(file == NULL) {
        fprintf(stderr, "Could not open file %s", filepath);
        exit(0);
    }

    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*) malloc(length + 1);
    buffer[length] = '\0';

    fread(buffer, sizeof(char), length, file);
    fclose(file);

    *out_length = length;
    return buffer;
}

static Lexer lexer_new(const char* filepath) {
    size_t string_data_length;
    char* string_data = read_file(filepath, &string_data_length);
    return (Lexer) {
        .tokens = token_array_new(string_data, string_data_length),
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

static void lexer_error(Lexer* self, LexerError error) {
    self->had_error = true;
    switch(error) {
        case LEXER_INVALID_CHARACTER:
            fprintf(stderr, "Invalid character '%c'", lexer_start_cur_char(self));
            break;
        case LEXER_UNCLOSED_STRING_LITERAL:
            fprintf(stderr, "Unclosed string literal");
            break;
    }
    fprintf(stderr, " on line %d, pos %d.\n", self->start_line, self->start_pos);
}

static char lexer_prev(Lexer* self) {
    return self->tokens.string_data[self->cur - 1];
}

static char lexer_peek(Lexer* self) {
    return self->tokens.string_data[self->cur];
}

static char lexer_peek_from_start(Lexer* self, size_t offset) {
    return self->start_cur + offset < self->tokens.string_data_length ? self->tokens.string_data[self->start_cur + offset] : '\0';
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

static bool lexer_matches(Lexer* self, char c) {
    if(lexer_peek(self) == c) {
        lexer_advance(self);
        return true;
    }
    return false;
}

static void lexer_skip_whitespace(Lexer* self) {
    while(isspace(lexer_peek(self))) {
        lexer_advance(self);
    }
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

static void lexer_add_str_token(Lexer* self) {
    lexer_update_start_indices(self);
    while(lexer_peek(self) != '"') {
        if(lexer_peek(self) == '\n' || lexer_peek(self) == '\0') {
            lexer_error(self, LEXER_UNCLOSED_STRING_LITERAL);
            return;
        }
        lexer_advance(self);
    }
    lexer_add_token(self, TOKEN_STR);
    lexer_advance(self);
}

static void lexer_add_int_token(Lexer* self) {
    while(isdigit(lexer_peek(self))) lexer_advance(self);
    lexer_add_token(self, TOKEN_INT);
}

static bool is_identifier_char(char c) {
    return isalnum(c) || c == '_';
}

static bool lexer_add_keyword_token(Lexer* self, size_t start, size_t length, char* expected, TokenType type) {
    for(size_t i = start; i < length; i++) {
        if(lexer_peek_from_start(self, i) != expected[i - start]) return false;
    }

    if(is_identifier_char(lexer_peek_from_start(self, length))) return false;

    while(self->cur < self->start_cur + length) lexer_advance(self);

    lexer_add_token(self, type);
    return true;
}

static void lexer_add_identifier_token(Lexer* self) {
    switch(lexer_peek_from_start(self, 0)) {
    case 'b':
        if(lexer_add_keyword_token(self, 1, 4, "ool", TOKEN_KEYWORD_BOOL)) return;
        break;
    case 'd':
        if(lexer_add_keyword_token(self, 1, 2, "o", TOKEN_KEYWORD_DO)) return;
        break;
    case 'e':
        switch(lexer_peek_from_start(self, 1)) {
        case 'l':
            switch(lexer_peek_from_start(self, 2)) {
            case 'i':
                if(lexer_add_keyword_token(self, 3, 4, "f", TOKEN_KEYWORD_ELIF)) return;
                break;
            case 's':
                if(lexer_add_keyword_token(self, 3, 4, "e", TOKEN_KEYWORD_ELSE)) return;
                break;
            }
        }
    case 'f':
        switch(lexer_peek_from_start(self, 1)) {
        case 'a':
            if(lexer_add_keyword_token(self, 2, 5, "lse", TOKEN_KEYWORD_FALSE)) return;
            break;
        case 'n':
            if(lexer_add_keyword_token(self, 2, 2, "", TOKEN_KEYWORD_FN)) return;
            break;
        }
    case 'i':
        switch(lexer_peek_from_start(self, 1)) {
        case 'f':
            if(lexer_add_keyword_token(self, 2, 2, "", TOKEN_KEYWORD_IF)) return;
            break;
        case 'n':
            if(lexer_add_keyword_token(self, 2, 3, "t", TOKEN_KEYWORD_INT)) return;
            break;
        }
    case 'l':
        if(lexer_add_keyword_token(self, 1, 3, "et", TOKEN_KEYWORD_LET)) return;
        break;
    case 'm':
        if(lexer_add_keyword_token(self, 1, 3, "ut", TOKEN_KEYWORD_MUT)) return;
        break;
    case 'p':
        switch(lexer_peek_from_start(self, 1)) {
        case 'r':
            switch(lexer_peek_from_start(self, 2)) {
            case 'i':
                switch(lexer_peek_from_start(self, 3)) {
                case 'n':
                    switch(lexer_peek_from_start(self, 4)) {
                    case 't':
                        switch(lexer_peek_from_start(self, 5)) {
                        case 'l':
                            if(lexer_add_keyword_token(self, 6, 7, "n", TOKEN_KEYWORD_PRINTLN)) return;
                            break;
                        default:
                            if(lexer_add_keyword_token(self, 5, 5, "", TOKEN_KEYWORD_PRINT)) return;
                            break;
                        }
                    }
                }
            }
        }
    case 'r':
        if(lexer_add_keyword_token(self, 1, 6, "eturn", TOKEN_KEYWORD_RETURN)) return;
        break;
    case 's':
        if(lexer_add_keyword_token(self, 1, 3, "tr", TOKEN_KEYWORD_STR)) return;
        break;
    case 't':
        if(lexer_add_keyword_token(self, 1, 4, "rue", TOKEN_KEYWORD_TRUE)) return;
        break;
    case 'w':
        if(lexer_add_keyword_token(self, 1, 5, "hile", TOKEN_KEYWORD_WHILE)) return;
        break;
    }

    while(is_identifier_char(lexer_peek(self))) lexer_advance(self);
    lexer_add_token(self, TOKEN_IDENTIFIER);
}

static void lexer_add_next_token(Lexer* self) {
    lexer_skip_whitespace(self);
    lexer_update_start_indices(self);
    char c = lexer_advance(self);
    switch(c) {
        case '"':
            lexer_add_str_token(self);
            break;
        case '+':
            lexer_add_token(self, TOKEN_PLUS);
            break;
        case '-':
            lexer_add_token(self, lexer_matches(self, '>') ? TOKEN_ARROW : TOKEN_MINUS);
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
        case '<':
            lexer_add_token(self, lexer_matches(self, '=') ? TOKEN_LE : TOKEN_LT);
            break;
        case '>':
            lexer_add_token(self, lexer_matches(self, '=') ? TOKEN_GE : TOKEN_GT);
            break;
        case '=':
            lexer_add_token(self, lexer_matches(self, '=') ? TOKEN_EQEQ : TOKEN_EQUAL);
            break;
        case '&':
            lexer_add_token(self, lexer_matches(self, '&') ? TOKEN_AND : TOKEN_BITWISE_AND);
            break;
        case '|':
            lexer_add_token(self, lexer_matches(self, '|') ? TOKEN_OR : TOKEN_BITWISE_OR);
            break;
        case '.':
            lexer_add_token(self, lexer_matches(self, '.') ? TOKEN_DOTDOT : TOKEN_DOT);
            break;
        case ',':
            lexer_add_token(self, TOKEN_COMMA);
            break;
        case ':':
            lexer_add_token(self, TOKEN_COLON);
            break;
        case ';':
            lexer_add_token(self, TOKEN_SEMICOLON);
            break;
        case '(':
            lexer_add_token(self, TOKEN_PAREN_LEFT);
            break;
        case ')':
            lexer_add_token(self, TOKEN_PAREN_RIGHT);
            break;
        case '[':
            lexer_add_token(self, TOKEN_BRACKET_LEFT);
            break;
        case ']':
            lexer_add_token(self, TOKEN_BRACKET_RIGHT);
            break;
        case '{':
            lexer_add_token(self, TOKEN_BRACE_LEFT);
            break;
        case '}':
            lexer_add_token(self, TOKEN_BRACE_RIGHT);
            break;
        case '\0':
            break;
        default:
            if(isdigit(c)) lexer_add_int_token(self);
            else if(isalpha(c)) lexer_add_identifier_token(self);
            else lexer_error(self, LEXER_INVALID_CHARACTER);
            break;
    }
}

TokenArray lex(const char* filepath, bool* had_error) {
    Lexer self = lexer_new(filepath);

    while(lexer_peek(&self) != '\0') lexer_add_next_token(&self);
    lexer_update_start_indices(&self);
    lexer_add_token(&self, TOKEN_EOF);

    *had_error = self.had_error;
    return self.had_error ? token_array_new(NULL, 0) : self.tokens;
}