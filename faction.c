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
    free(f);
}

faction *create_faction(cJSON *data)
{
    faction * f = calloc(1, sizeof(faction));
    if (f) {
        update_faction(f, data);
    }
    return f;
}

void update_faction(faction *f, cJSON *data)
{
    cJSON *child;
    for (child = data->child; child; child = child->next) {
        if (child->type == cJSON_Number) {
            if (f->id == 0 && (child->string, "id") == 0) {
                f->id = child->valueint;
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
    }
    cJSON_Delete(f->data);
    f->data = data;
}
