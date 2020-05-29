#include "region.h"

#include <strings.h>
#include <cJSON.h>

#include <assert.h>
#include <malloc.h>

#define MAXTERRAINS 32
char *terrainname[MAXTERRAINS];
static int num_terrains;

int get_terrain(const char *name)
{
    int i;
    for (i = 0; i != num_terrains; ++i) {
        if (strcmp(name, terrainname[i]) == 0) {
            return i;
        }
    }
    assert(num_terrains < MAXTERRAINS);
    terrainname[num_terrains] = str_strdup(name);
    return num_terrains++;
}

void free_terrains(void)
{
    int i;
    for (i = 0; i != num_terrains; ++i) {
        free(terrainname[i]);
    }
}

region *create_region(struct cJSON *data)
{
    region *r = calloc(1, sizeof(region));
    if (r) {
        update_region(r, data);
    }
    return r;
}

void update_region(region *r, cJSON *data)
{
    cJSON * child;
    for (child = data->child; child != NULL; child = child->next) {
        if (r->id == 0 && child->type == cJSON_Number) {
            if (strcmp(child->string, "id") == 0) {
                r->id = child->valueint;
            }
            else if (strcmp(child->string, "x") == 0) {
                r->x = child->valueint;
            }
            else if (strcmp(child->string, "y") == 0) {
                r->y = child->valueint;
            }
            else if (strcmp(child->string, "z") == 0) {
                r->plane = child->valueint;
            }
        }
        else if (child->type == cJSON_String) {
            if (strcmp(child->string, "Name") == 0) {
                r->name = str_strdup(child->valuestring);
            }
            if (strcmp(child->string, "Terrain") == 0) {
                r->terrain = get_terrain(child->valuestring);
            }
        }
    }
    r->data = data;
}

void free_region(region *r) {
    cJSON_Delete(r->data);
    free(r->name);
    free(r);
}

