#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parson.h"
#include "../Common/db_common.h"
#include "../Common/common.h"
#include "../Common/serialization.h"


#define DB_PATH "test.db"
#define RECREATE_AND_TEST_DB TRUE

// General functions testing SQLite API
static int sqlite_simple_callback(void* data, int argc, char** argv, char** azColName);
static int db_connect_verbose(const char* db_path, sqlite3** connection_ptr);
static int db_sql_execute_verbose(
  sqlite3* conn, const char* sql, const char* success_msg
);
static int create_and_test_db(sqlite3* conn);


// Functions related to JSON serialization
static JSON_Status execute_sql_for_serialization(
  void* exec_data, int cb(void*, int, char**, char**)
);
static void set_json_callback_data(void* exec_data, void* callback_data);
static db_info* get_execution_data(int connection_index);

// Global state
const char* CREATE_TABLE_SQL =
"DROP TABLE IF EXISTS COMPANY;"
"CREATE TABLE COMPANY("
"ID INT PRIMARY KEY NOT NULL,"
"NAME TEXT NOT NULL,"
"AGE INT NOT NULL,"
"ADDRESS CHAR(50),"
"SALARY REAL );";

const char* INSERT_DATA_SQL =
"INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "
"VALUES (1, 'Paul', 32, 'California', 20000.00 ); "

"INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "
"VALUES (2, 'Allen', 25, 'Texas', 15000.00 );      "

"INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "
"VALUES (3, 'Teddy', 23, 'Norway', 20000.00 );     "

"INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "
"VALUES (4, 'Mark', 25, 'Rich-Mond ', 65000.00 ); ";

const char* SELECT_DATA_SQL = "SELECT * from COMPANY";

db_info execution_data = {
  .serialized_string = NULL,
  .sqliteErrMsg = NULL,
  .file_path = DB_PATH,
  .connection = NULL,
  .sql = NULL,
  .serialization_callback_data = NULL
};


int main(int argc, char* argv[])
{
  db_info* edt = get_execution_data(0); // Mock multi-connection case
  const char* sql = argc > 1 ? argv[1] : "SELECT ID, NAME FROM COMPANY";
  JSON_Status ser_result = JSONFailure;

  /* Open database */

  if (db_connect_verbose(edt->file_path, &(edt->connection)) != SQLITE_OK) {
    exit(EXIT_FAILURE);
  }

  if (RECREATE_AND_TEST_DB && create_and_test_db(edt->connection) != SUCCESS) {
    exit(EXIT_FAILURE);
  }

  printf("\n\n\n ====== Testing SQL execution and serialization of result to JSON...======\n\n\n");

  /*
  **  Not strictly needed here, emulates the LibraryLink case, where the
  **  original string has to be disowned.
  */
  edt->sql = str_dup(sql);

  ser_result = serialize_to_json(
    (void*)edt,
    set_json_callback_data,
    execute_sql_for_serialization,
    &(edt->serialized_string)
  );

  if (ser_result == JSONSuccess
    ) {
    if (edt->serialized_string) {
      puts("SQL query execution and result serialization successful");
      puts(edt->serialized_string);
      free((void*)edt->serialized_string);
    }
  }
  else {
    puts("JSON serialization failed");
  }

  free((void*)(edt->sql));
  edt->sql = NULL;

  sqlite3_close(edt->connection);

  return 0;
}


static int sqlite_simple_callback(void* data, int argc, char** argv, char** azColName)
{
  int i;
  fprintf(stderr, "%s: \n", (const char*)data);
  for (i = 0; i < argc; i++)
  {
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}


int db_connect_verbose(const char* db_path, sqlite3** connection_ptr) {
  int result;
  result = sqlite3_open(db_path, connection_ptr);
  if (result) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(*connection_ptr));
  }
  else {
    fprintf(stderr, "Opened database successfully\n");
  }
  return result;
}


static int db_sql_execute_verbose(
  sqlite3* conn, const char* sql, const char* success_msg
) {
  int result;
  char* zErrMsg = 0;
  const char* data = "Callback function called";

  if (!success_msg) {
    success_msg = "Operation was successful";
  }
  result = sqlite3_exec(conn, sql, sqlite_simple_callback, (void*)data, &zErrMsg);
  if (result != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
  else {
    fprintf(stdout, "%s\n", success_msg);
  }
  return result;
}


static int create_and_test_db(sqlite3* conn) {
  int status = db_sql_execute_verbose(
    conn, CREATE_TABLE_SQL, "Table created successfully"
  );
  if (status != SQLITE_OK) {
    return FAILURE;
  }

  status = db_sql_execute_verbose(conn, INSERT_DATA_SQL, "Records inserted successfully");
  if (status != SQLITE_OK) {
    return FAILURE;
  }

  status = db_sql_execute_verbose(conn, SELECT_DATA_SQL, "Operation done successfully\n");
  if (status != SQLITE_OK) {
    return FAILURE;
  }

  return SUCCESS;
}


static void set_json_callback_data(void* exec_data, void* callback_data) {
  ((db_info*)exec_data)->serialization_callback_data = callback_data;
}


static db_info* get_execution_data(int connection_index) {
  return &execution_data;
}


static JSON_Status execute_sql_for_serialization(
  void* exec_data,
  int cb(void*, int, char**, char**)
)
{
  db_info* edt = (db_info*)exec_data;
  char* zErrMsg = NULL;
  int result = sqlite3_exec(edt->connection, edt->sql, cb, edt->serialization_callback_data, &zErrMsg);

  if (result != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    if (zErrMsg) {
      edt->sqliteErrMsg = str_dup(zErrMsg);
    }
    sqlite3_free(zErrMsg);
  }
  else if (edt->sqliteErrMsg) {
    free((void*)(edt->sqliteErrMsg));
    edt->sqliteErrMsg = NULL;
  }
  return result == SUCCESS ? JSONSuccess : JSONFailure;
}
