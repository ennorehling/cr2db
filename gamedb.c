#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "gamedb.h"
#include "gamedata.h"
#include "faction.h"
#include "region.h"

#include <strings.h>
#include <cJSON.h>
#include <sqlite3.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static sqlite3_stmt *g_stmt_select_all_terrains;
static sqlite3_stmt *g_stmt_insert_faction;
static sqlite3_stmt *g_stmt_select_faction;
static sqlite3_stmt *g_stmt_select_all_factions_meta;
static sqlite3_stmt *g_stmt_insert_region;
static sqlite3_stmt *g_stmt_select_region;
static sqlite3_stmt *g_stmt_select_all_regions_meta;

static int db_prepare(sqlite3 *db) {
    int err;
    err = sqlite3_prepare_v2(db,
        "SELECT `id`, `name`, `crname` FROM `terrains` ORDER by `id`", -1,
        &g_stmt_select_all_terrains, NULL);
    if (err != SQLITE_OK) goto db_prepare_fail;

    err = sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO `factions` "
        "(`id`, `data`, `name`, `email`) "
        "VALUES (?,?,?,?)", -1, &g_stmt_insert_faction, NULL);
    if (err != SQLITE_OK) goto db_prepare_fail;
    err = sqlite3_prepare_v2(db,
        "SELECT `data`, `name`, `email` "
        "FROM `factions` WHERE `id` = ?", -1, &g_stmt_select_faction, NULL);
    if (err != SQLITE_OK) goto db_prepare_fail;
    err = sqlite3_prepare_v2(db,
        "SELECT `id`, `name`, `email` FROM `factions`", -1,
        &g_stmt_select_all_factions_meta, NULL);
    if (err != SQLITE_OK) goto db_prepare_fail;

    err = sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO `regions` "
        "(`id`, `data`, `x`, `y`, `z`, `name`, `terrain_id`) "
        "VALUES (?,?,?,?,?,?,?)", -1, &g_stmt_insert_region, NULL);
    if (err != SQLITE_OK) goto db_prepare_fail;
    err = sqlite3_prepare_v2(db,
        "SELECT `data`, `id`, `terrain_id`, `name` "
        "FROM `regions` WHERE `x` = ? AND `y` = ? AND `z` = ?", -1,
        &g_stmt_select_region, NULL);
    if (err != SQLITE_OK) goto db_prepare_fail;
    err = sqlite3_prepare_v2(db,
        "SELECT `id`, `x`,  `y`, `z`, `name`, `terrain_id` FROM `regions`", -1,
        &g_stmt_select_all_regions_meta, NULL);
    if (err != SQLITE_OK) goto db_prepare_fail;
    return err;

db_prepare_fail:
    err = sqlite3_extended_errcode(db);
    fputs(sqlite3_errmsg(db), stderr);
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
        err = sqlite3_bind_null(stmt, index);
    }
    return err;
}

int db_write_faction(sqlite3 *db, const faction *f) {
    int err;
    err = sqlite3_reset(g_stmt_insert_faction);
    if (err != SQLITE_OK) goto db_write_faction_fail;
    err = sqlite3_bind_int64(g_stmt_insert_faction, 1, (sqlite3_int64)f->id);
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
    err = sqlite3_extended_errcode(db);
    fputs(sqlite3_errmsg(db), stderr);
    return err;
}

static void read_faction_row(faction *f, int ncols, char **values, char**names)
{
    assert(ncols>=4);
    (void)names;
    f->data = values[0] ? cJSON_Parse(values[0]) : NULL;
    f->id = values[1] ? atoi(values[1]) : 0;
    f->name = str_strdup(values[2]);
    f->email = str_strdup(values[3]);
}

struct walk_faction {
    int (*callback)(struct faction *, void *);
    void *arg;
};

static int cb_walk_faction(void *udata, int ncols, char **values, char **names)
{
    int err;
    faction cursor;
    struct walk_faction *ctx = (struct walk_faction *)udata;

    memset(&cursor, 0, sizeof cursor);
    read_faction_row(&cursor, ncols, values, names);
    err = ctx->callback(&cursor, ctx->arg);
    faction_free(&cursor);
    return err;
}

