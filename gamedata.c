#define _CRT_SECURE_NO_WARNINGS

#include "gamedata.h"

#include "config.h"

#include "gamedb.h"
#include "faction.h"
#include "region.h"
#include "unit.h"
#include "ship.h"
#include "building.h"

#include "stb_ds.h"

#include <strings.h>
#include <cJSON.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int factions_walk(struct gamedata *gd, int (*callback)(struct faction *, void *), void *arg)
{
    int i, len = stbds_arrlen(gd->factions.arr);
    for (i = 0; i != len; ++i) {
        faction * f = gd->factions.arr[i];
        int err = callback(f, arg);
        if (err) return err;
    }
    return 0;
}

int regions_walk(struct gamedata *gd, int (*callback)(struct region *, void *), void *arg)
{
    int i, len = stbds_arrlen(gd->regions.arr);
    for (i = 0; i != len; ++i) {
        region * r = gd->regions.arr[i];
        int err = callback(r, arg);
        if (err) return err;
    }
    return 0;
}

int buildings_walk(struct gamedata *gd, int (*callback)(struct building *, void *), void *arg)
{
    unsigned int i, len;
    for (i = 0, len = stbds_arrlen(gd->regions.arr); i != len; ++i) {
        region * r = gd->regions.arr[i];
        int n, nb;
        for (n = 0, nb = stbds_arrlen(r->buildings); n != nb; ++n) {
            building * b = r->buildings[n];
            int err = callback(b, arg);
            if (err) return err;
        }
    }
    return 0;
}

void gd_add_faction(gamedata *gd, faction *f)
{
    factions_add(&gd->factions, f);
}

faction *gd_create_faction(gamedata *gd, cJSON *data)
{
    faction * f = calloc(1, sizeof(faction));
    gd_update_faction(gd, f, data);
    gd_add_faction(gd, f);
    return f;
}

void gd_update_faction(gamedata *gd, faction *f, cJSON *data)
{
    (void)gd;
    assert(f);
    if (f->data != data) {
        if (data) {
            cJSON **p_child = &data->child;
            while (*p_child) {
                cJSON *child = *p_child;
                if (child->type == cJSON_Number) {
                    if (f->id == 0 && strcmp(child->string, "id") == 0) {
                        f->id = child->valueint;
                        *p_child = child->next;
                        child->next = NULL;
                        cJSON_Delete(child);
                        continue;
                    }
                }
                else if (child->type == cJSON_String) {
                    if (strcmp(child->string, "Parteiname") == 0) {
                        free(f->name);
                        f->name = str_strdup(child->valuestring);
                    }
                    else if (strcmp(child->string, "email") == 0) {
                        free(f->email);
                        f->email = str_strdup(child->valuestring);
                    }
                }
                p_child = &child->next;
            }
        }
        cJSON_Delete(f->data);
        f->data = data;
    }
}

void gd_add_building(gamedata *gd, building *b)
{
    assert(gd);
    assert(b);
    assert(b->region);
    stbds_arrpush(b->region->buildings, b);
    // buildings_add(&gd->buildings, b);
}

building *gd_create_building(gamedata *gd, region *r, cJSON *data)
{
    building * b = calloc(1, sizeof(building));
    assert(gd);
    assert(r);
    b->region = r;
    gd_update_building(gd, b, data);
    gd_add_building(gd, b);
    return b;
}

void gd_update_building(gamedata *gd, building *b, cJSON *data)
{
    assert(gd);
    assert(b);
    if (b->data != data) {
        if (data) {
            cJSON * child;
            for (child = data->child; child != NULL; child = child->next) {
                if (b->id == 0 && child->type == cJSON_Number) {
                    if (strcmp(child->string, "id") == 0) {
                        b->id = child->valueint;
                    }
                }
                else if (child->type == cJSON_String) {
                    if (strcmp(child->string, "Typ") == 0) {
                        b->type = config_find(&gd->building_types, child->valuestring);
                        if (b->type < 0) {
                            b->type = config_add(&gd->building_types, child->valuestring);
                        }
                    }
                    else if (strcmp(child->string, "Name") == 0) {
                        b->name = str_strdup(child->valuestring);
                    }
                }
            }
        }
        cJSON_Delete(b->data);
        b->data = data;
    }
}

