#include "log.h"
#include "gettext.h"
#include "crfile.h"
#include "database.h"

#include <cJSON.h>
#include <sqlite3.h>
#include <crpat.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

int import_cr(sqlite3 *db, FILE *F, const char *filename) {
    cJSON *json;
    json = crfile_read(F, filename);
    if (json) {
        // create database
        cJSON_Delete(json);
    }
    return 0;
}

const char *dbname = "game.db";
const char *command;

static void usage(const char *program) {
    fprintf(stderr, "usage: %s [options] [command] [arguments]\n", program);
    fputs("\nOPTIONS\n", stderr);
    fputs("  -d dbname  game database name.\n", stderr);
    fputs("\nCOMMANDS\n", stderr);
    fputs("  import [filename]   import CR from a file (or stdin).\n", stderr);
}

static int parseargs(int argc, const char **argv) {
    int i;
    for (i = 1; i != argc; ++i) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'd':
                dbname = (argv[i][2]) ? argv[i] + 2 : argv[++i];
                break;
            case 'h':
                usage(argv[0]);
                return 0;
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

int main(int argc, char **argv) {
    sqlite3 *db = NULL;
    int i = parseargs(argc, argv);
    if (i > 1) {
        db_create(&db, dbname, "crschema.sql");
        if (i<argc) {
            const char *command = argv[i++];
            if (strcmp(command, "import")==0) {
                FILE * F = stdin;
                const char *infile = "stdin";
                if (i < argc) {
                    infile = argv[i++];
                    F = fopen(infile, "r");
                }
                if (F) {
                    import_cr(db, F, infile);
                    if (F != stdin) {
                        fclose(F);
                    }
                }
            }
        }
    }
    return 0;
}
