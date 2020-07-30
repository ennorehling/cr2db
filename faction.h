#pragma once

struct cJSON;
struct message;

typedef unsigned int faction_id;

typedef struct faction {
    faction_id id;
    char * name;
    char * email;
    struct message *messages;
    struct cJSON *data;
} faction;

void free_faction(faction *f);
faction *create_faction(struct cJSON *data);

struct faction_index_uid {
    faction_id key;
    unsigned int value;
};

typedef struct factions {
    faction **arr;
    struct faction_index_uid *hash_uid;
} factions;

void factions_free(factions *all);
void factions_add(factions *all, faction *f);
faction *factions_get(factions *all, faction_id uid);
