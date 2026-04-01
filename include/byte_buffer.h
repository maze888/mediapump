#pragma once

#include "types.h"

struct byte_buffer {
    void *buf;
    size_t cap;

    unsigned char *wbuf;
    size_t used;
};

struct byte_buffer * alloc_byte_buffer(size_t len);
int realloc_byte_buffer(struct byte_buffer *bb, size_t size);
void free_byte_buffer(struct byte_buffer **bb);

size_t write_byte_buffer(struct byte_buffer *bb, void *buf, size_t len);


