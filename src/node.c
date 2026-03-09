#include "node.h"

#include "dynamic_array.h"

NodeArray node_array_new() {
    return (NodeArray) {
        .type_hash_set = type_hash_set_new(),
        DYNAMIC_ARRAY_NEW_PARTIAL()
    };
}

void node_array_destruct(NodeArray self) {
    type_hash_set_destruct(self.type_hash_set);
    for(size_t i = 0; i < self.length; i++) {
        switch(self.data[i].type) {
            case NODE_PROGRAM:
                free(self.data[i].as.program.data);
                break;
            case NODE_ARRAY_LIST_INIT:
                free(self.data[i].as.array_list_init.data);
                break;
            case NODE_FUN_DECL:
                free(self.data[i].as.fun_decl.param_names);
                break;
            case NODE_FUN_CALL:
                free(self.data[i].as.fun_call.args);
                break;
            default:
                break;
        }
    }
    free(self.data);
}

size_t node_array_push(NodeArray* self, NodeType type) {
    PUSH(self, (Node) { .type = type }, 16);
    return self->length - 1;
}

void node_fprintln(FILE* file, NodeArray* array, NodeIndex node_index, size_t indentation) {
    for(size_t i = 0; i < indentation; i++) fprintf(file, " ");
    Node* base_node = &array->data[node_index];
    switch(base_node->type) {
        case NODE_PROGRAM: {
            NodeProgram* node = &base_node->as.program;
            fprintf(file, "NodeProgram: %s\n", node->create_scope ? "creates scope" : "doesn't create scope");
            for(size_t i = 0; i < node->length; i++) node_fprintln(file, array, node->data[i], indentation + 4);
            break;
        }
        case NODE_VAR_DECL: {
            NodeVarDecl* node = &base_node->as.var_decl;
            fprintf(file, "NodeVarDecl: %s", node->is_mutable ? "mut" : "let");
            fprintf(file, " %.*s: ", node->name.text_len, node->name.text);
            type_fprintln(file, node->var_type);
            node_fprintln(file, array, node->value, indentation + 4);
            break;
        }
        case NODE_FUN_DECL: {
            NodeFunDecl* node = &base_node->as.fun_decl;
            fprintf(file, "NodeFunDecl: %.*s, ", node->name.text_len, node->name.text);
            function_type_fprint(file, node->type);
            fprintf(file, ", params(");
            for(size_t i = 0; i < node->type->params_length; i++) {
                fprintf(file, "%.*s", node->param_names[i].text_len, node->param_names[i].text);
                if(i + 1 < node->type->params_length) fprintf(file, ", ");
            }
            fprintf(file, ")\n");
            node_fprintln(file, array, node->body, indentation + 4);
            break;
        }
        case NODE_ASSIGN_STAT: {
            NodeAssignStat* node = &base_node->as.assign_stat;
            fprintf(file, "NodeAssignStat:\n");
            node_fprintln(file, array, node->lvalue, indentation + 4);
            node_fprintln(file, array, node->rvalue, indentation + 4);
            break;
        }
        case NODE_PRINT_STAT: {
            NodePrintStat* node = &base_node->as.print_stat;
            fprintf(file, "NodePrintStat: ");
            token_fprintln(file, node->print_token);
            node_fprintln(file, array, node->expr, indentation + 4);
            break;
        }
        case NODE_IF_STAT: {
            NodeIfStat* node = &base_node->as.if_stat;
            fprintf(file, "NodeIfStat: %s\n", node->has_else_body ? "if-else" : "if");
            node_fprintln(file, array, node->cond, indentation + 4);
            node_fprintln(file, array, node->body, indentation + 4);
            if(node->has_else_body) node_fprintln(file, array, node->else_body, indentation + 4);
            break;
        }
        case NODE_WHILE_STAT: {
            NodeWhileStat* node = &base_node->as.while_stat;
            fprintf(file, "NodeWhileStat:\n");
            node_fprintln(file, array, node->cond, indentation + 4);
            node_fprintln(file, array, node->body, indentation + 4);
            break;
        }
        case NODE_RETURN_STAT: {
            NodeReturnStat* node = &base_node->as.return_stat;
            fprintf(file, "NodeReturnStat:\n");
            if(node->has_expr) node_fprintln(file, array, node->expr, indentation + 4);
            break;
        }
        case NODE_BIN_OP: {
            NodeBinOp* node = &base_node->as.bin_op;
            fprintf(file, "NodeBinOp: ");
            token_fprintln(file, node->op);
            node_fprintln(file, array, node->left, indentation + 4);
            node_fprintln(file, array, node->right, indentation + 4);
            break;
        }
        case NODE_LOGICAL_OP: {
            NodeBinOp* node = &base_node->as.bin_op;
            fprintf(file, "NodeLogicalOp: ");
            token_fprintln(file, node->op);
            node_fprintln(file, array, node->left, indentation + 4);
            node_fprintln(file, array, node->right, indentation + 4);
            break;
        }
        case NODE_INDEX_OP: {
            NodeIndexOp* node = &base_node->as.index_op;
            fprintf(file, "NodeIndexOp: %s\n", node->should_set ? "SET" : "GET");
            node_fprintln(file, array, node->left, indentation + 4);
            node_fprintln(file, array, node->right, indentation + 4);
            break;
        }
        case NODE_FUN_CALL: {
            NodeFunCall* node = &base_node->as.fun_call;
            fprintf(file, "NodeFunCall:\n");
            node_fprintln(file, array, node->func_expr, indentation + 4);
            for(size_t i = 0; i < node->args_length; i++) node_fprintln(file, array, node->args[i], indentation + 4);
            break;
        }
        case NODE_VAR: {
            NodeVar* node = &base_node->as.var;
            fprintf(file, "NodeVar: ");
            token_fprint(file, node->name);
            fprintf(file, " %d %s\n", node->stack_index, node->should_set ? "SET" : "GET");
            break;
        }
        case NODE_ARRAY_LIST_INIT: {
            NodeArrayListInit* node = &base_node->as.array_list_init;
            fprintf(file, "NodeArrayListInit: \n");
            for(size_t i = 0; i < node->length; i++) node_fprintln(file, array, node->data[i], indentation + 4);
            break;
        }
        case NODE_ARRAY_LENGTH_INIT: {
            NodeArrayLengthInit* node = &base_node->as.array_length_init;
            fprintf(file, "NodeArrayLengthInit: \n");
            node_fprintln(file, array, node->expr, indentation + 4);
            node_fprintln(file, array, node->length, indentation + 4);
            break;
        }
        case NODE_INT: {
            NodeLiteral* node = &base_node->as.literal;
            fprintf(file, "NodeInt: ");
            token_fprintln(file, node->value);
            break;
        }
        case NODE_BOOL: {
            NodeLiteral* node = &base_node->as.literal;
            fprintf(file, "NodeBool: ");
            token_fprintln(file, node->value);
            break;
        }
        case NODE_STR: {
            NodeLiteral* node = &base_node->as.literal;
            fprintf(file, "NodeStr: ");
            token_fprintln(file, node->value);
            break;
        }
    }
}

void node_println(NodeArray* array, NodeIndex node_index) {
    node_fprintln(stdout, array, node_index, 0);
}

void node_array_fprintln(FILE* file, NodeArray* self) {
    node_fprintln(file, self, self->root, 0);
}

void node_array_println(NodeArray* self) {
    node_array_fprintln(stdout, self);
}