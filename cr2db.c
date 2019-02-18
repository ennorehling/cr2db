#include "crdb.h"

#include "cJSON/cJSON.h"

#include <stdio.h>

int main(int argc, char **argv) {
    crdb_t data;

    crdb_init(&data);
    if (0 == crdb_import(&data, argv[1])) {
        char *dump = cJSON_Print(data.json);
        puts(dump);
        cJSON_free(dump);
    }
    crdb_free(&data);
    return 0;
}
