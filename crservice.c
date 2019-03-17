#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "crdata.h"
#include "log.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ostream {
    size_t(*write)(void *udata, const char * buffer, size_t count);
    void *udata;
} ostream;

#define NUM_VARS 4

typedef struct request_t {
    char **env;
    char *vars[NUM_VARS];
} request_t;

typedef int(*route_handler)(struct request_t *req, ostream *response);

#define NUM_PARTS (NUM_VARS * 2)
typedef struct route_t {
    char *parts[NUM_PARTS];
    route_handler handler;
} route_t;

#define NUM_ROUTES 16
static route_t routes[NUM_ROUTES];
static unsigned int nroutes;

void route_add(const char *pattern, route_handler handler) {
    const char *tail = pattern;
    route_t *r = routes + nroutes;
    int n = 0;
    assert(nroutes < NUM_ROUTES);
    while (tail && *tail) {
        char *var = NULL, *frag = NULL;
        const char *pos = strchr(tail, ':');
        if (pos != tail) {
            size_t len = pos ? (pos - tail) : strlen(tail);
            frag = malloc(len + 1);
            memcpy(frag, tail, len);
            frag[len] = 0;
            if (pos) tail = pos + 1;
            else tail = NULL;
        }
        if (pos) {
            size_t len;
            pos = strchr(tail, '/');
            /* fragment is followed by a variable: */
            len = pos ? (pos - tail) : strlen(tail);
            var = malloc(len + 1);
            memcpy(var, tail, len);
            var[len] = 0;
            if (pos) tail = pos + 1;
            else tail = NULL;
        }
        assert(n + 2 < NUM_PARTS);
        r->parts[n++] = frag;
        r->parts[n++] = var;
    }
    assert(n + 2 < NUM_PARTS);
    r->parts[n++] = NULL;
    r->parts[n++] = NULL;
    r->handler = handler;
    ++nroutes;
}

const char *req_getenv(const request_t *req, const char *name) {
    int e;

    for (e = 0; req->env[e]; ++e) {
        size_t len = strlen(name);
        if (strncmp(req->env[e], name, len) == 0) {
            if (req->env[e][len] == '=') {
                return req->env[e] + len + 1;
            }
        }
    }
    return NULL;
}

const char * req_getvar(const request_t *req, const char *name) {
    int e;
    size_t len = strlen(name);
    for (e = 0; e != NUM_VARS && req->vars[e]; e += 2) {
        const char *var = req->vars[e];
        if (strncmp(var, name, len) == 0 && var[len] == '=') {
            const char *value = var + len + 1;
            return value;
        }
    }
    return NULL;
}

static int handle_not_found(ostream *stream) {
    const char *headers = "Content-Type: text/plain\nStatus: 404 Not Found\n\n";
    stream->write(stream->udata, headers, strlen(headers));
    return 0;
}

int route_handle(request_t *req, ostream *stream) {
    const char * request_path = req_getenv(req, "REQUEST_URI");
    if (request_path) {
        int i, res;
        route_t *r;

        if (request_path[0] == '/') ++request_path;
        for (i = 0; i != NUM_ROUTES && routes[i].handler; ++i) {
            const char * tail = request_path;
            int p, v = 0;

            r = routes + i;
            for (p = 0; r->parts[p] || r->parts[p + 1]; p += 2) {
                const char * frag = r->parts[p];
                const char * var = r->parts[p + 1];
                if (frag != NULL) {
                    size_t len = strlen(frag);
                    if (strncmp(tail, frag, len) != 0) {
                        /* no match, try next route */
                        break;
                    }
                    tail = tail + len;
                }
                if (var != NULL) {
                    const char * end = strchr(tail, '/');
                    size_t len = end ? (end - tail) : strlen(tail);
                    size_t varlen = strlen(var);
                    char *value = malloc(varlen + len + 2);
                    memcpy(value, var, varlen);
                    value[varlen] = '=';
                    memcpy(value + varlen + 1, tail, len);
                    value[varlen + 1 + len] = '\0';
                    assert(v < NUM_VARS);
                    req->vars[v++] = value;
                    if (end) tail = end + 1;
                    else tail = NULL;
                }
            }
            req->vars[v] = NULL;
            if (tail != NULL) {
                /* only a partial match */
                r = NULL;
            }
        }
        if (r) {
            res = r->handler(req, stream);
        }
        else {
            res = handle_not_found(stream);
        }
        for (i = 0; req->vars[i]; ++i) {
            free(req->vars[i]);
        }
        return res;
    }
    return -1;
}

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

static int handle_request(char **env, ostream *stream) {
    request_t req;
    req.env = env;
    req.vars[0] = NULL;
    return route_handle(&req, stream);
}

static size_t write_stdio(void *udata, const char *buffer, size_t length) {
    FILE * F = (FILE *)udata;
    return fwrite(buffer, 1, length, F);
}

#define ENVMAX 32
static int cgimain(FILE *F) {
    char *env[ENVMAX];
    int r, e = 0;
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
            r = handle_request(env, &stream);
            for (e = 0; env[e]; ++e) {
                free(env[e]);
            }
            e = 0;
            if (r != 0) {
                return r;
            }
        }
        else {
            size_t bytes = strlen(envvar);
            char *dup = malloc(bytes);
            memcpy(dup, envvar, bytes - 1);
            dup[bytes - 1] = '\0';
            env[e++] = dup;
            /* still have room for NULL terminator? */
            assert(e < ENVMAX);
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
        FCGX_PutS("Content-Type: text/plain\n\n", req.out);
        FCGX_FPrintF(req.out, "Hello World, request #%d\n", ++i);
    }
    return 0;
}
#endif

int main(int argc, char ** argv) {
    int err = 0;

    route_add("hello", hello_handler);
    route_add("hello/:name", hello_name_handler);

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
