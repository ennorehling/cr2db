#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "import.h"

#include "gamedata.h"
#include "faction.h"
#include "region.h"
#include "message.h"
#include "crfile.h"
#include "gettext.h"
#include "log.h"

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
    cJSON *root, *block;
    gamedata *gd;
    int turn;
    struct faction *faction;
    struct region *region;
    struct message **messages;
    int stack_top;
    struct {
        cJSON *object;
        const char*name;
    } stack[STACKSIZE];
} parser_t;

static struct crschema {
    const char *parent, *children[8];
} g_crschema[] = {
    {"PARTEI", {"ALLIANZ", "GEGENSTAENDE", "GRUPPE", "MESSAGE", "OPTIONEN", NULL}},
    {"BATTLE", {"MESSAGE", NULL}},
    {"GRUPPE", {"ALLIANZ", NULL}},
    {"EINHEIT", {"COMMANDS", "EFFECTS", "GEGENSTANDE", "KAMPFZAUBER", "SPRUECHE", "TALENTE", NULL}},
    {"REGION", {"DURCHREISE", "DURCHSCHIFFUNG", "GRENZE", "PREISE", "RESOURCE", NULL}},
    {"BURG", {"EFFECTS", NULL}},
    {"SCHIFF", {"EFFECTS", NULL}},
    {"MESSAGETYPE", {NULL}},
    {"TRANSLATION", {NULL}},
    {NULL, {NULL}},
};

static const struct {
    const char *name;
    enum {
        TYPE_OBJECT,
        TYPE_ARRAY,
        TYPE_SEQUENCE,
    } type;
} block_info[] = {
    { "ALLIANZ", TYPE_SEQUENCE },
    { "GRENZE", TYPE_SEQUENCE },
    { "KAMPFZAUBER", TYPE_SEQUENCE },
    { "RESOURCE", TYPE_SEQUENCE },
    { "DURCHREISE", TYPE_ARRAY },
    { "DURCHSCHIFFUNG", TYPE_ARRAY },
    { "SPRUECHE", TYPE_ARRAY },
    { "COMMANDS", TYPE_ARRAY },
    { "TRANSLATION", TYPE_OBJECT },
    { "PREISE", TYPE_OBJECT },
    { "TALENTE", TYPE_OBJECT },
    { NULL, TYPE_OBJECT },
};

static bool is_sequence(const char *name) {
    int i;
    for (i = 0; block_info[i].name; ++i) {
        if (strcmp(name, block_info[i].name) == 0) {
            return block_info[i].type == TYPE_SEQUENCE;
        }
    }
    return false;
}

static void gd_update(parser_t *p) {
    if (p->root) {
        if (p->faction) {
            faction_update(p->gd, p->faction, p->root);
            faction_add(p->gd, p->faction);
            p->root = NULL;
            p->faction = NULL;
        }
        if (p->region) {
            region_update(p->gd, p->region, p->root);
            region_add(p->gd, p->region);
            p->root = NULL;
            p->region = NULL;
        }
    }
    p->messages = NULL;
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
    p->block = object;
    p->stack[p->stack_top].name = name;
    p->stack[p->stack_top].object = object;
    p->stack_top++;
}

static enum CR_Error block_create(parser_t * p, const char *name, cJSON *parent) {
    int i;
    cJSON *block = NULL, *arr;

    for (i = 0; block_info[i].name; ++i) {
        if (strcmp(name, block_info[i].name) == 0) {
            switch (block_info[i].type) {
            case TYPE_ARRAY:
                block = cJSON_CreateArray();
                break;

            case TYPE_SEQUENCE:
                if (parent == NULL) {
                    return CR_ERROR_GRAMMAR;
                }
                arr = cJSON_GetObjectItem(parent, name);
                if (arr == NULL) {
                    arr = cJSON_CreateArray();
                    cJSON_AddItemToObject(parent, name, arr);
                }
                block = cJSON_CreateObject();
                cJSON_AddItemToArray(arr, block);
                break;

            case TYPE_OBJECT:
                block = cJSON_CreateObject();
                cJSON_AddItemToObject(parent, name, block);
                break;
            }
            break;
        }
    }
    if (block == NULL) {
        /* default: a non-array sub-block */
        block = cJSON_CreateObject();
        if (parent) {
            cJSON_AddItemToObject(parent, name, block);
        }
    }
    stack_push(p, block, name);
    p->block = block;
    return CR_ERROR_NONE;
}

