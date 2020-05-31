#include "jsondata.h"

#include "gamedb.h"
#include "faction.h"
#include "region.h"
#include "unit.h"
#include "ship.h"
#include "building.h"

#include <strings.h>

#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <cJSON.h>

#define MAXRACES 64
char *racename[MAXRACES];
static int num_races;

struct gamedata {
    struct sqlite3 *db;
    region *regions;
    faction *factions;
};

int factions_walk(struct gamedata *gd, int (*callback)(struct faction *, void *), void *arg)
{
    return db_factions_walk(gd->db, callback, arg);
}

int regions_walk(struct gamedata *gd, int (*callback)(struct region *, void *), void *arg)
{
    return db_regions_walk(gd->db, callback, arg);
}


void jsondata_init(void) {
}

void jsondata_done(void) {
    free_terrains();
    int i;
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

static void update_faction(gamedata *gd, faction *f, cJSON *data)
{
    if (f->data != data) {
        cJSON **p_child = &data->child;
        while (*p_child) {
            cJSON *child = *p_child;
            if (child->type == cJSON_Number) {
                if (f->id == 0 && strcmp(child->string, "id") == 0) {
                    f->id = child->valueint;
                    *p_child = child->next;
                    child->next = NULL;
                    cJSON_Delete(child);
                    continue;
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
            p_child = &child->next;
        }
        cJSON_Delete(f->data);
        f->data = data;
    }
}

static void faction_add(gamedata *gd, faction *f) {
    faction **iter = &gd->factions;
    assert(f->next == NULL);
    while (*iter) {
        faction *ff = *iter;
        if (ff->id == ff->id) {
            f->next = ff;
            *iter = f;
            break;
        }
    }
}

faction *faction_create(gamedata *gd, cJSON *data)
{
    faction * f = create_faction(NULL);
    if (f) {
        update_faction(gd, f, data);
        faction_add(gd, f);
        db_write_faction(gd->db, f);
    }
    return f;
}

faction *faction_get(gamedata *gd, unsigned int id)
{
    faction *f;
    for (f = gd->factions; f != NULL; f = f->next) {
        if (f->id == id) {
            return f;
        }
    }
    f = db_read_faction(gd->db, id);
    if (f) {
        faction_add(gd, f);
    }
    return f;
}

void faction_update(gamedata *gd, faction *f, cJSON *data)
{
    update_faction(gd, f, data);
    db_write_faction(gd->db, f);
}

static void update_region(gamedata *gd, region *r, cJSON *data)
{
    if (r->data != data) {
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
        cJSON_Delete(r->data);
        r->data = data;
    }
}

static void region_add(gamedata *gd, region *r) {
    region **iter = &gd->regions;
    assert(r->next == NULL);
    while (*iter) {
        region *rr = *iter;
        if (r->x == rr->x && r->y == rr->y && r->plane == rr->plane) {
            r->next = rr;
            *iter = r;
            break;
        }
    }
}

region *region_create(gamedata *gd, cJSON *data) {
    region *r = create_region(NULL);
    if (r) {
        update_region(gd, r, data);
        region_add(gd, r);
        db_write_region(gd->db, r);
    }
    return r;
}

region *region_get(gamedata *gd, int x, int y, int z) {
    region *r;
    for (r = gd->regions; r; r = r->next) {
        if (r->x == x && r->y == y && r->plane == z) {
            return r;
        }
    }
    r = db_read_region(gd->db, x, y, z);
    if (r) {
        /* TODO: we have already iterated the list to where this insert should take place: */
        region_add(gd, r);
    }
    return r;
}

int region_update(gamedata *gd, region *r, cJSON *data)
{
    if (data) {
        update_region(gd, r, data);
    }
    return db_write_region(gd->db, r);
}

int region_delete_objects(struct gamedata *gd, struct region *r)
{
    while (r->buildings) {
        building *c = r->buildings;
        r->buildings = c->next;
        free_building(c);
    }
    while (r->ships) {
        ship *c = r->ships;
        r->ships = c->next;
        free_ship(c);
    }
    while (r->units) {
        unit *u = r->units;
        r->units = u->next;
        free_unit(u);
    }
    return db_region_delete_objects(gd->db, r->id);
}

struct unit *unit_create(struct gamedata *gd, struct region *r, struct cJSON *data)
{
    unit * u = create_unit(data);
    if (u) {
        u->region = r;
    }
    return u;
}

struct ship *ship_create(struct gamedata *gd, struct region *r, struct cJSON *data)
{
    return NULL;
}

struct building *building_create(struct gamedata *gd, struct region *r, struct cJSON *data)
{
    return NULL;
}

/*
cJSON *region_get_child(const region *r, const char *key) {
    return cJSON_GetObjectItem(r->data, key);
}

const char *region_get_str(const region *r, const char *key, const char *def) {
    cJSON *child = region_get_child(r, key);
    return child ? child->valuestring : def;
}

int region_get_int(const region *r, const char *key, int def) {
    cJSON *child = region_get_child(r, key);
    return child ? child->valueint : def;
}

unit *unit_create(region *r, cJSON *data) {
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

unit *unit_get(gamedata *gd, int id, const region *r) {
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
gamedata *game_create(struct sqlite3 *db) {
    gamedata *gd = malloc(sizeof(gamedata));
    gd->db = db;
    gd->regions = NULL;
    gd->factions = NULL;
    return gd;
}

void game_free(gamedata *gd) {
    gd->db = NULL;
    while (gd->factions) {
        faction *f = gd->factions;
        gd->factions = f->next;
        free_faction(f);
        free(f);
    }
    while (gd->regions) {
        region *r = gd->regions;
        gd->regions = r->next;
        free_region(r);
        free(r);
    }
}
