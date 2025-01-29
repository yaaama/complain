#include "logging.h"
#include "pipeline.h"
#include <stdio.h>


/* Main entry point */
int main (void) {

    log_info("Program has started.");
    init_pipeline(stdin);

    return 0;
}
