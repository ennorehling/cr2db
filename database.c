#include "database.h"

#include <cJSON.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static sqlite3_stmt *g_stmt_insert_faction;

static int cb_int_col(void *data, int ncols, char **text, char **name) {
    int *p_int = (int *)data;
    *p_int = atoi(text[0]);
    return SQLITE_OK;
}

int db_write_faction(sqlite3 *db, struct cJSON *object) {
    int err;
    cJSON *jId = cJSON_GetObjectItem(object, "id");
    if (jId) {
        cJSON *jName = cJSON_GetObjectItem(object, "Parteiname");
        cJSON *jMail = cJSON_GetObjectItem(object, "email");
        char * data;
        err = sqlite3_reset(g_stmt_insert_faction);
        assert(err == SQLITE_OK);
        err = sqlite3_bind_int(g_stmt_insert_faction, 1, jId->valueint);
        assert(err == SQLITE_OK);
        if (jName) {
            err = sqlite3_bind_text(g_stmt_insert_faction, 2, jName->valuestring, -1, SQLITE_STATIC);
            err = sqlite3_bind_text(g_stmt_insert_faction, 5, jName->valuestring, -1, SQLITE_STATIC);
            assert(err == SQLITE_OK);
        }
        else {
            sqlite3_bind_null(g_stmt_insert_faction, 2);
            sqlite3_bind_null(g_stmt_insert_faction, 5);
        }
        if (jMail) {
            err = sqlite3_bind_text(g_stmt_insert_faction, 3, jMail->valuestring, -1, SQLITE_STATIC);
            err = sqlite3_bind_text(g_stmt_insert_faction, 6, jMail->valuestring, -1, SQLITE_STATIC);
            assert(err == SQLITE_OK);
        }
        else {
            sqlite3_bind_null(g_stmt_insert_faction, 3);
            sqlite3_bind_null(g_stmt_insert_faction, 6);
        }
        data = cJSON_Print(object);
        if (data) {
            int sz = (int) strlen(data);
            err = sqlite3_bind_blob(g_stmt_insert_faction, 4, data, sz, SQLITE_TRANSIENT);
            assert(err == SQLITE_OK);
            err = sqlite3_bind_blob(g_stmt_insert_faction, 7, data, sz, SQLITE_TRANSIENT);
            assert(err == SQLITE_OK);
            cJSON_free(data);
        }
        else {
            sqlite3_bind_null(g_stmt_insert_faction, 4);
            sqlite3_bind_null(g_stmt_insert_faction, 7);
        }
        err = sqlite3_step(g_stmt_insert_faction);
        if (err != SQLITE_DONE) return err;
    }
    return SQLITE_OK;
}

int db_open(sqlite3 **dbp, const char * dbname, const char *schema) {
    sqlite3 *db = NULL;
    FILE *F = NULL;
    int err, version = 0;

    err = sqlite3_open_v2(dbname, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (err != SQLITE_OK) goto fail;
    err = sqlite3_exec(db, "PRAGMA user_version", cb_int_col, &version, NULL);
    if (err != SQLITE_OK) goto fail;
    if (version == 0) {
        F = fopen(schema, "rb");
        if (F) {
            size_t size;
            char *sql;
            fseek(F, 0, SEEK_END);
            size = ftell(F);
            sql = malloc(size + 1);
            if (sql) {
                rewind(F);
                fread(sql, sizeof(char), size, F);
                sql[size] = 0;
                err = sqlite3_exec(db, sql, NULL, NULL, NULL);
                if (err != SQLITE_OK) goto fail;
            }
            fclose(F);
        }
    }
    err = sqlite3_prepare_v2(db,
        "INSERT INTO `faction` (`id`, `name`, `email`, `data`) VALUES (?,?,?,?)"
        " ON CONFLICT(`id`) DO UPDATE SET `name`=?, `email`=?, `data`=?",
        -1, &g_stmt_insert_faction, NULL);
    if (err != SQLITE_OK) {
        const char * msg = sqlite3_errmsg(db);
        fputs(msg, stderr);
        return err;
    }

    *dbp = db;
    return SQLITE_OK;
fail:
    if (F) fclose(F);
    if (db) {
        fputs(sqlite3_errmsg(db), stderr);
        sqlite3_close(db);
    }
    return err;
}

int db_close(sqlite3 * db) {
    int err;
    err = sqlite3_finalize(g_stmt_insert_faction);
    assert(err == SQLITE_OK);
    err = sqlite3_close(db);
    return err;
}
