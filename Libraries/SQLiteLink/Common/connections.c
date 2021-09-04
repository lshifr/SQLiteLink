#include <stdio.h>
#include "common.h"
#include "single_connection.h"
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


int new_connection(const char* path) {
    int index = get_first_available_connection_index();
    if (index == MAX_CONNECTIONS_EXCEEDED) {
        return MAX_CONNECTIONS_EXCEEDED;
    }
    sqlite_connection_create(&(connections[index]), path);
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
    return conn_info && sqlite_is_connection_in_use(conn_info);
}


int connect(int cindex) {
    int result;
    connection_info* conn_info;
    if (!connection_exists(cindex)) {
        return INVALID_CONNECTION_INDEX;
    }
    conn_info = get_connection_info(cindex);
    return sqlite_connect(conn_info) == SQLITE_SUCCESS? SUCCESS : FAILURE;
}


int disconnect(int cindex) {
    int result;
    connection_info* conn_info;
    if (!connection_exists(cindex)) {
        return INVALID_CONNECTION_INDEX;
    }
    conn_info = get_connection_info(cindex);
    return sqlite_disconnect(conn_info) == SQLITE_SUCCESS ? SUCCESS : FAILURE;
}


int destroy_connection(int cindex) {
    connection_info* conn_info;
    sqlite_rcode result;
    if (!connection_exists(cindex)) {
        return INVALID_CONNECTION_INDEX;
    }
    conn_info = get_connection_info(cindex);
    result = sqlite_disconnect(conn_info); // Just in case
    if (result != SQLITE_SUCCESS) {
        return FAILURE;
    }
    return sqlite_connection_destroy(conn_info) == SQLITE_SUCCESS ? SUCCESS : FAILURE;
}


BOOL is_connected(int cindex) {
    return connection_exists(cindex) && sqlite_is_connected(get_connection_info(cindex));
}