static const char *block_name(const char * name, int keyc, int keyv[]) {
    static char result[32];
    if (keyc == 0) {
        snprintf(result, sizeof(result), "%s", name);
    }
    else if (keyc == 1) {
        snprintf(result, sizeof(result), "%s %d", name, keyv[0]);
    }
    else if (keyc == 2) {
        snprintf(result, sizeof(result), "%s %d %d", name, keyv[0], keyv[1]);
    }
    else if (keyc >= 3) {
        snprintf(result, sizeof(result), "%s %d %d %d", name, keyv[0], keyv[1], keyv[2]);
    }
    return result;
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
    if (keyc == 1) {
        if (strcmp("PARTEI", name) == 0) {
            faction *f = create_faction(NULL);
            f->id = keyv[0];
            gd_update(p);
            p->root = block = cJSON_CreateObject();
            p->faction = f;
            p->stack_top = 0;
            stack_push(p, block, name);
        }
        else if (strcmp("MESSAGE", name) == 0) {
            if (p->messages == NULL) {
                if (p->faction) {
                    p->messages = &p->faction->messages;
                }
                else if (p->region) {
                    p->messages = &p->region->messages;
                }
                else {
                    /* TODO: BATTLE */
                    p->messages = NULL;
                }
            }
            if (p->messages) {
                message * prev = *p->messages;
                message * msg = calloc(1, sizeof(message));
                msg->id = keyv[0];
                if (prev) {
                    prev->next = msg;
                    p->messages = &prev->next;
                }
                *p->messages = msg;
            }
        }
        else {
            cJSON *parent = find_parent(p, name);
            return block_create(p, name, parent);
        }
    }
    else if (keyc > 1) {
        if (strcmp("REGION", name) == 0) {
            region *r = create_region(NULL);
            r->loc.x = keyv[0];
            r->loc.y = keyv[1];
            r->loc.z = (keyc > 2) ? keyv[2] : 0;
            gd_update(p);
            p->root = block = cJSON_CreateObject();
            p->region = r;
            p->stack_top = 0;
            stack_push(p, block, name);
        }
        else if (strcmp("BATTLE", name) == 0) {
            gd_update(p);
            p->messages = NULL;
            p->block = NULL;
        }
        else {
            /* only REGION blocks have more then 1 key */
            return CR_ERROR_GRAMMAR;
        }
    }
    else { /* keyc == 0 */
        cJSON *parent = find_parent(p, name);
        if (!parent) {
            fprintf(stderr, gettext("invalid object crschema at %s\n"), block_name(name, 0, NULL));
            return CR_ERROR_GRAMMAR;
        }
        return block_create(p, name, parent);
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
#if 0
        if (keyc == 0) {
            cJSON_AddItemToObject(parent, name, p->block);
        }
        else if (!is_sequence(name)) {
            /* objects with an id, like MESSAGE (but not GRENZE) */
            cJSON_AddNumberToObject(p->block, "id", keyv[0]);
        }
        else if (depth > p->top + 1) {
            fprintf(stderr, gettext("invalid object crschema at %s\n"), block_name(name, keyc, keyv));
            return CR_ERROR_GRAMMAR;
        }

        /* create json child object and add it to stack */
        if (keyc > 0) {
            if (p->top + 1 < STACKSIZE) {
                if (depth < 0) {
                    ++p->top;
                }
                else {
                    p->top = depth;
                }
            }
            else {
                fprintf(stderr, gettext("object crschema too deep at %s\n"), block_name(name, keyc, keyv));
                return CR_ERROR_GRAMMAR;
            }
        }
        p->parents[p->top] = p->block;
    }
#endif
    return CR_ERROR_NONE;
}

static enum CR_Error handle_element(void *udata, const char *name, unsigned int keyc, int keyv[]) {
    parser_t *p = (parser_t *)udata;
    return handle_object(p, name, keyc, keyv);
}

static void handle_string(void *udata, const char *name, const char *value) {
    parser_t *p = (parser_t *)udata;

    if (p->messages) {
        message *msg = *p->messages;
        if (strcmp("rendered", name) == 0) {
            msg->text = str_strdup(value);
        }
    }
    if (p->block && p->block->type == cJSON_Object) {
        cJSON_AddStringToObject(p->block, name, value);
    }
}

static void handle_number(void *udata, const char *name, long value) {
    parser_t *p = (parser_t *)udata;

    if (p->messages) {
        message *msg = *p->messages;
        if (strcmp("type", name) == 0) {
            msg->type = value;
        }
    }
    else if (p->block) {
        cJSON_AddNumberToObject(p->block, name, (double)value);
    }
    else if (strcmp("Runde", name) == 0) {
        p->turn = value;
    }
}

static void handle_text(void *udata, const char *text) {
    parser_t *p = (parser_t *)udata;

    if (p->block && p->block->type == cJSON_Array) {
        cJSON_AddItemToArray(p->block, cJSON_CreateString(text));
    }
}

static void free_state(parser_t *p) {
    if (p->faction) {
        free_faction(p->faction);
        free(p->faction);
    }
    if (p->region) {
        free_region(p->region);
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
    size_t len;

    cp = CR_ParserCreate();
    CR_SetElementHandler(cp, handle_element);
    CR_SetPropertyHandler(cp, handle_string);
    CR_SetNumberHandler(cp, handle_number);
    CR_SetTextHandler(cp, handle_text);

    memset(&state, 0, sizeof(state));
    state.stack_top = -1;
    state.gd = gd;
    state.parser = cp;
    state.root = NULL;
    CR_SetUserData(cp, (void *)&state);

    input = buf;
    len = fread(buf, 1, sizeof(buf), in);
    if (len >= 3 && buf[0] != 'V') {
        /* skip BOM */
        input += 3;
        len -= 3;
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
        if (CR_Parse(cp, input, len, done) == CR_STATUS_ERROR) {
            log_error(NULL, gettext("parse error at line %d of %s: %s\n"),
                CR_GetCurrentLineNumber(cp), filename,
                CR_ErrorString(CR_GetErrorCode(cp)));
            free_state(&state);
            break;
        }
        len = fread(buf, 1, sizeof(buf), in);
        input = buf;
    }
    CR_ParserFree(cp);

    gd_update(&state);
    if (state.turn > game_get_turn(gd)) {
        game_set_turn(gd, state.turn);
    }
    return 0;
}
