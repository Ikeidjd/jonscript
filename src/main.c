#include <stdio.h>

#include "lexer.h"
#include "jon_parser.h"
#include "semantic_analyzer.h"
#include "compiler.h"
#include "vm.h"

int main() {
    bool had_error;
    NodeArray nodes = node_array_new();
    Chunk chunk = chunk_new();

    TokenArray tokens = lex("res/main.jon", &had_error);
    if(had_error) goto End;
    for(size_t i = 0; i < tokens.length; i++) token_println(tokens.data[i]);
    printf("\n");

    nodes = parse(&tokens, &had_error);
    if(had_error) goto End;
    node_array_println(&nodes);
    printf("\n");

    analyze(&nodes, &had_error);
    if(had_error) goto End;
    node_array_println(&nodes);
    printf("\n");

    chunk = compile(&nodes, &had_error);
    if(had_error) goto End;
    chunk_disassemble(&chunk);
    printf("\n");

    run(&chunk);

    End:
    token_array_destruct(tokens);
    node_array_destruct(nodes);
    chunk_destruct(chunk);

    return 0;
}