void gd_add_ship(gamedata *gd, ship *obj)
{
    assert(gd);
    assert(obj);
    assert(obj->region);
    stbds_arrpush(obj->region->ships, obj);
    // ships_add(&gd->ships, obj);
}

ship *gd_create_ship(gamedata *gd, region *r, cJSON *data)
{
    ship * obj = calloc(1, sizeof(ship));
    assert(gd);
    assert(r);
    obj->region = r;
    gd_update_ship(gd, obj, data);
    gd_add_ship(gd, obj);
    return obj;
}

void gd_update_ship(gamedata *gd, ship *obj, cJSON *data)
{
    assert(gd);
    assert(obj);
    if (obj->data != data) {
        if (data) {
            cJSON * child;
            for (child = data->child; child != NULL; child = child->next) {
                if (obj->id == 0 && child->type == cJSON_Number) {
                    if (strcmp(child->string, "id") == 0) {
                        obj->id = child->valueint;
                    }
                }
                else if (child->type == cJSON_String) {
                    if (strcmp(child->string, "Typ") == 0) {
                        obj->type = config_find(&gd->ship_types, child->valuestring);
                        if (obj->type < 0) {
                            obj->type = config_add(&gd->ship_types, child->valuestring);
                        }
                    }
                    else if (strcmp(child->string, "Name") == 0) {
                        obj->name = str_strdup(child->valuestring);
                    }
                }
            }
        }
        cJSON_Delete(obj->data);
        obj->data = data;
    }
}

void gd_add_unit(gamedata *gd, unit *obj)
{
    assert(gd);
    assert(obj);
    assert(obj->region);
    stbds_arrpush(obj->region->units, obj);
}

unit *gd_create_unit(gamedata *gd, region *r, cJSON *data)
{
    unit * obj = calloc(1, sizeof(unit));
    assert(gd);
    assert(r);
    obj->region = r;
    gd_update_unit(gd, obj, data);
    gd_add_unit(gd, obj);
    return obj;
}

void gd_update_unit(gamedata *gd, unit *obj, cJSON *data)
{
    assert(gd);
    assert(obj);
    if (obj->data != data) {
        if (data) {
            cJSON * child;
            for (child = data->child; child != NULL; child = child->next) {
                if (obj->id == 0 && child->type == cJSON_Number) {
                    if (strcmp(child->string, "id") == 0) {
                        obj->id = child->valueint;
                    }
                    if (strcmp(child->string, "Partei") == 0) {
                        obj->faction = factions_get(&gd->factions, child->valueint);
                    }
                }
                else if (child->type == cJSON_String) {
                    if (strcmp(child->string, "Typ") == 0) {
                        index_t i = config_find(&gd->races, child->valuestring);
                        if (i < 0) {
                            i = config_add(&gd->races, child->valuestring);
                        }
                        obj->race = (race_t)i;
                    }
                    else if (strcmp(child->string, "Name") == 0) {
                        obj->name = str_strdup(child->valuestring);
                    }
                }
            }
        }
        cJSON_Delete(obj->data);
        obj->data = data;
    }
}

void gd_add_region(gamedata *gd, region *r)
{
    regions_add(&gd->regions, r);
}

region *gd_create_region(gamedata *gd, cJSON *data) {
    region *r = calloc(1, sizeof(region));
    gd_update_region(gd, r, data);
    gd_add_region(gd, r);
    return r;
}

void gd_update_region(struct gamedata *gd, struct region *r, struct cJSON *data)
{
    assert(gd);
    assert(r);
    if (r->data != data) {
        if (data) {
            cJSON * child;
            for (child = data->child; child != NULL; child = child->next) {
                if (r->id == 0 && child->type == cJSON_Number) {
                    if (strcmp(child->string, "id") == 0) {
                        r->id = child->valueint;
                    }
                    else if (strcmp(child->string, "x") == 0) {
                        r->loc.x = child->valueint;
                    }
                    else if (strcmp(child->string, "y") == 0) {
                        r->loc.y = child->valueint;
                    }
                    else if (strcmp(child->string, "z") == 0) {
                        r->loc.z = child->valueint;
                    }
                }
                else if (child->type == cJSON_String) {
                    if (strcmp(child->string, "Name") == 0) {
                        r->name = str_strdup(child->valuestring);
                    }
                    if (strcmp(child->string, "Terrain") == 0) {
                        const char *name = child->valuestring;
                        index_t index = config_find(&gd->terrains, name);
                        if (index < 0) {
                            /* special terrain, not in the config or database */
                            index = config_add(&gd->terrains, name);
                        }
                        r->terrain = (terrain_id) index;
                    }
                }
            }
        }
        cJSON_Delete(r->data);
        r->data = data;
    }
}

