#ifndef PIPELINE_H_
#define PIPELINE_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "lsp.h"

typedef enum method_type {
    UNKNOWN = 0,
    LSP_INITIALIZE,
    LSP_INITIALIZED,
    LSP_TEXTDOC_DIDOPEN,
    LSP_TEXTDOC_COMPLETION,
    LSP_TEXTDOC_DIDCHANGE,
    LSP_TEXTDOC_DIDCLOSE,
    LSP_SHUTDOWN,
    LSP_EXIT_,
} method_type;

typedef struct msg_t {
    char *content;
    uint64_t len;
    method_type method;
} msg_t;

/* Function declarations */
u64 pipeline_parse_content_len(char *text);
int pipeline_read(FILE *to_read, msg_t *out);
int pipeline_determine_method_type(char *method_str);
int pipeline_dispatcher(FILE *dest, msg_t *message, LspState *state);
int init_pipeline(FILE *to_read, FILE *to_send);

#endif  // PIPELINE_H_
