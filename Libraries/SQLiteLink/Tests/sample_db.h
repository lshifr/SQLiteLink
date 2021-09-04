#ifndef TEST_DB_INCLUDED
#define TEST_DB_INCLUDED

#include "../Common/common.h"
#include <sqlite3.h>

/**
 *  Simple wrappers around SQLite API, which add some debugging info
 *  and streamline creation of simple db for tests.
 *
 */

int db_connect_verbose(const char* db_path, sqlite3** connection_ptr, BOOL verbose);

int db_sql_execute_verbose(
    sqlite3* conn, const char* sql, const char* success_msg, BOOL verbose
);

int db_disconnect_verbose(sqlite3* conn, BOOL verbose);

int create_and_test_db(
    sqlite3* conn,
    const char* create_schema_sql,
    const char* insert_data_sql,
    const char* test_sql,
    BOOL verbose
);

int create_and_test_db_complete(
    const char* path, 
    const char* create_schema_sql, 
    const char* insert_data_sql,
    const char* test_sql,
    BOOL verbose
);


#endif