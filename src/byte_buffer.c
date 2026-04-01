#include "byte_buffer.h"
#include "logging.h"

#include <string.h>

struct byte_buffer * alloc_byte_buffer(size_t size)
{
    struct byte_buffer *bb = malloc(sizeof(struct byte_buffer));
    if (!bb) {
        LOG_CRIT("malloc(%zu) failed", sizeof(struct byte_buffer));
        abort();
    }

    bb->buf = malloc(size);
    if (!bb->buf) {
        LOG_CRIT("malloc(%zu) failed", size);
        abort();
    }
    bb->cap = size;

    bb->wbuf = bb->buf;
    bb->used = 0;

    return bb;
}

int realloc_byte_buffer(struct byte_buffer *bb, size_t size)
{
    if (!bb || size == 0) {
        LOG_ERROR("realloc_byte_buffer() failed: byte_buffer = %p, size = %zu", bb, size);
        return -1;
    }

    void *p = realloc(bb->buf, size);
    if (!p) {
        LOG_CRIT("realloc(%zu) failed. Original ptr: %p", size, bb->buf);
        abort(); 
    }

    bb->buf = p;
    bb->cap = size;

    bb->wbuf = bb->buf + bb->used;

    return 0;
}

void free_byte_buffer(struct byte_buffer **bb)
{
    if (bb && *bb) {
        safe_free((*bb)->buf);
        safe_free(*bb);
    }
}

size_t write_byte_buffer(struct byte_buffer *bb, void *src, size_t src_size)
{
    if (!bb || !src || src_size == 0) {
        LOG_ERROR("write_byte_buffer() failed: invalid argument: byte_buffer = %p, src = %p, src_size = %zu", bb, src, src_size);
        return 0;
    }

    if ((bb->cap - bb->used) < src_size) {
        if (realloc_byte_buffer(bb, bb->cap * 2) < 0) {
            LOG_ERROR("write_byte_buffer() failed: alloc error: byte_buffer = %p, src = %p, src_size = %zu", bb, src, src_size);
            return 0;
        }
    }

    memcpy(bb->wbuf, src, src_size);
    bb->wbuf += src_size;
    bb->used += src_size;

    return src_size;
}