int db_factions_walk(struct sqlite3 *db, int(*callback)(struct faction *, void *), void *arg)
{
    struct walk_faction ctx;
    ctx.arg = arg;
    ctx.callback = callback;
    return sqlite3_exec(db, "SELECT `data`, `id`, `name`, `email` FROM `factions`", cb_walk_faction, &ctx, NULL);
}

int db_read_faction(sqlite3 *db, faction *f)
{
    int err;
    const void *data;

    assert(f);
    assert(f->id > 0);
    err = sqlite3_reset(g_stmt_select_faction);
    if (err != SQLITE_OK) goto db_read_faction_fail;
    err = sqlite3_bind_int64(g_stmt_select_faction, 1, (sqlite3_int64)f->id);
    if (err != SQLITE_OK) goto db_read_faction_fail;
    err = sqlite3_step(g_stmt_select_faction);
    if (err == SQLITE_DONE) {
        /* no such database entry */
        return SQLITE_DONE;
    }
    else if (err != SQLITE_ROW) goto db_read_faction_fail;
    data = sqlite3_column_blob(g_stmt_select_faction, 0);
    if (data) {
        f->data = cJSON_Parse(data);
    }
    f->name = str_strdup((const char *) sqlite3_column_text(g_stmt_select_faction, 1));
    f->email = str_strdup((const char *) sqlite3_column_text(g_stmt_select_faction, 2));

    return SQLITE_OK;

db_read_faction_fail:
    fputs(sqlite3_errmsg(db), stderr);
    return err;
}

static int db_install(sqlite3 *db, const char *schema) {
    FILE *F = NULL;
    int err;

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
        return SQLITE_OK;
    }
    return SQLITE_NOTFOUND;

db_install_fail:
    if (F) fclose(F);
    if (db) {
        fputs(sqlite3_errmsg(db), stderr);
        sqlite3_close(db);
    }
    return err;
}

static int db_upgrade(sqlite3 *db, int from_version, int to_version) {
    if (from_version != to_version) {
        return db_install(db, "crschema.sql");
    }
    return 0;
}

static int cb_int_col(void *data, int ncols, char **text, char **name) {
    int *p_int = (int *)data;
    (void)ncols;
    (void)name;
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
    if (err != SQLITE_OK) goto db_open_fail;
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
    err = sqlite3_finalize(g_stmt_select_all_terrains);
    g_stmt_select_all_terrains = NULL;
    err = sqlite3_finalize(g_stmt_select_all_factions_meta);
    g_stmt_select_all_factions_meta = NULL;
    err = sqlite3_finalize(g_stmt_select_all_regions_meta);
    g_stmt_select_all_regions_meta = NULL;
    err = sqlite3_finalize(g_stmt_select_faction);
    g_stmt_select_faction = NULL;
    err = sqlite3_finalize(g_stmt_insert_faction);
    g_stmt_insert_faction = NULL;
    err = sqlite3_finalize(g_stmt_insert_region);
    g_stmt_insert_region = NULL;
    err = sqlite3_finalize(g_stmt_select_region);
    g_stmt_select_region = NULL;
/*
    err = sqlite3_finalize(g_stmt_insert_unit);
    g_stmt_insert_unit = NULL;
    err = sqlite3_finalize(g_stmt_insert_ship);
    g_stmt_insert_ship = NULL;
    err = sqlite3_finalize(g_stmt_insert_building);
    g_stmt_insert_building = NULL;
*/
    err = sqlite3_close(db);
    return err;
}

/*
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
*/

