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

static int cb_load_faction(void *udata, int ncols, char **text, char **name) {
    gamedata *gd = (gamedata *)udata;
    cJSON *json = cJSON_Parse(text[0]);
    faction_create(gd, json);
    return SQLITE_OK;
}

static int cb_load_region(void *udata, int ncols, char **text, char **name) {
    gamedata *gd = (gamedata *)udata;
    cJSON *json = cJSON_Parse(text[0]);
    int turn = atoi(text[1]);
    region_create(gd, json, turn);
    return SQLITE_OK;
}

static int cb_load_ship(void *udata, int ncols, char **text, char **name) {
    gamedata *gd = (gamedata *)udata;
    cJSON *json = cJSON_Parse(text[0]);
    int region_id = atoi(text[1]);
    region * r = region_get(gd, region_id);
    ship_create(gd, r, json);
    return SQLITE_OK;
}

static int cb_load_building(void *udata, int ncols, char **text, char **name) {
    gamedata *gd = (gamedata *)udata;
    cJSON *json = cJSON_Parse(text[0]);
    int region_id = atoi(text[1]);
    region * r = region_get(gd, region_id);
    building_create(gd, r, json);
    return SQLITE_OK;
}

static int cb_load_unit(void *udata, int ncols, char **text, char **name) {
    gamedata *gd = (gamedata *)udata;
    cJSON *json = cJSON_Parse(text[0]);
    int region_id = atoi(text[1]);
    region * r = region_get(gd, region_id);
    unit_create(gd, r, json);
    return SQLITE_OK;
}

int db_load(sqlite3 *db, gamedata *gd) {
    int err = SQLITE_OK;
    err = sqlite3_exec(db, "SELECT data FROM faction", cb_load_faction, gd, NULL);
    err = sqlite3_exec(db, "SELECT data, turn FROM region", cb_load_region, gd, NULL);
    err = sqlite3_exec(db, "SELECT data, region_id FROM ship", cb_load_ship, gd, NULL);
    err = sqlite3_exec(db, "SELECT data, region_id FROM building", cb_load_building, gd, NULL);
    err = sqlite3_exec(db, "SELECT data, region_id FROM unit", cb_load_unit, gd, NULL);
    return err;
}

static int db_prepare(sqlite3 *db) {
    int err;
    err = sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO `unit` (`id`, `data`, `name`, `orders`, `region_id`, `faction_id`) VALUES (?,?,?,?,?,?)", -1,
        &g_stmt_insert_unit, NULL);
    err = sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO `region` (`id`, `data`, `x`, `y`, `p`, `turn`, `name`, `terrain`) VALUES (?,?,?,?,?,?,?,?)", -1,
        &g_stmt_insert_region, NULL);
    err = sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO `faction` (`id`, `data`, `name`, `email`) VALUES (?,?,?,?)", -1,
        &g_stmt_insert_faction, NULL);
    err = sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO `ship` (`id`, `data`, `name`, `region_id`) VALUES (?,?,?,?)", -1,
        &g_stmt_insert_ship, NULL);
    err = sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO `building` (`id`, `data`, `name`, `region_id`) VALUES (?,?,?,?)", -1,
        &g_stmt_insert_building, NULL);
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
            snprintf(filename, sizeof(filename), "update%2d.sql", i);
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
    db = *dbp;
    if (err != SQLITE_OK) goto db_open_fail;
    err = sqlite3_exec(db, "PRAGMA journal_mode=OFF", NULL, NULL, NULL);
    if (err != SQLITE_OK) goto db_open_fail;
    err = sqlite3_exec(db, "PRAGMA synchronous=OFF", NULL, NULL, NULL);
    if (err != SQLITE_OK) goto db_open_fail;
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
    if (err != SQLITE_OK) goto db_write_unit_fail;
    err = sqlite3_bind_int(g_stmt_insert_unit, 1, u->id);
    if (err != SQLITE_OK) goto db_write_unit_fail;
    err = db_bind_json(g_stmt_insert_unit, 2, u->data);
    if (err != SQLITE_OK) goto db_write_unit_fail;
    err = sqlite3_bind_text(g_stmt_insert_unit, 3, u->name, -1, SQLITE_STATIC);
    if (err != SQLITE_OK) goto db_write_unit_fail;
    if (u->orders) {
        err = sqlite3_bind_text(g_stmt_insert_unit, 4, u->orders, -1, SQLITE_STATIC);
        if (err != SQLITE_OK) goto db_write_unit_fail;
    }
    else {
        err = sqlite3_bind_null(g_stmt_insert_unit, 4);
        if (err != SQLITE_OK) goto db_write_unit_fail;
    }
    if (u->region) {
        err = sqlite3_bind_int(g_stmt_insert_unit, 5, u->region->id);
        if (err != SQLITE_OK) goto db_write_unit_fail;
    }
    else {
        err = sqlite3_bind_null(g_stmt_insert_unit, 5);
        if (err != SQLITE_OK) goto db_write_unit_fail;
    }
    if (u->faction) {
        err = sqlite3_bind_int(g_stmt_insert_unit, 6, u->faction->id);
        if (err != SQLITE_OK) goto db_write_unit_fail;
    }
    else {
        err = sqlite3_bind_null(g_stmt_insert_unit, 6);
        if (err != SQLITE_OK) goto db_write_unit_fail;
    }

    err = sqlite3_step(g_stmt_insert_unit);
    if (err == SQLITE_DONE) {
        return SQLITE_OK;
    }
db_write_unit_fail:
    fputs(sqlite3_errmsg(db), stderr);
    return err;
}

