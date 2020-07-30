#include "region.h"

#include "stb_ds.h"

#include <strings.h>
#include <cJSON.h>

#include <assert.h>
#include <malloc.h>

void terrains_free(terrains *all)
{
    stbds_arrfree(all->arr);
    stbds_shfree(all->hash_name);
    stbds_shfree(all->hash_crname);
}

void terrains_add(terrains *all, const terrain *t)
{
    int index;

    assert(t);
    stbds_arrput(all->arr, *t);
    index = stbds_arrlen(all->arr);
    stbds_shput(all->hash_name, t->name, index);
    stbds_shput(all->hash_crname, t->crname, index);
}

const terrain *terrains_get_name(terrains *all, const char *name)
{
    struct terrain_index *index;
    index = stbds_shgetp(all->hash_name, name);
    if (index) {
        return all->arr + index->value;
    }
    return NULL;
}

const terrain *terrains_get_crname(terrains *all, const char *crname)
{
    struct terrain_index *index;
    index = stbds_shgetp(all->hash_crname, crname);
    if (index) {
        return all->arr + index->value;
    }
    return NULL;
}

region *create_region(cJSON *data)
{
    region *r = calloc(1, sizeof(region));
    r->data = data;
    return r;
}

void free_region(region *r)
{
    cJSON_Delete(r->data);
    free(r->name);
}

void regions_free(regions *all)
{
    int i, len = stbds_arrlen(all->arr);
    for (i = 0; i != len; ++i) {
        free_region(all->arr[i]);
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
    struct region_xyz xyz;
    xyz.x = x;
    xyz.y = y;
    xyz.z = z;
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