void region_reset(struct gamedata *gd, struct region *r)
{
    unsigned int i, len;
    assert(gd);
    for (i = 0, len = stbds_arrlen(r->buildings); i != len; ++i) {
        building * b = r->buildings[i];
        building_free(b);
        free(b);
    }
    stbds_arrfree(r->buildings);
    for (i = 0, len = stbds_arrlen(r->ships); i != len; ++i) {
        ship *s = r->ships[i];
        ship_free(s);
        free(s);
    }
    stbds_arrfree(r->ships);
    for (i = 0, len = stbds_arrlen(r->units); i; ++i) {
        unit * u = r->units[i];
        unit_free(u);
        free(u);
    }
    stbds_arrfree(r->units);
}

int gd_load_config(gamedata *gd)
{
    int err, result = 0;
    err = db_read_info(gd->db, gd);
    if (err) return err;
    err = config_load(&gd->races, "res/races.json");
    if (err && !result) result = err;
    err = config_load(&gd->terrains, "res/terrains.json");
    if (err && !result) result = err;
    err = config_load(&gd->building_types, "res/buildings.json");
    if (err && !result) result = err;
    err = config_load(&gd->ship_types, "res/ships.json");
    if (err && !result) result = err;
    return result;
}

static int gd_save_config(const gamedata *gd)
{
    return db_write_info(gd->db, gd);
}

gamedata *game_create(struct sqlite3 *db)
{
    gamedata *gd = calloc(1, sizeof(gamedata));
    gd->db = db;
    gd->turn = -1;
    return gd;
}

int game_save(gamedata *gd)
{
    unsigned int i, len;
    int err;

    err = db_execute(gd->db, "BEGIN TRANSACTION");
    if (err) goto abort_save;
    err = gd_save_config(gd);
    if (err) goto abort_save;

    for (i = 0, len = stbds_arrlen(gd->factions.arr); i != len; ++i) {
        faction *f = gd->factions.arr[i];
        err = db_write_faction(gd->db, f);
        if (err) goto abort_save;
    }

    for (i = 0, len = stbds_arrlen(gd->regions.arr); i != len; ++i) {
        region *r = gd->regions.arr[i];
        int err = db_write_region(gd->db, r);
        if (err) goto abort_save;
    }

    err = db_execute(gd->db, "COMMIT");
    if (err) goto abort_save;

    return 0;

abort_save:
    db_execute(gd->db, "ROLLBACK");
    return err;
}

static int cb_load_region(region *cursor, void *udata) {
    gamedata *gd = (gamedata *)udata;
    region *obj = malloc(sizeof(region));
    if (obj) {
        memcpy(obj, cursor, sizeof(region));
        cursor->name = NULL;
        cursor->data = NULL;
        gd_add_region(gd, obj);
        return 0;
    }
    return ENOMEM;
}

static int cb_load_faction(faction *cursor, void *udata) {
    gamedata *gd = (gamedata *)udata;
    faction *obj = malloc(sizeof(faction));
    if (obj) {
        memcpy(obj, cursor, sizeof(faction));
        gd_add_faction(gd, obj);
        cursor->name = NULL;
        cursor->email = NULL;
        cursor->data = NULL;
        return 0;
    }
    return ENOMEM;
}

int game_load(gamedata *gd)
{
    int err;
    err = db_read_info(gd->db, gd);
    if (err) return err;
    err = db_regions_walk(gd->db, cb_load_region, gd);
    if (err) return err;
    return db_factions_walk(gd->db, cb_load_faction, gd);
}

void game_free(gamedata *gd) {
    factions_free(&gd->factions);
    regions_free(&gd->regions);
    config_free(&gd->building_types);
    config_free(&gd->ship_types);
    config_free(&gd->terrains);
    gd->db = NULL;
}
