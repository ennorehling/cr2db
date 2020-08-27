#pragma once

struct cJSON;
struct building;
struct ship;
struct unit;
struct message;

typedef unsigned int terrain_id;

typedef struct terrain {
    terrain_id id;
    char name[16];
    char crname[16];
} terrain;

struct terrain_index {
    char *key;
    int value;
};

typedef struct terrains {
    terrain *arr;
    struct terrain_index *hash_name;
    struct terrain_index *hash_crname;
} terrains;

void terrains_free(terrains *all);
void terrains_add(struct terrains *all, const struct terrain *t);
const struct terrain * terrains_get_name(struct terrains *all, const char *name);
const struct terrain * terrains_get_crname(struct terrains *all, const char *crname);

typedef unsigned int region_id;

struct region_xyz {
    int x, y, z;
};

typedef struct region {
    region_id id;
    struct region_xyz loc;
    terrain_id terrain;
    char * name;
    struct building **buildings; /* stretchy_buffer */
    struct ship **ships; /* stretchy_buffer */
    struct unit **units; /* stretchy_buffer */
    struct message *messages; /* stretchy_buffer */
    struct cJSON *data;
} region;

region *create_region(void);
void region_free(region *r);

struct region_index_xyz {
    struct region_xyz key;
    unsigned int value;
};

struct region_index_uid {
    region_id key;
    unsigned int value;
};

typedef struct regions {
    struct region **arr;
    struct region_index_xyz *hash_xyz;
    struct region_index_uid *hash_uid;
} regions;

void regions_free(regions *all);

void regions_add(regions *all, region *r);
region *regions_get_xyz(regions *all, int x, int y, int z);
region *regions_get_uid(regions *all, region_id uid);
