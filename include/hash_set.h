#pragma once

#include <stdlib.h>

// (element->hash & (self->size - 1)) is like (element->hash % self->capacity) but more optimized. Only works with powers of two, though

#define HASH_SET_FIND_FUNCTION(hash_set_type, element_type, equality_function, prefix) \
    element_type prefix ## _find(hash_set_type* self, element_type element) { \
        size_t index = (element->hash & (self->size - 1)); \
        for(size_t counter = 0; counter < self->size && self->data[index] != NULL; counter++) { \
            if(equality_function(self->data[index], element)) return self->data[index]; \
            index = ((index + 1) & (self->size - 1)); \
        } \
        return NULL; \
    }

// A self-insert without resizing, huh... what would be a self-insert with resizing, an inflation fantasy? Really makes you think. The no_check_duplicates could be selfcest
#define HASH_SET_INSERT_NO_RESIZE_AND_NO_CHECK_DUPLICATES_FUNCTION(hash_set_type, element_type, prefix) \
    static void prefix ## _insert_no_resize_and_no_check_duplicates(hash_set_type* self, element_type element) { \
        size_t index = (element->hash & (self->size - 1)); \
        for(size_t counter = 0; counter < self->size && self->data[index] != NULL; counter++) index = ((index + 1) & (self->size - 1)); \
        self->data[index] = element; \
        self->occupied++; \
    }

#define HASH_SET_RESIZE_FUNCTION(hash_set_type, element_type, prefix) \
    static void prefix ## _resize(hash_set_type* self) { \
        size_t old_size = self->size; \
        element_type* old_data = self->data; \
\
        if(self->size == 0) self->size = 64; \
        else self->size *= 2; \
\
        self->data = malloc(self->size * sizeof(element_type)); \
\
        for(size_t i = 0; i < self->size; i++) self->data[i] = NULL; \
\
        for(size_t i = 0; i < old_size; i++) { \
            if(old_data[i] != NULL) prefix ## _insert_no_resize_and_no_check_duplicates(self, old_data[i]); \
        } \
\
        free(old_data); \
    }