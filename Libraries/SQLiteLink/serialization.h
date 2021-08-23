#ifndef SERIALIZATION_INCLUDED
#define SERIALIZATION_INCLUDED

#include "parson.h"

// void refresh_JSON_serialization_callback_state(void);

JSON_Status serialize_to_json(
  JSON_Status exec(
    void * exec_data,
    int cb(void * , int , char **, char **)
  ),
  char **result_ptr
);

#endif