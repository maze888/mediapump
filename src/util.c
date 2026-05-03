#include "util.h"

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
