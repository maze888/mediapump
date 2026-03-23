#include "logging.h"

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    LOG_CRIT("good....");
    LOG_CRIT("hi there %d", 13);
    LOG_ERROR("hi there %s", "good man");
    LOG_INFO("hi there %s %d", "good man", 256);
    LOG_DEBUG("hi there %s %d %d %s", "good man", 256, 512, "world");

    return 0;
}
