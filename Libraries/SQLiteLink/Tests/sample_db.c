#include <stdio.h>
#include "../Common/common.h"
#include "sample_db.h"


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


static int sqlite_simple_callback_verbose(void* data, int argc, char** argv, char** azColName)
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

static int sqlite_simple_callback(void* data, int argc, char** argv, char** azColName)
{
    return 0;
}


int db_connect_verbose(const char* db_path, sqlite3** connection_ptr, BOOL verbose) {
    int result;
    result = sqlite3_open(db_path, connection_ptr);
    if(verbose){
        if (result) {
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(*connection_ptr));
        }
        else {
            fprintf(stderr, "Opened database successfully\n");
        }
    }
    return result;
}


int db_sql_execute_verbose(
    sqlite3* conn, const char* sql, const char* success_msg, BOOL verbose
) {
    int result;
    char* zErrMsg = 0;
    const char* data = "Callback function called";

    if (!success_msg) {
        success_msg = "Operation was successful";
    }
    result = sqlite3_exec(
        conn, 
        sql, 
        verbose ? sqlite_simple_callback_verbose : sqlite_simple_callback, 
        (void*)data, 
        &zErrMsg
    );
    if (result != SQLITE_OK) {
        if(verbose){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
        }
        if (zErrMsg){
            sqlite3_free(zErrMsg);
        }
    }
    else {
        if(verbose){
            fprintf(stdout, "%s\n", success_msg);
        }
    }
    return result;
}


int create_and_test_db(
    sqlite3* conn,
    const char* create_schema_sql,
    const char* insert_data_sql,
    const char* test_sql, 
    BOOL verbose
) {
    int status = db_sql_execute_verbose(
        conn, 
        create_schema_sql ? create_schema_sql : CREATE_TABLE_SQL, 
        "Schema created successfully",
        verbose
    );

    if (status != SQLITE_OK) {
        return FAILURE;
    }

    status = db_sql_execute_verbose(
        conn, 
        insert_data_sql ? insert_data_sql : INSERT_DATA_SQL, 
        "Records inserted successfully",
        verbose
    );

    if (status != SQLITE_OK) {
        return FAILURE;
    }

    status = db_sql_execute_verbose(
        conn, 
        test_sql ? test_sql : SELECT_DATA_SQL, 
        "Operation done successfully\n",
        verbose
    );

    if (status != SQLITE_OK) {
        return FAILURE;
    }

    return SUCCESS;
}


int db_disconnect_verbose(sqlite3* conn, BOOL verbose){
    if(verbose){
        printf("About to disconnect from the database...\n");
    }
    if (sqlite3_close(conn) == SQLITE_OK) {
        if(verbose){
            printf("Successfully disconnected from the database\n");
        }
        return SUCCESS;
    } else {
        if(verbose){
            printf("Failed to successfully disconnect from the database\n");
        }
        return FAILURE;
    }
}


int create_and_test_db_complete(
    const char* path,
    const char* create_schema_sql,
    const char* insert_data_sql,
    const char* test_sql, 
    BOOL verbose
) {
    sqlite3* conn = NULL;

    if (db_connect_verbose(path, &conn, verbose) != SQLITE_OK) {
        return FAILURE;
    }

    if (create_and_test_db(conn, create_schema_sql, insert_data_sql, test_sql, verbose) != SUCCESS) {
        return FAILURE;
    }

    if (db_disconnect_verbose(conn, verbose) != SUCCESS){
        return FAILURE;
    }

    return SUCCESS;
}