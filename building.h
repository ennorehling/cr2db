#pragma once

struct region;

typedef unsigned int building_id;
typedef unsigned int building_t;

typedef struct building {
    building_id id;
    building_t type;
    struct region *region;
    char *name;
    struct cJSON *data;
} building;

building *create_building(building_id id, struct region *r, char *name, building_t type);
void building_free(building *b);
