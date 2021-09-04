#ifndef SINGLE_CONNECTION_INCLUDED
#define SINGLE_CONNECTION_INCLUDED

#include "common.h"
#include <sqlite3.h>


typedef struct connection_info {
    const char* serialized_string;
    const char* sqliteErrMsg;
    const char* file_path;
    sqlite3* connection;
    const char* sql;
    void* sqlite_callback_data;
    BOOL in_use;
} connection_info;


typedef enum sqlite_return_code {
    SQLITE_SUCCESS = 0,
    SQLITE_CONNECTION_FAILURE,
    SQLITE_DISCONNECTION_FAILURE,
    SQLITE_EXECUTION_FAILURE,
    SQLITE_CONNECTION_DISCONNECTED,
    SQLITE_MUST_DISCONNECT_FIRST
} sqlite_rcode;


void sqlite_connection_create(connection_info* cinfo, const char* path);
sqlite_rcode  sqlite_connection_destroy(connection_info* cinfo);
void sqlite_set_sql_string(connection_info* cinfo, const char* sql);
void sqlite_set_callback_data(connection_info* cinfo, void* cbdata);
BOOL sqlite_is_connection_in_use(connection_info* cinfo);


sqlite_rcode sqlite_connect(connection_info* cinfo);
sqlite_rcode sqlite_disconnect(connection_info* cinfo);
BOOL sqlite_is_connected(connection_info* cinfo);
const char* sqlite_get_error_message(connection_info* cinfo);
sqlite_rcode sqlite_execute_sql(connection_info* cinfo, int cb(void*, int, char**, char**));
const char* sqlite_get_serialized_string(connection_info* cinfo);


int sqlite_execute_sql_with_serialization(
    connection_info* cinfo,
    const char* sql,
    int serializer(
        void* exec_data,
        void set_callback_data(void* exec_data, void* callback_data),
        int exec(
            void* exec_data,
            int cb(void*, int, char**, char**)
        ),
        const char** result_ptr
    )
);


#endif