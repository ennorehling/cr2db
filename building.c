#include "building.h"

#include <cJSON.h>
#include <stdlib.h>

void building_free(building *b)
{
    cJSON_Delete(b->data);
    free(b->name);
}

building *create_building(building_id id, struct region *r, char *name, building_t type)
{
    building *b = malloc(sizeof(building));
    b->id = id;
    b->type = type;
    b->name = name;
    b->region = r;
    b->data = NULL;
    return b;
}
