#include <arpa/inet.h>
#include <string.h>

#include "ionet.h"
#include "util.h"
#include "logging.h"

#include "internal/ionet_internal.h"

#define DEFAULT_QUEUE_SIZE 1024

static void add_request(struct ionet_server *server, struct ionet_context *ctx) {
    // TODO: sqe 가 null 일때 처리는 요청을 별도 큐에 넣고 특정 시점 이후에 다시 처리하는 형태로 변경
    struct io_uring_sqe *sqe = io_uring_get_sqe(&server->ring);
    if (!sqe) {
        PANIC("io_uring_get_sqe() failed");
    }

    switch (ctx->io_type) {
        case IO_TYPE_ACCEPT:
            io_uring_prep_accept(sqe, server->accept_fd, NULL, NULL, 0);
            break;
        case IO_TYPE_MULTISHOT_ACCEPT:
            io_uring_prep_multishot_accept(sqe, server->accept_fd, NULL, NULL, 0);
            break;
        case IO_TYPE_READ:
            io_uring_prep_read(sqe, ctx->fd, ctx->read_buf, server->max_read_buf_size, 0);
            break;
        case IO_TYPE_WRITE:
            io_uring_prep_write(sqe, ctx->fd, ctx->send_buf + ctx->send_len, ctx->send_buf_size - ctx->send_len, 0);
            break;
        case IO_TYPE_SENDFILE:
            break;
    }

    io_uring_sqe_set_data(sqe, ctx);
}

struct ionet_server * create_ionet_server(const char *bind_ip, unsigned short bind_port, int queue_size)
{
    struct ionet_server *server = malloc(sizeof(struct ionet_server));
    if (!server) {
        PANIC("malloc(%zu) failed", sizeof(struct ionet_server));
    }
    memset(server, 0, sizeof(struct ionet_server));

    // bind address & port
    server->saddr.sin_family = AF_INET;
    if (!bind_ip || inet_pton(AF_INET, bind_ip, &(server->saddr.sin_addr)) <= 0) {
        server->saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    server->port = bind_port;
    server->saddr.sin_port = htons(bind_port);

    // create accept socket
    server->accept_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->accept_fd < 0) {
        LOG_ERROR("socket() failed");
        goto out;
    }

    // reuse address
    int opt = 1;
    setsockopt(server->accept_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server->accept_fd, (struct sockaddr *)&server->saddr, sizeof(server->saddr)) < 0) {
        LOG_ERROR("bind() failed");
        goto out;
    }

    if (listen(server->accept_fd, SOMAXCONN) < 0) {
        LOG_ERROR("listen() failed");
        goto out;
    }

    // queue_size must be power number
    if (queue_size <= 0) {
        server->queue_size = DEFAULT_QUEUE_SIZE;
    } else {
        server->queue_size = power_of_two(queue_size, 65536);
    }

    if (io_uring_queue_init(server->queue_size, &server->ring, 0) != 0) {
        LOG_ERROR("io_uring_queue_init() failed");
        goto out;
    }

    // default read size
    server->max_read_buf_size = BUFSIZ;

    return server;

out:
    delete_ionet_server(server);

    return NULL;
}

void delete_ionet_server(struct ionet_server *server) {
    if (server) {
        io_uring_queue_exit(&server->ring);
        safe_close(server->accept_fd);
        safe_free(server);
    }
}