int db_write_region(struct sqlite3 *db, const struct region *r) {
    // INSERT INTO `region` (`id`, `data`, `x`, `y`, `p`, `turn`, `name`, `terrain`) VALUES (?,?,?,?,?,?,?,?)
    int err;
    err = sqlite3_reset(g_stmt_insert_region);
    if (err != SQLITE_OK) goto db_write_region_fail;
    err = sqlite3_bind_int(g_stmt_insert_region, 1, r->id);
    if (err != SQLITE_OK) goto db_write_region_fail;
    err = db_bind_json(g_stmt_insert_region, 2, r->data);
    if (err != SQLITE_OK) goto db_write_region_fail;
    err = sqlite3_bind_int(g_stmt_insert_region, 3, r->loc.x);
    if (err != SQLITE_OK) goto db_write_region_fail;
    err = sqlite3_bind_int(g_stmt_insert_region, 4, r->loc.y);
    if (err != SQLITE_OK) goto db_write_region_fail;
    err = sqlite3_bind_int(g_stmt_insert_region, 5, r->loc.z);
    if (err != SQLITE_OK) goto db_write_region_fail;
    if (r->name) {
        err = sqlite3_bind_text(g_stmt_insert_region, 6, r->name, -1, SQLITE_STATIC);
    }
    else {
        err = sqlite3_bind_null(g_stmt_insert_region, 6);
    }
    if (err != SQLITE_OK) goto db_write_region_fail;
    err = sqlite3_bind_int(g_stmt_insert_region, 7, r->terrain);
    if (err != SQLITE_OK) goto db_write_region_fail;

    err = sqlite3_step(g_stmt_insert_region);
    if (err != SQLITE_DONE) goto db_write_region_fail;
    return SQLITE_OK;

db_write_region_fail:
    fputs(sqlite3_errmsg(db), stderr);
    return err;
}

static void read_region_row(region *r, int ncols, char **values, char **names) {
    assert(ncols >=7);
    (void)names;
    r->data = values[0] ? cJSON_Parse(values[0]) : NULL;
    r->id = values[1] ? atoi(values[1]) : 0;
    r->loc.x = values[2] ? atoi(values[2]) : 0;
    r->loc.y = values[3] ? atoi(values[3]) : 0;
    r->loc.z = values[4] ? atoi(values[4]) : 0;
    r->terrain = values[5] ? atoi(values[5]) : 0;
    r->name = str_strdup(values[6]);
}

struct walk_region {
    int(*callback)(struct region *, void *);
    void *arg;
};

static int cb_walk_region(void *udata, int ncols, char **values, char **names)
{
    int err;
    region cursor;
    struct walk_region *ctx = (struct walk_region *)udata;

    memset(&cursor, 0, sizeof(cursor));
    read_region_row(&cursor, ncols, values, names);
    err = ctx->callback(&cursor, ctx->arg);
    region_free(&cursor);
    return err;
}

int db_regions_walk(struct sqlite3 *db, int(*callback)(struct region *, void *), void *arg)
{
    struct walk_region ctx;
    ctx.arg = arg;
    ctx.callback = callback;
    return sqlite3_exec(db, "SELECT `data`, `id`, `x`, `y`, `z`, `terrain_id`, `name` FROM `regions`", cb_walk_region, &ctx, NULL);
}

int db_read_region(struct sqlite3 *db, region *r)
{
    int err;
    const void *data;

    err = sqlite3_reset(g_stmt_select_region);
    if (err != SQLITE_OK) goto db_read_region_fail;
    err = sqlite3_bind_int(g_stmt_select_region, 1, r->loc.x);
    err = sqlite3_bind_int(g_stmt_select_region, 2, r->loc.y);
    err = sqlite3_bind_int(g_stmt_select_region, 3, r->loc.z);
    if (err != SQLITE_OK) goto db_read_region_fail;
    err = sqlite3_step(g_stmt_select_region);
    if (err == SQLITE_DONE) {
        /* no matching row */
        return err;
    }
    else if (err != SQLITE_ROW) goto db_read_region_fail;
    data = sqlite3_column_blob(g_stmt_select_region, 0);
    if (data) {
        r->data = cJSON_Parse(data);
    }
    r->id = sqlite3_column_int(g_stmt_select_region, 1);
    r->terrain = sqlite3_column_int(g_stmt_select_region, 2);
    free(r->name);
    r->name = str_strdup(sqlite3_column_blob(g_stmt_select_region, 3));
    return SQLITE_OK;

db_read_region_fail:
    fputs(sqlite3_errmsg(db), stderr);
    return err;
}

int db_region_delete_objects(sqlite3 *db, unsigned int region_id)
{
    char zSQL[80];
    int err;
    sprintf(zSQL, "DELETE FROM buildings WHERE region_id=%u", region_id);
    if (SQLITE_OK != (err = sqlite3_exec(db, zSQL, NULL, NULL, NULL))) goto db_delete_objects_fail;
    sprintf(zSQL, "DELETE FROM ships WHERE region_id=%u", region_id);
    if (SQLITE_OK != (err = sqlite3_exec(db, zSQL, NULL, NULL, NULL))) goto db_delete_objects_fail;
    sprintf(zSQL, "DELETE FROM units WHERE region_id=%u", region_id);
    if (SQLITE_OK != (err = sqlite3_exec(db, zSQL, NULL, NULL, NULL))) goto db_delete_objects_fail;
    return err;

db_delete_objects_fail:
    fputs(sqlite3_errmsg(db), stderr);
    return err;
}

