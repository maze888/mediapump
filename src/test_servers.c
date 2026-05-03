#include <stdlib.h>
#include <string.h>

#include "ionet.h"
#include "logging.h"
#include "test_servers.h"

static void read_cb(int rv, ionet_context_t *ctx) {
    char *buf = malloc(ctx->read_len);
    if (!buf) {
        PANIC("malloc() failed");
    }
    memcpy(buf, ctx->read_buf, ctx->read_len);
    
    ionet_send_data(ctx->server, ctx->fd, -1, buf, ctx->read_len);
}

int test_echo_server(int port) {
    struct ionet_server *server = create_ionet_server(NULL, port, -1);
    if (!server) {
        fprintf(stderr, "create_ionet_server() failed");
        return -1;
    }

    ionet_add_listener(server, IO_TYPE_READ, read_cb);

    if (ionet_loop(server) < 0) {
        fprintf(stderr, "ionet_loop() failed");
        return -1;
    }

    printf("test echo server exited.\n");

    return 0;
}
