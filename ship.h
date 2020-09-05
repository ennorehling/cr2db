#pragma once

typedef unsigned int ship_id;
typedef unsigned int ship_t;

typedef struct ship {
    ship_id id;
    struct region *region;
    ship_t type;
    char * name;
    struct cJSON *data;
} ship;

void ship_free(ship *s);
ship *create_ship(ship_id id, struct region *r, char *name, ship_t type);
