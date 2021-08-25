#include <stdlib.h>
#include "parson.h"
#include "common.h"
#include "serialization.h"
#include "common.h"
#include <stdio.h> // puts

BOOL first_run = TRUE;
BOOL callback_operation_failed = FALSE;


static void refresh_JSON_serialization_callback_state() {
  first_run = TRUE;
  callback_operation_failed = FALSE;
}


JSON_Status execute_mock_db(
  void* exec_data, int cb(void*, int, char**, char**)
);

int json_serialization_callback(
  void* data, int argc, char** argv, char** azColName
) {
  // puts("In json_serialization_callback()");

  // TODO: improve sloppy resource / memory management
  JSON_Object* obj = (JSON_Object*)data;
  //static BOOL first_run = TRUE;

  // TODO : temporary, until we either start throwing exceptions from  callback, or
  // switch to using the step interface
  //static BOOL failed = FALSE; 

  if (callback_operation_failed) {
    return FAILURE;
  }

  for (int i = 0; i < argc; i++) {
    const char* col_name = azColName[i];
    JSON_Status status = JSONFailure;
    // printf("The column name: %s, value: %s\n", col_name, argv[i]);
    if (first_run) {
      // Create new key and json array for values, for a column
      char* key = str_dup(col_name);
      JSON_Value* col_values = json_value_init_array();
      if (!col_values) {
        callback_operation_failed = TRUE;
        return FAILURE;
      }
      status = json_object_set_value(obj, key, col_values);
      if (status != JSONSuccess) {
        callback_operation_failed = TRUE;
        return FAILURE;
      }
    }
    JSON_Array* cvals = json_object_get_array(obj, col_name);
    if (!cvals) {
      // Missing column should be considered a failure
      callback_operation_failed = TRUE;
      return FAILURE;
    }
    // Append column value for this row
    char* val_copy = str_dup(argv[i]);
    status = json_array_append_string(cvals, val_copy);
    if (status != JSONSuccess) {
      free(val_copy);
      callback_operation_failed = TRUE;
      return FAILURE;
    }
  }
  first_run = FALSE;
  return SUCCESS;
}


JSON_Status serialize_to_json(
  JSON_Status exec(
    void* exec_data,
    int cb(void*, int, char**, char**)
  ),
  char** result_ptr
) {
  char* serialized_string = NULL;
  JSON_Status status = JSONFailure;

  JSON_Value* root_val = json_value_init_object();
  JSON_Object* main_obj = json_value_get_object(root_val);

  refresh_JSON_serialization_callback_state();
  status = exec((void*)main_obj, json_serialization_callback);

  if (status == JSONSuccess) {
    serialized_string = json_serialize_to_string_pretty(root_val);
    if (serialized_string) {
      *result_ptr = str_dup(serialized_string);
      json_free_serialized_string(serialized_string);
    }
    else {
      status = JSONFailure;
      *result_ptr = NULL;
    }
  }
  // This seems to also free any strings in string arrays, keys, etc.
  // TODO: double-check that!
  json_value_free(root_val);
  return status;
}

