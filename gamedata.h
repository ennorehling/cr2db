#pragma once

#include "region.h"
#include "faction.h"

#include <stdbool.h>

struct cJSON;
struct sqlite3;

struct unit;
struct building;
struct ship;

typedef struct gamedata {
    struct sqlite3 *db;
    int turn;
    struct terrains terrains;
    struct regions regions;
    struct factions factions;
} gamedata;

int factions_walk(struct gamedata *gd, int (*callback)(struct faction *, void *), void *arg);
int regions_walk(struct gamedata *gd, int (*callback)(struct region *, void *), void *arg);

struct gamedata *game_create(struct sqlite3 *db);
int game_load(struct gamedata *gd);
int game_save(struct gamedata *gd);
void game_free(struct gamedata *gd);
int game_get_turn(struct gamedata *gd);
void game_set_turn(gamedata *gd, int turn);

struct faction *gd_create_faction(struct gamedata *gd, struct cJSON *data);
void gd_update_faction(struct gamedata *gd, struct faction *f, struct cJSON *data);
void gd_add_faction(struct gamedata *gd, struct faction *f);

struct region *gd_create_region(struct gamedata *gd, struct cJSON *data);
void gd_update_region(struct gamedata *gd, struct region *r, struct cJSON *data);
void gd_add_region(struct gamedata *gd, struct region *r);
void region_reset(struct gamedata *gd, struct region *r);

struct unit *unit_create(struct gamedata *gd, struct region *r, struct cJSON *data);
struct ship *ship_create(struct gamedata *gd, struct region *r, struct cJSON *data);
struct building *building_create(struct gamedata *gd, struct region *r, struct cJSON *data);
