#include "byte_buffer.h"
#include "logging.h"

struct byte_buffer * alloc_byte_buffer(size_t len)
{
    struct byte_buffer *bb = malloc(sizeof(struct byte_buffer));
    if (!bb) {
        LOG_CRIT("malloc(%zu) failed", sizeof(struct byte_buffer));
        abort();
    }

    bb->buf = malloc(len);
    if (!bb->buf) {
        LOG_CRIT("malloc(%zu) failed", len);
        abort();
    }
    bb->len = len;

    return bb;
}

void realloc_byte_buffer(struct byte_buffer *bb, size_t len)
{
    if (!bb || len == 0) {
        LOG_ERROR("realloc_byte_buffer() failed: byte_buffer = %p, len = %zu", bb, len);
        return;
    }

    void *tmp = realloc(bb->buf, len);

    if (!tmp) {
        LOG_CRIT("realloc(%zu) failed. Original ptr: %p", len, bb->buf);
        abort(); 
    }

    bb->buf = tmp;
    bb->len = len;
}

void free_byte_buffer(struct byte_buffer *bb)
{
    if (bb) {
        safe_free(bb->buf);
        safe_free(bb);
    }
}
