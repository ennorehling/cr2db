#include "ship.h"

#include <cJSON.h>
#include <stdlib.h>

void ship_free(ship *s) {
    cJSON_Delete(s->data);
    free(s->name);
}

ship *create_ship(ship_id id, struct region *r, char *name, ship_t type)
{
    ship *b = malloc(sizeof(ship));
    b->id = id;
    b->type = type;
    b->name = name;
    b->region = r;
    b->data = NULL;
    return b;
}
