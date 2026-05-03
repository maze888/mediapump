#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "test_servers.h"

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    signal(SIGPIPE, SIG_IGN);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s (port)\n", argv[0]);
        return 1;
    }

    if (test_echo_server(atoi(argv[1])) < 0) {
        fprintf(stderr, "test_echo_server() failed");
        return 1;
    }

    return 0;
}
