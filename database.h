#pragma once

#include <sqlite3.h>
struct cJSON;

int db_open(sqlite3 **dbp, const char * filename, const char *schema);
int db_close(sqlite3 *db);
int db_write_faction(sqlite3 *db, struct cJSON *json);
int db_write_region(sqlite3 *db, struct cJSON *json);
int db_write_unit(sqlite3 *db, struct cJSON *json);
int db_write_ship(sqlite3 *db, struct cJSON *json);
int db_write_building(sqlite3 *db, struct cJSON *json);
