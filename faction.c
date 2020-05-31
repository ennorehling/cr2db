#include "faction.h"

#include <strings.h>
#include <cJSON.h>

#include <malloc.h>
#include <string.h>

void free_faction(faction *f)
{
    cJSON_Delete(f->data);
    free(f->name);
    free(f->email);
}

faction *create_faction(cJSON *data)
{
    faction * f = calloc(1, sizeof(faction));
    f->data = data;
    return f;
}
