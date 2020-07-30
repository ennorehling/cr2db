#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "config.h"
#include "gamedata.h"
#include "gamedb.h"
#include "region.h"
#include "faction.h"

#include "export.h"
#include "import.h"

#include <cJSON.h>
#include <sqlite3.h>
#include <crpat.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SCHEMA_VERSION 1

static sqlite3 * g_db;
static const char *g_dbname = ":memory:";
const char *g_program = "atl-cli";
static bool g_modified = false;

int eval_command(struct gamedata *gd, int argc, char **argv);

static int usage(void) {
    fprintf(stderr, "usage: %s [options] [command] [arguments]\n", g_program);
    fputs("\nOPTIONS\n", stderr);
    fputs("  -d dbname  game database name.\n", stderr);
    fputs("\nCOMMANDS\n", stderr);
    fputs("  script [filename]\n\texecute commands from a script.\n", stderr);
    fputs("  import-cr [filename]\n\timport CR from a file.\n", stderr);
    fputs("  export-cr [filename]\n\texport game data to a CR file.\n", stderr);
    fputs("  export-map [filename]\n\texport map data only to a CR file.\n", stderr);
    fputs("  template  [filename]\n\tcreate an order template.\n", stderr);
    return -1;
}

static int parseargs(int argc, char **argv) {
    int i;
    for (i = 1; i != argc; ++i) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'd':
                g_dbname = (argv[i][2]) ? argv[i] + 2 : argv[++i];
                break;
            case 'h':
                return usage();
            default:
                return i;
            }
        }
        else {
            break;
        }
    }
    return i;
}

#define MAXARGS 10

static int eval_script(struct gamedata *gd, int argc, char **argv) {
    if (argc > 0) {
        const char * filename = argv[0];
        FILE * F = fopen(filename, "rt");
        if (F) {
            while (!feof(F)) {
                char line[80];
                char *av[MAXARGS], *tok;
                int ac = 0, err;
                if (!fgets(line, sizeof(line), F)) {
                    break;
                }
                tok = strtok(line, " \n");
                while (tok && ac < MAXARGS) {
                    av[ac++] = tok;
                    tok = strtok(NULL, " \n");
                }
                err = eval_command(gd, ac, av);
                if (err) {
                    return err;
                }
            }
            fclose(F);
        }
    }
    return 0;
}

/*
static int merge_ship(gamedata *gd, cJSON *obj, struct region *r) {
    ship *sh;
    int id;
    assert(obj->type == cJSON_Object);
    id = json_get_int(obj, "id");
    sh = ship_get(gd, id, r);
    if (sh == NULL) {
        sh = ship_create(r, obj);
        ship_add(gd, sh);
    }
    return db_write_ship(g_db, sh);
}

static int merge_ships(gamedata *gd, cJSON *arr, struct region *r) {
    int err = SQLITE_OK;
    cJSON *child;
    assert(arr->type == cJSON_Array);
    for (child = arr->child; child; child = child->next) {
        assert(child->type == cJSON_Object);
        err = merge_ship(gd, child, r);
        if (err != SQLITE_OK) break;
    }
    return err;
}

static int merge_building(gamedata *gd, cJSON *obj, struct region *r) {
    building *b;
    int id;
    assert(obj->type == cJSON_Object);
    id = json_get_int(obj, "id");
    b = building_get(gd, id, r);
    if (b == NULL) {
        b = building_create(r, obj);
        building_add(gd, b);
    }
    return db_write_building(g_db, b);
}

static int merge_buildings(gamedata *gd, cJSON *arr, struct region *r) {
    int err = SQLITE_OK;
    cJSON *child;
    assert(arr->type == cJSON_Array);
    for (child = arr->child; child; child = child->next) {
        assert(child->type == cJSON_Object);
        err = merge_building(gd, child, r);
        if (err != SQLITE_OK) break;
    }
    return err;
}

static int merge_unit(gamedata *gd, cJSON *obj, struct region *r) {
    unit *u;
    int id;
    assert(obj->type == cJSON_Object);
    id = json_get_int(obj, "id");
    u = unit_get(gd, id, r);
    if (u == NULL) {
        u = unit_create(r, obj);
        unit_add(gd, u);
    }
    return db_write_unit(g_db, u);
}

static int merge_units(gamedata *gd, cJSON *arr, struct region *r) {
    int err = SQLITE_OK;
    cJSON *child;
    assert(arr->type == cJSON_Array);
    for (child = arr->child; child; child = child->next) {
        assert(child->type == cJSON_Object);
        err = merge_unit(gd, child, r);
        if (err != SQLITE_OK) break;
    }
    return err;
}
*/

static int export_cr(struct gamedata *gd, int argc, char **argv) {
    const char *filename = "stdout";
    FILE *F = stdout;
    int err;

    if (argc > 0) {
        filename = argv[0];
        F = fopen(filename, "wt");
        if (!F) {
            perror(filename);
            return errno;
        }
    }
    err = export(gd, F);
    if (F != stdout) {
        fclose(F);
    }
    return err;
}

static int import_cr(struct gamedata *gd, int argc, char **argv) {
    const char *filename = "stdin";
    FILE *F = stdin;
    int err;

    if (argc > 0) {
        filename = argv[0];
        F = fopen(filename, "rt");
        if (!F) {
            perror(filename);
            return errno;
        }
    }
    err = import(gd, F, filename);
    g_modified |= (err == 0);
    if (F != stdin) {
        fclose(F);
    }
    return err;
}

int eval_command(struct gamedata *gd, int argc, char **argv) {
    struct process {
        const char *command;
        int(*eval)(struct gamedata *, int, char **);
    } procs[] = {
        {"export-cr", export_cr},
        {"export-map", NULL},
        {"import-cr", import_cr},
        {"script", eval_script},
        {NULL, NULL}
    };
    const char *command = argv[0];
    int i;
    if (command[0] == '#') return 0;
    for (i = 0; procs[i].command; ++i) {
        if (strcmp(command, procs[i].command) == 0) {
            if (procs[i].eval) {
                return procs[i].eval(gd, argc - 1, argv + 1);
            }
            else {
                fprintf(stderr, "not implemented: %s\n", command);
                return 0;
            }
        }
    }
    fprintf(stderr, "unknown command: %s\n", command);
    return usage();
}

int main(int argc, char **argv) {
    int err = 0, i;
    struct gamedata *gd;

    g_program = argv[0];
    i = parseargs(argc, argv);

    if (SQLITE_OK != (err = db_open(g_dbname, &g_db, SCHEMA_VERSION))) {
        return err;
    }

    gamedata_init();
    config_init(g_db);
    gd = game_create(g_db);
    if (i >= 1 && i <= argc) {
        err = eval_command(gd, argc-i, argv + i);
        if (err) return err;
    }
    if (g_modified) {
        err = game_save(gd);
        g_modified = false;
    }
    if (err) return err;
    game_free(gd);
    config_done();
    gamedata_done();

    if (g_db) {
        err = db_close(g_db);
    }
    return err;
}
