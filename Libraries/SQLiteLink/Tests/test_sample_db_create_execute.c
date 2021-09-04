#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "../Common/common.h"
#include "sample_db.h"


#define DB_PATH "test.db"
#define TEST_DB_VERBOSE TRUE

int main(int argc, char** argv){
    const char* sql = argc > 1 ? argv[1] : "SELECT ID, NAME FROM COMPANY";
    sqlite3* conn = NULL;

    printf("Testing stepwise db creation...\n");

    if (db_connect_verbose(DB_PATH, &conn, TEST_DB_VERBOSE) != SQLITE_OK) {
        return EXIT_FAILURE;
    }

    if (create_and_test_db(conn, NULL, NULL, NULL, TEST_DB_VERBOSE) != SUCCESS) {
        return EXIT_FAILURE;
    }

    if(db_disconnect_verbose(conn, TEST_DB_VERBOSE) != SUCCESS){
        return EXIT_FAILURE;
    }

    printf("\n\nTesting create_and_test_db_complete()\n\n");

    if (create_and_test_db_complete(DB_PATH, NULL, NULL, NULL, TEST_DB_VERBOSE) != SUCCESS){
        return EXIT_FAILURE;
    }

    printf("\nAll tests passed...");

    return EXIT_SUCCESS;
}