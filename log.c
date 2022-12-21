#include <stdio.h>
#include "log.h"

static struct {
    int level;
} L;

void log_printf(int level, const char *file, int line, const char *fmt, ...) {
    
}

void log_init(int level) {
    L.level = level;
}


#if 0 // test

int main(void) {
    log_debug("h");
    return 0;
}

#endif