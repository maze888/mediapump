#pragma once

#include "types.h"

struct byte_buffer {
    void *buf;
    size_t len;
};

struct byte_buffer * alloc_byte_buffer(size_t len);
struct byte_buffer * realloc_byte_buffer(struct byte_buffer *bb, size_t len);
void free_byte_buffer(struct byte_buffer *bb);