int db_write_ship(struct sqlite3 *db, const struct ship *sh) {
    int err;
    assert(sh);
    err = sqlite3_reset(g_stmt_insert_ship);
    if (err != SQLITE_OK) goto db_write_ship_fail;
    err = sqlite3_bind_int(g_stmt_insert_ship, 1, sh->id);
    if (err != SQLITE_OK) goto db_write_ship_fail;
    err = db_bind_json(g_stmt_insert_ship, 2, sh->data);
    if (err != SQLITE_OK) goto db_write_ship_fail;
    err = sqlite3_bind_text(g_stmt_insert_ship, 3, sh->name, -1, SQLITE_STATIC);
    if (err != SQLITE_OK) goto db_write_ship_fail;
    if (sh->region) {
        err = sqlite3_bind_int(g_stmt_insert_ship, 4, sh->region->id);
        if (err != SQLITE_OK) goto db_write_ship_fail;
    }
    else {
        err = sqlite3_bind_null(g_stmt_insert_ship, 4);
        if (err != SQLITE_OK) goto db_write_ship_fail;
    }

    err = sqlite3_step(g_stmt_insert_ship);
    if (err == SQLITE_DONE) {
        return SQLITE_OK;
    }
db_write_ship_fail:
    fputs(sqlite3_errmsg(db), stderr);
    return err;
}

int db_write_building(struct sqlite3 *db, const struct building *b) {
    int err;
    assert(b);
    err = sqlite3_reset(g_stmt_insert_building);
    if (err != SQLITE_OK) goto db_write_building_fail;
    err = sqlite3_bind_int(g_stmt_insert_building, 1, b->id);
    if (err != SQLITE_OK) goto db_write_building_fail;
    err = db_bind_json(g_stmt_insert_building, 2, b->data);
    if (err != SQLITE_OK) goto db_write_building_fail;
    err = sqlite3_bind_text(g_stmt_insert_building, 3, b->name, -1, SQLITE_STATIC);
    if (err != SQLITE_OK) goto db_write_building_fail;
    if (b->region) {
        err = sqlite3_bind_int(g_stmt_insert_building, 4, b->region->id);
        if (err != SQLITE_OK) goto db_write_building_fail;
    }
    else {
        err = sqlite3_bind_null(g_stmt_insert_building, 4);
        if (err != SQLITE_OK) goto db_write_building_fail;
    }

    err = sqlite3_step(g_stmt_insert_building);
    if (err == SQLITE_DONE) {
        return SQLITE_OK;
    }
db_write_building_fail:
    fputs(sqlite3_errmsg(db), stderr);
    return err;
}

int db_write_region(struct sqlite3 *db, const struct region *r) {
    // INSERT INTO `region` (`id`, `data`, `x`, `y`, `p`, `turn`, `name`, `terrain`) VALUES (?,?,?,?,?,?,?,?)
    int err;
    err = sqlite3_reset(g_stmt_insert_region);
    if (err != SQLITE_OK) goto db_write_region_fail;
    err = sqlite3_bind_int(g_stmt_insert_region, 1, r->id);
    if (err != SQLITE_OK) goto db_write_region_fail;
    err = db_bind_json(g_stmt_insert_region, 2, r->data);
    if (err != SQLITE_OK) goto db_write_region_fail;
    err = sqlite3_bind_int(g_stmt_insert_region, 3, r->x);
    if (err != SQLITE_OK) goto db_write_region_fail;
    err = sqlite3_bind_int(g_stmt_insert_region, 4, r->y);
    if (err != SQLITE_OK) goto db_write_region_fail;
    err = sqlite3_bind_int(g_stmt_insert_region, 5, r->plane);
    if (err != SQLITE_OK) goto db_write_region_fail;
    err = sqlite3_bind_int(g_stmt_insert_region, 6, r->turn);
    if (err != SQLITE_OK) goto db_write_region_fail;
    if (r->name) {
        err = sqlite3_bind_text(g_stmt_insert_region, 7, r->name, -1, SQLITE_STATIC);
    }
    else {
        err = sqlite3_bind_null(g_stmt_insert_region, 7);
    }
    if (err != SQLITE_OK) goto db_write_region_fail;
    err = sqlite3_bind_text(g_stmt_insert_region, 8, terrainname[r->terrain], -1, SQLITE_STATIC);
    if (err != SQLITE_OK) goto db_write_region_fail;

    err = sqlite3_step(g_stmt_insert_region);
    if (err != SQLITE_DONE) goto db_write_region_fail;
    return SQLITE_OK;
db_write_region_fail:
    fputs(sqlite3_errmsg(db), stderr);
    return err;
}

int db_write_faction(struct sqlite3 *db, const struct faction *f) {
    int err;
    err = sqlite3_reset(g_stmt_insert_faction);
    if (err != SQLITE_OK) goto db_write_faction_fail;
    err = sqlite3_bind_int(g_stmt_insert_faction, 1, f->id);
    if (err != SQLITE_OK) goto db_write_faction_fail;
    err = db_bind_json(g_stmt_insert_faction, 2, f->data);
    if (err != SQLITE_OK) goto db_write_faction_fail;
    err = sqlite3_bind_text(g_stmt_insert_faction, 3, f->name, -1, SQLITE_STATIC);
    if (err != SQLITE_OK) goto db_write_faction_fail;
    err = sqlite3_bind_text(g_stmt_insert_faction, 4, f->email, -1, SQLITE_STATIC);
    if (err != SQLITE_OK) goto db_write_faction_fail;

    err = sqlite3_step(g_stmt_insert_faction);
    if (err != SQLITE_DONE) goto db_write_faction_fail;
    return SQLITE_OK;
db_write_faction_fail:
    fputs(sqlite3_errmsg(db), stderr);
    return err;
}
