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
#define SERIALIZED_COLUMN_NAME "ID"

#define MAX_ROWS 10

typedef struct column {
    const char* rows[MAX_ROWS];
    int len;
    const char* col_name;
} column;


connection_info execution_data;
column storage;

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


static const char* concat_all(int argc, char* delimeter, const char** argv) {
    int total_len = 0;
    char* result = NULL;
    for (int i = 0; i < argc; i++) {
        total_len += strlen(argv[i]);
    }
    if(delimeter){
        total_len += (argc - 1) * strlen(delimeter);
    }
    result = (char*)malloc(total_len + 1);
    if (!result) {
        return NULL;
    }
    for (int i = 0, j = 0; i < argc; i++) {
        const char* start = argv[i];
        while (*start) {
            result[j++] = *start++;
        }
        if(delimeter){
            start = delimeter;
            while (*start) {
                result[j++] = *start++;
            }
        }
    }
    result[total_len] = '\0';
    return result;
}

static int simple_column_serializer_callback(void* data, int argc, char** argv, char** azColName){
    column * storage = (column*) data;
    for(int i = 0; i < argc; i++){
        if(!strcmp(azColName[i], storage->col_name) && storage->len < MAX_ROWS){
            (storage -> rows)[storage->len++] = str_dup(argv[i]); // TODO: handle NULL returned by str_dup
        }
    }
    return 0;
}

/**
 *  Simple table column serializer. Column name is pre-defined globally.
 *  Serializes single column of resulting (virtual) table obtained by 
 *  executing a query, using simple string concatenation with comma as a 
 *  delimeter. 
 */ 
static int column_serializer(
    void* exec_data,
    void set_callback_data(void* exec_data, void* callback_data),
    int exec(
        void* exec_data,
        int cb(void*, int, char**, char**)
    ),
    const char** result_ptr
){
    int result;
    column* col = &storage;
    const char* serialized = NULL;
    col->col_name = SERIALIZED_COLUMN_NAME;
    col->len = 0;
    set_callback_data(exec_data, (void*)col);
    result = exec(exec_data, simple_column_serializer_callback);
    if(result == SUCCESS){
        serialized = concat_all(col->len, ",", col->rows);
        if (!serialized) {
            result = FAILURE;
        } 
    } 
    for (int i = 0; i < col->len; i++) {
        if (col->rows[i]) {
            free((void *) col->rows[i]);
        }
    }
    if(result_ptr){
        *result_ptr = serialized;
    } else {
        free((void *) serialized);
    }
    return result;
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
        "Testing sqlite_execute_sql_with_serialization(), simple column serializer",
        NULL,
        sqlite_execute_sql_with_serialization(
            conn, 
            "SELECT * FROM COMPANY", 
            column_serializer
        ) != SUCCESS || strcmp(conn->serialized_string, "1,2,3,4")
    );

    return_on_failure(
        "Testing sqlite_disconnect()",
        NULL,
        sqlite_disconnect(conn) != SQLITE_SUCCESS || sqlite_is_connected(conn)
    );

    printf("\n\n******* All tests passed ********\n\n");

    return EXIT_SUCCESS;
}