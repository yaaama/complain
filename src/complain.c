#include "logging.h"
#include "pipeline.h"

#undef NDEBUG

/* Main entry point */
int main (void) {
    log_init_file("log.txt");
    log_info("We've begun!");
    init_pipeline(stdin, stdout);

    return 0;
}
