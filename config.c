#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "config.h"

#include <sqlite3.h>

#include <stddef.h>
#include <stdio.h>

static struct sqlite3 *config_db;
static sqlite3_stmt *g_stmt_update_config;
static sqlite3_stmt *g_stmt_delete_config;
static sqlite3_stmt *g_stmt_select_config;

int config_init(struct sqlite3 *db)
{
    int err;
    if ((err = sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO `config` (`key`, `value`) VALUES (?,?)", -1,
        &g_stmt_update_config, NULL)) != SQLITE_OK) return err;
    if ((err = sqlite3_prepare_v2(db,
        "SELECT `value` FROM `config` WHERE `key` = ?", -1,
        &g_stmt_select_config, NULL)) != SQLITE_OK) return err;
    if ((err = sqlite3_prepare_v2(db,
        "DELETE FROM `config` WHERE `key` = ?", -1,
        &g_stmt_delete_config, NULL)) != SQLITE_OK) return err;

    config_db = db;
    return SQLITE_OK;
}

const char *config_get(const char *key, const char *def)
{
    int err;
    
    err = sqlite3_reset(g_stmt_select_config);
    if (err != 0) return NULL;
    err = sqlite3_bind_text(g_stmt_select_config, 1, key, -1, SQLITE_STATIC);
    if (err != 0) return NULL;

    err = sqlite3_step(g_stmt_select_config);
    if (SQLITE_ROW == err) {
        return (const char *)sqlite3_column_text(g_stmt_select_config, 0);
    }
    else if (SQLITE_DONE == err) {
        return def;
    }
    return NULL;
}

int config_set(const char *key, const char *value)
{
    int err;

    err = sqlite3_reset(g_stmt_update_config);
    if (err != 0) goto config_set_fail;
    err = sqlite3_bind_text(g_stmt_update_config, 1, key, -1, SQLITE_STATIC);
    if (err != 0) goto config_set_fail;
    if (value != NULL) {
        err = sqlite3_bind_text(g_stmt_update_config, 2, value, -1, SQLITE_STATIC);
    }
    else {
        err = sqlite3_bind_null(g_stmt_update_config, 2);
    }
    if (err != 0) goto config_set_fail;
    err = sqlite3_step(g_stmt_update_config);
    if (err != 0) goto config_set_fail;

    return SQLITE_OK;

config_set_fail:
    fputs(sqlite3_errmsg(config_db), stderr);
    return err;
}

int config_delete(const char *key) {
    int err;

    err = sqlite3_reset(g_stmt_delete_config);
    if (err != 0) goto config_delete_fail;
    err = sqlite3_bind_text(g_stmt_delete_config, 1, key, -1, SQLITE_STATIC);
    if (err != 0) goto config_delete_fail;
    err = sqlite3_step(g_stmt_delete_config);
    if (err != 0) goto config_delete_fail;

    return SQLITE_OK;

config_delete_fail:
    fputs(sqlite3_errmsg(config_db), stderr);
    return err;
}

int config_done(void)
{
    int err;
    if ((err = sqlite3_finalize(g_stmt_delete_config)) != SQLITE_OK) return err;
    g_stmt_delete_config = NULL;
    if ((err = sqlite3_finalize(g_stmt_select_config)) != SQLITE_OK) return err;
    g_stmt_select_config = NULL;
    if ((err = sqlite3_finalize(g_stmt_update_config)) != SQLITE_OK) return err;
    g_stmt_update_config = NULL;
    config_db = NULL;
    return SQLITE_OK;
}
