#include "export.h"

#include "gamedata.h"
#include "gamedb.h"
#include "faction.h"
#include "region.h"
#include "building.h"
#include "ship.h"
#include "unit.h"
#include "message.h"

#include "stb_ds.h"

#include <cJSON.h>

#include <assert.h>

static void json_to_cr(cJSON *json, FILE *F);

static void cr_strings(cJSON *json, FILE *F)
{
    cJSON *child;
    fprintf(F, "%s\n", json->string);
    for (child = json->child; child; child = child->next) {
        fprintf(F, "\"%s\"\n", child->valuestring);
    }
}

#define WRITE_ATTR (1 << 0)
#define WRITE_SIMPLE (1 << 1)
#define WRITE_INDEXED (1 << 2)
#define WRITE_ALL (WRITE_ATTR|WRITE_SIMPLE|WRITE_INDEXED)

static void cr_object(cJSON *json, FILE *F, int mode, const char *name, int id)
{
    cJSON *child, *start = NULL;

    assert(json->type == cJSON_Object || json->type == cJSON_Array);
    if (mode & WRITE_ATTR) {
        bool skills = name && strcmp(name, "TALENTE") == 0;
        assert(json->type == cJSON_Object);
        if (name) {
            if (id > 0) {
                fprintf(F, "%s %d\n", name, id);
            }
            else {
                fprintf(F, "%s\n", name);
            }
        }
        for (child = json->child; child; child = child->next) {
            /* first, print all simple attributes */
            if (child->type != cJSON_Object && child->type != cJSON_Array) {
                if (skills) {
                    /* TALENTE are not strings */
                    fprintf(F, "%s;%s\n", child->valuestring, child->string);
                }
                else {
                    json_to_cr(child, F);
                }
            }
            else if (!start) {
                /* very small optimization */
                start = child;
            }
        }
    }
    if (mode & WRITE_SIMPLE) {
        for (child = start ? start : json->child, start = NULL; child; child = child->next) {
            /* next, all non-indexed child objects */
            if (child->type == cJSON_Object) {
                cJSON *jId = cJSON_GetObjectItem(child, "_index");
                if (jId == NULL) {
                    cr_object(child, F, WRITE_ATTR, child->string, 0);
                }
                else if (!start) {
                    /* very small optimization */
                    start = child;
                }
            }
            else if (!start && child->type == cJSON_Array) {
                /* very small optimization */
                start = child;
            }
        }
    }
    if (mode & WRITE_INDEXED) {
        for (child = start ? start : json->child; child; child = child->next) {
            /* next, all indexed child objects */
            if (child->type == cJSON_Object) {
                cJSON *jId = cJSON_GetObjectItem(child, "_index");
                if (jId) {
                    cr_object(child, F, WRITE_ALL, json->string, jId->valueint);
                }
            }
            else if (child->type == cJSON_Array) {
                json_to_cr(child, F);
            }
        }
    }
}

static void json_to_cr(cJSON *json, FILE *F)
{
    if (json->string && json->string[0] == '_') {
        /* special internal property, do not emit */
        return;
    }
    else if (json->type == cJSON_Array) {
        cJSON *child = json->child;

        assert(child);
        if (child->type == cJSON_Object) {
            int index = 0;
            /* 
             * array contains indexed objects (TYPE_MAP, e.g. EINHEIT)
             * or not (TYPE_SEQUENCE, e.g. GRENZE)
             */
            for (child = json->child; child; child = child->next) {
                cJSON *jId = cJSON_GetObjectItem(child, "_index");
                cr_object(child, F, WRITE_ALL, json->string, jId ? jId->valueint : ++index);
            }
        }
        else {
            /* e.g. SPRUECHE, BEFEHLE, DURCHREISE */
            cr_strings(json, F);
        }
    }
    else if (json->type == cJSON_Object) {
        cr_object(json, F, WRITE_ATTR, json->string, 0);
        cr_object(json, F, WRITE_SIMPLE, json->string, 0);
        cr_object(json, F, WRITE_INDEXED, json->string, 0);
    }
    else if (json->type == cJSON_Number) {
        fprintf(F, "%d;%s\n", json->valueint, json->string);
    }
    else if (json->type == cJSON_String) {
        fprintf(F, "\"%s\";%s\n", json->valuestring, json->string);
    }
}

