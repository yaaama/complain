#include "common.h"

#include <assert.h>
#include <stdbool.h>

int xis_space (const char c) {
    return (c == ' ' || c == '\n' || c == '\f' || c == '\t' || c == '\r' ||
            c == '\v');
}

char *trim_leading_ws (char *str) {
    while (str && xis_space(*str)) {
        str++;
    }
    return str;
}

void trim_trailing_ws (char *str, u64 len) {

    assert(str && len > 0);

    if ((!str) || (len == 0)) {
        return;
    }

    char *end = str + len - 1;

    /* assert((str + len - 1) > (end)); */

    while ((end > str) && (xis_space(*end))) {
        --end;
    }
    *(end + 1) = '\0';
}
