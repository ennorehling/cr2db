#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "crdata.h"
#include "log.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int reqno;

typedef struct ostream {
    size_t (*write)(void *udata, const char * buffer, size_t count);
    void *udata;
} ostream;

static int handle_request(char **env, ostream *stream) {
    int e;
    const char *headers = "Content-Type: text/plain\r\n\r\n";
    char hello[64];

    stream->write(stream->udata, headers, strlen(headers));
    snprintf(hello, sizeof(hello), "Hello World, request #%d\n", ++reqno);
    stream->write(stream->udata, hello, strlen(hello));
    for (e = 0; env[e]; ++e) {
        const char *s = env[e];
        stream->write(stream->udata, s, strlen(s));
        stream->write(stream->udata, "\n", 1);
    }
    return 0;
}

static size_t write_stdio(void *udata, const char *buffer, size_t length) {
    FILE * F = (FILE *)udata;
    return fwrite(buffer, 1, length, F);
}

#define ENVMAX 32
static int cgimain(FILE *F) {
    char *env[ENVMAX];
    int e = 0;
    ostream stream;
    stream.write = write_stdio;
    stream.udata = stdout;
    while (!feof(F)) {
        char envvar[1024];
        if (NULL == fgets(envvar, sizeof(envvar), F)) {
            int err = ferror(F);
            if (err) {
                log_error(NULL, "read error: %s", strerror(err));
                return err;
            }
        }
        else if (envvar[1] == 0) {
            /* blank line, request done */
            env[e] = NULL;
            e = handle_request(env, &stream);
            if (e != 0) {
                return e;
            }
        }
        else {
            size_t bytes = strlen(envvar);
            char *dup = malloc(bytes);
            memcpy(dup, envvar, bytes - 1);
            dup[bytes - 1] = '\0';
            env[e++] = dup;
        }
    }
    return 0;
}

#ifdef HAVE_FCGI
#include <fcgiapp.h>

int fcgimain(void) {
    int err, i = 0;
    FCGX_Request req;
    err = FCGX_Init();
    err = FCGX_InitRequest(&req, 0, 0);
    while((err = FCGX_Accept_r(&req)) >= 0) {
        FCGX_PutS("Content-Type: text/plain\r\n\r\n", req.out);
        FCGX_FPrintF(req.out, "Hello World, request #%d\n", ++i);
    }
    return 0;
}
#endif

int main(int argc, char ** argv) {
    int err = 0;
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
