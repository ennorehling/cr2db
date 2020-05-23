#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "crfile.h"
#include "config.h"
#include "jsondata.h"
#include "gamedb.h"

#include <cJSON.h>
#include <sqlite3.h>
#include <crpat.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static sqlite3 * g_db;
static const char *g_dbname = ":memory:";
const char *g_program = "atl-cli";

int eval_command(int argc, char **argv);

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

int eval_script(int argc, char **argv) {
    if (argc > 0) {
        const char * filename = argv[0];
        FILE * F = fopen(filename, "rt");
        if (F) {
            while (!feof(F)) {
                char line[80];
                char *av[MAXARGS], *tok;
                int ac = 0, err;
                fgets(line, sizeof(line), F);
                tok = strtok(line, " \n");
                while (tok && ac < MAXARGS) {
                    av[ac++] = tok;
                    tok = strtok(NULL, " \n");
                }
                err = eval_command(ac, av);
                if (err) {
                    return err;
                }
            }
            fclose(F);
        }
    }
    return 0;
}

int json_get_int(cJSON *obj, const char *key) {
    cJSON *child = cJSON_GetObjectItem(obj, key);
    return child->valueint;
}

static int merge_faction(gamedata *gd, cJSON *obj, bool is_new) {
    faction *f;
    int id;
    assert(obj->type == cJSON_Object);
    id = json_get_int(obj, "id");
    f = faction_get(gd, id);
    if (f == NULL) {
        f = faction_create(obj);
        if (f != NULL) {
            faction_add(gd, f);
        }
    }
    else if (is_new) {
        faction_update(f, obj);
    }
    else {
        return 0;
    }
    return db_write_faction(g_db, f);
}

static int merge_factions(gamedata *gd, cJSON *arr, bool is_new) {
    int err = SQLITE_OK;
    cJSON *child;
    assert(arr->type == cJSON_Array);
    for (child = arr->child; child; child = child->next) {
        assert(child->type == cJSON_Object);
        err = merge_faction(gd, child, is_new);
        if (err != SQLITE_OK) break;
    }
    return err;
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

static int merge_region(gamedata *gd, cJSON *obj, int turn) {
    cJSON **p_child = &obj->child;
    cJSON *jUnits = NULL;
    cJSON *jShips = NULL;
    cJSON *jBuildings = NULL;
    region *r, *orig;
    assert(obj->type == cJSON_Object);
    while (*p_child) {
        cJSON * child = *p_child;
        if (child->type == cJSON_Array) {
            if (strcmp("EINHEIT", child->string) == 0) {
                jUnits = child;
                *p_child = child->next;
                continue;
            }
            else if (strcmp("BURG", child->string) == 0) {
                jBuildings = child;
                *p_child = child->next;
                continue;
            }
            else if (strcmp("SCHIFF", child->string) == 0) {
                jShips = child;
                *p_child = child->next;
                continue;
            }
        }
        p_child = &child->next;
    }
    r = region_create(obj);
    if (r) {
        // TODO: this is leaking the arrays!
        if (jShips) {
            merge_ships(gd, jShips, r);
        }
        if (jBuildings) {
            merge_buildings(gd, jBuildings, r);
        }
        if (jUnits) {
            merge_units(gd, jUnits, r);
        }
        orig = region_get(gd, r->id);
        if (orig) {
            // TODO: merge the two depending on turn
        }
        return db_write_region(g_db, r);
    }
    return -1;
}

static int merge_regions(gamedata *gd, cJSON *arr, int turn) {
    int err = SQLITE_OK;
    cJSON *child;
    assert(arr->type == cJSON_Array);
    for (child = arr->child; child; child = child->next) {
        assert(child->type == cJSON_Object);
        err = merge_region(gd, child, turn);
        if (err != SQLITE_OK) break;
    }
    return err;
}
*/

int gd_merge(gamedata *gd, int game_turn, cJSON *json) {
    int turn = 0;
    int err = SQLITE_OK;
    cJSON *child;
    assert(json->type == cJSON_Object);
    child = cJSON_GetObjectItem(json, "Runde");
    if (child) {
        turn = child->valueint;
    }
    for (child = json->child; child; child = child->next) {
        if (child->type == cJSON_Array) {
            if (0 == strcmp(child->string, "PARTEI")) {
                 err = merge_factions(gd, child, turn >= game_turn);
                 if (err != SQLITE_OK) break;
            }
            else if (0 == strcmp(child->string, "REGION")) {
/*
                err = merge_regions(gd, child, turn >= game_turn);
                if (err != SQLITE_OK) break;
*/
            }
        }
    }
    if (game_turn < turn) {
        char buf[12];
        snprintf(buf, sizeof(buf), "%d", turn);
        config_set("turn", buf);
    }
    return err;
}

static int import_cr(int argc, char **argv) {
    const char *filename = "stdin";
    FILE *F = stdin;
    int err;
    cJSON *json = NULL;
    gamedata *gd = NULL;
    if (argc > 0) {
        filename = argv[0];
        F = fopen(filename, "rt");
        if (!F) {
            perror(filename);
            return errno;
        }
    }
    gd = game_create();
    if (gd) {
        json = crfile_read(F, filename);
        if (json) {
            int game_turn;

            if (!g_db) {
                err = sqlite3_open(g_dbname, &g_db);
                if (err != SQLITE_OK) goto fail;
            }
            game_turn = atoi(config_get("turn", "0"));
            gd_merge(gd, game_turn, json);
        }
        cJSON_Delete(json);
    }
    if (F != stdin) {
        fclose(F);
    }
    return 0;
fail:
    if (json) {
        cJSON_Delete(json);
    }
    if (F && F != stdin) {
        fclose(F);
    }
    return err;
}

int eval_command(int argc, char **argv) {
    struct process {
        const char *command;
        int(*eval)(int, char **);
    } procs[] = {
        {"export-cr", NULL},
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
                return procs[i].eval(argc - 1, argv + 1);
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

    g_program = argv[0];
    i = parseargs(argc, argv);

    if (SQLITE_OK != (err = db_open(g_dbname, &g_db, 1))) {
        return err;
    }

    jsondata_init();
    config_init(g_db);
    if (i >= 1 && i <= argc) {
        err = eval_command(argc-i, argv + i);
    }
    config_done();
    jsondata_done();

    if (g_db) {
        err = db_close(g_db);
    }
    return err;
}
