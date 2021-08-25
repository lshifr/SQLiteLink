#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "serialization.h"

static JSON_Status execute_mock_db(
  void* exec_data, int cb(void*, int, char**, char**)
);

int test_serialization(void) {
  char* result = NULL;

  if (serialize_to_json(execute_mock_db, &result) == JSONSuccess) {
    puts(result);
    free(result);
    return SUCCESS;
  }
  else {
    puts("JSON serialization failed");
    return FAILURE;
  }
}

JSON_Status execute_mock_db(
  void* exec_data, int cb(void*, int, char**, char**)
) {
  char* colnames[] = { "ID", "NAME", "AGE", "ADDRESS", "SALARY" };

  char* test_array[][5] = {
    {"1", "Paul", "32", "California", "20000.00"},
    {"2", "Allen", "25", "Texas", "15000.00" },
    {"3", "Teddy", "23", "Norway", "20000.00" },
    {"4", "Mark", "25", "RichMond", "65000.00"}
  };

  size_t
    n_rows = sizeof(test_array) / sizeof(test_array[0]),
    n_cols = sizeof(test_array[0]) / sizeof(test_array[0][0]);

  for (int i = 0; i < n_rows; i++) {
    if (cb(exec_data, n_cols, test_array[i], colnames) != SUCCESS) {
      puts("Error encountered in callback. Terminating...");
      return JSONFailure;
    }
  }

  return JSONSuccess;
}