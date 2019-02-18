#include "crdb.h"
#include "crpat/crpat.h"

#include "cJSON/cJSON.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HAVE_GETTEXT
#define ngettext(msgid1, msgid2, n) ((n==1) ? (msgid1) : (msgid2))
#define gettext(msgid) (msgid)
#endif

typedef enum blocktype_t {
    BLOCK_OBJECT,
    BLOCK_STRINGS,
} blocktype_t;

#define MAXPARENTS 3
static const struct {
    const char *name;
    blocktype_t type;
} blocktypes[] = {
    { "COMMANDS", BLOCK_STRINGS },
    { "SPRUECHE", BLOCK_STRINGS },
    { NULL,  BLOCK_OBJECT }
};

static blocktype_t element_type(const char *name) {
    int i;
    for (i = 0; blocktypes[i].name != NULL; ++i) {
        if (0 == strcmp(name, blocktypes[i].name)) {
            return blocktypes[i].type;
        }
    }
    return BLOCK_OBJECT;
}

#define STACKSIZE 4

typedef struct parser_t {
    CR_Parser parser;
    cJSON *root, *block;
    cJSON *stack[STACKSIZE];
    int sp;
} parser_t;

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

static int block_depth(const char *name) {
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

static void handle_element(void *udata, const char *name, unsigned int keyc, int keyv[]) {
    parser_t *p = (parser_t *)udata;
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
    else if (keyc == 0) {
        cJSON *parent = p->stack[p->sp];
        cJSON *block = cJSON_CreateObject();
        p->block = block;
        if (strcmp("TRANSLATION", name) == 0) {
            parent = p->root;
        }
        if (parent) {
            cJSON_AddItemToObject(parent, name, block);
        }
        else {
            fprintf(stderr, gettext("invalid object hierarchy at %s\n"), name);
        }
    }
    else if (keyc > 0) {
        int depth = block_depth(name);
        bool sequence = is_sequence(name);
        cJSON *parent;
        cJSON *block = cJSON_CreateObject();
        if (depth < 0) {
            // TODO: error handling (i.e. VERSION -> MESSAGE)
            if (p->sp >= 0 && p->sp < STACKSIZE) {
                parent = p->stack[p->sp];
            }
            else {
                fprintf(stderr, gettext("invalid object hierarchy at %s\n"), name);
            }
        }
        else {
            if (depth == 0 || depth > p->sp + 1) {
                fprintf(stderr, gettext("invalid object hierarchy at %s\n"), name);
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
    else {
        fprintf(stderr, gettext("unknown element type %s\n"), name);
        p->block = NULL;
    }
    if (arr) {
        cJSON_AddItemToArray(arr, p->block);
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

void crdb_free(crdb_t *crdb) {
    cJSON_Delete(crdb->json);
}

void crdb_init(struct crdb_t *crdb) { 
    memset(crdb, 0, sizeof(crdb));
}

int crdb_import(crdb_t *crdb, const char *filename) {
    CR_Parser cp;
    int done = 0;
    char buf[2048], *input;
    parser_t state;
    size_t len;
    FILE *in;

    in = fopen(filename, "r");
    if (in == NULL) {
        int err = errno;
        errno = 0;
        fprintf(stderr, gettext("could not open %s: %s\n"),
            filename, strerror(err));
        return err;
    }
    cp = CR_ParserCreate();
    CR_SetElementHandler(cp, handle_element);
    CR_SetPropertyHandler(cp, handle_string);
    CR_SetNumberHandler(cp, handle_number);
    CR_SetTextHandler(cp, handle_text);

    memset(&state, 0, sizeof(state));
    state.parser = cp;
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
            fprintf(stderr, gettext("read error at line %d of %s: %s\n"),
                CR_GetCurrentLineNumber(cp), filename, strerror(err));
            return err;
        }
        done = feof(in);
        if (CR_Parse(cp, input, len, done) == CR_STATUS_ERROR) {
            fprintf(stderr, gettext("parse error at line %d of %s: %s\n"),
                CR_GetCurrentLineNumber(cp), filename,
                CR_ErrorString(CR_GetErrorCode(cp)));
            return -1;
        }
        len = fread(buf, 1, sizeof(buf), in);
        input = buf;
    }
    CR_ParserFree(cp);
    fclose(in);

    crdb->json = state.root;
    return 0;
}