int db_load_terrains(struct sqlite3 *db, terrains *list)
{
    int err;

    err = sqlite3_reset(g_stmt_select_all_terrains);
    if (err != SQLITE_OK) goto db_load_terrains_fail;
    for (;;) {
        err = sqlite3_step(g_stmt_select_all_terrains);
        if (err == SQLITE_DONE) {
            return SQLITE_OK;
        }
        else if (err == SQLITE_ROW) {
            const char * str;
            terrain t;
            t.id = sqlite3_column_int(g_stmt_select_all_terrains, 0);
            str = (const char *) sqlite3_column_text(g_stmt_select_all_terrains, 1);
            str_strlcpy(t.name, str, sizeof(t.name));
            str = (const char *) sqlite3_column_text(g_stmt_select_all_terrains, 2);
            if (str) {
                str_strlcpy(t.crname, str, sizeof(t.crname));
            }
            else {
                t.crname[0] = 0;
            }
            terrains_add(list, &t);
        }
        else goto db_load_terrains_fail;
    }
    return SQLITE_OK;

db_load_terrains_fail:
    fputs(sqlite3_errmsg(db), stderr);
    return err;
}
/*
    err = sqlite3_prepare_v2(db,
        "SELECT `id`, `x`,  `y`, `z`, `name`, `terrain` FROM `regions`", -1,
        &g_stmt_select_region_meta, NULL);
*/
int db_load_map(struct sqlite3 *db, regions *list)
{
    int err;

    err = sqlite3_reset(g_stmt_select_all_regions_meta);
    if (err != SQLITE_OK) goto db_load_map_fail;
    for (;;) {
        err = sqlite3_step(g_stmt_select_all_regions_meta);
        if (err == SQLITE_DONE) {
            return SQLITE_OK;
        }
        else if (err == SQLITE_ROW) {
            region *r = create_region();
            r->id = sqlite3_column_int(g_stmt_select_all_regions_meta, 0);
            r->loc.x = sqlite3_column_int(g_stmt_select_all_regions_meta, 1);
            r->loc.y = sqlite3_column_int(g_stmt_select_all_regions_meta, 2);
            r->loc.z = sqlite3_column_int(g_stmt_select_all_regions_meta, 3);
            r->name = str_strdup((const char *) sqlite3_column_text(g_stmt_select_all_regions_meta, 4));
            r->terrain = sqlite3_column_int(g_stmt_select_all_regions_meta, 5);
            regions_add(list, r);
        }
        else goto db_load_map_fail;
    }

db_load_map_fail:
    fputs(sqlite3_errmsg(db), stderr);
    return err;
}

/*
    err = sqlite3_prepare_v2(db,
        "SELECT `id`, `name`, `email` FROM `factions`", -1,
        &g_stmt_select_all_factions_meta, NULL);
*/
int db_load_factions(struct sqlite3 *db, factions *list)
{
    int err;

    err = sqlite3_reset(g_stmt_select_all_factions_meta);
    if (err != SQLITE_OK) goto db_load_factions_fail;
    for (;;) {
        err = sqlite3_step(g_stmt_select_all_factions_meta);
        if (err == SQLITE_DONE) {
            return SQLITE_OK;
        }
        else if (err == SQLITE_ROW) {
            faction *f = create_faction();
            f->id = sqlite3_column_int(g_stmt_select_all_factions_meta, 0);
            f->name = str_strdup((const char *) sqlite3_column_text(g_stmt_select_all_factions_meta, 1));
            f->email = str_strdup((const char *) sqlite3_column_text(g_stmt_select_all_factions_meta, 2));
            factions_add(list, f);
        }
        else goto db_load_factions_fail;
    }

db_load_factions_fail:
    fputs(sqlite3_errmsg(db), stderr);
    return err;
}

int db_execute(struct sqlite3 *db, const char *sql)
{
    return sqlite3_exec(db, sql, NULL, NULL, NULL);
}
