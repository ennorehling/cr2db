#include "database.h"

#include <stdlib.h>
#include <stdio.h>

static int cb_int_col(void *data, int ncols, char **text, char **name) {
    int *p_int = (int *)data;
    *p_int = atoi(text[0]);
    return SQLITE_OK;
}

int db_create(sqlite3 **dbp, const char * dbname, const char *schema) {
    sqlite3 *db = NULL;
    FILE *F = NULL;
    int err, version = 0;

    err = sqlite3_open(dbname, &db);
    if (err != SQLITE_OK) goto fail;
    err = sqlite3_exec(db, "PRAGMA user_version", cb_int_col, &version, NULL);
    if (err != SQLITE_OK) goto fail;
    if (version == 0) {
        F = fopen(schema, "r");
        if (F) {
            size_t size;
            char *sql;
            fseek(F, 0, SEEK_END);
            size = ftell(F);
            sql = malloc(size);
            if (sql) {
                rewind(F);
                fread(sql, sizeof(char), size, F);
                err = sqlite3_exec(db, sql, NULL, NULL, NULL);
                if (err != SQLITE_OK) goto fail;
            }
            fclose(F);
        }
    }
    *dbp = db;
    return SQLITE_OK;
fail:
    if (F) fclose(F);
    if (db) sqlite3_close(db);
    return err;
}
