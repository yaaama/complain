#ifndef LSP_H_
#define LSP_H_

#include <cjson/cJSON.h>

char *lsp_initialize(cJSON *message);
int lsp_initialized(cJSON *message);
int lsp_exit(cJSON *message);
int lsp_shutdown(cJSON *message);
int lsp_textDocument_didOpen(cJSON *message);
int lsp_textDocument_didChange(cJSON *message);
int lsp_textDocument_didClose(cJSON *message);
int lsp_textDocument_completion(cJSON *message);

#endif  // LSP_H_
