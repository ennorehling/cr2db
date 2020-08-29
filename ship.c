#include "ship.h"

#include <cJSON.h>
#include <stdlib.h>

void ship_free(ship *s) {
    cJSON_Delete(s->data);
    free(s->name);
}
