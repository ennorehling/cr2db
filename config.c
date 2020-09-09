#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "config.h"

#include "stb_ds.h"

#include <cJSON.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>

index_t config_find(config *all, const char * name)
{
    struct config_index *index;
    index = stbds_shgetp(all->hash_name, name);
    if (index) {
        return index->value;
    }
    return -1;
}

index_t config_add(config *cfg, const char * name)
{
    index_t index = (index_t)stbds_arraddnoff(cfg->arr, 1);
    config_data *c = config_get(cfg, index);

    strncpy(c->name, name, sizeof(c->name));
    c->data = NULL;
    return index;
}

config_data *config_get(config *cfg, index_t index)
{
    assert(index >= 0);
    if (index >= stbds_arrlen(cfg->arr)) {
        return NULL;
    }
    return cfg->arr + index;
}

static cJSON *load_json(FILE *F)
{
    cJSON *config;
    char *data;
    size_t sz;
    long pos;
    fseek(F, 0, SEEK_END);
    pos = ftell(F);
    rewind(F);
    data = malloc(pos + 1);
    if (!data) abort();
    sz = fread(data, 1, (size_t)pos, F);
    data[sz] = 0;
    config = cJSON_Parse(data);
    free(data);
    return config;
}

int config_load(struct config *cfg, const char * filename)
{
    FILE * F = fopen(filename, "rt");
    cJSON *json;
    if (!F) {
        return errno;
    }
    /* TODO: implement loading */
    json = load_json(F);
    if (json && json->type == cJSON_Object) {
        cJSON **it;
        for (it = &json->child; *it;) {
            cJSON *child = *it;
            index_t id = config_find(cfg, child->string);
            config_data *c;
            if (id < 0) {
                id = config_add(cfg, child->string);
            }
            c = config_get(cfg, id);
            if (c->data) {
                cJSON_Delete(c->data);
            }
            c->data = child;
            *it = child->next;
            child->next = NULL;
        }
        json->child = NULL;
        cJSON_Delete(json);
    }
    return fclose(F);
}

void config_free(struct config *cfg)
{
    int i, len = stbds_arrlen(cfg->arr);
    for (i = 0; i != len; ++i) {
        cJSON_Delete(cfg->arr[i].data);
    }
    stbds_arrfree(cfg->arr);
    stbds_shfree(cfg->hash_name);
}
