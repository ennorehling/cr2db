#include "unit.h"

#include <cJSON.h>

#include <malloc.h>

void free_unit(unit *u) {
    free(u->name);
    free(u->orders);
    free(u);
}

void update_unit(unit *u, cJSON *data) {
    u->data = data;
}

unit *create_unit(struct cJSON *data) {
    unit * u = calloc(1, sizeof(unit));
    if (u) {
        update_unit(u, data);
    }
    return u;
}
