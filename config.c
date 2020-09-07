#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "config.h"
#include "terrain.h"

#include "stb_ds.h"

#include <cJSON.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>

building_t bt_find(building_types *all, const char * name)
{
    struct building_type_index *index;
    index = stbds_shgetp(all->hash_name, name);
    if (index) {
        return index->value;
    }
    return -1;
}

building_t bt_add(building_types *all, const char * name)
{
    building_t index = (building_t)stbds_arraddnoff(all->arr, 1);
    building_type *btype = bt_get(all, index);

    strncpy(btype->name, name, sizeof(btype->name));
    btype->data = NULL;
    return index;
}

building_type *bt_get(building_types *all, building_t type)
{
    assert(type >= 0);
    if (type >= stbds_arrlen(all->arr)) {
        return NULL;
    }
    return all->arr + type;
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

int config_load_terrains(struct terrains *types, const char * filename)
{
    FILE * F = fopen(filename, "rt");
    cJSON *json;
    if (!F) {
        return errno;
    }
    /* TODO: implement loading */
    json = load_json(F);
    if (json && json->type == cJSON_Object) {
        cJSON *child;
        for (child = json->child; child; child = child->next) {
            terrain_id id = terrains_find(types, child->string);
            terrain *t;
            if (id == 0) {
                id = terrains_add(types, child->string);
            }
            t = terrains_get(types, id);
            if (t->data) {
                cJSON_Delete(t->data);
            }
            t->data = child;
        }
        json->child = NULL;
        cJSON_Delete(json);
    }
    (void)types;
    return fclose(F);
}

int config_load_buildings(struct building_types *types, const char * filename)
{
    FILE * F = fopen(filename, "rt");
    if (!F) {
        return errno;
    }
    /* TODO: implement loading */
    (void)types;
    return fclose(F);
}

int config_load_ships(struct ship_types *types, const char * filename)
{
    FILE * F = fopen(filename, "rt");
    if (!F) {
        return errno;
    }
    /* TODO: implement loading */
    (void)types;
    return fclose(F);
}
