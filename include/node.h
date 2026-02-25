#pragma once

#include "token.h"
#include "type.h"

typedef size_t NodeIndex;

typedef struct NodeProgram {
    NodeIndex* data;
    size_t length;
    size_t capacity;
} NodeProgram;

typedef struct NodeVarDecl {
    Type* var_type;
    Token name;
    Token equals;
    NodeIndex value;
} NodeVarDecl;

typedef struct NodeAssignStat {
    NodeIndex lvalue;
    Token equals;
    NodeIndex rvalue;
} NodeAssignStat;

typedef struct NodeBinOp {
    Token op;
    NodeIndex left;
    NodeIndex right;
} NodeBinOp;

typedef struct NodeIndexOp {
    Token bracket_left;
    NodeIndex left;
    NodeIndex right;
    bool should_set;
} NodeIndexOp;

typedef struct NodeVar {
    Token name;
    size_t stack_index;
    bool should_set;
} NodeVar;

typedef struct NodeArrayListInit {
    Token bracket_left;
    NodeIndex* data;
    size_t length;
    size_t capacity;
} NodeArrayListInit;

typedef struct NodeArrayLengthInit {
    Token bracket_left;
    NodeIndex expr;
    NodeIndex length;
} NodeArrayLengthInit;

typedef struct NodeInt {
    Token value;
} NodeInt;

typedef enum NodeType {
    NODE_PROGRAM,

    NODE_VAR_DECL,

    NODE_ASSIGN_STAT,

    NODE_BIN_OP,
    NODE_INDEX_OP,

    NODE_VAR,
    NODE_ARRAY_LIST_INIT,
    NODE_ARRAY_LENGTH_INIT,
    NODE_INT
} NodeType;

typedef struct Node {
    NodeType type;
    union {
        NodeProgram program;

        NodeVarDecl var_decl;

        NodeAssignStat assign_stat;

        NodeBinOp bin_op;
        NodeIndexOp index_op;

        NodeVar var;
        NodeArrayListInit array_list_init;
        NodeArrayLengthInit array_length_init;
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