#pragma once

struct cJSON;

typedef struct crdb_t {
    struct cJSON *json;
} crdb_t;

void crdb_init(struct crdb_t *crdb);
void crdb_free(struct crdb_t *crdb);
int crdb_import(struct crdb_t *crdb, const char *crname);

