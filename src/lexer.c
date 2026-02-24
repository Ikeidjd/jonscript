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

    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*) malloc(length + 1);
    buffer[length] = '\0';

    fread(buffer, sizeof(char), length, file);
    fclose(file);

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

static void lexer_error(Lexer* self, LexerError error) {
    self->had_error = true;
    switch(error) {
        case LEXER_INVALID_CHARACTER:
            fprintf(stderr, "Invalid character '%c'", lexer_start_cur_char(self));
            break;
    }
    fprintf(stderr, " at line %d, pos %d.\n", self->start_line, self->start_pos);
}

static char lexer_prev(Lexer* self) {
    return self->tokens.string_data[self->cur - 1];
}

static char lexer_peek(Lexer* self) {
    return self->tokens.string_data[self->cur];
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

static void lexer_add_int_token(Lexer* self) {
    while(isdigit(lexer_peek(self))) lexer_advance(self);
    lexer_add_token(self, TOKEN_INT);
}

static bool lexer_add_keyword_token(Lexer* self, size_t length, char* expected, TokenType type) {
    if(length == 0 && isalpha(lexer_peek(self))) return false;

    for(size_t i = 0; i < length; i++) {
        if(lexer_peek(self) != expected[i]) return false;
        lexer_advance(self);
    }

    lexer_add_token(self, type);
    return true;
}

static void lexer_add_identifier_token(Lexer* self) {
    switch(lexer_prev(self)) {
    case 'i':
        if(lexer_add_keyword_token(self, 2, "nt", TOKEN_KEYWORD_INT)) return;
        break;
    }
    while(isalpha(lexer_peek(self))) lexer_advance(self);
    lexer_add_token(self, TOKEN_IDENTIFIER);
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
        case '=':
            lexer_add_token(self, TOKEN_EQUAL);
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
    return self.had_error ? token_array_new(NULL) : self.tokens;
}