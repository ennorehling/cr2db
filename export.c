#include "export.h"

#include "gamedata.h"
#include "gamedb.h"
#include "faction.h"
#include "region.h"
#include "message.h"

#include "stb_ds.h"
#include "stretchy_buffer.h"

#include <cJSON.h>

#include <assert.h>

static void json_to_cr(cJSON *json, FILE *F)
{
    if (json->type == cJSON_Array) {
        cJSON *child = json->child;
        int index = 0;
        while (child) {
            if (child->type == cJSON_Object) {
                cJSON *jId = cJSON_GetObjectItem(child, "id");
                if (jId) {
                    fprintf(F, "%s %d\n", json->string, jId->valueint);
                }
                else {
                    fprintf(F, "%s %d\n", json->string, ++index);
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

static void cr_messages(FILE * F, struct message *messages) {
    int i, len = stb_sb_count(messages);
    for (i = 0; i != len; ++i) {
        message *msg = messages + i;
        int a, nattr = stbds_shlen(msg->attr);
        fprintf(F, "MESSAGE %d\n%d;type\n\"%s\";rendered", msg->id, msg->type, msg->text);
        for (a = 0; a != nattr; ++a) {
            struct message_attr *attr = msg->attr + a;
            if (attr->value.valuestring) {
                if (0 == strcmp("region", attr->key)) {
                    /* the stupid way to reference a region */
                    fprintf(F, "%s;%s\n", attr->value.valuestring, attr->key);
                }
                else {
                    fprintf(F, "\"%s\";%s\n", attr->value.valuestring, attr->key);
                }
            }
            else {
                fprintf(F, "%d;%s\n", attr->value.valueint, attr->key);
            }
        }
    }
}

static int cr_faction(faction *f, void *arg)
{
    FILE * F = (FILE *)arg;
    fprintf(F, "PARTEI %u\n", f->id);
    if (f->data) {
        json_to_cr(f->data, F);
    }
    if (f->messages) {
        cr_messages(F, f->messages);
    }
    return 0;
}

static int cr_region(region *r, void *arg)
{
    FILE * F = (FILE *)arg;
    if (r->loc.z == 0) {
        fprintf(F, "REGION %d %d\n", r->loc.x, r->loc.y);
    }
    else {
        fprintf(F, "REGION %d %d %d\n", r->loc.x, r->loc.y, r->loc.z);
    }
    if (r->data) {
        json_to_cr(r->data, F);
    }
    if (r->messages) {
        cr_messages(F, r->messages);
    }
    return 0;
}

int export_db(struct gamedata *gd, FILE *F)
{
    int err;
    assert(gd);

    fprintf(F, "VERSION 66\n\"UTF-8\";charset\n%d;Runde\n36;Basis\n", game_get_turn(gd));
    err = db_factions_walk(gd->db, cr_faction, F);
    if (err != 0) return err;
    err = db_regions_walk(gd->db, cr_region, F);
    if (err != 0) return err;
    return 0;
}

int export_map(struct gamedata *gd, FILE *F)
{
    int err;
    assert(gd);

    err = db_load_map(gd->db, &gd->regions);
    if (err != 0) return err;
    fprintf(F, "VERSION 66\n%d;Runde\n36;Basis\n", game_get_turn(gd));
    err = regions_walk(gd, cr_region, F);
    if (err != 0) return err;
    return 0;
}
