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