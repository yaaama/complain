#include "pipeline.h"

#include <assert.h>
#include <cjson/cJSON.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>

#include "common.h"
#include "logging.h"
#include "lsp.h"

#define COMPLAIN_REQ_AFTER_SDN 998
#define COMPLAIN_GOOD_EXIT 999
#define COMPLAIN_EXIT_ABRUPT 1000

#ifdef NDEBUG
    #define buffer_size 1024
#else
    #define buffer_size 64
#endif

static void pipeline_send(FILE *dest, LspState *state);
static inline int is_header_break_line(char *line);
static inline int valid_message(msg_t *message);

/**
 * pipeline_parse_content_len
 * Parses a `Content-Length: x` string, and returns the number value x.
 *
 * Arguments: `char *`, is the string to be parsed.
 * Returns: `unsigned long long`, the Content-Length header value.
 **/
u64 pipeline_parse_content_len (char *text) {

    assert(text);

    log_debug("Parsing header from: `%s`", text);

    /* Searching for something like this: 'Content-Length: 12345' */
    const char *content_len_prefix = "Content-Length:";
    u32 prefix_len = strlen(content_len_prefix);

    /* When the text we are given does not match 'Content-Length:', exit with 0
    length. */
    if (strncmp(content_len_prefix, text, prefix_len) != 0) {
        return 0;
    }

    /* Skip over the 'Content-Length:' part and go to the number */
    char *beginning = text + prefix_len;

    /* Skip over leading whitespace */
    beginning = trim_leading_ws(beginning);

    char *end;

    const int base = 10; /* We are searching in base 10 */
    i64 length = strtol(beginning, &end, base);

    /* Check for parsing errors/invalid length */
    if (beginning == end || length <= 0) {
        return 0;
    }

    /* Skip over any trailing whitespace */
    while (xis_space(*end)) {
        ++end;
    }

    assert(*end == '\0');

    return length;
}

/**
 * is_header_break_line
 * Searches for `\r\n\r\n` within `char *`.
 *
 * Arguments: `char *` the string to search.
 * Returns: true if found, false otherwise.
 **/
static inline int is_header_break_line (char *line) {

    assert(line);

    const char *header_break = "\r\n";

    /* If it matches, return true */
    if (strcmp(header_break, (line)) == 0) {
        return true;
    }
    return false;
}

/**
 * pipeline_read
 * Will read from file descriptor indefinitely until it encounters a LSP
 headerbreak,
 * it will then return the contents read to the out parameter.
 *
 * Arguments: `FILE * to_read`, where to read from.
 *            `msg_t *out` , where contents read is stored.
 * Returns: Int 0 when success, -1 if failure.
 *
 * It is considered a failure if any of the parameters received are NULL,
   if the message length does not match with the `Content-Length` given, or if
   the content length given is invalid.
 **/
int pipeline_read (FILE *to_read, msg_t *out) {

    if (!to_read || !out) {
        log_debug("FILE or *out param is NULL.\n");
        return -1;
    }

    /* We keep the previous line because we are searching for the
    headerbreak which is on the line AFTER the Content-Length. */
    char prev_line[buffer_size] = {0};
    /* Current line we are reading */
    char line[buffer_size] = {0};

    while (true) {

        if (fgets(line, buffer_size, to_read) == NULL) {
            log_debug("Could not read from file.");
            return -1;
        }

        if (is_header_break_line(line)) {
            log_debug("Found header break!");
            break;
        }
        memcpy(prev_line, line, buffer_size);
    }

    /* Try parsing the content length number */
    u64 content_len = pipeline_parse_content_len(prev_line);
    log_debug("Content length: `%llu`", content_len);

    if (!(content_len > 0)) {
        log_debug("Content length not bigger than 0.");
        return -1;
    }

    out->content = malloc(sizeof(char) * (content_len + 1));

    /* Read content_len num of bytes from FILE and place into out param */
    u64 read_len = fread(out->content, 1, content_len, to_read);

    /* Check if we have read the correct size */
    if (read_len != content_len) {
        log_debug("Bytes read (`%llu`) does not match content length (`%llu`)",
                  read_len, content_len);
        free(out->content);
        return -1;
    }

    out->len = content_len;
    /* Null terminate */
    *((out->content) + content_len) = '\0';

    return 0;
}

int pipeline_determine_method_type (char *method_str) {

    /* When method str is null */
    if (!method_str) {
        return -1;
    }

    if (strcmp(method_str, "initialize") == 0) {
        return initialize;
    }
    if (strcmp(method_str, "initialized") == 0) {
        return initialized;
    }
    if (strcmp(method_str, "shutdown") == 0) {
        return shutdown;
    }
    if (strcmp(method_str, "exit") == 0) {
        return exit_;
    }
    if (strcmp(method_str, "textDocument/didOpen") == 0) {
        return textDocument_didOpen;
    }
    if (strcmp(method_str, "textDocument/didChange") == 0) {
        return textDocument_didChange;
    }
    if (strcmp(method_str, "textDocument/didClose") == 0) {
        return textDocument_didClose;
    }
    if (strcmp(method_str, "textDocument/completion") == 0) {
        return textDocument_completion;
    }
    return -1;
}

