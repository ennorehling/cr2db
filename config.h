#pragma once

struct cJSON;

typedef int index_t;
typedef int ship_t;

typedef struct config_data {
    char name[32];
    struct cJSON *data;
} config_data;

struct config_index {
    char *key;
    index_t value;
};

typedef struct config {
    struct config_data *arr;
    struct config_index *hash_name;
} config;

index_t config_find(config *cfg, const char * name);
index_t config_add(config *cfg, const char * name);
config_data *config_get(config *cfg, index_t type);

int config_load(struct config *cfg, const char * filename);
void config_free(struct config *types);

