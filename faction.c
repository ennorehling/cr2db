#include "faction.h"
#include "gamedata.h"
#include "message.h"

#include "stb_ds.h"

#include <strings.h>
#include <cJSON.h>

#include <string.h>

void message_arrfree(message *arr) {
    unsigned int i, len;
    for (i = 0, len = stbds_arrlen(arr); i != len; ++i) {
        message_free(arr + i);
    }
    stbds_arrfree(arr);
}

void faction_free(faction *f)
{
    unsigned int i, len;
    free(f->name);
    free(f->email);
    message_arrfree(f->messages);
    f->messages = NULL;
    for (i = 0, len = stbds_arrlen(f->battles); i != len; ++i) {
        struct battle *b = f->battles + i;
        message_arrfree(b->messages);
    }
    stbds_arrfree(f->battles);
    cJSON_Delete(f->data);
}

faction *create_faction(faction_id id, char *name, char *email)
{
    faction * f = malloc(sizeof(faction));
    f->id = id;
    f->name = name;
    f->email = email;
    f->messages = NULL;
    f->data = NULL;
    return f;
}

void factions_free(factions *all)
{
    int i, len = stbds_arrlen(all->arr);
    for (i = 0; i != len; ++i) {
        faction_free(all->arr[i]);
        free(all->arr[i]);
    }
    stbds_arrfree(all->arr);
    stbds_hmfree(all->hash_uid);
}

void factions_add(factions *all, faction *f)
{
    unsigned int index;
    stbds_arrput(all->arr, f);
    index = stbds_arrlen(all->arr);
    stbds_hmput(all->hash_uid, f->id, index);
}

faction *factions_get(factions *all, faction_id uid)
{
    struct faction_index_uid *index;
    index = stbds_hmgetp(all->hash_uid, uid);
    if (index) {
        return all->arr[index->value];
    }
    return NULL;
}