/* Checks for validity of a message. */
static inline int valid_message (msg_t *message) {

    if (!message) {
        log_debug("Uninitialised message struct!");
        return false;
    }
    if (!message->content) {
        log_debug("Uninitialised message content!");
        return false;
    }
    if (!message->len) {
        log_debug("Uninitialised message length!");
        return false;
    }

    return true;
}

/* Takes a message, and then acts on it. */
int pipeline_dispatcher (FILE *dest, msg_t *message, LspState *state) {

    /* assert(dest && message && state); */

    int return_val = 0;
    cJSON *json = NULL;

    if (!valid_message(message)) {
        log_warn("Invalid message, returning.");
        return_val = RPC_ParseError;
        goto pre_dispatch_error_cleanup;
    }

#if 0
    char *fresh_str = NULL;
    fresh_str = calloc(sizeof(char), message->len + 1);
    if (!fresh_str) {
        log_err("Failed to allocate mem. Exiting.");
        exit(-1);
    }

    memcpy(fresh_str, message->content, message->len);
    fresh_str[message->len] = '\0';

    /* Parse the string into JSON */
    json = cJSON_Parse(fresh_str);
    free(fresh_str);
#endif  // 0

    json = cJSON_Parse(message->content);

    /* Check if JSON has been parsed correctly */
    if (json == NULL) {
        log_err("Failed to create JSON object from message: `%s`",
                message->content);

        /* Get internal error pointer */
        const char *err_ptr = cJSON_GetErrorPtr();

        if (err_ptr == NULL) {
            log_err("Unknown cJSON error.");
            return_val = RPC_InternalError;
            goto pre_dispatch_error_cleanup;
        }

        log_warn("Invalid JSON. Error from: `%s`", err_ptr);
        return_val = RPC_ParseError;
        goto pre_dispatch_error_cleanup;
    }

    cJSON *method = cJSON_GetObjectItem(json, "method");

    if (!cJSON_IsString(method)) {
        log_debug("Could not retrieve `method` from JSON.");
        return_val = RPC_MethodNotFound;
        goto pre_dispatch_error_cleanup;
    }

    char *method_str = method->valuestring;

    int methodtype = pipeline_determine_method_type(method_str);
    if (methodtype < 0) {
        log_debug("Received unsupported method type: `%s`", method_str);
        return_val = RPC_InvalidRequest;
        goto pre_dispatch_error_cleanup;
    }

    /* Handle when we are shutting down or when we receive exit method */
    if (state->client.shutdown_requested == true) {
        if (methodtype != exit_) {
            log_info("Ignoring methods whilst waiting for 'exit'");
            return_val = COMPLAIN_REQ_AFTER_SDN;
            goto pre_dispatch_error_cleanup;
        }
        cJSON_Delete(json);
        return COMPLAIN_GOOD_EXIT;
    }

    /* Return value */
    int result = 0;

    log_info("Message type: `%s`", method_str);

    switch (methodtype) {

        case (initialize):
            result = lsp_initialize(state, json);
            break;
        case (initialized):
            result = lsp_initialized(state, json);
            break;
        case (textDocument_didOpen):
            result = lsp_textDocument_didOpen(state, json);
            break;
        case (textDocument_didChange):
            result = lsp_textDocument_didChange(json);
            break;
        case (textDocument_didClose):
            result = lsp_textDocument_didClose(json);
            break;
        case (textDocument_completion):
            result = lsp_textDocument_completion(json);
            break;
        case (shutdown):
            result = lsp_shutdown(state, json);
            break;
        case (exit_):
            result = lsp_exit(state, json);
            break;
        default:
            log_debug("Cannot handle method type: `%s`", method_str);
            result = -1;
            break;
    }

    if (state->has_err) {
        /* TODO Handle lsp errors here... */
        log_err("We have encountered an error!");
        /* COMPLAIN_TODO("Have not yet implemented lsp error handling yet."); */
    }

    if (state->has_msg) {
        pipeline_send(dest, state);
    }

    cJSON_Delete(json);

    return result;

pre_dispatch_error_cleanup:
    {
        cJSON_Delete(json);
        state->has_err = true;
        state->error.code = return_val;
        return return_val;
    }
}

static inline void pipeline_send (FILE *dest, LspState *state) {

    assert(state && state->reply.msg);
    log_debug("Sending message of length `%zu`:\n`%s`", state->reply.msg_len,
              state->reply.msg);

    /* log_debug("`%s%s`", state->reply.header, state->reply.msg); */
    fprintf(dest, "%s%s", state->reply.header, state->reply.msg);
    fflush(dest);
    state->reply.msg_len = 0;
    free(state->reply.header);
    free(state->reply.msg);
    state->has_msg = false;
}

