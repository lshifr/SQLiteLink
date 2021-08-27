#ifndef CONNECTIONS_INCLUDED
#define CONNECTIONS_INCLUDED

#include "common.h"

#include <sqlite3.h>

typedef struct connection_info {
    const char* serialized_string;
    const char* sqliteErrMsg;
    const char* file_path;
    sqlite3* connection;
    const char* sql;
    void* serialization_callback_data;
    BOOL in_use;
} connection_info;

#define MAX_ACTIVE_CONNECTIONS 10000
#define SQLITE_SUCCESS 0
#define MAX_CONNECTIONS_EXCEEDED -1
#define INVALID_CONNECTION_INDEX 1
#define SQLITE_CONNECTION_FAILURE 2
#define SQLITE_DISCONNECTION_FAILURE 3


int new_connection(const char* path);
int destroy_connection(int cindex);
BOOL connection_exists(int cindex);

int connect(int cindex);
int disconnect(int cindex);
BOOL is_connected(int cindex);

connection_info* get_connection_info(int cindex);
// const char* get_connection_info_JSON_string(int index);


#endif