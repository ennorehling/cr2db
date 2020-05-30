#pragma once

struct cJSON;

typedef struct faction {
    unsigned int id;
    char * name;
    char * email;
    struct cJSON *data;
    struct faction *next;
} faction;


void free_faction(faction *f);
faction *create_faction(struct cJSON *data);
