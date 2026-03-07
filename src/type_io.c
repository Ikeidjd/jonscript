#include "type.h"

#include <stdio.h>

void type_fprint(FILE* file, Type* self) {
    switch(self->type) {
        case TTYPE_PRIMITIVE:
            primitive_type_fprint(file, ((PrimitiveTypeObj*) self)->type);
            break;
        case TTYPE_ARRAY:
            array_type_fprint(file, (ArrayType*) self);
            break;
        case TTYPE_FUNCTION:
            function_type_fprint(file, (FunctionType*) self);
            break;
        case TTYPE_VOID:
            fprintf(file, "void");
            break;
    }
}

void type_fprintln(FILE* file, Type* self) {
    type_fprint(file, self);
    fprintf(file, "\n");
}

void type_print(Type* self) {
    type_fprint(stdout, self);
}

void type_println(Type* self) {
    type_fprintln(stdout, self);
}

void primitive_type_fprint(FILE* file, PrimitiveType self) {
    switch(self) {
        case TYPE_INT:
            fprintf(file, "int");
            break;
        case TYPE_BOOL:
            fprintf(file, "bool");
            break;
        case TYPE_STR:
            fprintf(file, "str");
            break;
    }
}

void primitive_type_fprintln(FILE* file, PrimitiveType self) {
    primitive_type_fprint(file, self);
    fprintf(file, "\n");
}

void primitive_type_print(PrimitiveType self) {
    primitive_type_fprint(stdout, self);
}

void primitive_type_println(PrimitiveType self) {
    primitive_type_fprintln(stdout, self);
}

void array_type_fprint(FILE* file, ArrayType* self) {
    type_fprint(file, self->type);
    fprintf(file, "[]");
}

void array_type_fprintln(FILE* file, ArrayType* self) {
    array_type_fprint(file, self);
    fprintf(file, "\n");
}

void array_type_print(ArrayType* self) {
    array_type_fprint(stdout, self);
}

void array_type_println(ArrayType* self) {
    array_type_fprintln(stdout, self);
}

void function_type_fprint(FILE* file, FunctionType* self) {
    fprintf(file, "(function(");
    for(size_t i = 0; i < self->params_length; i++) {
        type_fprint(file, self->param_types[i]);
        if(i + 1 < self->params_length) fprintf(file, ", ");
    }
    fprintf(file, ") -> ");
    type_fprint(file, self->return_type);
    fprintf(file, ")");
}

void function_type_fprintln(FILE* file, FunctionType* self) {
    function_type_fprint(file, self);
    fprintf(file, "\n");
}

void function_type_print(FunctionType* self) {
    function_type_fprint(stdout, self);
}

void function_type_println(FunctionType* self) {
    function_type_fprintln(stdout, self);
}