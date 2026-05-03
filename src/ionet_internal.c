#include "internal/ionet_internal.h"

#include "logging.h"

ionet_context_t * create_ionet_context(io_type_t io_type, int fd, int file_fd, struct ionet_server *server) {
    ionet_context_t *ctx = NULL;

    switch (io_type) {
        case IO_TYPE_ACCEPT:
        case IO_TYPE_MULTISHOT_ACCEPT:
        case IO_TYPE_READ:
        case IO_TYPE_WRITE:
        case IO_TYPE_SENDFILE:
            break;
        default:
            LOG_ERROR_NO_STRERROR("invalid argument: unknown io_type(%d)", io_type);
            goto out;
    }

    ctx = malloc(sizeof(ionet_context_t));
    if (!ctx) {
        PANIC("malloc() failed");
    }

    ctx->io_type = io_type;

    if (io_type == IO_TYPE_SENDFILE) {
        ctx->file_fd = file_fd;
    } else {
        ctx->file_fd = -1;
    }

    ctx->fd = fd;
    ctx->read_buf = NULL;
    ctx->send_buf = NULL;
    ctx->read_buf_size = 0;
    ctx->send_buf_size = 0;
    ctx->read_len = 0;
    ctx->send_len = 0;
    ctx->worked_offset = 0;

    ctx->server = server;

    return ctx;

out:
    return NULL;
}

void delete_ionet_context(ionet_context_t **ctx) {
    if (ctx && *ctx) {
        print_ionet_context("DELETE_CONTEXT", *ctx);
        safe_close((*ctx)->fd);
        safe_close((*ctx)->file_fd);
        //free_ionet_context(ctx);
        (*ctx)->read_buf_size = 0;
        (*ctx)->send_buf_size = 0;
        (*ctx)->read_len = 0;
        (*ctx)->send_len = 0;
        safe_free((*ctx)->read_buf);
        safe_free((*ctx)->send_buf);
        safe_free((*ctx));
    }
}

void free_ionet_context(ionet_context_t **ctx) {
    if (ctx && *ctx) {
        print_ionet_context("FREE_CONTEXT", *ctx);
        (*ctx)->read_buf_size = 0;
        (*ctx)->send_buf_size = 0;
        (*ctx)->read_len = 0;
        (*ctx)->send_len = 0;
        safe_free((*ctx)->read_buf);
        safe_free((*ctx)->send_buf);
        safe_free((*ctx));
    }
}

const char * get_io_type_string(io_type_t io_type) {
    switch (io_type) {
        case IO_TYPE_ACCEPT:
            return "IO_TYPE_ACCEPT";
        case IO_TYPE_MULTISHOT_ACCEPT:
            return "IO_TYPE_MULTISHOT_ACCEPT";
        case IO_TYPE_READ:
            return "IO_TYPE_READ";
        case IO_TYPE_WRITE:
            return "IO_TYPE_WRITE";
        case IO_TYPE_SENDFILE:
            return "IO_TYPE_SENDFILE";
        default:
            return "IO_TYPE_UNKNOWN";
    }
}

void print_ionet_context(const char *context_string, ionet_context_t *ctx) {
#ifndef NDEBUG
    char subject[1024] = {0};
    char endline[1024] = {0};
    snprintf(subject, sizeof(subject), "-------------------- %s --------------------", context_string);
    printf("%s\n", subject);
    printf("%-16s: %p\n", "ctx", ctx);
    printf("%-16s: %d\n", "fd", ctx->fd);
    printf("%-16s: %s\n", "io_type", get_io_type_string(ctx->io_type));
    printf("%-16s: %p\n", "read_buf", ctx->read_buf);
    printf("%-16s: %lu\n", "read_buf_size", ctx->read_buf_size);
    printf("%-16s: %lu\n", "read_len", ctx->read_len);
    printf("%-16s: %p\n", "send_buf", ctx->send_buf);
    printf("%-16s: %lu\n", "send_buf_size", ctx->read_buf_size);
    printf("%-16s: %lu\n", "send_len", ctx->send_len);
    printf("%-16s: %d\n", "file_fd", ctx->file_fd);
    printf("%-16s: %ld\n", "worked_offset", ctx->worked_offset);
    printf("%-16s: %p\n", "server", ctx->server);
    memset(endline, '-', strlen(subject));
    printf("%s\n", endline);
#endif
}
