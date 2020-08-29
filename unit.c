#include "unit.h"

#include "stb_ds.h"

#include <cJSON.h>

#include <stdlib.h>

void unit_free(unit *u) {
    stbds_arrfree(u->orders);
    free(u->name);
}

unit *create_unit(unit_id id, char *name, race_t race, struct cJSON *data)
{
    unit * u = malloc(sizeof(unit));
    u->id = id;
    u->name = name;
    u->race = race;
    u->data = data;

    u->faction = NULL;
    u->region = NULL;
    u->ship = NULL;
    u->building = NULL;
    u->orders = NULL;
    return u;
}
