#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "crfile.h"
#include "jsondata.h"
#include "gamedb.h"

#include <cJSON.h>
#include <sqlite3.h>
#include <crpat.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

static sqlite3 * g_db;
static const char *g_dbname = "game.db";
const char *g_program = "atl-cli";

int eval_command(int argc, char **argv);

static int usage(void) {
    fprintf(stderr, "usage: %s [options] [command] [arguments]\n", g_program);
    fputs("\nOPTIONS\n", stderr);
    fputs("  -d dbname  game database name.\n", stderr);
    fputs("\nCOMMANDS\n", stderr);
    fputs("  script [filename]\n\texecute commands from a script.\n", stderr);
    fputs("  create-database\n\tcreate a new (empty) game databse.\n", stderr);
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

int create_database(int argc, char **argv) {
    return db_open(g_dbname, &g_db, 1);
}

#define MAXARGS 10

int eval_script(int argc, char **argv) {
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
    }
    return 0;
}

int db_load(sqlite3 *db, gamedata *gd) {
    int err = SQLITE_OK;
    return err;
}

int db_save(sqlite3 *db, gamedata *gd) {
    int err = SQLITE_OK;
    return err;
}

int gd_merge(gamedata *gd, cJSON *json) {
    return 0;
}

int import_cr(int argc, char **argv) {
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
            if (!g_db) {
                err = sqlite3_open(g_dbname, &g_db);
                if (err != SQLITE_OK) goto fail;
            }
            err = db_load(g_db, gd);
            if (err != SQLITE_OK) goto fail;
            gd_merge(gd, json);
            err = db_save(g_db, gd);
            if (err != SQLITE_OK) goto fail;
        }
        char *text = cJSON_PrintUnformatted(json);
        if (text) {
            fputs(text, stdout);
            cJSON_free(text);
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
        {"create-database", create_database},
        {"export-cr", NULL},
        {"export-map", NULL},
        {"import-cr", import_cr},
        {"script", eval_script},
        {NULL, NULL}
    };
    const char *command = argv[0];
    int i;
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
    jsondata_open();
    if (i >= 1 && i < argc) {
        err = eval_command(argc-i, argv + i);
    }
    jsondata_close();
    if (g_db) {
        err = sqlite3_close(g_db);
    }
    return err;
}
