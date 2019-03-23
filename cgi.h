#pragma once

#include <stdio.h>

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

void route_get(const char *pattern, route_handler handler);

const char *req_getenv(const request_t *req, const char *name);
const char * req_getvar(const request_t *req, const char *name);

int cgimain(FILE *F);
#ifdef HAVE_FCGI
int fcgimain(void);
#endif
