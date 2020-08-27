#pragma once

struct cJSON;
struct faction;
struct region;
struct ship;
struct building;

typedef struct unit {
    unsigned int id;
    struct faction *faction;
    struct region *region;
    struct ship *ship;
    struct building *building;
    char * name;
    char * orders; /* stretchy_buffer */
    int race;
    struct cJSON *data;
} unit;

void free_unit(unit *u);
unit *create_unit(struct cJSON *data);
