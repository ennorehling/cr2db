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
    char * name; /* stretchy_buffer */
    char * orders;
    int race;
    struct cJSON *data;
    struct unit *next;
} unit;

void free_unit(unit *u);
unit *create_unit(struct cJSON *data);
