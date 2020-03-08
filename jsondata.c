#include "jsondata.h"

#include <strings.h>

#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <cJSON.h>

#define MAXTERRAINS 32

char *terrainname[MAXTERRAINS];
static int num_terrains;

void jsondata_open(void) {
    num_terrains = 0;
}

void jsondata_close(void) {
    int i;
    for (i = 0; i != num_terrains; ++i) {
        free(terrainname[i]);
    }
}

static int terrain_get(const char *name) {
    int i;
    for (i = 0; i != num_terrains; ++i) {
        if (strcmp(name, terrainname[i]) == 0) {
            return i;
        }
    }
    ++num_terrains;
    assert(num_terrains < MAXTERRAINS);
    terrainname[num_terrains] = str_strdup(name);
    return num_terrains;
}

struct gamedata {
    region *regions;
    faction *factions;
};

faction *faction_create(gamedata *gd, cJSON *data) {
    faction * f = calloc(1, sizeof(faction));
    if (f) {
        cJSON *child;
        for (child = data->child; child; child = child->next) {
            if (data->type == cJSON_Number) {
                if (strcmp(data->string, "id") == 0) {
                    f->id = data->valueint;
                }
            }
            else if (data->type == cJSON_String) {
                if (strcmp(data->string, "Parteiname") == 0) {
                    f->name = str_strdup(data->valuestring);
                }
                else if (strcmp(data->string, "email") == 0) {
                    f->email = str_strdup(data->valuestring);
                }
            }
        }
        f->data = data;

        if (gd) {
            f->next = gd->factions;
            gd->factions = f;
        }
    }
    return f;
}

faction *faction_get(gamedata *gd, int id) {
    faction *f;
    for (f = gd->factions; f != NULL; f = f->next) {
        if (f->id == id) {
            return f;
        }
    }
    return NULL;
}

region *region_create(gamedata *gd, cJSON *data) {
    region *r = calloc(1, sizeof(region));
    if (r) {
        cJSON * child;
        for (child = data->child; data != NULL; data = data->next) {
            if (data->type == cJSON_Number) {
                if (strcmp(data->string, "id") == 0) {
                    r->id = data->valueint;
                }
                else if (strcmp(data->string, "x") == 0) {
                    r->x = data->valueint;
                }
                else if (strcmp(data->string, "y") == 0) {
                    r->y = data->valueint;
                }
                else if (strcmp(data->string, "z") == 0) {
                    r->plane = data->valueint;
                }
            }
            else if (data->type == cJSON_String) {
                if (strcmp(data->string, "Name") == 0) {
                    r->name = str_strdup(data->valuestring);
                }
                if (strcmp(data->string, "Terrain") == 0) {
                    r->terrain = terrain_get(data->valuestring);
                }
            }
        }
        r->data = data;
        if (gd) {
            r->next = gd->regions;
            gd->regions = r;
        }
    }
    return r;
}

ship *ship_create(gamedata *gd, region *r, cJSON *data) {
    ship * sh = calloc(1, sizeof(ship));
    if (sh) {
        cJSON * child;
        sh->id = 0;
        sh->name = NULL;
        for (child = data->child; data != NULL; data = data->next) {
            if (data->type == cJSON_Number) {
                if (strcmp(data->string, "id") == 0) {
                    sh->id = data->valueint;
                }
            }
            else if (data->type == cJSON_String) {
                if (strcmp(data->string, "Name") == 0) {
                    sh->name = str_strdup(data->valuestring);
                }
            }
        }
        sh->data = data;
        if (r) {
            sh->next = r->ships;
            r->ships = sh;
        }
    }
    return sh;
}

ship *ship_get(gamedata *gd, int id) {
    region *r;
    for (r = gd->regions; r; r = r->next) {
        ship *sh;
        for (sh = r->ships; sh != NULL; sh = sh->next) {
            if (sh->id == id) {
                return sh;
            }
        }
    }
    return NULL;
}

static void faction_free(faction *f) {
    cJSON_Delete(f->data);
    free(f->name);
    free(f->email);
    free(f);
}

static void region_free(region *r) {
    cJSON_Delete(r->data);
    free(r->name);
    free(r);
}

gamedata *game_create(void) {
    gamedata *gd = malloc(sizeof(gamedata));
    gd->regions = NULL;
    gd->factions = NULL;
    return gd;
}

void game_free(gamedata *gd) {
    while (gd->factions) {
        faction *f = gd->factions;
        gd->factions = f->next;
        faction_free(f);
    }
    while (gd->regions) {
        region *r = gd->regions;
        gd->regions = r->next;
        region_free(r);
    }
}
