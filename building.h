#pragma once

typedef struct building {
    unsigned int id;
    struct region *region;
    int type;
    char * name;
    struct cJSON *data;
    struct building *next;
} building;
