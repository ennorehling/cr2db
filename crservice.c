#include "crdata.h"

#include <stdio.h>

#ifdef HAVE_FCGI
#include <fcgiapp.h>

int fcgimain(void) {
    int err, i = 0;
    FCGX_Request req;
    err = FCGX_Init();
    err = FCGX_InitRequest(&req, 0, 0);
    while((err = FCGX_Accept_r(&req)) >= 0) {
        FCGX_PutS("Content-Type: text/plain\r\n\r\n", req.out);
        FCGX_FPrintF(req.out, "Hello World, request #%d\n", ++i);
    }
    return 0;
}
#endif

int main(void) {
    int err = 0;
#ifdef HAVE_FCGI
    fprintf(stderr, "crservice started.\n");
    err = fcgimain();
#endif
    fprintf(stderr, "crservice done.\n");
    return err;
}
