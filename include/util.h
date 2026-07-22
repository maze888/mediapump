#include <stdio.h>
#include <stdlib.h>

#define safe_free(p) if (p) { free(p); p = NULL; }
#define safe_close(fd) if (fd >= 0) { close(fd); fd = -1; }
#define safe_fclose(fp) if (fp) { fclose(fp); fp = NULL; }
#define CKNUL(p) p ? "VALID" : "NULL"

int power_of_two(int n, int limit);
off_t get_file_size_stat(const char *filename);
size_t full_fread(void *ptr, size_t size, FILE *stream);
