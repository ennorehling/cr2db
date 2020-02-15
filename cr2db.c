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
    int err;
    assert(db);
    json = crfile_read(F, filename);
    if (json) {
        cJSON *factions = cJSON_GetObjectItem(json, "PARTEI");
        if (factions->type == cJSON_Array) {
            sqlite3_stmt *insert_faction = NULL;
            cJSON *object;
            err = sqlite3_prepare_v2(db,
                "INSERT INTO `faction` (`id`, `name`, `email`, `data`) VALUES (?,?,?,?)",
                -1, &insert_faction, NULL);
            if (err != SQLITE_OK) {
                const char * msg = sqlite3_errmsg(db);
                fputs(msg, stderr);
                return err;
            }
            cJSON_ArrayForEach(object, factions) {
                cJSON *jId = cJSON_GetObjectItem(object, "id");
                if (jId) {
                    cJSON *jName = cJSON_GetObjectItem(object, "Parteiname");
                    cJSON *jMail = cJSON_GetObjectItem(object, "email");
                    char * data;
                    err = sqlite3_reset(insert_faction);
                    assert(err == SQLITE_OK);
                    err = sqlite3_bind_int(insert_faction, 1, jId->valueint);
                    assert(err == SQLITE_OK);
                    if (jName) {
                        err = sqlite3_bind_text(insert_faction, 2, jName->valuestring, -1, SQLITE_STATIC);
                        assert(err == SQLITE_OK);
                    }
                    else {
                        sqlite3_bind_null(insert_faction, 2);
                    }
                    if (jMail) {
                        err = sqlite3_bind_text(insert_faction, 3, jMail->valuestring, -1, SQLITE_STATIC);
                        assert(err == SQLITE_OK);
                    }
                    else {
                        sqlite3_bind_null(insert_faction, 3);
                    }
                    data = cJSON_Print(object);
                    if (data) {
                        size_t sz = strlen(data);
                        err = sqlite3_bind_blob(insert_faction, 4, data, sz, SQLITE_TRANSIENT);
                        assert(err == SQLITE_OK);
                        cJSON_free(data);
                    }
                    else {
                        sqlite3_bind_null(insert_faction, 4);
                    }
                    err = sqlite3_step(insert_faction);
                    if (err != SQLITE_DONE) return err;
                }
            }
            err = sqlite3_finalize(insert_faction);
            assert(err == SQLITE_OK);
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
            db_create(&db, dbname, "crschema.sql");
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
        }
    }
    return 0;
}
