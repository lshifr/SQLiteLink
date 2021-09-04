#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED

#include <stdlib.h>

#define TRUE 1
#define FALSE 0
#define BOOL int

#define SUCCESS 0
#define FAILURE 1

#define DEBUG FALSE

#define DEBUG_STMT(e) do {\
    if(DEBUG){ \
        (e);\
    }\
}\
while(0)

#define SET_DALLOC_POINTER_FIELD(pointer, field, value)     \
do {                                                        \
    if ((pointer)->field) {                                 \
        free((void*)(pointer)->field);                      \
    }                                                       \
    (pointer)->field = (value);                             \
} while(0)  

char* str_dup(const char* src);

#endif