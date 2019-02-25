#include <fastcgi.h>

int main(int argc, char **argv) {
    FCGX_Request request;
    FCGX_Init();
    FCGX_InitRequest(&request, 0, 0);
    while(FCGX_Accept_r(&request) == 1) {
        FCGX_PutS("Content-Type: text/plain\r\n\r\n", Req.pOut);
        FCGX_PutS("Hello World\n", Req.pOut);
    }

    return 0;
}
