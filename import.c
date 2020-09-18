#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "import.h"

#include "gamedata.h"
#include "faction.h"
#include "unit.h"
#include "ship.h"
#include "building.h"
#include "ship.h"
#include "region.h"
#include "message.h"
#include "crfile.h"
#include "gettext.h"
#include "log.h"

#include "stb_ds.h"

#include <crpat.h>
#include <cJSON.h>
#include <strings.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACKSIZE 4

typedef struct parser_t {
    CR_Parser parser;
    cJSON *root, *current;
    gamedata *gd;
    int turn;
    struct faction *faction;
    struct region *region;
    struct battle *battle;
    struct unit *unit;
    struct ship *ship;
    struct building *building;
    char **text; /* stbds_arr */
    struct message **messages;
    int stack_top;
    struct {
        cJSON *object;
        const char *name;
    } stack[STACKSIZE];
    char **block_names; /* stbds_arr */
} parser_t;

static struct crschema {
    const char *parent, *children[10];
} g_crschema[] = {
    {"PARTEI", {"ALLIANZ", "GEGENSTAENDE", "GRUPPE", "MESSAGE", "OPTIONEN", NULL}},
    {"BATTLE", {"MESSAGE", NULL}},
    {"GRUPPE", {"ALLIANZ", NULL}},
    {"EINHEIT", {"COMMANDS", "EFFECTS", "GEGENSTAENDE", "KAMPFZAUBER", "SPRUECHE", "TALENTE", NULL}},
    {"REGION", {"EINHEIT", "BURG", "SCHIFF", "DURCHREISE", "DURCHSCHIFFUNG", "GRENZE", "PREISE", "RESOURCE", "EFFECTS", NULL}},
    {"BURG", {"EFFECTS", NULL}},
    {"SCHIFF", {"EFFECTS", NULL}},
    {"MESSAGETYPE", {NULL}},
    {"TRANSLATION", {NULL}},
    {NULL, {NULL}},
};

static const struct {
    const char *name;
    enum block_type {
        TYPE_OBJECT,
        TYPE_STRINGS,
        TYPE_SEQUENCE,
        TYPE_MAP,
    } type;
} block_info[] = {
    { "GRENZE", TYPE_SEQUENCE },
    { "KAMPFZAUBER", TYPE_SEQUENCE },
    { "DURCHREISE", TYPE_STRINGS },
    { "DURCHSCHIFFUNG", TYPE_STRINGS },
    { "EFFECTS", TYPE_STRINGS },
    { "SPRUECHE", TYPE_STRINGS },
    { "COMMANDS", TYPE_STRINGS },
    { NULL, TYPE_OBJECT },
};

static enum block_type block_type(parser_t *p, const char *name, int key, const char **block_name) {
    unsigned int i, len;
    const char *result = NULL;
    for (i = 0; block_info[i].name; ++i) {
        if (strcmp(name, block_info[i].name) == 0) {
            *block_name = block_info[i].name;
            return block_info[i].type;
        }
    }
    for (i = 0, len = stbds_arrlen(p->block_names); i != len; ++i) {
        const char *str = p->block_names[i];
        if (strcmp(str, name) == 0) {
            result = str;
            break;
        }
    }
    if (result == NULL) {
        char *str = str_strdup(name);
        stbds_arrpush(p->block_names, str);
        result = str;
    }
    *block_name = result;
    return (key<0) ? TYPE_OBJECT : TYPE_MAP;
}

static void gd_update(parser_t *p) {
    if (p->root) {
        if (p->faction) {
            gd_update_faction(p->gd, p->faction, p->root);
            gd_add_faction(p->gd, p->faction);
            p->root = NULL;
        }
        else if (p->building) {
            p->building->region = p->region;
            gd_update_building(p->gd, p->building, p->root);
            gd_add_building(p->gd, p->building);
            p->root = NULL;
        }
        else if (p->ship) {
            p->ship->region = p->region;
            gd_update_ship(p->gd, p->ship, p->root);
            gd_add_ship(p->gd, p->ship);
            p->root = NULL;
        }
        else if (p->region) {
            gd_update_region(p->gd, p->region, p->root);
            gd_add_region(p->gd, p->region);
            p->root = NULL;
        }
    }
    /* preserve current region and faction, but these objects have no sub-objects: */
    p->messages = NULL;
    p->unit = NULL;
    p->building = NULL;
    p->ship = NULL;
}

