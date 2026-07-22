#pragma once

#include "ionet.h"

const char * get_io_type_string(io_type_t io_type);
void print_ionet_context(const char *context_string, ionet_context_t *ctx);

ionet_context_t * create_ionet_context(io_type_t io_type, int fd, int file_fd, struct ionet_server *server); 

void close_ionet_context(struct ionet_context *ctx);
void delete_ionet_context(ionet_context_t *ctx);

void add_request(struct ionet_context *ctx, int flags, int sqe_flags);

void change_io_type(struct ionet_context *ctx, io_type_t io_type);
