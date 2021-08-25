#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include "parson.h"
#include "db_sql.h"
#include "common.h"
#include "serialization.h"
#include "serialization_test.h"

#define DB_PATH "test.db"


#define RECREATE_AND_TEST_DB TRUE

char* serialized_string = NULL;
char* sqliteErrMsg = NULL;
sqlite3* connection;
const char* sql = NULL;


JSON_Status execute_sql(
  void* exec_data, int cb(void*, int, char**, char**)
);


int main(int argc, char* argv[])
{
  sqlite3* db;
  char* zErrMsg = 0;
  int rc;
  const char* data = "Callback function called";

  /* Open database */

  if (db_connect_verbose(DB_PATH, &db) != SQLITE_OK) {
    exit(EXIT_FAILURE);
  }

  if (RECREATE_AND_TEST_DB && create_and_test_db(db) != SUCCESS) {
    exit(EXIT_FAILURE);
  }

  printf("\n\n\n ====== Testing SQL execution and serialization of result to JSON...======\n\n\n");

  connection = db;
  sql = str_dup("SELECT ID, NAME FROM COMPANY");

  if (serialize_to_json(execute_sql, &serialized_string) == JSONSuccess) {
    if (serialized_string) {
      puts(serialized_string);
      free(serialized_string);
    }
  }
  else {
    puts("JSON serialization failed");
  }

  free((void*)sql);
  sql = NULL;

  sqlite3_close(db);

  // test_serialization();

  return 0;
}


JSON_Status execute_sql(
  void* exec_data, int cb(void*, int, char**, char**)
)
{
  char* zErrMsg = NULL;
  int result = sqlite3_exec(connection, sql, cb, exec_data, &zErrMsg);
  if (result != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqliteErrMsg = str_dup(zErrMsg);
    sqlite3_free(zErrMsg);
  }
  else {
    if (sqliteErrMsg) {
      free(sqliteErrMsg);
      sqliteErrMsg = NULL;
    }
  }
  return result == SUCCESS ? JSONSuccess : JSONFailure;
}
