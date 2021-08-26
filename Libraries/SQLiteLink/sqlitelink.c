#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sqlite3.h"

#include "WolframLibrary.h"
#include "parson.h"
#include "Common/common.h"
#include "Common/db_common.h"
#include "Common/serialization.h"


db_info execution_data = {
  .serialized_string = NULL,
  .sqliteErrMsg = NULL,
  .file_path = NULL,
  .connection = NULL,
  .sql = NULL,
  .serialization_callback_data = NULL
};


static db_info* get_execution_data(int connection_index) {
    return &execution_data;
}


static JSON_Status exec_sql(void* exec_data, int cb(void*, int, char**, char**));
static void free_globals(void);


DLLEXPORT mint WolframLibrary_getVersion()
{
    return WolframLibraryVersion;
}


DLLEXPORT int WolframLibrary_initialize(WolframLibraryData libData)
{
    return 0;
}


DLLEXPORT void WolframLibrary_uninitialize(WolframLibraryData libData)
{
    return;
}


DLLEXPORT int sqlite_connect(WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res) {
    char* path = MArgument_getUTF8String(Args[0]);
    int result = 0;
    db_info* dbinfo = get_execution_data(0);
    if (!dbinfo->connection) {
        // TODO: as written, this can segfault. Needs a better test that connection is valid
        // TODO: handle error, or disallow connecting when connection is open
        // sqlite3_close(connection);
        dbinfo->file_path = str_dup(path);
        result = sqlite3_open(dbinfo->file_path, &(dbinfo->connection));
    }
    // file_path = str_dup(path);
    // result = sqlite3_open(file_path, &connection);
    libData->UTF8String_disown(path);
    MArgument_setInteger(Res, result);
    return LIBRARY_NO_ERROR;
}

DLLEXPORT int sqlite_disconnect(WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res) {
    int result = 0;
    db_info* dbinfo = get_execution_data(0);
    if (dbinfo->connection) {
        result = sqlite3_close(dbinfo->connection);
        dbinfo->connection = NULL;
    }
    MArgument_setInteger(Res, result);
    return LIBRARY_NO_ERROR;
}


static void set_json_callback_data(void* exec_data, void* callback_data) {
    ((db_info*)exec_data)->serialization_callback_data = callback_data;
}


DLLEXPORT int sqlite_execute(WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res) {
    char* sql_string = MArgument_getUTF8String(Args[0]);
    db_info* dbinfo = get_execution_data(0);
    dbinfo->sql = str_dup(sql_string);
    JSON_Status result;
    libData->UTF8String_disown(sql_string);
    free_globals();
    result = serialize_to_json(
        (void*)dbinfo,
        set_json_callback_data,
        exec_sql,
        &(dbinfo->serialized_string)
    );
    free((void*)dbinfo->sql);
    printf("Serialized to %s\n", dbinfo->serialized_string);
    dbinfo->sql = NULL;
    if (result == JSONSuccess && dbinfo->serialized_string) {
        MArgument_setUTF8String(Res, (char*)dbinfo->serialized_string);
        return LIBRARY_NO_ERROR;
    }
    else {
        if (dbinfo->serialized_string) {
            free((void*)dbinfo->serialized_string);
            dbinfo->serialized_string = NULL;
        }
        return LIBRARY_FUNCTION_ERROR;
    }
}


DLLEXPORT int sqlite_get_last_error_string(
    WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res
)
{
    db_info* dbinfo = get_execution_data(0);
    const char* result = dbinfo->sqliteErrMsg ? dbinfo->sqliteErrMsg : "";
    MArgument_setUTF8String(Res, (char*)result);
    return LIBRARY_NO_ERROR;
}


static void free_globals(void) {
    db_info* dbinfo = get_execution_data(0);
    if (dbinfo->serialized_string) {
        free((void*)dbinfo->serialized_string);
        dbinfo->serialized_string = NULL;
    }
    if (dbinfo->sqliteErrMsg) {
        free((void*)dbinfo->sqliteErrMsg);
        dbinfo->sqliteErrMsg = NULL;
    }
    if (dbinfo->file_path) {
        free((void*)dbinfo->file_path);
        dbinfo->file_path = NULL;
    }
}


static JSON_Status exec_sql(void* exec_data, int cb(void*, int, char**, char**)) {
    char* zErrMsg = NULL;
    db_info* edt = (db_info*)exec_data;
    printf(
        "About to execute SQL statement: \n%s\n, for connection %s\n",
        edt->sql,
        edt->file_path
    );
    int result = sqlite3_exec(
        edt->connection,
        edt->sql,
        cb,
        edt->serialization_callback_data,
        &zErrMsg
    );
    if (result != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        if (zErrMsg) {
            edt->sqliteErrMsg = str_dup(zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }
    else {
        if (edt->sqliteErrMsg) {
            free((void*)edt->sqliteErrMsg);
            edt->sqliteErrMsg = NULL;
        }
    }
    return result == SUCCESS ? JSONSuccess : JSONFailure;
}