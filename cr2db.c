#include "log.h"
#include "gettext.h"
#include "crfile.h"
#include "database.h"

#include <cJSON.h>
#include <sqlite3.h>
#include <crpat.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int import_cr(sqlite3 *db, FILE *F, const char *filename) {
    cJSON *json;
    assert(db);
    json = crfile_read(F, filename);
    if (json) {
        cJSON *factions = cJSON_GetObjectItem(json, "PARTEI");
        if (factions->type == cJSON_Array) {
            cJSON *object;
            cJSON_ArrayForEach(object, factions) {
                db_write_faction(db, object);
            }
        }
        cJSON_Delete(json);
    }
    return 0;
}

int convert_cr(FILE *F, const char *filename) {
    cJSON *json;
    json = crfile_read(F, filename);
    if (json) {
        char *text = cJSON_PrintUnformatted(json);
        if (text) {
            fputs(text, stdout);
            cJSON_free(text);
        }
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
    fputs("  convert [filename]  convert CR from a file (or stdin) to JSON and print it.\n", stderr);
    fputs("  import [filename]   import CR from a file.\n", stderr);
    fputs("  export [filename]   export game data to a CR file.\n", stderr);
    fputs("  template [filename] create an order template.\n", stderr);
}

static int parseargs(int argc, char **argv) {
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
    int i = parseargs(argc, argv);
    if (i >= 1 && i < argc) {
        const char *command = argv[i++];
        if (strcmp(command, "convert") == 0) {
            FILE * F = stdin;
            const char *infile = "stdin";
            if (i < argc) {
                infile = argv[i++];
                F = fopen(infile, "r");
            }
            if (F) {
                convert_cr(F, infile);
                fclose(F);
            }
        }
        else {
            sqlite3 *db = NULL;
            db_open(&db, dbname, "crschema.sql");
            if (strcmp(command, "import") == 0) {
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
            db_close(db);
        }
    }
    return 0;
}
