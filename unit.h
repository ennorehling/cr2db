#pragma once

struct cJSON;
struct faction;
struct region;
struct ship;
struct building;

typedef unsigned int race_t;
typedef unsigned int unit_id;

typedef struct unit {
    unit_id id;
    struct faction *faction;
    struct region *region;
    struct ship *ship;
    struct building *building;
    char * name;
    char * orders; /* stbds_arr */
    race_t race;
    struct cJSON *data;
} unit;

void unit_free(unit *u);
unit *create_unit(unit_id id, char *name, race_t race, struct cJSON *data);
