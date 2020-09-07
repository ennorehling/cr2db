#pragma once

struct cJSON;
struct terrains;
struct building_type;
struct building_types;
struct ship_type;
struct ship_types;

typedef int building_t;

typedef struct building_type {
    char name[32];
    struct cJSON *data;
} building_type;

struct building_type_index {
    char *key;
    building_t value;
};

typedef struct building_types {
    struct building_type *arr;
    struct building_type_index *hash_name;
} building_types;

building_t bt_find(building_types *all, const char * name);
building_t bt_add(building_types *all, const char * name);
building_type *bt_get(building_types *all, building_t type);

typedef struct ship_type {
    char name[32];
    struct cJSON *data;
} ship_type;

typedef struct ship_types {
    struct ship_type *arr;
} ship_types;

int config_load_terrains(struct terrains *types, const char * filename);
int config_load_buildings(struct building_types *types, const char * filename);
int config_load_ships(struct ship_types *types, const char * filename);

