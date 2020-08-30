#pragma once

typedef unsigned int terrain_id;

struct building;
struct ship;
struct unit;
struct message;

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

typedef struct region_xyz {
    int x, y, z;
} region_xyz;

typedef struct region {
    region_id id;
    region_xyz loc;
    terrain_id terrain;
    char * name;
    struct building **buildings; /* stb_arr */
    struct ship **ships; /* stb_arr */
    struct unit **units; /* stb_arr */
    struct message *messages; /* stb_arr */
    struct cJSON *data;
} region;

region *create_region(region_id id, int x, int y, int z, char *name, terrain_id terrain);
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
