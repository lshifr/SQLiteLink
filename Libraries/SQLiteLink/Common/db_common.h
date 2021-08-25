#ifndef DB_COMMON_INCLUDED
#define DB_COMMON_INCLUDED

#include <sqlite3.h>

typedef struct db_info {
    const char* serialized_string;
    const char* sqliteErrMsg;
    const char* file_path;
    sqlite3* connection;
    const char* sql;
    void* serialization_callback_data;
} db_info;

#endif