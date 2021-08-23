#include <stdio.h>
#include <sqlite3.h>
#include "db_sql.h"

char* CREATE_TABLE_SQL = 
          "DROP TABLE IF EXISTS COMPANY;"
          "CREATE TABLE COMPANY("
          "ID INT PRIMARY KEY NOT NULL,"
          "NAME TEXT NOT NULL,"
          "AGE INT NOT NULL,"
          "ADDRESS CHAR(50),"
          "SALARY REAL );";

char * INSERT_DATA_SQL = 
          "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "
          "VALUES (1, 'Paul', 32, 'California', 20000.00 ); "

          "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "
          "VALUES (2, 'Allen', 25, 'Texas', 15000.00 );      "

          "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "
          "VALUES (3, 'Teddy', 23, 'Norway', 20000.00 );     "

          "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "
          "VALUES (4, 'Mark', 25, 'Rich-Mond ', 65000.00 ); ";

char * SELECT_DATA_SQL = "SELECT * from COMPANY";


static int callback(void *data, int argc, char **argv, char **azColName)
{
  int i;
  fprintf(stderr, "%s: \n", (const char *)data);
  for (i = 0; i < argc; i++)
  {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}

int db_connect_verbose(char* db_path, sqlite3 ** connection_ptr){
  int result;
  result = sqlite3_open(db_path, connection_ptr);
  if (result){
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(*connection_ptr)); 
  } else{
    fprintf(stderr, "Opened database successfully\n");
  }
  return result;
}

static int db_sql_execute_verbose(
  sqlite3 *conn, const char* sql, const char* success_msg
){
  int result;
  char *zErrMsg = 0;
  const char *data = "Callback function called";

  if(!success_msg){
    success_msg = "Operation was successful";
  }

  result = sqlite3_exec(conn, sql, callback, (void *) data, &zErrMsg);
  if (result != SQLITE_OK){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
  } else {
      fprintf(stdout, "%s\n", success_msg);
  }
  return result;
}


int create_and_test_db(sqlite3 *conn){
  if(db_sql_execute_verbose(
        conn, CREATE_TABLE_SQL, "Table created successfully"
      ) != SQLITE_OK){
      return FAILURE;
    }
  
    if(db_sql_execute_verbose(
        conn, INSERT_DATA_SQL, "Records inserted successfully") != SQLITE_OK
      ){
      return FAILURE;
    }

    if(db_sql_execute_verbose(
        conn, SELECT_DATA_SQL, "Operation done successfully\n") != SQLITE_OK
      ){
      return FAILURE;
    }
    return SUCCESS;
}

