#pragma once

typedef struct crdb_t {
    void *impl;
} crdb_t;

struct crdb_t *crdb_open(const char *dbname);
void crdb_close(struct crdb_t *crdb);
int crdb_import(struct crdb_t *crdb, const char *crname);

