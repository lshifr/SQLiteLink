#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sqlite3.h"

#include "WolframLibrary.h"
#include "parson.h"
#include "db_sql.h"
#include "common.h"
#include "serialization.h"
//#include "serialization_test.h"


char* serialized_string = NULL;
char* sqliteErrMsg = NULL;
sqlite3* connection = NULL;
const char* sql = NULL;
char* file_path = NULL;


static JSON_Status exec_sql(
    void* exec_data, int cb(void*, int, char**, char**)
);

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
    if (connection) {
        // TODO: as written, this can segfault. Needs a better test that connection is valid
        // TODO: handle error, or disallow connecting when connection is open
        sqlite3_close(connection);
    }
    file_path = str_dup(path);
    result = sqlite3_open(file_path, &connection);
    libData->UTF8String_disown(path);
    MArgument_setInteger(Res, result);
    return LIBRARY_NO_ERROR;
}

DLLEXPORT int sqlite_disconnect(WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res) {
    int result = 0;
    if (connection) {
        result = sqlite3_close(connection);
        connection = NULL;
    }
    MArgument_setInteger(Res, result);
    return LIBRARY_NO_ERROR;
}


DLLEXPORT int sqlite_execute(WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res) {
    char* sql_string = MArgument_getUTF8String(Args[0]);
    sql = str_dup(sql_string);
    JSON_Status result;
    libData->UTF8String_disown(sql_string);
    free_globals();
    result = serialize_to_json(exec_sql, &serialized_string);
    free((void*)sql);
    sql = NULL;
    if (result == JSONSuccess && serialized_string) {
        MArgument_setUTF8String(Res, serialized_string);
        return LIBRARY_NO_ERROR;
    }
    else {
        if (serialized_string) {
            free(serialized_string);
            serialized_string = NULL;
        }
        return LIBRARY_FUNCTION_ERROR;
    }
}

DLLEXPORT int sqlite_get_last_error_string(
    WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res
)
{
    char* result = sqliteErrMsg ? sqliteErrMsg : "";
    MArgument_setUTF8String(Res, result);
    return LIBRARY_NO_ERROR;
}

DLLEXPORT int free_serialized_JSON_string(
    WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res
) {

    return LIBRARY_NO_ERROR;
}

static void free_globals(void) {
    if (serialized_string) {
        free(serialized_string);
        serialized_string = NULL;
    }
    if (sqliteErrMsg) {
        free(sqliteErrMsg);
        sqliteErrMsg = NULL;
    }
    if (file_path) {
        free(file_path);
        file_path = NULL;
    }
}

static JSON_Status exec_sql(
    void* exec_data, int cb(void*, int, char**, char**)
)
{
    char* zErrMsg = NULL;
    int result = sqlite3_exec(connection, sql, cb, exec_data, &zErrMsg);
    if (result != SQLITE_OK) {
        // fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqliteErrMsg = str_dup(zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else {
        if (sqliteErrMsg) {
            free(sqliteErrMsg);
            sqliteErrMsg = NULL;
        }
    }
    return result == SUCCESS ? JSONSuccess : JSONFailure;
}