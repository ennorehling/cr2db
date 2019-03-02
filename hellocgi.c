#include <fcgiapp.h>
#include <stdio.h>

int main(void) {
    int err, i = 0;
    FCGX_Request req;
    err = FCGX_Init();
    err = FCGX_InitRequest(&req, 0, 0);
    while((err = FCGX_Accept_r(&req)) >= 0) {
        FCGX_PutS("Content-Type: text/plain\r\n\r\n", req.out);
        FCGX_FPrintF(req.out, "Hello World, request #%d\n", ++i);
    }

    fprintf(stderr, "hellocgi done with err=%d\n", err);
    return 0;
}
