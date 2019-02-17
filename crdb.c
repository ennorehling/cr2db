#include "crdb.h"
#include "crpat/crpat.h"

#include "cJSON/cJSON.h"

#include <errno.h>
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

#define STACKSIZE 6

typedef struct parser_t {
    CR_Parser parser;
    cJSON *root, *block;
    cJSON *stack[STACKSIZE];
    int sp;
} parser_t;

static void handle_element(void *udata, const char *name, unsigned int keyc, int keyv[]) {
    parser_t *p = (parser_t *)udata;

    if (p->root) {
        fprintf(stderr, "unknown element type %s\n", name);
        p->block = NULL;
    } else {
        if (strcmp("VERSION", name) != 0) {
            fprintf(stderr, "expecting first element to be VERSION, got %s\n", name);
        }
        else {
            cJSON *block;
            int version = (keyc > 0) ? keyv[0] : 0;
            if (version != 66) {
                fprintf(stderr, "unknown version %d\n", version);
            }
            block = cJSON_CreateObject();
            p->root = p->block = block;
        }
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

