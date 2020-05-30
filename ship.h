#pragma once

typedef struct ship {
    unsigned int id;
    struct region *region;
    int type;
    char * name;
    struct cJSON *data;
    struct ship *next;
} ship;

void free_ship(ship *s);
