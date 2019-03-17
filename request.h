#include <stdio.h>

typedef struct request_t {
    enum reqtype_e {
        REQ_TYPE_FILE,
        REQ_TYPE_ENV,
        REQ_TYPE_FCGI,
    } type;
    char **envp;
    void *userdata;
} request;

int request_init(request *req, enum reqtype_e type, void *arg);
int request_accept(request *req);
void request_delete(request *req);
