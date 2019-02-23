#include "crfile.h"
#include "gettext.h"
#include "log.h"

#include "cJSON/cJSON.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    cJSON *json;
    const char * filename = "stdin";
    FILE * F = stdin;

    if (argc > 0) {
        filename = argv[1];
        F = fopen(filename, "r");
        if (F == NULL) {
            int err = errno;
            errno = 0;
            log_error(NULL, gettext("could not open %s: %s\n"), filename, strerror(err));
            return NULL;
        }
    }

    json = crfile_read(F, filename);
    fclose(F);
    if (json) {
        char *dump = cJSON_PrintUnformatted(json);
        puts(dump);
        cJSON_free(dump);
    }
    cJSON_Delete(json);
    return 0;
}
