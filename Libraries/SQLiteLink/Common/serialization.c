#include <stdlib.h>
#include <stdio.h> // puts
#include "parson.h"
#include "common.h"
#include "serialization.h"


BOOL first_run = TRUE;
BOOL callback_operation_failed = FALSE;


static void refresh_JSON_serialization_callback_state() {
  first_run = TRUE;
  callback_operation_failed = FALSE;
}


int json_serialization_callback(
  void* data, int argc, char** argv, char** azColName
) {
  // TODO: improve sloppy resource / memory management
  JSON_Object* obj = (JSON_Object*)data;
  //static BOOL first_run = TRUE;

  // TODO : temporary, until we either start throwing exceptions from  callback, or
  // switch to using the step interface
  if (callback_operation_failed) {
    return FAILURE;
  }

  for (int i = 0; i < argc; i++) {
    const char* col_name = azColName[i];
    JSON_Status status = JSONFailure;
    DEBUG_STMT(printf("The column name: %s, value: %s\n", col_name, argv[i]));
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


int serialize_to_json(
  void* exec_data,
  void set_callback_data(void* exec_data, void* callback_data),
  int exec(
    void* exec_data,
    int cb(void*, int, char**, char**)
  ),
  const char** result_ptr
) {
  char* serialized_string = NULL;
  int result = FAILURE;
  JSON_Value* root_val = json_value_init_object();
  JSON_Object* main_obj = json_value_get_object(root_val);
  refresh_JSON_serialization_callback_state();
  set_callback_data(exec_data, (void*)main_obj);
  result = exec(exec_data, json_serialization_callback);
  if (result == SUCCESS) {
    serialized_string = json_serialize_to_string_pretty(root_val);
    if (serialized_string) {
      *result_ptr = str_dup(serialized_string); // TODO: handle NULL returned from str_dup
      json_free_serialized_string(serialized_string);
      DEBUG_STMT(printf("Serialized to %s\n", *result_ptr));
    }
    else {
      *result_ptr = NULL;
      DEBUG_STMT(printf("Execution / serialization failed\n"));
    }
  }
  // This seems to also free any strings in string arrays, keys, etc.
  // TODO: double-check that!
  json_value_free(root_val);
  set_callback_data(exec_data, NULL);
  return result;
}

