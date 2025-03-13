#ifndef LSP_H_
#define LSP_H_

#include <cjson/cJSON.h>

#include "common.h"

enum lspErrCode {
    RPC_ParseError = -32700,
    RPC_InvalidRequest = -32600,
    RPC_MethodNotFound = -32601,
    RPC_InvalidParams = -32602,
    RPC_InternalError = -32603,

    /**
     * Error code indicating that a server received a notification or
     * request before the server has received the `initialize` request.
     */
    ServerNotInitialized = -32002,
    UnknownErrorCode = -32001,

    /**
     * A request failed but it was syntactically correct, e.g the
     * method name was known and the parameters were valid. The error
     * message should contain human readable information about why
     * the request failed.
     */
    RequestFailed = -32803,

    /**
     * The server cancelled the request. This error code should
     * only be used for requests that explicitly support being
     * server cancellable.
     */
    ServerCancelled = -32802,

    /**
     * The server detected that the content of a document got
     * modified outside normal conditions. A server should
     * NOT send this error code if it detects a content change
     * in it unprocessed messages. The result even computed
     * on an older state might still be useful for the client.
     *
     * If a client decides that a result is not of any use anymore
     * the client should cancel the request.
     */
    ContentModified = -32801,

    /**
     * The client has canceled a request and a server has detected
     * the cancel.
     */
    RequestCancelled = -32800,
};

#define CLIENT_SUPP_COMPLETION (1 << 0)
#define CLIENT_SUPP_DOC_SYNC (1 << 1)
#define CLIENT_SUPP_SYNC_INCREMENTAL (1 << 2)
#define CLIENT_SUPP_SYNC_FULL (1 << 3)
#define CLIENT_SUPP_DOC_willSave (1 << 4)
#define CLIENT_SUPP_DOC_didSave (1 << 5)
#define CLIENT_SUPP_DOC_willSaveWaitUntil (1 << 6)

typedef struct LspClient {
    u32 capability;
    char *root_uri;
    u32 processID;
    bool initialized;
    bool shutdown_requested;
} LspClient;

typedef struct LspError {
    char *msg;
    int code;
} LspError;

typedef struct changeRange {
    size_t line;
    size_t pos;
} changeRange;

typedef struct DocChange {
    changeRange start;
    changeRange end;
    size_t range_len;
    char *text;
} DocChange;

typedef struct Document {
    char *uri;
    bool open;
    u64 version;
    char *text;
    DocChange **changes;
} Document;

typedef struct LspReply {
    char *header;
    char *msg;
    size_t msg_len;
} LspReply;

typedef struct LspState {
    LspClient client;
    bool has_err;
    bool has_msg;
    LspError error;
    Document **documents;
    DocChange **changes;
    LspReply reply;
} LspState;

int lsp_initialize(LspState *state, cJSON *message);
int lsp_initialized(LspState *state, cJSON *message);
int lsp_exit(LspState *state, cJSON *message);
int lsp_shutdown(LspState *state, cJSON *message);
int lsp_textDocument_didOpen(LspState *state, cJSON *message);
int lsp_textDocument_didChange(cJSON *message);
int lsp_textDocument_didClose(cJSON *message);
int lsp_textDocument_completion(cJSON *message);

#endif  // LSP_H_
