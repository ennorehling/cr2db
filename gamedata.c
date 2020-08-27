#include "gamedata.h"

#include "gamedb.h"
#include "faction.h"
#include "region.h"
#include "unit.h"
#include "ship.h"
#include "building.h"
#include "config.h"

#include "stb_ds.h"
#include "stretchy_buffer.h"

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
                        const char *crname = child->valuestring;
                        const terrain *t = terrains_get_crname(&gd->terrains, crname);
                        if (t) {
                            r->terrain = t->id;
                        }
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
    int i;
    assert(gd);
    for (i = stb_sb_count(r->buildings) - 1; i; --i) {
        free_building(r->buildings[i]);
    }
    stb_sb_free(r->buildings);
    for (i = stb_sb_count(r->ships) - 1; i; --i) {
        free_ship(r->ships[i]);
    }
    stb_sb_free(r->ships);
    for (i = stb_sb_count(r->units) - 1; i; --i) {
        free_unit(r->units[i]);
    }
    stb_sb_free(r->units);
}

struct unit *unit_create(struct gamedata *gd, struct region *r, struct cJSON *data)
{
    unit * u = create_unit(data);
    assert(gd);
    assert(r);
    if (u) {
        u->region = r;
        stb_sb_push(r->units, u);
    }
    return u;
}

struct ship *ship_create(struct gamedata *gd, struct region *r, struct cJSON *data)
{
    ship *sh = calloc(1, sizeof(ship));
    assert(gd);
    assert(r);
    if (sh) {
        sh->data = data;
        sh->region = r;
        stb_sb_push(r->ships, sh);
    }
    return sh;
}

struct building *building_create(struct gamedata *gd, struct region *r, struct cJSON *data)
{
    building *b = calloc(1, sizeof(building));
    assert(gd);
    assert(r);
    if (b) {
        b->data = data;
        b->region = r;
        stb_sb_push(r->buildings, b);
    }
    return b;
}

gamedata *game_create(struct sqlite3 *db)
{
    gamedata *gd = calloc(1, sizeof(gamedata));
    gd->db = db;
    gd->turn = -1;
    db_load_terrains(gd->db, &gd->terrains);
    return gd;
}

int game_save(gamedata *gd)
{
    int i, err;
    int nfactions = stbds_arrlen(gd->factions.arr);
    int nregions = stbds_arrlen(gd->regions.arr);

    err = db_execute(gd->db, "BEGIN TRANSACTION");
    if (err) return err;

    for (i = 0; i != nfactions; ++i) {
        faction *f = gd->factions.arr[i];
        err = db_write_faction(gd->db, f);
        if (err) return err;
    }

    for (i = 0; i != nregions; ++i) {
        region *r = gd->regions.arr[i];
        int err = db_write_region(gd->db, r);
        if (err) return err;
    }

    err = db_execute(gd->db, "COMMIT");
    if (err) return err;

    return 0;
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
    err = db_load_terrains(gd->db, &gd->terrains);
    if (err) return err;
    err = db_regions_walk(gd->db, cb_load_region, gd);
    if (err) return err;
    return db_factions_walk(gd->db, cb_load_faction, gd);
}

int game_get_turn(struct gamedata *gd)
{
    if (gd->turn < 0) {
        const char *cfg = config_get("turn", "0");
        gd->turn = cfg ? atoi(cfg) : 0;
    }
    return gd->turn;
}

void game_set_turn(gamedata *gd, int turn)
{
    char buf[12];
    snprintf(buf, sizeof(buf), "%d", turn);
    config_set("turn", buf);
    gd->turn = turn;
}

void game_free(gamedata *gd) {
    factions_free(&gd->factions);
    regions_free(&gd->regions);
    gd->db = NULL;
}
