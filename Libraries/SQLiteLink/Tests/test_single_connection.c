#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Common/common.h"
#include "../Common/single_connection.h"
#include "sample_db.h"

#define return_on_failure(msg, init, cond)              \
do{                                                     \
    BOOL failed;                                        \
    printf("%s...", msg);                               \
    do {init;} while(0);                                \
    failed = (cond);                                    \
    printf("%s\n", failed ? "Failed" : "Ok");           \
    if (failed) {                                       \
        return EXIT_FAILURE;                            \
    }                                                   \
} while(0)


#define SCONN_DB_PATH "test_sconn.db"


connection_info execution_data;

const char* simple_sql = "SELECT COUNT(*) AS row_count FROM COMPANY";
const char* col_name;
const char* result;


static int simple_test_callback(void* data, int argc, char** argv, char** azColName)
{
    int i;
    if(argc == 1){
        col_name = azColName[0];
        result = argv[0];
    }
    return 0;
}


int main(int argc, char** argv){
    const char* sql = argc > 1 ? argv[1] : "SELECT ID, NAME FROM COMPANY";
    connection_info* conn = &execution_data;

    return_on_failure(
        "Creating sample db for tests",
        NULL,
        create_and_test_db_complete(SCONN_DB_PATH, NULL, NULL, NULL, FALSE) != SUCCESS
    );

    printf("Creating a connection...\n");
    sqlite_connection_create(conn, SCONN_DB_PATH);
    
    return_on_failure(
        "Connecting to db",
        NULL,
        sqlite_connect(conn) != SQLITE_SUCCESS
    );

    return_on_failure(
        "Testing sqlite_is_connected(), should yield TRUE",
        NULL,
        sqlite_is_connected(conn) != TRUE
    );

    return_on_failure(
        "Testing sqlite_execute_sql()",
        sqlite_set_sql_string(conn, simple_sql),
        sqlite_execute_sql(conn, simple_test_callback) != SQLITE_SUCCESS
            || !strcmp(col_name, "row_count")
            || !strcmp(result, "4")
    );

    return_on_failure(
        "Testing sqlite_disconnect()",
        NULL,
        sqlite_disconnect(conn) != SQLITE_SUCCESS || sqlite_is_connected(conn)
    );

    printf("\n\n******* All tests passed ********\n\n");

    return EXIT_SUCCESS;
}