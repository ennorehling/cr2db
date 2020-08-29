#pragma once

typedef struct ship {
    unsigned int id;
    struct region *region;
    int type;
    char * name;
    struct cJSON *data;
} ship;

void ship_free(ship *s);
