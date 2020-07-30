#include "building.h"

#include <cJSON.h>
#include <stdlib.h>

void free_building(building *b)
{
    cJSON_Delete(b->data);
    free(b->name);
    free(b);
}
