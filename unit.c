#include "unit.h"

#include "stretchy_buffer.h"

#include <cJSON.h>

#include <stdlib.h>

void free_unit(unit *u) {
    stb_sb_free(u->orders);
    free(u->name);
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
