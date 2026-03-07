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
    StrPool str_pool = str_pool_new();

    TokenArray tokens = lex("res/main.jon", &had_error);
    if(had_error) goto End1;

#ifdef DEBUG_PRINT_TOKENS
    for(size_t i = 0; i < tokens.length; i++) token_println(tokens.data[i]);
    printf("\n");
#endif

    nodes = parse(&tokens, &had_error);
    if(had_error) goto End1;

#ifdef DEBUG_PRINT_NODES_ONE
    node_array_println(&nodes);
    printf("\n");
#endif

    analyze(&nodes, &had_error);
    if(had_error) goto End1;

#ifdef DEBUG_PRINT_NODES_TWO
    node_array_println(&nodes);
    printf("\n");
#endif

    compile(&nodes, &chunk, &str_pool, &had_error);
End1:
    token_array_destruct(tokens);
    node_array_destruct(nodes);
    if(had_error) goto End2;

#ifdef DEBUG_PRINT_STRING_LITERALS
    printf("String literals: ");
    for(size_t i = 0; i < str_pool.size; i++) {
        if(str_pool.data[i] != NULL) {
            printf("[");
            object_print((Object*) str_pool.data[i]);
            printf("]");
        }
    }
    printf("\n\n");
#endif

#ifdef DEBUG_DISASSEMBLE_CHUNK
    chunk_disassemble(&chunk);
    printf("\n");
#endif

    run(&chunk, str_pool);

End2:
    chunk_destruct(chunk);

    return 0;
}