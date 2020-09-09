#pragma once

#include "region.h"
#include "faction.h"

#include "config.h"

#include <stdbool.h>

struct cJSON;
struct sqlite3;

struct unit;
struct building;
struct ship;
struct message;

typedef struct battle {
    region_xyz loc;
    struct message *messages; /* stb_arr */
} battle;

typedef struct gamedata {
    struct sqlite3 *db;
    int turn;
    struct regions regions;
    struct factions factions;

    struct config terrains;
    struct config building_types;
    struct config ship_types;
} gamedata;

int factions_walk(struct gamedata *gd, int (*callback)(struct faction *, void *), void *arg);
int regions_walk(struct gamedata *gd, int (*callback)(struct region *, void *), void *arg);
int buildings_walk(struct gamedata *gd, int (*callback)(struct building *, void *), void *arg);

struct gamedata *game_create(struct sqlite3 *db);
int game_load(struct gamedata *gd);
int game_save(struct gamedata *gd);
void game_free(struct gamedata *gd);

int gd_load_config(struct gamedata *gd);

struct faction *gd_create_faction(struct gamedata *gd, struct cJSON *data);
void gd_update_faction(struct gamedata *gd, struct faction *f, struct cJSON *data);
void gd_add_faction(struct gamedata *gd, struct faction *f);

struct region *gd_create_region(struct gamedata *gd, struct cJSON *data);
void gd_update_region(struct gamedata *gd, struct region *r, struct cJSON *data);
void gd_add_region(struct gamedata *gd, struct region *r);
void region_reset(struct gamedata *gd, struct region *r);

struct ship *gd_create_ship(struct gamedata *gd, struct region *r, struct cJSON *data);
void gd_update_ship(struct gamedata *gd, struct ship *b, struct cJSON *data);
void gd_add_ship(struct gamedata *gd, struct ship *b);

struct building *gd_create_building(struct gamedata *gd, struct region *r, struct cJSON *data);
void gd_update_building(struct gamedata *gd, struct building *b, struct cJSON *data);
void gd_add_building(struct gamedata *gd, struct building *b);
