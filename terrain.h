#pragma once

typedef unsigned int terrain_id;

typedef struct terrain {
    char name[16];
    struct terrain_data {
        unsigned int flags;
        int max_work;
        int road_cost;
    } data;
} terrain;

struct terrain_index {
    char *key;
    terrain_id value;
};

typedef struct terrains {
    terrain *arr;
    struct terrain_index *hash_name;
} terrains;

void terrains_free(terrains *all);
terrain *terrains_get(struct terrains *all, terrain_id id);
terrain_id terrains_add(struct terrains *all, const char *name);
void terrains_update(struct terrains *all, terrain_id id);
terrain_id terrains_find(struct terrains *all, const char *name);
