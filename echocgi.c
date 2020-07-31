#ifdef HAVE_FCGI
#include <fcgiapp.h>
#include <stdio.h>
#endif

int main(void) {
#ifdef HAVE_FCGI
    FCGX_Request req;
    int err, i = 0;
    err = FCGX_Init();
    err = FCGX_InitRequest(&req, 0, 0);
    while((err = FCGX_Accept_r(&req)) >= 0) {
        int e;
        FCGX_PutS("Content-Type: text/plain\r\n\r\n", req.out);
        FCGX_FPrintF(req.out, "Hello World, request #%d\n", ++i);
        for (e = 0; req.envp[e]; ++e) {
            FCGX_FPrintF(req.out, "%s\n", req.envp[e]);
        }
    }

    fprintf(stderr, "hellocgi done with err=%d\n", err);
#endif
    return 0;
}
