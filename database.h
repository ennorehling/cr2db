#pragma once

#include <sqlite3.h>

int db_create(sqlite3 **dbp, const char * filename, const char *schema);
int db_open(sqlite3 **dbp, const char * filename);
