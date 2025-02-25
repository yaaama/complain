#ifndef PIPELINE_H_
#define PIPELINE_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "common.h"

typedef struct msg_t {
    char *content;
    uint64_t len;
} msg_t;

enum method_type {
    initialize,
    initialized,
    textDocument_didOpen,
    textDocument_completion,
    textDocument_didChange,
    textDocument_didClose,
    shutdown,
    exit_,
};

// Function declarations
u64 pipeline_parse_content_len(char *text);
int pipeline_read(FILE *to_read, msg_t *out);
int pipeline_determine_method_type(char *method_str);
int pipeline_dispatcher(msg_t *message);
int init_pipeline(FILE *to_read);

#endif  // PIPELINE_H_
