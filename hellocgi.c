#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "crdata.h"
#include "cgi.h"
#include "log.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int hello_name_handler(request_t *req, ostream *stream) {
    const char *headers = "Content-Type: text/plain\nStatus: 200 OK\n\n";
    const char *name;

    stream->write(stream->udata, headers, strlen(headers));
    name = req_getvar(req, "name");
    if (name) {
        char hello[64];
        snprintf(hello, sizeof(hello), "Hello %s\n", name);
        stream->write(stream->udata, hello, strlen(hello));
    }
    return 0;
}

static int hello_handler(request_t *req, ostream *stream) {
    static int reqno;
    int e;
    const char *headers = "Content-Type: text/plain\nStatus: 200 OK\n\n";
    char hello[64];

    stream->write(stream->udata, headers, strlen(headers));
    snprintf(hello, sizeof(hello), "Hello World, request #%d\n", ++reqno);
    stream->write(stream->udata, hello, strlen(hello));
    for (e = 0; req->env[e]; ++e) {
        const char *s = req->env[e];
        stream->write(stream->udata, s, strlen(s));
        stream->write(stream->udata, "\n", 1);
    }
    return 0;
}

int main(int argc, char ** argv) {
    int err = 0;

    route_get("hello", hello_handler);
    route_get("hello/:name", hello_name_handler);

    fprintf(stderr, "crservice started.\n");
    if (argc > 1) {
        FILE * F = fopen(argv[1], "rt");
        if (F) {
            err = cgimain(F);
            fclose(F);
        }
        else {
            log_error(NULL, "open %s: %s", argv[1], strerror(errno));
        }
    }
    else {
#ifdef HAVE_FCGI
        err = fcgimain();
#else
        err = cgimain(stdin);
#endif
    }
    fprintf(stderr, "crservice done.\n");
    return err;
}
