#pragma once

#include "token.h"

typedef struct NodeBinOp {
    Token op;
    size_t left;
    size_t right;
} NodeBinOp;

typedef struct NodeInt {
    Token value;
} NodeInt;

typedef enum NodeType {
    NODE_BIN_OP,
    NODE_INT
} NodeType;

typedef struct Node {
    NodeType type;
    union {
        NodeBinOp bin_op;
        NodeInt int_literal;
    } as;
} Node;

typedef struct NodeArray {
    Node* data;
    size_t length;
    size_t capacity;
    size_t root;
} NodeArray;

NodeArray node_array_new();
void node_array_destruct(NodeArray self);
Node* node_array_push(NodeArray* self, NodeType type);
size_t node_array_get_index(NodeArray* self, Node* node);

void node_fprintln(FILE* file, NodeArray* array, size_t index, int indentation);
void node_println(NodeArray* array, size_t index);

void node_array_fprintln(FILE* file, NodeArray* self);
void node_array_println(NodeArray* self);