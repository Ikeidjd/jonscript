#include "str_pool.h"

#include <stdlib.h>

#include "hash_set.h"

StrPool str_pool_new() {
    return (StrPool) {
        .data = NULL,
        .size = 0,
        .occupied = 0
    };
}

void str_pool_destruct(StrPool self) {
    for(size_t i = 0; i < self.size; i++) {
        if(self.data[i] != NULL) object_str_free(self.data[i]);
    }
    free(self.data);
}

HASH_SET_FIND_FUNCTION(StrPool, ObjectStr*, str_pool)
HASH_SET_INSERT_NO_RESIZE_AND_NO_CHECK_DUPLICATES_FUNCTION(StrPool, ObjectStr*, str_pool)
HASH_SET_RESIZE_FUNCTION(StrPool, ObjectStr*, str_pool)

ObjectStr* str_pool_insert(StrPool* self, ObjectStr* str) {
    if(self->size <= self->occupied * 2) str_pool_resize(self);

    ObjectStr* found = str_pool_find(self, str);
    if(found != NULL) {
        object_str_free(str);
        return found;
    }

    str_pool_insert_no_resize_and_no_check_duplicates(self, str);
    return str;
}