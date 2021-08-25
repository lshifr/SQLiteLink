#ifndef DB_SQL_INCLUDED
#define DB_SQL_INCLUDED

#define SUCCESS 0
#define FAILURE 1

int create_and_test_db(sqlite3* conn);
int db_connect_verbose(char* db_path, sqlite3** connection_ptr);

#endif