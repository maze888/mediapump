#include <stdlib.h>

#define safe_free(p) if (p) { free(p); p = NULL; }
