#include "crdb.h"

int main(int argc, char **argv) {
    crdb_t *data;

    data = crdb_open("test.db");
    crdb_import(data, "test.cr");
    crdb_close(data);
    return 0;
}

