#pragma once

struct sqlite3;

int config_init(struct sqlite3 *db);
int config_done(void);

const char *config_get(const char *key, const char *def);
int config_set(const char *key, const char *value);
int config_delete(const char *key);