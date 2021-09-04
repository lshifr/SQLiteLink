#include "WolframLibrary.h"
#include "Common/single_connection.h"
#include "Common/connections.h"
#include "Common/serialization.h"


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


DLLEXPORT int SQLiteLink_new_connection(WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res) {
    char* path = MArgument_getUTF8String(Args[0]);
    int chandle_index = new_connection(path);
    libData->UTF8String_disown(path); // This is safe
    MArgument_setInteger(Res, chandle_index); // chandle_index can be negative, should be interpreted as error on WL side
    return LIBRARY_NO_ERROR;
}


DLLEXPORT int SQLiteLink_destroy_connection(WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res) {
    int chandle_index = MArgument_getInteger(Args[0]);
    int result = destroy_connection(chandle_index);
    MArgument_setInteger(Res, result);
    return LIBRARY_NO_ERROR;
}


DLLEXPORT int SQLiteLink_connect(WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res) {
    int chandle_index = MArgument_getInteger(Args[0]);
    int result = connect(chandle_index);
    MArgument_setInteger(Res, result);
    return LIBRARY_NO_ERROR;
}


DLLEXPORT int SQLiteLink_disconnect(WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res) {
    int chandle_index = MArgument_getInteger(Args[0]);
    int result = disconnect(chandle_index);
    MArgument_setInteger(Res, result);
    return LIBRARY_NO_ERROR;
}


DLLEXPORT int SQLiteLink_is_connected(WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res) {
    int chandle_index = MArgument_getInteger(Args[0]);
    int result = is_connected(chandle_index);
    MArgument_setInteger(Res, result);
    return LIBRARY_NO_ERROR;
}


DLLEXPORT int SQLiteLink_execute(WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res) {
    int chandle_index = MArgument_getInteger(Args[0]);
    char* sql_string = MArgument_getUTF8String(Args[1]);
    int result;
    connection_info* conn = get_connection_info(chandle_index);
    if (!conn) {
        result = INVALID_CONNECTION_INDEX;
    }
    else if (!sqlite_is_connection_in_use(conn)) {
        result = CONNECTION_DOES_NOT_EXIST;
    }
    else if (!sqlite_is_connected(conn)) {
        result = CONNECTION_DISCONNECTED;
    }
    else {
        result = sqlite_execute_sql_with_serialization(
            conn,
            sql_string,
            serialize_to_json
        );
    }
    libData->UTF8String_disown(sql_string);
    MArgument_setInteger(Res, result);
    return LIBRARY_NO_ERROR;
}


DLLEXPORT int SQLiteLink_get_serialized_string(WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res) {
    const char* result = NULL;
    int chandle_index = MArgument_getInteger(Args[0]);
    connection_info* conn = get_connection_info(chandle_index);
    if (!conn || !sqlite_is_connection_in_use(conn)) {
        return LIBRARY_FUNCTION_ERROR;
    }
    result = sqlite_get_serialized_string(conn);
    if (!result) {
        return LIBRARY_FUNCTION_ERROR;
    }
    MArgument_setUTF8String(Res, (char*)result);
    return LIBRARY_NO_ERROR;
}


DLLEXPORT int SQLiteLink_get_error_string(WolframLibraryData libData, mint Argc, MArgument* Args, MArgument Res) {
    const char* result = NULL;
    int chandle_index = MArgument_getInteger(Args[0]);
    connection_info* conn = get_connection_info(chandle_index);
    if (!conn || !sqlite_is_connection_in_use(conn)) {
        return LIBRARY_FUNCTION_ERROR;
    }
    result = sqlite_get_error_message(conn);
    if (!result) {
        return LIBRARY_FUNCTION_ERROR;
    }
    MArgument_setUTF8String(Res, (char*)result);
    return LIBRARY_NO_ERROR;
}