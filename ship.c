#include "ship.h"

#include <cJSON.h>

#include <malloc.h>

void free_ship(ship *s) {
    cJSON_Delete(s->data);
    free(s->name);
    free(s);
}
