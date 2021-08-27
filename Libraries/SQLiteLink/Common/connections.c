#include <stdio.h>
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
    conn->file_path = path;
    conn->in_use = TRUE;
    conn->connection = NULL;
    conn->serialization_callback_data = NULL;
    conn->sql = NULL;
    conn->sqliteErrMsg = NULL;
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
    init_connection(conn_info, NULL);
    conn_info->in_use = FALSE;
    return SQLITE_SUCCESS;
}


BOOL is_connected(int cindex) {
    return connection_exists(cindex) && is_connected_unchecked(get_connection_info(cindex));
}


// const char* get_connection_info_JSON_string(int index);
