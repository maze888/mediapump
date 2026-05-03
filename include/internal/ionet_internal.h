#pragma once

#include "ionet.h"

ionet_context_t * create_ionet_context(io_type_t io_type, int fd, int file_fd, struct ionet_server *server); 
void delete_ionet_context(struct ionet_context **ctx);
void free_ionet_context(struct ionet_context **ctx);

const char * get_io_type_string(io_type_t io_type);
void print_ionet_context(const char *context_string, ionet_context_t *ctx);
