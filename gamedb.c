#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "jsondata.h"

#include <cJSON.h>
#include <sqlite3.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static sqlite3_stmt *g_stmt_insert_unit;
static sqlite3_stmt *g_stmt_insert_ship;
static sqlite3_stmt *g_stmt_insert_building;
static sqlite3_stmt *g_stmt_insert_region;
static sqlite3_stmt *g_stmt_insert_faction;

static int db_prepare(sqlite3 *db) {
    int err;
    err = sqlite3_prepare_v2(db,
        "INSERT INTO `unit` (`id`, `data`, `name`, `orders`, `region_id`, `faction_id`) VALUES (?,?,?,?,?,?)", -1,
        &g_stmt_insert_unit, NULL);
    err = sqlite3_prepare_v2(db,
        "INSERT INTO `region` (`id`, `data`, `x`, `y`, `p`, `turn`, `name`, `terrain`) VALUES (?,?,?,?,?,?,?,?)", -1,
        &g_stmt_insert_region, NULL);
    err = sqlite3_prepare_v2(db,
        "INSERT INTO `faction` (`id`, `data`, `name`, `email`) VALUES (?,?,?,?)", -1,
        &g_stmt_insert_faction, NULL);
    return err;
}

static int db_install(sqlite3 *db, const char *schema) {
    FILE *F = NULL;
    int err, version = 0;

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
            if (err != SQLITE_OK) goto db_install_fail;
        }
        fclose(F);
    }
    return SQLITE_OK;
db_install_fail:
    if (F) fclose(F);
    if (db) {
        fputs(sqlite3_errmsg(db), stderr);
        sqlite3_close(db);
    }
    return err;
}

static int db_upgrade(sqlite3 *db, int from_version, int to_version) {
    int err;
    if (from_version == 0) {
        err = db_install(db, "crschema.sql");
    }
    else {
        int i;
        for (i = from_version + 1; i <= to_version; ++i) {
            char filename[20];
            snprintf(filename, sizeof(filename), "crupdate-%d.sql", i);
            err = db_install(db, filename);
        }
    }
    return err;
}

static int cb_int_col(void *data, int ncols, char **text, char **name) {
    int *p_int = (int *)data;
    *p_int = atoi(text[0]);
    return SQLITE_OK;
}

int db_open(const char * filename, sqlite3 **dbp, int version) {
    sqlite3 *db = NULL;
    int err, user_version;
    assert(dbp);
    err = sqlite3_open_v2(filename, dbp, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (err != SQLITE_OK) goto db_open_fail;
    db = *dbp;
    err = sqlite3_exec(db, "PRAGMA user_version", cb_int_col, &user_version, NULL);
    if (err != SQLITE_OK) goto db_open_fail;
    if (user_version != version) {
        err = db_upgrade(db, user_version, version);
    }
    if (err != SQLITE_OK) goto db_open_fail;
    err = db_prepare(db);
    return SQLITE_OK;
db_open_fail:
    if (db) {
        fputs(sqlite3_errmsg(db), stderr);
        sqlite3_close(db);
    }
    return err;
}

int db_close(sqlite3 * db) {
    int err;
    err = sqlite3_finalize(g_stmt_insert_faction);
    g_stmt_insert_faction = NULL;
    err = sqlite3_finalize(g_stmt_insert_region);
    g_stmt_insert_region = NULL;
    err = sqlite3_finalize(g_stmt_insert_unit);
    g_stmt_insert_unit = NULL;
    err = sqlite3_finalize(g_stmt_insert_ship);
    g_stmt_insert_ship = NULL;
    err = sqlite3_finalize(g_stmt_insert_building);
    g_stmt_insert_building = NULL;
    err = sqlite3_close(db);
    return err;
}

static int db_bind_json(sqlite3_stmt *stmt, int index, cJSON *object) {
    int err;
    char *data = cJSON_PrintUnformatted(object);
    if (data) {
        int sz = (int)strlen(data);
        err = sqlite3_bind_blob(stmt, index, data, sz, SQLITE_TRANSIENT);
        cJSON_free(data);
    }
    else {
        err = sqlite3_bind_null(stmt, 4);
    }
    return err;
}

int db_write_unit(struct sqlite3 *db, const struct unit *u) {
    // INSERT INTO `unit` (`id`, `data`, `name`, `orders`, `region_id`, `faction_id`) VALUES (?,?,?,?,?,?)
    int err;
    assert(u);
    err = sqlite3_reset(g_stmt_insert_unit);
    err = sqlite3_bind_int(g_stmt_insert_unit, 1, u->id);
    err = db_bind_json(g_stmt_insert_unit, 2, u->data);
    err = sqlite3_bind_text(g_stmt_insert_unit, 3, u->name, -1, SQLITE_STATIC);
    err = sqlite3_bind_text(g_stmt_insert_unit, 4, u->orders, -1, SQLITE_STATIC);
    if (u->region) {
        err = sqlite3_bind_int(g_stmt_insert_unit, 5, u->region->id);
    }
    else {
        err = sqlite3_bind_null(g_stmt_insert_unit, 5);
    }
    if (u->faction) {
        err = sqlite3_bind_int(g_stmt_insert_unit, 6, u->faction->id);
    }
    else {
        err = sqlite3_bind_null(g_stmt_insert_unit, 6);
    }

    err = sqlite3_step(g_stmt_insert_unit);
    return err;
}

int db_write_ship(struct sqlite3 *db, const struct ship *sh) {
    return SQLITE_OK;
}

int db_write_building(struct sqlite3 *db, const struct building *b) {
    return SQLITE_OK;
}

int db_write_region(struct sqlite3 *db, const struct region *r) {
    // INSERT INTO `region` (`id`, `data`, `x`, `y`, `p`, `turn`, `name`, `terrain`) VALUES (?,?,?,?,?,?,?,?)
    int err;
    err = sqlite3_reset(g_stmt_insert_region);
    err = sqlite3_bind_int(g_stmt_insert_region, 1, r->id);
    err = db_bind_json(g_stmt_insert_region, 2, r->data);
    err = sqlite3_bind_int(g_stmt_insert_region, 3, r->x);
    err = sqlite3_bind_int(g_stmt_insert_region, 4, r->y);
    err = sqlite3_bind_int(g_stmt_insert_region, 5, r->plane);
    err = sqlite3_bind_int(g_stmt_insert_region, 6, r->turn);
    if (r->name) {
        err = sqlite3_bind_text(g_stmt_insert_region, 7, r->name, -1, SQLITE_STATIC);
    }
    else {
        err = sqlite3_bind_null(g_stmt_insert_region, 7);
    }
    err = sqlite3_bind_text(g_stmt_insert_region, 8, terrainname[r->terrain], -1, SQLITE_STATIC);

    err = sqlite3_step(g_stmt_insert_region);
    return err;
}

int db_write_faction(struct sqlite3 *db, const struct faction *f) {
    int err;
    err = sqlite3_reset(g_stmt_insert_faction);
    err = sqlite3_bind_int(g_stmt_insert_faction, 1, f->id);
    err = db_bind_json(g_stmt_insert_faction, 2, f->data);
    err = sqlite3_bind_text(g_stmt_insert_faction, 3, f->name, -1, SQLITE_STATIC);
    err = sqlite3_bind_text(g_stmt_insert_faction, 4, f->email, -1, SQLITE_STATIC);

    err = sqlite3_step(g_stmt_insert_region);
    return err;
}
