#include <stdio.h>

#include "lexer.h"

int main() {
    TokenArray array = lex("res/main.jon");
    for(int i = 0; i < array.length; i++) token_println(array.tokens[i]);
    token_array_destruct(array);
    return 0;
}