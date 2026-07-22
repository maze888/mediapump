#include "internal/ionet_internal.h"

#include "logging.h"

const char * get_io_type_string(io_type_t io_type) {
    switch (io_type) {
        case IO_TYPE_ACCEPT:
            return "IO_TYPE_ACCEPT";
        case IO_TYPE_MULTISHOT_ACCEPT:
            return "IO_TYPE_MULTISHOT_ACCEPT";
        case IO_TYPE_CLOSE:
            return "IO_TYPE_CLOSE";
        case IO_TYPE_FILE_CLOSE:
            return "IO_TYPE_FILE_CLOSE";
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
    // printf("%-16s: %p\n", "send_buf", ctx->send_buf);
    printf("%-16s: %lu\n", "send_buf_size", ctx->send_buf_size);
    printf("%-16s: %lu\n", "send_len", ctx->send_len);
    printf("%-16s: %d\n", "file_fd", ctx->file_fd);
    printf("%-16s: %ld\n", "worked_offset", ctx->worked_offset);
    printf("%-16s: %p\n", "server", ctx->server);
    memset(endline, '-', strlen(subject));
    printf("%s\n", endline);
#endif
}

ionet_context_t * create_ionet_context(io_type_t io_type, int fd, int file_fd, struct ionet_server *server) {
    ionet_context_t *ctx = NULL;

    // switch (io_type) {
    //     case IO_TYPE_ACCEPT:
    //     case IO_TYPE_MULTISHOT_ACCEPT:
    //     case IO_TYPE_CLOSE:
    //     case IO_TYPE_FILE_CLOSE:
    //     case IO_TYPE_READ:
    //     case IO_TYPE_WRITE:
    //     case IO_TYPE_SENDFILE:
    //         break;
    //     default:
    //         LOG_ERROR_NO_STRERROR("invalid argument: unknown io_type(%d)", io_type);
    //         goto out;
    // }

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

void close_ionet_context(ionet_context_t *ctx) {
    if (ctx) {
        print_ionet_context("CLOSE_CONTEXT", ctx);

        if (ctx->fd >= 0) {
            // FIN 전송 및 수신 중단
            change_io_type(ctx, IO_TYPE_SHUTDOWN);
            add_request(ctx, SHUT_RDWR, IOSQE_IO_LINK);
            
            // 커널 워커 참조 강제 해제
            change_io_type(ctx, IO_TYPE_CANCEL);
            add_request(ctx, IORING_ASYNC_CANCEL_ALL | IORING_ASYNC_CANCEL_FD, IOSQE_IO_LINK);
            
            // 최종 자원 정리
            change_io_type(ctx, IO_TYPE_CLOSE);
            add_request(ctx, 0, 0);
        }
        if (ctx->file_fd >= 0) {
            // TODO: 처리
            // ionet_context_t *ctx = create_ionet_context(IO_TYPE_FILE_CLOSE, -1, file_fd, server);
            // add_request(server, ctx, 0);
        }
        // ctx->read_buf, ctx 는 close 완료후 해제한다.
        // safe_free(ctx->read_buf);
        // send_buf 의 해제는 앱단에서 해제해야한다.
        // safe_free((*ctx)->send_buf);
        // safe_free(ctx);
    }
}

void delete_ionet_context(ionet_context_t *ctx) {
    if (ctx) {
        safe_free(ctx->read_buf);
        safe_free(ctx);
    }
}

// 
void add_request(struct ionet_context *ctx, int flags, int sqe_flags) {
    // TODO: sqe 가 null 일때 처리는 요청을 별도 큐에 넣고 특정 시점 이후에 다시 처리하는 형태로 변경
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->server->ring);
    if (!sqe) {
        PANIC("io_uring_get_sqe() failed");
    }
    sqe->flags |= sqe_flags; 

    switch (ctx->io_type) {
        case IO_TYPE_ACCEPT:
            io_uring_prep_accept(sqe, ctx->server->accept_fd, NULL, NULL, 0);
            break;
        case IO_TYPE_MULTISHOT_ACCEPT:
            io_uring_prep_multishot_accept(sqe, ctx->server->accept_fd, NULL, NULL, 0);
            break;
        case IO_TYPE_CANCEL:
            io_uring_prep_cancel(sqe, NULL, flags);
            sqe->fd = ctx->fd;
            break;
        case IO_TYPE_CLOSE:
            io_uring_prep_close(sqe, ctx->fd);
            break;
        case IO_TYPE_FILE_CLOSE:
            io_uring_prep_close(sqe, ctx->file_fd);
            break;
        case IO_TYPE_READ:
            io_uring_prep_read(sqe, ctx->fd, ctx->read_buf, ctx->read_buf_size, 0);
            break;
        case IO_TYPE_WRITE:
            size_t remaining = ctx->send_buf_size - ctx->send_len;

            LOG_DEBUG("remain send: %ld bytes", remaining);

            unsigned send_len = remaining > ctx->server->max_send_size ? ctx->server->max_send_size : remaining;
            io_uring_prep_write(sqe, ctx->fd, ctx->send_buf + ctx->send_len, send_len, 0);
            break;
        case IO_TYPE_SENDFILE:
            break;
    }

    io_uring_sqe_set_data(sqe, ctx);
}

void change_io_type(struct ionet_context *ctx, io_type_t io_type)
{
    if (ctx) {
        ctx->io_type = io_type;
    }
}
