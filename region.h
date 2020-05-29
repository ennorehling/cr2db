#pragma once

struct cJSON;
struct building;
struct ship;
struct unit;

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

int get_terrain(const char *name);
void free_terrains(void);

region *create_region(struct cJSON *data);
void update_region(region *r, struct cJSON *data);
void free_region(region *r);
