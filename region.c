#include "region.h"
#include "message.h"

#include "stb_ds.h"

#include <strings.h>
#include <cJSON.h>

#include <assert.h>

region *create_region(region_id id, int x, int y, int z, char *name, terrain_id terrain)
{
    region *r = malloc(sizeof(region));
    r->id = id;
    r->loc.x = x;
    r->loc.y = y;
    r->loc.z = z;
    r->terrain = terrain;
    r->name = name;
    r->buildings = NULL;
    r->ships = NULL;
    r->units = NULL;
    r->messages = NULL;
    r->data = NULL;
    return r;
}

void region_free(region *r)
{
    unsigned int i, len;
    for (i = 0, len = stbds_arrlen(r->messages); i != len; ++i) {
        message_free(r->messages + i);
    }
    stbds_arrfree(r->messages);
    cJSON_Delete(r->data);
    free(r->name);
}

void regions_free(regions *all)
{
    unsigned int i, len;
    for (i = 0, len = stbds_arrlen(all->arr); i != len; ++i) {
        region_free(all->arr[i]);
        free(all->arr[i]);
    }
    stbds_arrfree(all->arr);
    stbds_hmfree(all->hash_uid);
    stbds_hmfree(all->hash_xyz);
}

void regions_add(regions *all, region *r)
{
    unsigned int index;
    stbds_arrput(all->arr, r);
    index = stbds_arrlen(all->arr);
    stbds_hmput(all->hash_xyz, r->loc, index);
    if (r->id) {
        stbds_hmput(all->hash_uid, r->id, index);
    }
}
region *regions_get_xyz(regions *all, int x, int y, int z)
{
    struct region_index_xyz *index;
    struct region_xyz xyz = { x, y, z };
    index = stbds_hmgetp(all->hash_xyz, xyz);
    if (index) {
        return all->arr[index->value];
    }
    return NULL;
}

region *regions_get_uid(regions *all, region_id uid)
{
    struct region_index_uid *index;
    index = stbds_hmgetp(all->hash_uid, uid);
    if (index) {
        return all->arr[index->value];
    }
    return NULL;
}
