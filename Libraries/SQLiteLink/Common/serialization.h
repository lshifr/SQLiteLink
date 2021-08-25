#ifndef SERIALIZATION_INCLUDED
#define SERIALIZATION_INCLUDED

#include "parson.h"

// void refresh_JSON_serialization_callback_state(void);

JSON_Status serialize_to_json(
  void* exec_data,
  void set_callback_data(void* exec_data, void* callback_data),
  JSON_Status exec(
    void* exec_data,
    int cb(void*, int, char**, char**)
  ),
  const char** result_ptr
);

#endif