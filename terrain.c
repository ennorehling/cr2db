#define _CRT_SECURE_NO_WARNINGS

#include "terrain.h"

#include "stb_ds.h"

#include <strings.h>
#include <cJSON.h>

#include <assert.h>

void terrains_free(terrains *all)
{
    stbds_arrfree(all->arr);
    stbds_shfree(all->hash_name);
}

terrain_id terrains_add(struct terrains *all, const char *name)
{
    unsigned int index = stbds_arraddnoff(all->arr, 1);
    terrain_id id = (terrain_id) index + 1;
    terrain *t = terrains_get(all, id);
    strncpy(t->name, name, sizeof(t->name));
    t->data = NULL;
    terrains_update(all, id);
    return id;
}

terrain *terrains_get(struct terrains *all, terrain_id id)
{
    assert(id > 0);
    if (id > stbds_arrlenu(all->arr)) {
        return NULL;
    }
    return all->arr + id - 1;
}

void terrains_update(struct terrains *all, terrain_id id)
{
    assert(id > 0);
    assert(id <= stbds_arrlenu(all->arr));
    stbds_shput(all->hash_name, all->arr[id - 1].name, id);
}

terrain_id terrains_find(terrains *all, const char *name)
{
    struct terrain_index *index;
    index = stbds_shgetp(all->hash_name, name);
    if (index) {
        return index->value;
    }
    return 0;
}
