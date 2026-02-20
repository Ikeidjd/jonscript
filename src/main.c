#include <stdio.h>

#include "lexer.h"
#include "jon_parser.h"

int main() {
    TokenArray tokens = lex("res/main.jon");
    // for(int i = 0; i < tokens.length; i++) token_println(tokens.data[i]);

    NodeArray nodes = parse(&tokens);
    node_array_println(&nodes);

    token_array_destruct(tokens);
    node_array_destruct(nodes);
    return 0;
}