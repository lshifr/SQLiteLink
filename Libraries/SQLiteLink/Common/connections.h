#ifndef CONNECTIONS_INCLUDED
#define CONNECTIONS_INCLUDED

#include "common.h"
#include "single_connection.h"


#define MAX_ACTIVE_CONNECTIONS 1000
#define MAX_CONNECTIONS_EXCEEDED -1
#define INVALID_CONNECTION_INDEX 2
#define CONNECTION_DOES_NOT_EXIST 3
#define CONNECTION_DISCONNECTED 4


int new_connection(const char* path);
int destroy_connection(int cindex);
BOOL connection_exists(int cindex);
int connect(int cindex);
int disconnect(int cindex);
BOOL is_connected(int cindex);
connection_info* get_connection_info(int cindex);


#endif