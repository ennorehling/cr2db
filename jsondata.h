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

void jsondata_init(void);
void jsondata_done(void);

int factions_walk(struct gamedata *gd, int (*callback)(struct faction *, void *), void *arg);
int regions_walk(struct gamedata *gd, int (*callback)(struct region *, void *), void *arg);

struct gamedata *game_create(struct sqlite3 *db);
void game_free(struct gamedata *gd);

struct faction *faction_create(struct gamedata *gd, struct cJSON *data);
struct faction *faction_get(struct gamedata *gd, unsigned int id);
void faction_update(struct gamedata *gd, struct faction *f, struct cJSON *data);

struct region *region_create(struct gamedata *gd, struct cJSON *data);
struct region *region_get(struct gamedata *gd, int x, int y, int z);
int region_update(struct gamedata *gd, struct region *r, struct cJSON *data);
int region_delete_objects(struct gamedata *gd, struct region *r);

struct unit *unit_create(struct gamedata *gd, struct region *r, struct cJSON *data);
struct ship *ship_create(struct gamedata *gd, struct region *r, struct cJSON *data);
struct building *building_create(struct gamedata *gd, struct region *r, struct cJSON *data);

extern char *terrainname[];
extern char *racename[];
