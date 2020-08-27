#pragma once

struct region;

typedef struct building {
    unsigned int id;
    struct region *region;
    int type;
    char * name;
    struct cJSON *data;
} building;

void free_building(building *b);
