#include <stdlib.h>

#include "logging.h"
#include "pipeline.h"

#undef NDEBUG

/* Main entry point */
int main (void) {
    log_init_file("log.txt");
    log_info("We've begun!");
    int ret_code = init_pipeline(stdin, stdout);
    if (ret_code) {
        fprintf(stderr, "We have experienced an error.");
        exit(ret_code);
    };

    return 0;
}
