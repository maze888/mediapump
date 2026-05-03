#pragma once

#include <stdio.h>
#include <netinet/in.h>
#include <liburing.h>

typedef enum {
    IO_TYPE_ACCEPT = 1,
    IO_TYPE_MULTISHOT_ACCEPT = 2,
    IO_TYPE_READ = 4,
    IO_TYPE_WRITE = 8,
    IO_TYPE_SENDFILE = 16,
} io_type_t;

typedef struct ionet_context {
    int fd;
    io_type_t io_type;

    unsigned char *read_buf, *send_buf;
    size_t read_buf_size, send_buf_size;
    size_t read_len, send_len;
    
    // for sendfile
    int file_fd;
    off_t worked_offset;

    struct ionet_server *server;
} ionet_context_t;

// accept 의 경우: rv = client_fd
// read, write 의 경우: rv = success_data_len
typedef void (*request_completion_cb)(int rv, ionet_context_t *ctx);

struct ionet_server {
    int accept_fd;
    struct sockaddr_in saddr;
    unsigned short port;

    struct io_uring ring;
    int queue_size;
    // unsigned char *provided_buffers;

    size_t max_read_buf_size;

    request_completion_cb accept_cb, read_cb, write_cb;
};

struct ionet_server * create_ionet_server(const char *bind_ip, unsigned short bind_port, int queue_size);
void delete_ionet_server(struct ionet_server *server);

int ionet_loop(struct ionet_server *server);
int ionet_add_listener(struct ionet_server *server, io_type_t type, request_completion_cb cb); 

// data pointer must be allocated(e.g. malloc). and free the data pointer in request_completion_cb.
int ionet_send_data(struct ionet_server *server, int fd, int file_fd, void *data, size_t data_len);

void set_read_buf_size(struct ionet_server *server, size_t size);

