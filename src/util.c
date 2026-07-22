#include "util.h"

#include <sys/stat.h>

int power_of_two(int n, int limit) {
    if (n <= 0) return 1;
    if (limit <= 0) return 1;

    int v = 1;

    while (v < n) {
        if (v > (1 << 29)) break; 
        v <<= 1;
    }

    if (limit <= 0) return v;

    while (v > limit) {
        v >>= 1;
    }

    return v;
}

off_t get_file_size_stat(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0) {
        return st.st_size;
    }

    return -1;
}

size_t full_fread(void *ptr, size_t size, FILE *stream) {
    size_t total_read = 0;
    char *buf = (char *)ptr;

    while (total_read < size) {
        // fread(dest, size, count, stream)
        // 여기서는 size를 1로 고정하여 1바이트씩 count를 조절하며 읽음
        size_t n = fread(buf + total_read, 1, size - total_read, stream);
        
        if (n == 0) {
            // 더 이상 읽을 데이터가 없거나(EOF), 읽기 에러 발생
            break; 
        }
        
        total_read += n;
    }

    return total_read;
}
