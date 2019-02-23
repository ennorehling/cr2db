#include "crdb.h"
#include "gettext.h"
#include "crpat/crpat.h"

#include "cJSON/cJSON.h"

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
    cJSON *stack[STACKSIZE];
    int sp;
} parser_t;

static const struct {
    const char *name;
    int depth;
    enum {
        TYPE_OBJECT,
        TYPE_ARRAY
    } type;
} block_info[] = {
    { "TRANSLATION", 1, TYPE_OBJECT },
    { "DURCHREISE", 2, TYPE_ARRAY },
    { "DURCHSCHIFFUNG", 2, TYPE_ARRAY },
    { "RESOURCE", 2, TYPE_OBJECT },
    { "PREISE", 2, TYPE_OBJECT },
    { "TALENTE", 3, TYPE_OBJECT },
    { "SPRUECHE", 3, TYPE_ARRAY },
    { "COMMANDS", 3, TYPE_ARRAY },
    { NULL, 0,  TYPE_OBJECT },
};

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

static cJSON *block_create(parser_t * p, const char *name) {
    cJSON *block = NULL, *parent = p->stack[p->sp];
    int i;

    for (i = 0; block_info[i].name; ++i) {
        if (strcmp(name, block_info[i].name) == 0) {
            int depth = block_info[i].depth;
            if (block_info[i].type == TYPE_ARRAY) {
                block = cJSON_CreateArray();
            } else {
                block = cJSON_CreateObject();
            }
            if (depth > p->sp + 1) {
                log_error(NULL, gettext("invalid object hierarchy at %s\n"), block_name(name, 0, NULL));
                parent = NULL;
            }
            else {
                // Adjust the stack so our parent is on top:
                p->sp = depth - 1;
                parent = p->stack[p->sp];
            }
            break;
        }
    }
    if (block == NULL) {
        block = cJSON_CreateObject();
    }
    p->block = block;
    return parent;
}

static void handle_block(parser_t *p, const char * name) {
    cJSON *parent = block_create(p, name);

    if (parent) {
        cJSON_AddItemToObject(parent, name, p->block);
    }
    else {
        fprintf(stderr, gettext("invalid object hierarchy at %s\n"), block_name(name, 0, NULL));
    }
}

static const char * seq_blocks[] = {
    "KAMPFZAUBER", "GRENZE", NULL
};

static bool is_sequence(const char *name) {
    int i;
    for (i = 0; seq_blocks[i]; ++i) {
        if (strcmp(name, seq_blocks[i]) == 0) {
            return true;
        }
    }
    return false;
}

static const char * top_blocks[] = {
    "PARTEI", "BATTLE", "REGION", "MESSAGETYPE", "TRANSLATION", NULL
};

static const char * sub_blocks[] = {
    "MESSAGE", "GRUPPE", "SCHIFF", "BURG", "EINHEIT", "GRENZE", NULL
};

static int object_depth(const char *name) {
    int i;
    for (i = 0; top_blocks[i]; ++i) {
        if (strcmp(name, top_blocks[i]) == 0) {
            return 1;
        }
    }
    for (i = 0; sub_blocks[i]; ++i) {
        if (strcmp(name, sub_blocks[i]) == 0) {
            return 2;
        }
    }
    return -1;
}

static void handle_object(parser_t *p, const char *name, unsigned int keyc, int keyv[]) {
    cJSON *arr = NULL;
    if (p->root == NULL) {
        if (strcmp("VERSION", name) == 0) {
            cJSON *block;
            int version = (keyc > 0) ? keyv[0] : 0;
            if (version != 66) {
                fprintf(stderr, gettext("unknown version %d\n"), version);
            }
            block = cJSON_CreateObject();
            p->stack[0] = p->root = p->block = block;
            p->sp = 0;
        }
        else {
            fprintf(stderr, gettext("expecting first element to be VERSION, got %s\n"), name);
        }
    }
    else {
        int depth = object_depth(name);
        bool sequence = is_sequence(name);
        cJSON *parent;
        cJSON *block = cJSON_CreateObject();
        if (depth < 0) {
            // TODO: error handling (i.e. VERSION -> MESSAGE)
            if (p->sp >= 0 && p->sp < STACKSIZE) {
                parent = p->stack[p->sp];
            }
            else {
                fprintf(stderr, gettext("invalid object hierarchy at %s\n"), block_name(name, keyc, keyv));
            }
        }
        else {
            if (depth == 0 || depth > p->sp + 1) {
                fprintf(stderr, gettext("invalid object hierarchy at %s\n"), block_name(name, keyc, keyv));
            }
            else {
                parent = p->stack[depth - 1];
                // Make this object top of stack, it may have sub-blocks:
                p->sp = depth;
                p->stack[p->sp] = block;
            }
        }
        p->block = block;
        if (keyc == 1) {
            if (!sequence) {
                cJSON_AddNumberToObject(block, "id", keyv[0]);
            }
        } else if (keyc >= 2) {
            cJSON_AddNumberToObject(block, "x", keyv[0]);
            cJSON_AddNumberToObject(block, "y", keyv[1]);
            if (keyc > 2) {
                cJSON_AddNumberToObject(block, "z", keyv[2]);
            }
        }
        arr = cJSON_GetObjectItem(parent, name);
        if (!arr) {
            arr = cJSON_CreateArray();
            cJSON_AddItemToObject(parent, name, arr);
        }
    }
    if (arr) {
        cJSON_AddItemToArray(arr, p->block);
    }
}

static void handle_element(void *udata, const char *name, unsigned int keyc, int keyv[]) {
    parser_t *p = (parser_t *)udata;
    if (keyc == 0) {
        handle_block(p, name);
    }
    else {
        handle_object(p, name, keyc, keyv);
    }
}

static void handle_string(void *udata, const char *name, const char *value) {
    parser_t *p = (parser_t *)udata;

    if (p->block && p->block->type == cJSON_Object) {
        cJSON_AddStringToObject(p->block, name, value);
    }
}

static void handle_number(void *udata, const char *name, long value) {
    parser_t *p = (parser_t *)udata;

    if (p->block && p->block->type == cJSON_Object) {
        cJSON_AddNumberToObject(p->block, name, (double)value);
    }
}

static void handle_text(void *udata, const char *text) {
    parser_t *p = (parser_t *)udata;

    if (p->block && p->block->type == cJSON_Array) {
        cJSON_AddItemToArray(p->block, cJSON_CreateString(text));
    }
}

cJSON *crfile_read(FILE *in, const char *filename) {
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
            cJSON_Delete(state.root);
            state.root = NULL;
            break;
        }
        done = feof(in);
        if (CR_Parse(cp, input, len, done) == CR_STATUS_ERROR) {
            log_error(NULL, gettext("parse error at line %d of %s: %s\n"),
                CR_GetCurrentLineNumber(cp), filename,
                CR_ErrorString(CR_GetErrorCode(cp)));
            cJSON_Delete(state.root);
            state.root = NULL;
            break;
        }
        len = fread(buf, 1, sizeof(buf), in);
        input = buf;
    }
    CR_ParserFree(cp);

    return state.root;
}

