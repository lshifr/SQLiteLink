#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "common.h"

char* str_dup(const char* src) {
  char* result = (char*)malloc(strlen(src) + 1), * p = result;
  if (!result) {
    return NULL;
  }
  while ((*p++ = *src++))
    ;
  return result;
}