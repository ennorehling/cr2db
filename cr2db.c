#include "log.h"
#include "gettext.h"
#include "crfile.h"

#include <cJSON.h>
#include <sqlite3.h>
#include <crpat.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    cJSON *json;
    const char * filename = "stdin";
    FILE *F;
    if (argc > 1) {
        filename = argv[1];
        fprintf(stderr, "reading %s.\n", filename);
        F = fopen(filename, "r");
        if (F == NULL) {
            int err = errno;
            log_error(NULL,
                    gettext("could not open %s: %s\n"),
                    filename, strerror(err));
            return err;
        }
    }
    else {
        F = stdin;
    }
    json = crfile_read(F, filename);
    if (F != stdin) fclose(F);
    if (json) {
        cJSON_Delete(json);
    }
    return 0;
}

