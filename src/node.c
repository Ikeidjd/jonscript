#include "node.h"

#include "dynamic_array.h"

NodeArray node_array_new() {
    return (NodeArray) {
        .type_hash_set = type_hash_set_new(),
        .data = NULL,
        .length = 0,
        .capacity = 0
    };
}

void node_array_destruct(NodeArray self) {
    type_hash_set_destruct(self.type_hash_set);
    for(size_t i = 0; i < self.length; i++) {
        if(self.data[i].type == NODE_PROGRAM) free(self.data[i].as.program.data);
    }
    free(self.data);
}

size_t node_array_push(NodeArray* self, NodeType type) {
    PUSH(self, (Node) { .type = type }, 16);
    return self->length - 1;
}

void node_fprintln(FILE* file, NodeArray* array, size_t index, size_t indentation) {
    for(size_t i = 0; i < indentation; i++) fprintf(file, " ");
    Node* base_node = &array->data[index];
    switch(base_node->type) {
        case NODE_PROGRAM: {
            NodeProgram* node = &base_node->as.program;
            fprintf(file, "NodeProgram:\n");
            for(size_t i = 0; i < node->length; i++) node_fprintln(file, array, node->data[i], indentation + 4);
            break;
        }
        case NODE_VAR_DECL: {
            NodeVarDecl* node = &base_node->as.var_decl;
            fprintf(file, "NodeVarDecl: ");
            type_fprint(file, node->type);
            fprintf(file, " %.*s\n", node->name.text_len, node->name.text);
            node_fprintln(file, array, node->value_index, indentation + 4);
            break;
        }
        case NODE_BIN_OP: {
            NodeBinOp* node = &base_node->as.bin_op;
            fprintf(file, "NodeBinOp: ");
            token_fprintln(file, node->op);
            node_fprintln(file, array, node->left_index, indentation + 4);
            node_fprintln(file, array, node->right_index, indentation + 4);
            break;
        }
        case NODE_VAR: {
            NodeVar* node = &base_node->as.var;
            fprintf(file, "NodeVar: ");
            token_fprint(file, node->name);
            fprintf(file, " %d\n", node->stack_index);
            break;
        }
        case NODE_INT: {
            NodeInt* node = &base_node->as.int_literal;
            fprintf(file, "NodeInt: ");
            token_fprintln(file, node->value);
            break;
        }
    }
}

void node_println(NodeArray* array, size_t index) {
    node_fprintln(stdout, array, index, 0);
}

void node_array_fprintln(FILE* file, NodeArray* self) {
    node_fprintln(file, self, self->root, 0);
}

void node_array_println(NodeArray* self) {
    node_array_fprintln(stdout, self);
}