#include <stdlib.h>

#define safe_free(p) if (p) { free(p); p = NULL; }
#define safe_close(fd) if (fd >= 0) { close(fd); fd = -1; }
#define CKNUL(p) p ? "VALID" : "NULL"

int power_of_two(int n, int limit);
