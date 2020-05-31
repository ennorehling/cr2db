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

region *create_region(cJSON *data)
{
    region *r = calloc(1, sizeof(region));
    r->data = data;
    return r;
}

void free_region(region *r) {
    cJSON_Delete(r->data);
    free(r->name);
}

