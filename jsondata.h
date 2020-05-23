#pragma once

struct cJSON;
struct sqlite3;

typedef struct gamedata gamedata;

typedef struct region {
    unsigned int id;
    int x, y, plane;
    int terrain;
    char * name;
    struct building *buildings;
    struct ship *ships;
    struct unit *units;
    int turn;
    struct cJSON *data;
    struct region *next;
} region;

typedef struct faction {
    unsigned int id;
    char * name;
    char * email;
    struct cJSON *data;
    struct faction *next;
} faction;

typedef struct building {
    unsigned int id;
    struct region *region;
    int type;
    char * name;
    struct cJSON *data;
    struct building *next;
} building;

typedef struct ship {
    unsigned int id;
    struct region *region;
    int type;
    char * name;
    struct cJSON *data;
    struct ship *next;
} ship;

typedef struct unit {
    unsigned int id;
    struct faction *faction;
    struct region *region;
    struct ship *ship;
    struct building *building;
    char * name;
    char * orders;
    int race;
    struct cJSON *data;
    struct unit *next;
} unit;

void jsondata_init(void);
void jsondata_done(void);

struct gamedata *game_create(void);
void game_free(struct gamedata *gd);

struct faction *faction_create(struct cJSON *data);
struct faction *faction_get(struct gamedata *gd, unsigned int id);
void faction_update(struct faction *f, struct cJSON *data);
void faction_add(struct gamedata *gd, faction *f);

struct region *region_create(struct cJSON *data);
struct region *region_get(struct gamedata *gd, unsigned int id);
void region_update(struct region *r, struct cJSON *data);
void region_add(struct gamedata *gd, region *r);

struct ship *ship_create(struct region *r, struct cJSON *data);
struct ship *ship_get(struct gamedata *gd, unsigned int id, const struct region *r);
void ship_update(struct ship *s, struct cJSON *data);
void ship_add(struct gamedata *gd, ship *s);

struct building *building_create(struct region *r, struct cJSON *data);
struct building *building_get(struct gamedata *gd, unsigned int id, const struct region *r);
void building_update(struct building *b, struct cJSON *data);
void building_add(struct gamedata *gd, building *b);

struct unit *unit_create(struct region *r, struct cJSON *data);
struct unit *unit_get(struct gamedata *gd, unsigned int id, const struct region *r);
void unit_update(struct unit *u, struct cJSON *data);
void unit_add(struct gamedata *gd, unit *u);

struct cJSON *region_get_child(const struct region *r, const char *key);
const char *region_get_str(const struct region *r, const char *key, const char *def);
int region_get_int(const struct region *r, const char *key, int def);

extern char *terrainname[];
extern char *racename[];
