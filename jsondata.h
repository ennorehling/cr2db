#pragma once

struct cJSON;

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

void jsondata_open(void);
void jsondata_close(void);

struct gamedata *game_create(void);
void game_free(struct gamedata *gd);

struct faction *faction_create(struct gamedata *gd, struct cJSON *data);
struct faction *faction_get(struct gamedata *gd, int id);

struct region *region_create(struct gamedata *gd, struct cJSON *data);
struct region *region_get(struct gamedata *gd, int id);

struct ship *ship_create(struct gamedata *gd, struct region *r, struct cJSON *data);
struct ship *ship_get(struct gamedata *gd, int id);

extern char *terrainname[];
