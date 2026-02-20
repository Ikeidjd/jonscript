#include <stdio.h>

#include "lexer.h"
#include "jon_parser.h"
#include "semantic_analyzer.h"
#include "compiler.h"

int main() {
    TokenArray tokens = lex("res/main.jon");
    // for(int i = 0; i < tokens.length; i++) token_println(tokens.data[i]);

    NodeArray nodes = parse(&tokens);
    // node_array_println(&nodes);

    // printf("%d", analyze(&nodes));

    Chunk chunk = compile(&nodes);
    chunk_disassemble(&chunk);

    token_array_destruct(tokens);
    node_array_destruct(nodes);

    return 0;
}