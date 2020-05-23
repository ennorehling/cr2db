#include "jsondata.h"

#include <strings.h>

#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <cJSON.h>

#define MAXTERRAINS 32
#define MAXRACES 64

char *terrainname[MAXTERRAINS];
static int num_terrains;

char *racename[MAXRACES];
static int num_races;

void jsondata_init(void) {
    num_terrains = 0;
    num_races = 0;
}

void jsondata_done(void) {
    int i;
    for (i = 0; i != num_terrains; ++i) {
        free(terrainname[i]);
    }
    for (i = 0; i != num_races; ++i) {
        free(racename[i]);
    }
}

static int race_get(const char *name) {
    int i;
    for (i = 0; i != num_races; ++i) {
        if (strcmp(name, racename[i]) == 0) {
            return i;
        }
    }
    assert(num_races < MAXRACES);
    racename[num_races] = str_strdup(name);
    return num_races++;
}

static int terrain_get(const char *name) {
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

struct gamedata {
    struct region *regions;
    struct faction *factions;
};

static void faction_free(faction *f) {
    cJSON_Delete(f->data);
    free(f->name);
    free(f->email);
    free(f);
}

faction *faction_create(cJSON *data)
{
    faction * f = calloc(1, sizeof(faction));
    if (f) {
        faction_update(f, data);
    }
    return f;
}

void faction_add(struct gamedata *gd, faction *f) {
    faction **fiter = &gd->factions;
    assert(f->next == NULL);
    while (*fiter) {
        if ((*fiter)->id > f->id) {
            f->next = *fiter;
            *fiter = f;
            break;
        }
    }
}

void faction_update(faction *f, cJSON *data)
{
    cJSON *child;
    for (child = data->child; child; child = child->next) {
        if (child->type == cJSON_Number) {
            if (strcmp(child->string, "id") == 0) {
                f->id = child->valueint;
            }
        }
        else if (child->type == cJSON_String) {
            if (strcmp(child->string, "Parteiname") == 0) {
                free(f->name);
                f->name = str_strdup(child->valuestring);
            }
            else if (strcmp(child->string, "email") == 0) {
                free(f->email);
                f->email = str_strdup(child->valuestring);
            }
        }
    }
    cJSON_Delete(f->data);
    f->data = data;
}

faction *faction_get(gamedata *gd, unsigned int id)
{
    faction *f;
    for (f = gd->factions; f != NULL; f = f->next) {
        if (f->id == id) {
            return f;
        }
    }
    return NULL;
}

static void region_free(region *r) {
    cJSON_Delete(r->data);
    free(r->name);
    free(r);
}

/*
region *region_create(cJSON *data) {
    region *r = calloc(1, sizeof(region));
    if (r) {
        cJSON * child;
        for (child = data->child; child != NULL; child = child->next) {
            if (child->type == cJSON_Number) {
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
                    r->terrain = terrain_get(child->valuestring);
                }
            }
        }
        r->data = data;
    }
    return r;
}

region *region_get(struct gamedata *gd, int id) {
    region *r;
    for (r = gd->regions; r; r = r->next) {
        if (r->id == id) return r;
    }
    return NULL;
}

struct cJSON *region_get_child(const struct region *r, const char *key) {
    return cJSON_GetObjectItem(r->data, key);
}

const char *region_get_str(const struct region *r, const char *key, const char *def) {
    cJSON *child = region_get_child(r, key);
    return child ? child->valuestring : def;
}

int region_get_int(const struct region *r, const char *key, int def) {
    cJSON *child = region_get_child(r, key);
    return child ? child->valueint : def;
}

struct unit *unit_create(struct region *r, struct cJSON *data) {
    unit * u = calloc(1, sizeof(unit));
    if (u) {
        cJSON * child;
        u->region = r;
        for (child = data->child; child != NULL; child = child->next) {
            if (child->type == cJSON_Number) {
                if (child->string[0] == 'i' && strcmp(child->string, "id") == 0) {
                    u->id = child->valueint;
                }
                else if (child->string[0] == 'P' && strcmp(child->string, "Partei") == 0) {
                    u->faction = faction_get(gd, child->valueint);
                }
                else if (child->string[0] == 'B' && strcmp(child->string, "Burg") == 0) {
                    u->building = building_get(gd, child->valueint, r);
                }
                else if (child->string[0] == 'S' && strcmp(child->string, "Schiff") == 0) {
                    u->ship = ship_get(gd, child->valueint, r);
                }
            }
            else if (child->type == cJSON_String) {
                if (child->string[0] == 'N' && strcmp(child->string, "Name") == 0) {
                    u->name = str_strdup(child->valuestring);
                }
                else if (child->string[0] == 'T' && strcmp(child->string, "Typ") == 0) {
                    u->race = race_get(child->valuestring);
                }
            }
            else if (child->type == cJSON_Array) {
                if (child->string[0] == 'C' && strcmp(child->string, "COMMANDS") == 0) {
                    cJSON *line;
                    char orders[4096];
                    sbstring sbs;
                    sbs_init(&sbs, orders, sizeof(orders));
                    for (line = child->child; line; line = line->next) {
                        sbs_strcat(&sbs, line->valuestring);
                        sbs_strcat(&sbs, "\n");
                    }
                    u->orders = str_strdup(orders);
                }
            }
        }
        u->data = data;
    }
    return u;
}

struct unit *unit_get(struct gamedata *gd, int id, const region *r) {
    if (r) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            if (id == u->id) return u;
        }
    }
    else {
        for (r = gd->regions; r; r = r->next) {
            unit *u = unit_get(gd, id, r);
            if (u) return u;
        }
    }
    return NULL;
}

ship *ship_create(region *r, cJSON *data) {
    ship * sh = calloc(1, sizeof(ship));
    if (sh) {
        cJSON * child;
        sh->region = r;
        for (child = data->child; child != NULL; child = child->next) {
            if (child->type == cJSON_Number) {
                if (strcmp(child->string, "id") == 0) {
                    sh->id = child->valueint;
                }
            }
            else if (child->type == cJSON_String) {
                if (strcmp(child->string, "Name") == 0) {
                    sh->name = str_strdup(child->valuestring);
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

building *building_create(region *r, cJSON *data) {
    building * b = calloc(1, sizeof(building));
    if (b) {
        cJSON * child;
        b->region = r;
        for (child = data->child; child != NULL; child = child->next) {
            if (child->type == cJSON_Number) {
                if (strcmp(child->string, "id") == 0) {
                    b->id = child->valueint;
                }
            }
            else if (child->type == cJSON_String) {
                if (strcmp(child->string, "Name") == 0) {
                    b->name = str_strdup(child->valuestring);
                }
            }
        }
        b->data = data;
        if (r) {
            b->next = r->buildings;
            r->buildings = b;
        }
    }
    return b;
}

ship *ship_get(gamedata *gd, int id, const region *r) {
    if (r) {
        ship *sh;
        for (sh = r->ships; sh != NULL; sh = sh->next) {
            if (sh->id == id) {
                return sh;
            }
        }
    }
    else {
        for (r = gd->regions; r; r = r->next) {
            ship *sh = ship_get(gd, id, r);
            if (sh) return sh;
        }
    }
    return NULL;
}

building *building_get(gamedata *gd, int id, const region *r) {
    if (r) {
        building *b;
        for (b = r->buildings; b != NULL; b = b->next) {
            if (b->id == id) {
                return b;
            }
        }
    }
    else {
        for (r = gd->regions; r; r = r->next) {
            building *b = building_get(gd, id, r);
            if (b) return b;
        }
    }
    return NULL;
}

*/
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
