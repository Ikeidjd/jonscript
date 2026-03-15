#pragma once

#include "token.h"
#include "type.h"

typedef size_t NodeIndex;

typedef struct NodeProgram {
    size_t pop_amount;
    NodeIndex* data;
    size_t length;
    size_t capacity;
    bool create_scope;
} NodeProgram;

typedef struct NodeVarDecl {
    Type* var_type;
    Token name;
    Token equals;
    NodeIndex value;
    bool is_mutable;
} NodeVarDecl;

typedef struct NodeFunDecl {
    Token name;
    FunctionType* type;
    Token* param_names;
    struct {
        size_t* data;
        size_t length;
        size_t capacity;
    } captured_locals;
    NodeIndex body;
} NodeFunDecl;

typedef struct NodeAssignStat {
    NodeIndex lvalue;
    Token equals;
    NodeIndex rvalue;
} NodeAssignStat;

typedef struct NodePrintStat {
    Token print_token;
    NodeIndex expr;
} NodePrintStat;

typedef struct NodeIfStat {
    Token if_token;
    NodeIndex cond;
    NodeIndex body;
    NodeIndex else_body;
    bool has_else_body;
} NodeIfStat;

typedef struct NodeWhileStat {
    Token while_token;
    NodeIndex cond;
    NodeIndex body;
} NodeWhileStat;

typedef struct NodeReturnStat {
    Token return_token;
    NodeIndex expr;
    bool has_expr;
} NodeReturnStat;

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

typedef struct NodeFunCall {
    Token paren_left;
    NodeIndex func_expr;
    NodeIndex* args;
    size_t args_length;
} NodeFunCall;

typedef struct NodeVar {
    Token name;
    size_t stack_index;
    bool should_set;
    bool captured;
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

typedef struct NodeLiteral {
    Token value;
} NodeLiteral;

typedef enum NodeType {
    NODE_PROGRAM,

    NODE_VAR_DECL,
    NODE_FUN_DECL,

    NODE_ASSIGN_STAT,
    NODE_PRINT_STAT,
    NODE_IF_STAT,
    NODE_WHILE_STAT,
    NODE_RETURN_STAT,

    NODE_BIN_OP,
    NODE_LOGICAL_OP,
    NODE_INDEX_OP,
    NODE_FUN_CALL,

    NODE_VAR,
    NODE_ARRAY_LIST_INIT,
    NODE_ARRAY_LENGTH_INIT,
    NODE_INT,
    NODE_BOOL,
    NODE_STR
} NodeType;

typedef struct Node {
    NodeType type;
    union {
        NodeProgram program;

        NodeVarDecl var_decl;
        NodeFunDecl fun_decl;

        NodeAssignStat assign_stat;
        NodePrintStat print_stat;
        NodeIfStat if_stat;
        NodeWhileStat while_stat;
        NodeReturnStat return_stat;

        NodeBinOp bin_op;
        NodeIndexOp index_op;
        NodeFunCall fun_call;

        NodeVar var;
        NodeArrayListInit array_list_init;
        NodeArrayLengthInit array_length_init;
        NodeLiteral literal;
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

void node_fprintln(FILE* file, NodeArray* array, NodeIndex node_index, size_t indentation);
void node_println(NodeArray* array, NodeIndex node_index);

void node_array_fprintln(FILE* file, NodeArray* self);
void node_array_println(NodeArray* self);