#include <stdlib.h>

#define PUSH(array, value, initial_capacity) \
    do { \
        if((array)->capacity <= (array)->length) { \
            if((array)->capacity == 0) (array)->capacity = (initial_capacity); \
            else (array)->capacity *= 2; \
            (array)->data = realloc((array)->data, (array)->capacity * sizeof(value)); \
        } \
        (array)->data[(array)->length++] = (value); \
    } while(0)