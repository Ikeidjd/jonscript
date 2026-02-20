#include "node.h"

#include "dynamic_array.h"

NodeArray node_array_new() {
    return (NodeArray) {
        .data = NULL,
        .length = 0,
        .capacity = 0
    };
}

void node_array_destruct(NodeArray self) {
    free(self.data);
}

Node* node_array_push(NodeArray* self, NodeType type) {
    PUSH(self, (Node) { .type = type }, 16);
    return &self->data[self->length - 1];
}

size_t node_array_get_index(NodeArray* self, Node* node) {
    return node - self->data;
}

void node_fprintln(FILE* file, NodeArray* array, size_t index, int indentation) {
    for(int i = 0; i < indentation; i++) fprintf(file, " ");
    Node* base_node = &array->data[index];
    switch(base_node->type) {
        case NODE_BIN_OP: {
            NodeBinOp* node = &base_node->as.bin_op;
            fprintf(file, "NodeBinOp: ");
            token_fprintln(file, node->op);
            node_fprintln(file, array, node->left, indentation + 4);
            node_fprintln(file, array, node->right, indentation + 4);
            break;
        }
        case NODE_INT: {
            NodeInt* node = &base_node->as.int_literal;
            fprintf(file, "NodeInt: ");
            token_fprintln(file, node->value);
            break;
        }
        default:
            fprintf(file, "Unknown NodeType %d\n", base_node->type);
            break;
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