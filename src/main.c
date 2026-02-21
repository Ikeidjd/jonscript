#include <stdio.h>

#include "lexer.h"
#include "jon_parser.h"
#include "semantic_analyzer.h"
#include "compiler.h"
#include "vm.h"

int main() {
    TokenArray tokens = lex("res/main.jon");
    for(int i = 0; i < tokens.length; i++) token_println(tokens.data[i]);
    printf("\n");

    NodeArray nodes = parse(&tokens);
    node_array_println(&nodes);
    printf("\n");

    printf("%d", analyze(&nodes));

    Chunk chunk = compile(&nodes);
    chunk_disassemble(&chunk);
    printf("\n");

    run(&chunk);

    token_array_destruct(tokens);
    node_array_destruct(nodes);
    chunk_destruct(chunk);

    return 0;
}