int ionet_loop(struct ionet_server *server)
{
    if (!server) {
        LOG_ERROR_NO_STRERROR("invalid argument: server is null");
        goto out;
    }

    // 예외 상황 처리 (중요):
    // 만약 커널이 multishot을 처리하다가 에러(예: EMFILE - 시스템 열린 파일 제한 도달 등)를 만나면, 해당 multishot 요청은 자동으로 종료(Cancel)됩니다.
    //
    // 대응 전략: CQE의 res 값을 확인하여 에러가 발생했는지 항상 체크하십시오. 만약 에러로 인해 multishot이 중단되었다면, 에러 원인을 해결한 뒤 다시 한 번 accept 요청을 제출해야 합니다.
    // ionet_context_t *ctx = create_ionet_context(IO_TYPE_ACCEPT, server->accept_fd, -1, server);
    ionet_context_t *ctx = create_ionet_context(IO_TYPE_MULTISHOT_ACCEPT, server->accept_fd, -1, server);
    add_request(server, ctx);
    io_uring_submit(&server->ring);

    while (1) {
        struct io_uring_cqe *cqe;

        // 완료된 이벤트가 있을 때까지 대기
        int rv = io_uring_wait_cqe(&server->ring, &cqe);
        if (rv != 0) {
            if (rv == -EINTR) {
                LOG_DEBUG("io_uring_wait_cqe() failed: EINTR");
                continue;
            }
            LOG_ERROR_CODE(rv, "io_uring_wait_cqe() failed");
            goto out;
        }

        struct io_uring_cqe *cqes[server->queue_size];
        int cqe_count = io_uring_peek_batch_cqe(&server->ring, cqes, server->queue_size);

        for (int i = 0; i < cqe_count; i++) {
            cqe = cqes[i];

            struct ionet_context *ctx = (struct ionet_context *)io_uring_cqe_get_data(cqe);
            if (cqe->res < 0) {
                // 시스템 에러 발생 (예: -ENOBUFS, -ECONNRESET 등)
                LOG_ERROR_CODE(cqe->res, "IO Error");
                delete_ionet_context(&ctx);
                continue;
            }
            if (cqe->res <= 0) {
                if (cqe->res == 0) {
                    if (ctx->io_type == IO_TYPE_READ) {
                        print_ionet_context("CLIENT_EXIT_CONTEXT", ctx);
                        delete_ionet_context(&ctx);
                        LOG_DEBUG("client exited");
                        continue;
                    }
                } 
            }

            switch (ctx->io_type) {
                case IO_TYPE_ACCEPT:
                case IO_TYPE_MULTISHOT_ACCEPT:
                    {
                        LOG_DEBUG("accept fd: %d", cqe->res);

                        if (ctx->io_type == IO_TYPE_ACCEPT) {
                            add_request(server, ctx);
                        }

                        ionet_context_t *new_read_ctx = create_ionet_context(IO_TYPE_READ, cqe->res, -1, server);

                        LOG_DEBUG("max_read_buf_size: %lu", server->max_read_buf_size);

                        new_read_ctx->read_buf = malloc(server->max_read_buf_size);
                        if (!new_read_ctx->read_buf) {
                            PANIC("malloc() failed");
                        }

                        print_ionet_context("NEW_READ_CONTEXT", new_read_ctx);

                        add_request(server, new_read_ctx);
                        
                        if (server->accept_cb) {
                            server->accept_cb(cqe->res, ctx);
                        }

                        break;
                    }
                case IO_TYPE_READ:
                    LOG_DEBUG("IO_TYPE_READ(cqe->res: %d ctx->fd: %d)", cqe->res, ctx->fd);

                    ctx->read_len = cqe->res;

                    add_request(server, ctx);
                    
                    if (server->read_cb) {
                        server->read_cb(cqe->res, ctx);
                    }

                    break;
                case IO_TYPE_WRITE:
                    ctx->send_len += cqe->res;

                    print_ionet_context("WRITE_CONTEXT", ctx);

                    if (ctx->send_buf_size == ctx->send_len) {
                        // send complete
                        LOG_DEBUG("send complete: %lu bytes", ctx->send_len);
                        free_ionet_context(&ctx);
                    } else {
                        // partially sent
                        add_request(server, ctx);
                    }
                    
                    if (server->write_cb) {
                        server->write_cb(cqe->res, ctx);
                    }

                    break;
                case IO_TYPE_SENDFILE:
                    break;
            }
        }
        io_uring_cq_advance(&server->ring, cqe_count);

        io_uring_submit(&server->ring);
    }

    return 0;

out:
    return -1;
}

int ionet_add_listener(struct ionet_server *server, io_type_t type, request_completion_cb cb) {
    if (!server || !cb) {
        LOG_ERROR_NO_STRERROR("ionet_add_listner() failed: invalid argument: (server: %s, cb: %s)", CKNUL(server), CKNUL(cb));
        return -1;
    }

    switch (type) {
        case IO_TYPE_ACCEPT:
            server->accept_cb = cb;
            break;
        case IO_TYPE_READ:
            server->read_cb = cb;
            break;
        case IO_TYPE_WRITE:
            server->write_cb = cb;
            break;
        default:
            LOG_ERROR_NO_STRERROR("ionet_add_listner() failed: invalid argument: (type: %d)", type);
            return -1;
    }

    return 0;
}

int ionet_send_data(struct ionet_server *server, int fd, int file_fd, void *data, size_t data_len) {
    LOG_DEBUG("ionet_send_data");

    ionet_context_t *ctx = create_ionet_context(IO_TYPE_WRITE, fd, file_fd, server);
    if (!ctx) {
        LOG_ERROR_NO_STRERROR("create_ionet_context() failed");
        return -1;
    }

    ctx->send_buf = data;
    ctx->send_buf_size = data_len;
    add_request(server, ctx);

    return 0;
}

void set_read_buf_size(struct ionet_server *server, size_t size)
{
    if (server) {
        server->max_read_buf_size = size;
    }
}