static void cr_messages(FILE * F, message *list) {
    int i, len = stbds_arrlen(list);
    for (i = 0; i != len; ++i) {
        message *msg = list + i;
        unsigned int a, nattr;
        fprintf(F, "MESSAGE %d\n%d;type\n\"%s\";rendered\n", msg->id, msg->type, msg->text);
        for (a = 0, nattr = stbds_arrlen(msg->args); a != nattr; ++a) {
            struct message_attr *attr = msg->args + a;
            if (attr->type == ATTR_TEXT) {
                fprintf(F, "\"%s\";%s\n", attr->value.text, attr->key);
            }
            else if (attr->type == ATTR_MULTI) {
                fprintf(F, "%s;%s\n", attr->value.text, attr->key);
            }
            else {
                fprintf(F, "%d;%s\n", attr->value.number, attr->key);
            }
        }
    }
}

static void cr_battles(FILE *F, battle *list) {
    int i, len = stbds_arrlen(list);
    for (i = 0; i != len; ++i) {
        battle *b = list + i;
        if (b->loc.z != 0) {
            fprintf(F, "BATTLE %d %d %d\n", b->loc.x, b->loc.y, b->loc.z);
        }
        else {
            fprintf(F, "BATTLE %d %d\n", b->loc.x, b->loc.y);
        }
        cr_messages(F, b->messages);
    }
}

static int cr_orders(FILE *F, const char *orders)
{
    if (orders && *orders) {
        const char *tok = orders;
        fputs("COMMANDS\n", F);
        while (tok && *tok) {
            const char *pos = strchr(tok, '\n');
            if (pos > tok) {
                fputc('"', F);
                // TODO: escaping?
                fwrite(tok, 1, tok - pos, F);
                fputs("\"\n", F);
                tok = pos + 1;
            }
            else {
                fputc('"', F);
                // TODO: escaping?
                fputs(tok, F);
                fputs("\"\n", F);
                break;
            }
        }
    }
    return 0;
}

static int cb_faction(faction *f, void *arg)
{
    FILE * F = (FILE *)arg;

    cr_object(f->data, F, WRITE_ALL, "PARTEI", f->id);
    if (f->messages) {
        cr_messages(F, f->messages);
    }
    if (f->battles) {
        cr_battles(F, f->battles);
    }
    return 0;
}

static int cb_region(region *r, void *arg)
{
    FILE * F = (FILE *)arg;
    if (r->loc.z == 0) {
        fprintf(F, "REGION %d %d\n", r->loc.x, r->loc.y);
    }
    else {
        fprintf(F, "REGION %d %d %d\n", r->loc.x, r->loc.y, r->loc.z);
    }
    if (r->data) {
        cr_object(r->data, F, WRITE_ALL, NULL, 0);
    }
    if (r->messages) {
        cr_messages(F, r->messages);
    }
    if (r->buildings) {
        int i, len = stbds_arrlen(r->buildings);
        for (i = 0; i != len; ++i) {
            building *obj = r->buildings[i];
            cr_object(obj->data, F, WRITE_ALL, "BURG", obj->id);
        }
    }
    if (r->ships) {
        int i, len = stbds_arrlen(r->ships);
        for (i = 0; i != len; ++i) {
            ship *obj = r->ships[i];
            cr_object(obj->data, F, WRITE_ALL, "SCHIFF", obj->id);
        }
    }
    if (r->units) {
        int i, len = stbds_arrlen(r->units);
        for (i = 0; i != len; ++i) {
            unit *obj = r->units[i];
            cr_object(obj->data, F, WRITE_ALL, "EINHEIT", obj->id);
        }
    }
    return 0;
}

static void cr_header(struct gamedata *gd, FILE *F)
{
    assert(gd);
    fprintf(F, "VERSION 66\n36;Basis\n\"UTF-8\";charset\n%d;Runde\n", gd->turn);
}

int export_db(struct gamedata *gd, FILE *F)
{
    int err;
    assert(gd);

    cr_header(gd, F);
    err = db_factions_walk(gd->db, cb_faction, F);
    if (err != 0) return err;
    err = db_regions_walk(gd->db, cb_region, F);
    if (err != 0) return err;
    return 0;
}

int export_gd(struct gamedata *gd, FILE *F)
{
    int err;
    cr_header(gd, F);
    err = factions_walk(gd, cb_faction, F);
    if (err != 0) return err;
    err = regions_walk(gd, cb_region, F);
    if (err != 0) return err;
    return 0;
}

int export_map(struct gamedata *gd, FILE *F)
{
    int err;
    assert(gd);

    err = db_load_map(gd->db, &gd->regions);
    if (err != 0) return err;
    fprintf(F, "VERSION 66\n%d;Runde\n36;Basis\n", gd->turn);
    err = regions_walk(gd, cb_region, F);
    if (err != 0) return err;
    return 0;
}
