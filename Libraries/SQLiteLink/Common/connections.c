#include <stdio.h>
#include "common.h"
#include "connections.h"


static connection_info connections[MAX_ACTIVE_CONNECTIONS];


static int get_first_available_connection_index() {
    for (int i = 0; i < MAX_ACTIVE_CONNECTIONS; i++) {
        if (!connections[i].in_use) {
            return i;
        }
    }
    return MAX_CONNECTIONS_EXCEEDED;
}


static void init_connection(connection_info* conn, const char* path) {
    if (conn->in_use) {
        return; // Can only init unused connection
    }
    conn->file_path = str_dup(path); // TODO: handle NULL pointer from str_dup
    conn->in_use = TRUE;
    conn->connection = NULL;
    conn->sqlite_callback_data = NULL;
    conn->sql = NULL;
    conn->sqliteErrMsg = NULL;
}


static void refresh_connection_info(connection_info* cinfo) {
    if (cinfo->serialized_string) {
        free((void*)cinfo->serialized_string);
        cinfo->serialized_string = NULL;
    }
    if (cinfo->sqliteErrMsg) {
        free((void*)cinfo->sqliteErrMsg);
        cinfo->sqliteErrMsg = NULL;
    }
    if (cinfo->sql) {
        free((void*)cinfo->sql);
        cinfo->sql = NULL;
    }
}


static int is_connected_unchecked(connection_info* conn) {
    return conn->connection != NULL;
}


int new_connection(const char* path) {
    int index = get_first_available_connection_index();
    if (index == MAX_CONNECTIONS_EXCEEDED) {
        return MAX_CONNECTIONS_EXCEEDED;
    }
    init_connection(&(connections[index]), path);
    return index;
}


connection_info* get_connection_info(int cindex) {
    if (cindex < 0 || cindex > MAX_ACTIVE_CONNECTIONS) {
        return NULL;
    }
    return &(connections[cindex]);
}


BOOL connection_exists(int cindex) {
    connection_info* conn_info = get_connection_info(cindex);
    return conn_info && conn_info->in_use;
}


int connect(int cindex) {
    int result;
    connection_info* conn_info;
    if (!connection_exists(cindex)) {
        return INVALID_CONNECTION_INDEX;
    }
    conn_info = get_connection_info(cindex);
    assert(conn_info->file_path);
    if (is_connected_unchecked(conn_info)) {
        return SQLITE_SUCCESS; // If already connected, do nothing
    }
    result = sqlite3_open(conn_info->file_path, &(conn_info->connection));
    return result == SQLITE_OK ? SQLITE_SUCCESS : SQLITE_CONNECTION_FAILURE;
}


int disconnect(int cindex) {
    int result;
    connection_info* conn_info;
    if (!connection_exists(cindex)) {
        return INVALID_CONNECTION_INDEX;
    }
    conn_info = get_connection_info(cindex);
    if (!is_connected_unchecked(conn_info)) {
        return SQLITE_SUCCESS; // If already disconnected, do nothing
    }
    result = sqlite3_close(conn_info->connection);
    if (result == SQLITE_OK) {
        conn_info->connection = NULL;
        return SQLITE_SUCCESS;
    }
    return SQLITE_DISCONNECTION_FAILURE;
}


int destroy_connection(int cindex) {
    connection_info* conn_info;
    int result;
    if (!connection_exists(cindex)) {
        return INVALID_CONNECTION_INDEX;
    }
    conn_info = get_connection_info(cindex);
    result = disconnect(cindex); // Just in case
    if (result != SQLITE_SUCCESS) {
        return result;
    }
    refresh_connection_info(conn_info);
    if (conn_info->file_path) {
        free((void*)conn_info->file_path);
        conn_info->file_path = NULL;
    }
    conn_info->in_use = FALSE;
    return SQLITE_SUCCESS;
}


BOOL is_connected(int cindex) {
    return connection_exists(cindex) && is_connected_unchecked(get_connection_info(cindex));
}


// const char* get_connection_info_JSON_string(int index);


const char* get_sqlite_error_message(connection_info* cinfo) {
    return cinfo->sqliteErrMsg;
}

/*
**  A wrapper around sqlite3_exec, that takes care of error message memory
**  management, and adds a few debugging print statements.
**
**  To execute this on its own, one has to fill in sql string and callback data,
**  into the passed <connection_info> structure, prior to this call.
*/
int execute_sql(connection_info* cinfo, int cb(void*, int, char**, char**)) {
    char* zErrMsg = NULL;
    if (!cinfo->connection) {
        // TODO: This should not actually happen. assert() might be better here.
        return SQLITE_CONNECTION_DISCONNECTED;
    }
    DEBUG_STMT(printf(
        "About to execute SQL statement: \n%s\n, for connection %s\n",
        cinfo->sql,
        cinfo->file_path
    ));
    int result = sqlite3_exec(
        cinfo->connection,
        cinfo->sql,
        cb,
        cinfo->sqlite_callback_data,
        &zErrMsg
    );
    if (result != SQLITE_OK) {
        DEBUG_STMT(fprintf(stderr, "SQL error: %s\n", zErrMsg));
        if (zErrMsg) {
            cinfo->sqliteErrMsg = str_dup(zErrMsg);  // TODO: handle NULL pointer from str_dup
            sqlite3_free(zErrMsg);
        }
    }
    else {
        if (cinfo->sqliteErrMsg) {
            free((void*)cinfo->sqliteErrMsg);
            cinfo->sqliteErrMsg = NULL;
        }
    }
    return result == SQLITE_OK ? SQLITE_SUCCESS : SQLITE_EXECUTION_FAILURE;
}


static void set_sql_string(connection_info* cinfo, const char* sql) {
    if (cinfo->sql) {
        free((void*)cinfo->sql);
    }
    cinfo->sql = str_dup(sql); // TODO: handle NULL pointer from str_dup
}


static void set_sqlite_callback_data(void* cinfo, void* cbdata) {
    ((connection_info*)cinfo)->sqlite_callback_data = cbdata;
}


static int exec_sql_for_serialization(void* exec_data, int cb(void*, int, char**, char**)) {
    return execute_sql((connection_info*)exec_data, cb);
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
int execute_sql_with_serialization(
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
)
{
    refresh_connection_info(cinfo);
    set_sql_string(cinfo, sql);
    return serializer(
        (void*)cinfo,
        set_sqlite_callback_data,
        exec_sql_for_serialization,
        &(cinfo->serialized_string)
    );
}