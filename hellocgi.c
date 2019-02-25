#include <fcgiapp.h>

int main(int argc, char **argv) {
    FCGX_Request req;
    FCGX_Init();
    FCGX_InitRequest(&req, 0, 0);
    while(FCGX_Accept_r(&req) == 1) {
        FCGX_PutS("Content-Type: text/plain\r\n\r\n", req.out);
        FCGX_PutS("Hello World\n", req.out);
    }

    return 0;
}
