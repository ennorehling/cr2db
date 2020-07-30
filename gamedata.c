#include "gamedata.h"

#include "gamedb.h"
#include "faction.h"
#include "region.h"
#include "unit.h"
#include "ship.h"
#include "building.h"
#include "config.h"

#include "stb_ds.h"

#include <strings.h>
#include <cJSON.h>

#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXRACES 64
static int num_races;
char *racename[MAXRACES];

int get_race(const char *name) {
    int i;
    for (i = 0; i != num_races; ++i) {
        if (strcmp(name, racename[i]) == 0) {
            return i;
        }
    }
    assert(num_races < MAXRACES);
    racename[num_races] = str_strdup(name);
    return num_races++;
}

void gamedata_init(void)
{
}

void gamedata_done(void)
{
    int i;
    for (i = 0; i != num_races; ++i) {
        free(racename[i]);
    }
}

struct gamedata {
    struct sqlite3 *db;
    int turn;
    struct terrains terrains;
    struct regions regions;
    struct factions factions;
};


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

static void update_faction(gamedata *gd, faction *f, cJSON *data)
{
    (void)gd;

    if (f->data != data) {
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
        cJSON_Delete(f->data);
        f->data = data;
    }
}

void faction_add(gamedata *gd, faction *f)
{
    factions_add(&gd->factions, f);
}

faction *faction_create(gamedata *gd, cJSON *data)
{
    faction * f = create_faction(NULL);
    if (f) {
        update_faction(gd, f, data);
    }
    return f;
}

void faction_update(gamedata *gd, faction *f, cJSON *data)
{
    update_faction(gd, f, data);
}

static void update_region(gamedata *gd, region *r, cJSON *data)
{
    assert(data);
    if (r->data != data) {
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
                    r->terrain = t->id;
                }
            }
        }
        cJSON_Delete(r->data);
        r->data = data;
    }
}

void region_add(gamedata *gd, region *r)
{
    regions_add(&gd->regions, r);
}

region *region_create(gamedata *gd, cJSON *data) {
    region *r = create_region(NULL);
    if (r) {
        update_region(gd, r, data);
    }
    return r;
}

void region_update(gamedata *gd, region *r, cJSON *data)
{
    update_region(gd, r, data);
}

void region_reset(struct gamedata *gd, struct region *r)
{
    while (r->buildings) {
        building *c = r->buildings;
        r->buildings = c->next;
        free_building(c);
    }
    while (r->ships) {
        ship *c = r->ships;
        r->ships = c->next;
        free_ship(c);
    }
    while (r->units) {
        unit *u = r->units;
        r->units = u->next;
        free_unit(u);
    }
}

struct unit *unit_create(struct gamedata *gd, struct region *r, struct cJSON *data)
{
    unit * u = create_unit(data);
    if (u) {
        u->region = r;
    }
    return u;
}

struct ship *ship_create(struct gamedata *gd, struct region *r, struct cJSON *data)
{
    return NULL;
}

struct building *building_create(struct gamedata *gd, struct region *r, struct cJSON *data)
{
    return NULL;
}

gamedata *game_create(struct sqlite3 *db)
{
    gamedata *gd = calloc(1, sizeof(gamedata));
    gd->db = db;
    db_load_map(gd->db, &gd->regions);
    db_load_factions(gd->db, &gd->factions);
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

static int cb_load_region(region *tmp, void *udata) {
    gamedata *gd = (gamedata *)udata;
    region *obj = create_region(NULL);
    if (obj) {
        memcpy(obj, tmp, sizeof(region));
        tmp->name = NULL;
        tmp->data = NULL;
        region_add(gd, obj);
        return 0;
    }
    return ENOMEM;
}

static int cb_load_faction(faction *tmp, void *udata) {
    gamedata *gd = (gamedata *)udata;
    faction *obj = create_faction(NULL);
    if (obj) {
        memcpy(obj, tmp, sizeof(faction));
        tmp->name = NULL;
        tmp->email = NULL;
        tmp->data = NULL;
        faction_add(gd, obj);
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
    if (gd->turn <= 0) {
        gd->turn = atoi(config_get("turn", "0"));
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
