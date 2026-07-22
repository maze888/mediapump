#include <stdlib.h>
#include <string.h>

#include "ionet.h"
#include "internal/ionet_internal.h"
#include "logging.h"
#include "test_servers.h"

typedef void (*read_cb_t)(int, ionet_context_t *);

static struct ionet_server * test_server(int port, request_completion_cb accept_cb, request_completion_cb read_cb, request_completion_cb write_cb) {
    struct ionet_server *server = create_ionet_server(NULL, port, -1);
    if (!server) {
        fprintf(stderr, "create_ionet_server() failed");
        return NULL;
    }

    if (accept_cb) {
        ionet_add_listener(server, IO_TYPE_ACCEPT, accept_cb);
    }
    if (read_cb) {
        ionet_add_listener(server, IO_TYPE_READ, read_cb);
    }
    if (write_cb) {
        ionet_add_listener(server, IO_TYPE_WRITE, write_cb);
    }

    if (ionet_loop(server) < 0) {
        fprintf(stderr, "ionet_loop() failed");
        return NULL;
    }

    printf("test server exited.\n");

    return server;
}

// echo test
static void echo_read_cb(int rv, ionet_context_t *ctx) {
    char *buf = malloc(ctx->read_len);
    if (!buf) {
        PANIC("malloc() failed");
    }
    memcpy(buf, ctx->read_buf, ctx->read_len);
    
    ionet_send_data(ctx->server, ctx->fd, -1, buf, ctx->read_len);
}

int test_echo_server(int bind_port) {
    return test_server(bind_port, NULL, echo_read_cb, NULL) ? 0 : -1;
}

// file receive test
static void file_read_cb(int rv, ionet_context_t *ctx) {
    FILE *fp = fopen("./test_file", "ab");
    if (!fp) {
        PANIC("fopen() failed");
    }

    fwrite(ctx->read_buf, ctx->read_len, 1, fp);

    safe_fclose(fp);
}

int test_file_server_1(int bind_port) {
    return test_server(bind_port, NULL, file_read_cb, NULL) ? 0 : -1;
}

// file send test
static void file_accept_cb(int rv, ionet_context_t *ctx) {
    const char *file_name = "./test_file";

    FILE *fp = fopen(file_name, "rb");
    if (!fp) {
        PANIC("fopen() failed: %s", file_name);
    }

    off_t file_size = get_file_size_stat(file_name);
    if (file_size < 0) {
        PANIC("get_file_size_stat() failed");
    }

    unsigned char *buf = malloc(file_size);
    if (!buf) {
        PANIC("malloc(%ld) failed", file_size);
    }

    size_t read_len = full_fread(buf, file_size, fp);
    if (read_len != file_size) {
        PANIC("full_fread() failed");
    }

    ionet_send_data(ctx->server, ctx->fd, -1, buf, read_len);

    fclose(fp);
}

static void file_send_cb(int rv, ionet_context_t *ctx) {
    safe_free(ctx->send_buf);
    close_ionet_context(ctx);
}

int test_file_server_2(int bind_port) {
    return test_server(bind_port, file_accept_cb, NULL, file_send_cb) ? 0 : -1;
}
