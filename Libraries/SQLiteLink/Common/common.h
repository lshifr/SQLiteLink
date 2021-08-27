#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED

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

char* str_dup(const char* src);

#endif