#include "export.h"

#include "jsondata.h"
#include "faction.h"
#include "region.h"

#include <cJSON.h>

#include <assert.h>

static void json_to_cr(cJSON *json, FILE *F)
{
    if (json->type == cJSON_Array) {
        cJSON *child = json->child;
        while (child) {
            if (child->type == cJSON_Object) {
                cJSON *jId = cJSON_GetObjectItem(child, "id");
                if (jId) {
                    fprintf(F, "%s %d\n", json->string, jId->valueint);
                }
                json_to_cr(child, F);
            }
            else if (child->type == cJSON_String) {
                fprintf(F, "\"%s\";%s\n", child->valuestring, child->string);
            }
            child = child->next;
        }
    }
    else if (json->type == cJSON_Object) {
        cJSON *child = json->child;
        while (child) {
            if (child->type == cJSON_Object) {
                fprintf(F, "%s\n", child->string);
            }
            json_to_cr(child, F);
            child = child->next;
        }
    }
    else if (json->type == cJSON_Number) {
        fprintf(F, "%d;%s\n", json->valueint, json->string);
    }
    else if (json->type == cJSON_String) {
        fprintf(F, "\"%s\";%s\n", json->valuestring, json->string);
    }
}

static int cr_faction(faction *f, void *arg)
{
    FILE * F = (FILE *)arg;
    fprintf(F, "PARTEI %d\n", f->id);
    json_to_cr(f->data, F);
    return 0;
}

static int cr_region(region *r, void *arg)
{
    FILE * F = (FILE *)arg;
    if (r->plane == 0) {
        fprintf(F, "REGION %d %d\n", r->x, r->y);
    }
    else {
        fprintf(F, "REGION %d %d %d\n", r->x, r->y, r->plane);
    }
    json_to_cr(r->data, F);
    return 0;
}

int export(struct gamedata *gd, FILE *F)
{
    int err;
    assert(gd);

    fputs("VERSION 66\n", F);
    err = factions_walk(gd, cr_faction, F);
    if (err != 0) return err;
    err = regions_walk(gd, cr_region, F);
    if (err != 0) return err;
    return 0;
}