static bool is_child(parser_t *p, const char *name) {
    int i;
    if (p->stack_top > 0) {
        const char *top_name = p->stack[p->stack_top - 1].name;
        for (i = 0; g_crschema[i].parent; ++i) {
            if (strcmp(top_name, g_crschema[i].parent) == 0) {
                int c;
                for (c = 0; g_crschema[i].children[c]; ++c) {
                    if (strcmp(name, g_crschema[i].children[c]) == 0) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

static cJSON *stack_pop(parser_t *p) {
    cJSON *result = NULL;
    if (p->stack_top > 0) {
        result = p->stack[--p->stack_top].object;
        p->stack[p->stack_top].object = NULL;
        p->stack[p->stack_top].name = NULL;
    }
    return result;
}

static void stack_push(parser_t *p, cJSON *object, const char *name) {
    p->current = object;
    p->stack[p->stack_top].name = name;
    p->stack[p->stack_top].object = object;
    p->stack_top++;
}

static enum CR_Error block_create(parser_t * p, const char *name, int key, cJSON *parent) {
    cJSON *block = NULL, *arr;
    const char *block_name = NULL;
    enum block_type type = block_type(p, name, key, &block_name);

    switch (type) {
    case TYPE_STRINGS:
        block = cJSON_CreateArray();
        cJSON_AddItemToObject(parent, name, block);
        break;

    case TYPE_MAP:
        if (parent != NULL) {
            arr = cJSON_GetObjectItem(parent, name);
            if (arr == NULL) {
                arr = cJSON_CreateArray();
                cJSON_AddItemToObject(parent, name, arr);
            }
            block = cJSON_CreateObject();
            cJSON_AddNumberToObject(block, "_index", key);
            cJSON_AddItemToArray(arr, block);
        }
        break;

    case TYPE_SEQUENCE:
        if (parent != NULL) {
            arr = cJSON_GetObjectItem(parent, name);
            if (arr == NULL) {
                arr = cJSON_CreateArray();
                cJSON_AddItemToObject(parent, name, arr);
            }
            block = cJSON_CreateObject();
            cJSON_AddItemToArray(arr, block);
        }
        break;

    case TYPE_OBJECT:
        block = cJSON_CreateObject();
        if (parent) {
            cJSON_AddItemToObject(parent, name, block);
        }
        break;
    }
    stack_push(p, block, block_name);
    p->current = block;
    return CR_ERROR_NONE;
}

static cJSON *find_parent(parser_t *p, const char *name) {
    do {
        if (is_child(p, name)) {
            return p->stack[p->stack_top - 1].object;
        }
    } while (stack_pop(p) != NULL);
    return NULL;
}

static enum CR_Error handle_block(parser_t *p, const char * name, int keyc, int keyv[]) {
    cJSON * block = NULL;

    gd_update(p);
    if (keyc == 1) {
        static const char *name_faction = "PARTEI";
        static const char *name_building = "BURG";
        static const char *name_ship = "SCHIFF";
        if (strcmp(name_faction, name) == 0) {
            faction *f = calloc(1, sizeof(faction));
            f->id = keyv[0];
            p->faction = f;
            p->battle = NULL;
            p->root = block = cJSON_CreateObject();
            /* pop the entire stack, and start over: */
            p->stack_top = 0;
            stack_push(p, block, name_faction);
        }
        else if (strcmp(name_building, name) == 0) {
            building *b = calloc(1, sizeof(building));
            b->id = keyv[0];
            b->region = p->region;
            p->building = b;
            p->root = block = cJSON_CreateObject();
            p->stack_top = 1; /* only REGOIN is below this on the stack */
            stack_push(p, block, name_building);
        }
        else if (strcmp(name_ship, name) == 0) {
            ship *b = calloc(1, sizeof(ship));
            b->id = keyv[0];
            b->region = p->region;
            p->ship = b;
            p->root = block = cJSON_CreateObject();
            p->stack_top = 1; /* only REGOIN is below this on the stack */
            stack_push(p, block, name_ship);
        }
        else if (!p->battle && strcmp("MESSAGE", name) == 0) {
            if (p->faction) {
                p->messages = &p->faction->messages;
            }
            else if (p->region) {
                p->messages = &p->region->messages;
            }
            if (p->messages) {
                struct message *msg = stbds_arraddnptr(*p->messages, 1);
                msg->id = keyv[0];
                msg->text = NULL;
                msg->type = 0;
                msg->args = NULL;
            }
            else {
                return CR_ERROR_GRAMMAR;
            }
        }
        else {
            cJSON *parent = find_parent(p, name);
            p->messages = NULL;
            return block_create(p, name, keyv[0], parent);
        }
    }
    else if (keyc > 1) {
        static const char *name_region = "REGION";
        if (strcmp(name_region, name) == 0) {
            region *r = create_region(0, keyv[0], keyv[1], (keyc > 2) ? keyv[2] : 0, NULL, 0);
            r->loc.x = keyv[0];
            r->loc.y = keyv[1];
            r->loc.z = (keyc > 2) ? keyv[2] : 0;
            p->region = r;
            p->faction = NULL;
            p->text = NULL;
            p->battle = NULL;
            p->root = block = cJSON_CreateObject();
            p->stack_top = 0;
            stack_push(p, block, name_region);
        }
        else if (strcmp("BATTLE", name) == 0) {
            battle *b = stbds_arraddnptr(p->faction->battles, 1);
            p->battle = b;
            p->text = &b->report;
            b->report = NULL;
            b->loc.x = keyv[0];
            b->loc.y = keyv[1];
            b->loc.z = (keyc > 2) ? keyv[2] : 0;
            p->root = NULL;
            p->current = NULL;
            p->stack_top = 1; /* only FACTION is below BATTLE on the stack */
        }
        else {
            /* only REGION and BATTLE blocks can have more then 1 key */
            return CR_ERROR_GRAMMAR;
        }
    }
    else { /* keyc == 0 */
        cJSON *parent = find_parent(p, name);
        if (!parent) {
            return CR_ERROR_GRAMMAR;
        }
        return block_create(p, name, -1, parent);
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_object(parser_t *p, const char *name, unsigned int keyc, int keyv[]) {
    if (name[0] == 'V' && 0 == strcmp("VERSION", name)) {
        int version = (keyc > 0) ? keyv[0] : 0;
        if (p->stack_top >= 0) {
            fprintf(stderr, gettext("expecting first element to be VERSION, got %s\n"), name);
            return CR_ERROR_GRAMMAR;
        }
        if (version < 66) {
            fprintf(stderr, gettext("unknown version %d\n"), version);
        }
        p->stack_top = 0;
    }
    else {
        if (p->stack_top < 0) {
            fprintf(stderr, gettext("expecting first element to be VERSION, got %s\n"), name);
            return CR_ERROR_GRAMMAR;
        }
        return handle_block(p, name, keyc, keyv);
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_element(void *udata, const char *name, unsigned int keyc, int keyv[]) {
    parser_t *p = (parser_t *)udata;
    return handle_object(p, name, keyc, keyv);
}

static void append_text(char **ptext, const char *value) {
    char *text = *ptext;
    size_t len = strlen(value);
    char *tail;
    if (text != NULL) {
        /* replace nul-terminator with newline */
        stbds_arrlast(text) = '\n';
    }
    tail = stbds_arraddnptr(text, (int)len + 1);
    memcpy(tail, value, len + 1);
    *ptext = text;
}

static enum CR_Error handle_string(void *udata, const char *name, const char *value) {
    parser_t *p = (parser_t *)udata;

    if (p->messages) {
        message *msg = &stbds_arrlast(*p->messages);

        if (strcmp("rendered", name) == 0) {
            msg->text = str_strdup(value);
        }
        else if (strcmp("type", name) == 0) {
            msg->type = atoi(value);
        }
        else {
            struct message_attr *a = stbds_arraddnptr(msg->args, 1);
            create_attribute(a, name, ATTR_TEXT, value, 0);
        }
    }
    else if (p->text) {
        if (strcmp("rendered", name) == 0) {
            append_text(p->text, value);
        }
    }
    else if (p->current && p->current->type == cJSON_Object) {
        cJSON_AddStringToObject(p->current, name, value);
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_multi(void *udata, const char *name, const char *value) {
    parser_t *p = (parser_t *)udata;

    if (p->messages) {
        message *msg = &stbds_arrlast(*p->messages);

        if (strcmp("rendered", name) == 0) {
            msg->text = str_strdup(value);
        }
        else if (strcmp("type", name) == 0) {
            msg->type = atoi(value);
        }
        else {
            struct message_attr *a = stbds_arraddnptr(msg->args, 1);
            create_attribute(a, name, ATTR_MULTI, value, 0);
        }
    }
    else if (p->current && p->current->type == cJSON_Object) {
        cJSON_AddStringToObject(p->current, name, value);
    }
    else {
        return CR_ERROR_GRAMMAR;
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_number(void *udata, const char *name, long value) {
    parser_t *p = (parser_t *)udata;

    if (p->messages) {
        message *msg = &stbds_arrlast(*p->messages);

        if (strcmp("type", name) == 0) {
            msg->type = (int) value;
        }
        else {
            struct message_attr *a = stbds_arraddnptr(msg->args, 1);
            create_attribute(a, name, ATTR_NUMBER, NULL, value);
        }
    }
    else {
        if (p->current) {
            cJSON_AddNumberToObject(p->current, name, (double)value);
        }
        else if (strcmp("Runde", name) == 0) {
            p->turn = value;
        }
    }
    return CR_ERROR_NONE;
}

static enum CR_Error handle_text(void *udata, const char *text) {
    parser_t *p = (parser_t *)udata;

    if (p->text) {
        append_text(p->text, text);
        return CR_ERROR_NONE;
    }
    if (p->current && p->current->type == cJSON_Array) {
        cJSON_AddItemToArray(p->current, cJSON_CreateString(text));
        return CR_ERROR_NONE;
    }
    return CR_ERROR_GRAMMAR;
}

static void free_state(parser_t *p) {
    if (p->faction) {
        faction_free(p->faction);
        free(p->faction);
    }
    if (p->region) {
        region_free(p->region);
        free(p->region);
    }
    cJSON_Delete(p->root);
    p->root = NULL;
}

int import(gamedata *gd, FILE *in, const char *filename)
{
    CR_Parser cp;
    int done = 0;
    char buf[2048], *input;
    parser_t state;
    size_t bytes;
    unsigned int i, len, err = CR_STATUS_OK;

    cp = CR_ParserCreate();
    CR_SetElementHandler(cp, handle_element);
    CR_SetPropertyHandler(cp, handle_string);
    CR_SetNumberHandler(cp, handle_number);
    CR_SetLocationHandler(cp, handle_multi);
    CR_SetTextHandler(cp, handle_text);

    memset(&state, 0, sizeof(state));
    state.stack_top = -1;
    state.gd = gd;
    state.parser = cp;
    state.root = NULL;
    state.block_names = NULL;
    CR_SetUserData(cp, (void *)&state);

    input = buf;
    bytes = fread(buf, 1, sizeof(buf), in);
    if (bytes >= 3 && buf[0] != 'V') {
        /* skip BOM */
        input += 3;
        bytes -= 3;
    }

    while (!done) {
        if (ferror(in)) {
            int err = errno;
            errno = 0;
            log_error(NULL, gettext("read error at line %d of %s: %s\n"),
                CR_GetCurrentLineNumber(cp), filename, strerror(err));
            free_state(&state);
            break;
        }
        done = feof(in);
        if ((err = CR_Parse(cp, input, bytes, done)) == CR_STATUS_ERROR) {
            log_error(NULL, gettext("parse error at line %d of %s: %s\n"),
                CR_GetCurrentLineNumber(cp), filename,
                CR_ErrorString(CR_GetErrorCode(cp)));
            free_state(&state);
            break;
        }
        bytes = fread(buf, 1, sizeof(buf), in);
        input = buf;
    }
    CR_ParserFree(cp);

    gd_update(&state);

    /* FIXME: would this not indicate an error? */
    assert(state.root == NULL);
    if (state.root) {
        cJSON_Delete(state.root);
    }
    for (i = 0, len = stbds_arrlen(state.block_names); i != len; ++i) {
        free(state.block_names[i]);
    }
    stbds_arrfree(state.block_names);
    if (state.turn > gd->turn) {
        gd->turn = state.turn;
    }
    return err;
}
