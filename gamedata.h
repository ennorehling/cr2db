#pragma once

#include <stdbool.h>

struct cJSON;
struct sqlite3;

struct faction;
struct region;
struct unit;
struct building;
struct ship;

typedef struct gamedata gamedata;

int factions_walk(struct gamedata *gd, int (*callback)(struct faction *, void *), void *arg);
int regions_walk(struct gamedata *gd, int (*callback)(struct region *, void *), void *arg);

struct gamedata *game_create(struct sqlite3 *db);
int game_load(struct gamedata *gd);
int game_save(struct gamedata *gd);
void game_free(struct gamedata *gd);
int game_get_turn(struct gamedata *gd);
void game_set_turn(gamedata *gd, int turn);

struct faction *faction_create(struct gamedata *gd, struct cJSON *data);
void faction_update(struct gamedata *gd, struct faction *f, struct cJSON *data);
void faction_add(struct gamedata *gd, struct faction *f);

struct region *region_create(struct gamedata *gd, struct cJSON *data);
void region_update(struct gamedata *gd, struct region *r, struct cJSON *data);
void region_add(struct gamedata *gd, struct region *r);
void region_reset(struct gamedata *gd, struct region *r);

struct unit *unit_create(struct gamedata *gd, struct region *r, struct cJSON *data);
struct ship *ship_create(struct gamedata *gd, struct region *r, struct cJSON *data);
struct building *building_create(struct gamedata *gd, struct region *r, struct cJSON *data);


void gamedata_init(void);
void gamedata_done(void);

int get_terrain(const char *name);
int get_race(const char *name);

extern char *terrainname[];
extern char *racename[];
