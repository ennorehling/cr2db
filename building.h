#pragma once

struct region;

typedef unsigned int building_id;
typedef unsigned int building_t;

typedef struct building {
    building_id id;
    struct region *region;
    building_t type;
    char *name;
    struct cJSON *data;
} building;

void free_building(building *b);
building *create_building(building_id id, struct region *r, char *name, building_t type);