cJSON *create_error_object (int client_msg_id, int err_code, char *message) {

    assert(message);

    /* Build error object */
    cJSON *errJSON = cJSON_CreateObject();
    cJSON_AddStringToObject(errJSON, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(errJSON, "code", err_code);

    if (message) {
        cJSON_AddStringToObject(errJSON, "message", message);
    }

    return errJSON;
}

void add_content_length_hdr (char *str, size_t len, char *out) {
    assert(str);
    assert(len > 0);

    char temp[buffer_size] = {0};
    int tempheader_size = 0;
    tempheader_size =
        snprintf(temp, buffer_size, "Content-Length: %zu\r\n\r\n", len);

    if (tempheader_size <= 0) {
        COMPLAIN_UNREACHABLE("temp header length is invalid... aborting.");
    }

    size_t message_size = (tempheader_size + len + 1);
    out = calloc(sizeof(char), message_size);
    int final_size = snprintf(out, message_size - 1, "%s%s", temp, str);

    if (final_size <= 0) {
        COMPLAIN_UNREACHABLE("Our final message is of an invalid size!");
    }
}

int handle_lsp_code (LspState *state, int lsp_result) {

    assert(state);

    int msgID = state->error.msg_id;
    int err_code = state->error.code;
    char *msg;

    switch (lsp_result) {

        /* TODO Fill these out with the appropiate error handling...
         Or perhaps move these into the default case and just print out the
         error message? They are meant to be in JSON FYI. */
        case (RPC_ParseError):
            {
                msg = "This message could not be parsed.";
                log_warn("Parsing error.");
                break;
            }

        case (RPC_InvalidRequest):
            {
                msg = "Invalid request made.";
                log_warn("Invalid request.");
                break;
            }

        case (RPC_MethodNotFound):
            {
                msg =
                    "Requested method could not be found or is not available.";
                log_warn("Method not found.");
                break;
            }
        case (RPC_InvalidParams):
            {
                msg = "Parameters received were invalid.";
                break;
            }
        case (RPC_InternalError):
            {
                msg = "Internal JSON RPC error.";
                break;
            }

        /* 998 is for when we are in shutdown mode and we receive a message
           with an irrelevant method  */
        case (998):
            {
                msg =
                    "Server has initialised shutdown, waiting for `exit` "
                    "method";
                break;
            }
        case (999):
            {
                msg = "Server is exiting successfully. Bye bye!";
                log_info("Exiting successfully");
                break;
            }
            /* Handling forceful exit request */
        case (1000):
            {
                log_err("Received abrupt exit request.\nBye bye.");
                exit(1);
                break;
            }
        default:
            {
                COMPLAIN_UNREACHABLE("We should never be able to reach this.");
                break;
            }
    }

    /* Build error object */
    cJSON *errJSON = cJSON_CreateObject();
    cJSON_AddStringToObject(errJSON, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(errJSON, "code", err_code);
    cJSON_AddStringToObject(errJSON, "message", msg);

    char *err_response = NULL;
    err_response = cJSON_Print(errJSON);

    if (!err_response) {
        log_err("Could not create error object... Exiting.");
        exit(-1);
    }

    state->reply.msg = err_response;
    state->reply.msg_len = strlen(err_response);

    char *final_msg = NULL;
    add_content_length_hdr(err_response, state->reply.msg_len, final_msg);

    fprintf(stdout, "%s", final_msg);

    cJSON_Delete(errJSON);
    free(err_response);
    free(final_msg);

    return 0;
}

/* Initialise reading from FILE */
int init_pipeline (FILE *to_read, FILE *to_send) {

    /* Ensure we have something to read */
    if (!to_read || !to_send) {
        log_err("Invalid FILE stream.");
        return -1;
    }

    msg_t message = {0};
    message.method = UNKNOWN;
    int io_result;
    int lsp_result;
    int await_shutdown = 0;

    LspState *state = NULL;
    state = calloc(sizeof(LspState), 1);
    if (!state) {
        log_err("Out of memory.");
        exit(-1);
    }
    state->client.shutdown_requested = false;
    state->client.initialized = false;
    state->has_err = false;

    while (true) {

        /* We read from standard input */
        io_result = pipeline_read(to_read, &message);

        /* Check for error */
        if (io_result < 0) {
            log_debug("Pipeline IO reading has failed, returning.");

            if (message.content) {
                free(message.content);
            }
            return -1;
        }

        log_debug("Content read: `%.24s [...]`\nContent-Length: `%llu`",
                  message.content, message.len);

        lsp_result = pipeline_dispatcher(to_send, &message, state);

        /* Free the content after processing */
        free(message.content);
        message.len = 0;

        if (lsp_result < 0) {
            log_err("Dispatcher finished with error code `%d`");
            handle_lsp_code(state, lsp_result);
        }
    }
}
