#include <stdio.h>
#include <assert.h>
#include "common.h"
#include "single_connection.h"


static void clear_error_message(connection_info* cinfo) {
    SET_DALLOC_POINTER_FIELD(cinfo, sqliteErrMsg, NULL);
}


static void clear_sql_string(connection_info* cinfo) {
    SET_DALLOC_POINTER_FIELD(cinfo, sql, NULL);
}


static void refresh_connection_info(connection_info* cinfo) {
    SET_DALLOC_POINTER_FIELD(cinfo, serialized_string, NULL);
    clear_sql_string(cinfo);
    clear_error_message(cinfo);
}


static void set_sql_string(connection_info* cinfo, const char* sql) {
    clear_sql_string(cinfo);
    cinfo->sql = str_dup(sql); // TODO: handle NULL pointer from str_dup
}


void sqlite_connection_create(connection_info* conn, const char* path) {
    if (conn->in_use) {
        return; // Can only init unused connection
    }
    conn->file_path = str_dup(path); // TODO: handle NULL pointer from str_dup
    conn->serialized_string = NULL;
    conn->in_use = TRUE;
    conn->connection = NULL;
    conn->sqlite_callback_data = NULL;
    conn->sql = NULL;
    conn->sqliteErrMsg = NULL;
}


sqlite_rcode sqlite_connection_destroy(connection_info* cinfo) {
    if (sqlite_is_connected(cinfo)) {
        return SQLITE_MUST_DISCONNECT_FIRST;
    }
    refresh_connection_info(cinfo);
    SET_DALLOC_POINTER_FIELD(cinfo, file_path, NULL);
    cinfo->in_use = FALSE;
    return SQLITE_SUCCESS;
}


void sqlite_set_sql_string(connection_info* cinfo, const char* sql) {
    if (cinfo->sql) {
        free((void*)cinfo->sql);
    }
    cinfo->sql = str_dup(sql); // TODO: handle NULL pointer from str_dup
}


void sqlite_set_callback_data(connection_info* cinfo, void* cbdata) {
    cinfo->sqlite_callback_data = cbdata;
}


BOOL sqlite_is_connection_in_use(connection_info* cinfo) {
    return cinfo->in_use;
}


sqlite_rcode sqlite_connect(connection_info* cinfo) {
    int result;
    assert(cinfo->file_path);
    if (sqlite_is_connected(cinfo)) {
        return SQLITE_SUCCESS; // If already connected, do nothing
    }
    result = sqlite3_open(cinfo->file_path, &(cinfo->connection));
    return result == SQLITE_OK ? SQLITE_SUCCESS : SQLITE_CONNECTION_FAILURE;
}


sqlite_rcode sqlite_disconnect(connection_info* cinfo) {
    int result;
    if (!sqlite_is_connected(cinfo)) {
        return SQLITE_SUCCESS; // If already disconnected, do nothing
    }
    result = sqlite3_close(cinfo->connection);
    if (result == SQLITE_OK) {
        cinfo->connection = NULL;
        return SQLITE_SUCCESS;
    }
    return SQLITE_DISCONNECTION_FAILURE;
}


BOOL sqlite_is_connected(connection_info* cinfo) {
    return cinfo->connection != NULL;
}


const char* sqlite_get_error_message(connection_info* cinfo) {
    return cinfo->sqliteErrMsg;
}


const char* sqlite_get_serialized_string(connection_info* cinfo) {
    return cinfo->serialized_string;
}

/*
**  A wrapper around sqlite3_exec, that takes care of error message memory
**  management, and adds a few debugging print statements.
**
**  To execute this on its own, one has to fill in sql string and callback data,
**  into the passed <connection_info> structure, prior to this call.
*/
sqlite_rcode sqlite_execute_sql(connection_info* cinfo, int cb(void*, int, char**, char**)) {
    char* zErrMsg = NULL;
    if (!cinfo->connection) {
        // TODO: This should not actually happen. assert() might be better here.
        DEBUG_STMT(printf("Connection disconnected. SHOULD CONNECT FIRST. Exiting...\n"));
        return SQLITE_CONNECTION_DISCONNECTED;
    }
    assert(cinfo->sql);
    assert(cinfo->file_path);
    DEBUG_STMT(printf(
        "About to execute SQL statement: \n%s\n, for connection %s\n",
        cinfo->sql,
        cinfo->file_path
    ));
    clear_error_message(cinfo);
    int result = sqlite3_exec(
        cinfo->connection,
        cinfo->sql,
        cb,
        cinfo->sqlite_callback_data,
        &zErrMsg
    );
    if (zErrMsg) {
        DEBUG_STMT(fprintf(stderr, "SQL error: %s\n", zErrMsg));
        cinfo->sqliteErrMsg = str_dup(zErrMsg);  // TODO: handle NULL pointer from str_dup
        sqlite3_free(zErrMsg);
    }
    return result == SQLITE_OK ? SQLITE_SUCCESS : SQLITE_EXECUTION_FAILURE;
}


static void set_callback_data(void* cinfo, void* cbdata) {
    ((connection_info*)cinfo)->sqlite_callback_data = cbdata;
}


static int exec_sql_for_serialization(void* exec_data, int cb(void*, int, char**, char**)) {
    return sqlite_execute_sql((connection_info*)exec_data, cb) == SQLITE_SUCCESS ? SUCCESS : FAILURE;
}

/*
**  This executes SQL with a given connection, and serializes the result to string.
**  The serializer function should be passed as a third argument. It can use the
**  passed callback data setter (set_sqlite_callback_data here), on exec_data
**  (which is a pointer to connection data, in disguise), to set whatever callback
**  information it will need. It is supposed to call <exec> (<exec_sql_for_serialization>
**  here), somewhere in its body, providing its own custom callback function, which
**  will be called inside <exec> for each data row. That custom callback function
**  call allows serializer to collect the necessary info for that row serialization,
**  adding it to the callback data field that it has supposedly previously set.
*/
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
) {
    refresh_connection_info(cinfo);
    set_sql_string(cinfo, sql);
    return serializer(
        (void*)cinfo,
        set_callback_data,
        exec_sql_for_serialization,
        &(cinfo->serialized_string)
    );
}