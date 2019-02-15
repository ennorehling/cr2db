#include "crdb.h"
#include "crpat/crpat.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HAVE_GETTEXT
#define ngettext(msgid1, msgid2, n) ((n==1) ? (msgid1) : (msgid2))
#define gettext(msgid) (msgid)
#endif

typedef struct parser_t {
    CR_Parser parser;
} parser_t;

static void handle_element(void *udata, const char *name, unsigned int keyc, int keyv[]) {
}

static void handle_string(void *udata, const char *name, const char *value) {
}

static void handle_number(void *udata, const char *name, long value) {
}

static void handle_text(void *udata, const char *text) {
}

crdb_t *crdb_open(const char *filename) {
    return NULL;
}

void crdb_close(crdb_t *crdb) {
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
    return 0;
}

