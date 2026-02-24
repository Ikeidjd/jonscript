#pragma once

#include "token.h"
#include "type.h"

typedef struct NodeProgram {
    size_t* data;
    size_t length;
    size_t capacity;
} NodeProgram;

typedef struct NodeVarDecl {
    Type* type;
    Token name;
    Token equals;
    size_t value_index;
} NodeVarDecl;

typedef struct NodeBinOp {
    Token op;
    size_t left_index;
    size_t right_index;
} NodeBinOp;

typedef struct NodeVar {
    Token name;
    size_t stack_index;
} NodeVar;

typedef struct NodeInt {
    Token value;
} NodeInt;

typedef enum NodeType {
    NODE_PROGRAM,
    NODE_VAR_DECL,
    NODE_BIN_OP,
    NODE_VAR,
    NODE_INT
} NodeType;

typedef struct Node {
    NodeType type;
    union {
        NodeProgram program;
        NodeVarDecl var_decl;
        NodeBinOp bin_op;
        NodeVar var;
        NodeInt int_literal;
    } as;
} Node;

typedef struct NodeArray {
    TypeHashSet type_hash_set;
    Node* data;
    size_t length;
    size_t capacity;
    size_t root;
} NodeArray;

NodeArray node_array_new();
void node_array_destruct(NodeArray self);
size_t node_array_push(NodeArray* self, NodeType type);

void node_fprintln(FILE* file, NodeArray* array, size_t index, size_t indentation);
void node_println(NodeArray* array, size_t index);

void node_array_fprintln(FILE* file, NodeArray* self);
void node_array_println(NodeArray* self);