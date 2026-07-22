#include <arpa/inet.h>
#include <string.h>

#include "ionet.h"
#include "util.h"
#include "logging.h"

#include "internal/ionet_internal.h"

#define DEFAULT_QUEUE_SIZE 1024

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
    
    // default send size
    server->max_send_size = 131072;

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
    add_request(ctx, 0, 0);
    io_uring_submit(&server->ring);

    while (1) {
        struct io_uring_cqe *cqe;

        // 완료된 이벤트가 있을 때까지 대기
        LOG_DEBUG("io_uring_wait_cqe...");
        int rv = io_uring_wait_cqe(&server->ring, &cqe);
        if (rv != 0) {
            if (rv == -EINTR) {
                LOG_DEBUG("io_uring_wait_cqe() failed: EINTR");
                continue;
            }
            LOG_ERROR_CODE(rv, "io_uring_wait_cqe() failed");
            goto out;
        }
        LOG_DEBUG("io_uring_wait_cq return: %d", rv);

        struct io_uring_cqe *cqes[server->queue_size];
        int cqe_count = io_uring_peek_batch_cqe(&server->ring, cqes, server->queue_size);
        
        LOG_DEBUG("cqe_count: %d", cqe_count);
        for (int i = 0; i < cqe_count; i++) {
            cqe = cqes[i];

            struct ionet_context *ctx = (struct ionet_context *)io_uring_cqe_get_data(cqe);
            LOG_DEBUG("cqe[%d]->io_type: %s", i, get_io_type_string(ctx->io_type));
        }


        for (int i = 0; i < cqe_count; i++) {
            cqe = cqes[i];

            struct ionet_context *ctx = (struct ionet_context *)io_uring_cqe_get_data(cqe);
            if (cqe->res <= 0) {
                if (cqe->res == 0) {
                    switch (ctx->io_type) {
                        case IO_TYPE_READ:
                            print_ionet_context("CLIENT_EXIT_CONTEXT", ctx);
                            LOG_DEBUG("client exited");
                            close_ionet_context(ctx);
                            break;
                        case IO_TYPE_CLOSE:
                            LOG_DEBUG("close fd: %d", ctx->fd);
                            LOG_DEBUG("client socket closed");
                            delete_ionet_context(ctx);
                            // break;
                            continue;
                        case IO_TYPE_FILE_CLOSE:
                            // file close complete
                            LOG_DEBUG("file closed");
                            break;
                        default:
                            LOG_DEBUG("not processed: %s", get_io_type_string(ctx->io_type));
                            break;
                    }
                } 
                else {
                    // TODO: 시스템 에러 발생 처리 (예: -ENOBUFS, -ECONNRESET 등)
                    LOG_ERROR_CODE(cqe->res, "%s failed", get_io_type_string(ctx->io_type));
                }
                continue;
            }

            switch (ctx->io_type) {
                case IO_TYPE_ACCEPT:
                case IO_TYPE_MULTISHOT_ACCEPT:
                    {
                        print_ionet_context("ACCEPT_CONTEXT", ctx);

                        if (ctx->io_type == IO_TYPE_ACCEPT) {
                            add_request(ctx, 0, 0);
                        }
                        
                        ionet_context_t *new_read_ctx = create_ionet_context(IO_TYPE_READ, cqe->res, -1, server);

                        LOG_DEBUG("max_read_buf_size: %lu", server->max_read_buf_size);

                        new_read_ctx->read_buf = malloc(server->max_read_buf_size);
                        if (!new_read_ctx->read_buf) {
                            PANIC("malloc() failed");
                        }
                        new_read_ctx->read_buf_size = server->max_read_buf_size;

                        print_ionet_context("NEW_READ_CONTEXT", new_read_ctx);

                        add_request(new_read_ctx, 0, 0);
                        
                        if (server->accept_cb) {
                            server->accept_cb(cqe->res, new_read_ctx);
                        }

                        break;
                    }
                case IO_TYPE_SHUTDOWN:
                    break;
                case IO_TYPE_CANCEL:
                    break;
                case IO_TYPE_CLOSE:
                    break;
                case IO_TYPE_FILE_CLOSE:
                    break;
                case IO_TYPE_READ:
                    LOG_DEBUG("IO_TYPE_READ(cqe->res: %d ctx->fd: %d)", cqe->res, ctx->fd);

                    ctx->read_len = cqe->res;
                    
                    if (server->read_cb) {
                        server->read_cb(cqe->res, ctx);
                    }

                    add_request(ctx, 0, 0);

                    break;
                case IO_TYPE_WRITE:
                    ctx->send_len += cqe->res;

                    if (ctx->send_buf_size == ctx->send_len) {
                        // send complete
                        LOG_DEBUG("send complete: %lu bytes", ctx->send_len);
                        if (server->write_cb) {
                            server->write_cb(cqe->res, ctx);
                        }
                    } else {
                        // partially sent
                        add_request(ctx, 0, 0);
                    }
                    
                    break;
                case IO_TYPE_SENDFILE:
                    break;
                default:
                    LOG_DEBUG("unknown io_type: %s", get_io_type_string(ctx->io_type));
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
    add_request(ctx, 0, 0);

    return 0;
}

void set_read_buf_size(struct ionet_server *server, size_t size)
{
    if (server) {
        server->max_read_buf_size = size;
    }
}

void set_max_send_size(struct ionet_server *server, unsigned size)
{
    if (server) {
        server->max_send_size = size;
    }
}



