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

#ifdef NDEBUG
    #define buffer_size 1024
#else
    #define buffer_size 64
#endif

static void pipeline_send(FILE *dest, char *msg);
static inline bool is_header_break_line(char *line);
static inline bool valid_message(msg_t *message);

/* Parse Content Length
 * ********************************************************/
/* Takes a Content Length header line, and tries to parse the length value from
it.  */
u64 pipeline_parse_content_len (char *text) {

    assert(text);

    log_debug("Parsing content length from string:\n`%s`", text);

    /* Searching for something like this: 'Content-Length: 12345' */
    const char *content_len_prefix = "Content-Length:";
    u32 prefix_len = strlen(content_len_prefix);

    /* When the text we are given does not match 'Content-Length:' */
    if (strncmp(content_len_prefix, text, prefix_len) != 0) {
        return 0;
    }

    /* Skip over the 'Content-Length:' part and go to the number */
    char *beginning = text + prefix_len;

    /* Skip over whitespace */
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

    /* If the end characters are neither of these, then something is wrong */
    if (*end != '\0' && *end != '\r' && *end != '\n') {
        return 0;
    }

    /* printf("Returning length: `%lld`\n", length); */
    return length;
}

/* Searches for the break \r\n */
static inline bool is_header_break_line (char *line) {

    assert(line);

    if (!line) {
        log_warn("Received NULL ptr");
        return false;
    }

    const char *header_break = "\r\n";

    if (strcmp(header_break, (line)) == 0) {
        return true;
    }
    return false;
}

/* Reads from fd and assigns the message to *out */
int pipeline_read (FILE *to_read, msg_t *out) {

    if (!to_read) {
        log_debug("File to read is NULL.\n");
        return -1;
    }

    if (!out) {
        log_debug("message struct is NULL.\n");
        return -1;
    }

    char line[buffer_size];
    char prev_line[buffer_size];

    while (true) {

        if (fgets(line, buffer_size, to_read) == NULL) {
            return 0;
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
static inline bool valid_message (msg_t *message) {

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

/* Determines whether we are exiting gracefully or abruptly, or still waiting
for a 'exit' method. */
static inline int shutdown_retcode (bool sdn, int method_type) {

    /* Successful exit */
    if ((sdn == true) && (method_type == exit_)) {
        log_info("Preparing to now gracefully exit.");
        return 999;
    }
    if ((sdn == true) && (method_type != exit_)) {
        log_info(
            "Ignoring all methods except `exit` whilst in shutdown state.");
        return 998;
    }
    if ((sdn == false) && (method_type == exit_)) {
        log_info(
            "Exiting abruptly due to `exit` method received. Next time "
            "send shutdown request first.");
        return 1000;
    }

    /* Else just return -1 */
    return -1;
}

/* Takes a message, and then acts on it. */
int pipeline_dispatcher (FILE *dest, msg_t *message,
                         LspState *state) {

    if (!valid_message(message)) {
        log_warn("Invalid message, returning.");
        return -1;
    }

    log_info("Message length: `%d`\nMessage:\n`%s`", message->len,
             message->content);
    cJSON *json = cJSON_ParseWithLength(message->content, message->len);

    /* Check if JSON has been parsed correctly */
    if (json == NULL) {
        log_err("Failed to create JSON object from message: `%s`",
                message->content);
        const char *err_ptr = cJSON_GetErrorPtr();

        if (!err_ptr) {
            log_err("Error before: `%s`", err_ptr);
        }
        cJSON_Delete(json);
        return -1;
    }

    cJSON *method = cJSON_GetObjectItem(json, "method");

    if (!cJSON_IsString(method)) {
        log_debug("Could not retrieve `method` item from JSON.");
        cJSON_Delete(json);
        return -1;
    }

    char *method_str = cJSON_GetStringValue(method);

    if (!method_str) {
        log_debug("Method string is not initialised.");
        cJSON_Delete(json);
        return -1;
    }

    int methodtype = pipeline_determine_method_type(method_str);
    if (methodtype < 0) {
        log_debug("Received unsupported method type: `%s`, returning.",
                  method_str);
        cJSON_Delete(json);
        return -1;
    }

    /* Handle when we are shutting down or when we receive exit method */
    if (state->client.shutdown_requested == true) {
        if (methodtype != exit_) {
            cJSON_Delete(json);
            log_info("Ignoring methods whilst waiting for 'exit'");
            return 998;
        }
    }

    message->method = methodtype;

    /* Return value */
    int result = 0;

    log_info("Message type: `%s`", method_str);
    /* Our response, if we have any. */
    char *response = NULL;

    switch (message->method) {

        case (initialize):
            result = lsp_initialize(state, json);
            break;
        case (initialized):
            lsp_initialized(state, json);
            break;
        case (textDocument_didOpen):
            lsp_textDocument_didOpen(state, json);
            break;
        case (textDocument_didChange):
            lsp_textDocument_didChange(json);
            break;
        case (textDocument_didClose):
            lsp_textDocument_didClose(json);
            break;
        case (textDocument_completion):
            lsp_textDocument_completion(json);
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
        COMPLAIN_TODO("Have not yet implemented lsp error handling yet.");
    }


    if (result == 0) {
        response = state->reply.msg;
        pipeline_send(dest, response);
        free(state->reply.msg);
        state->reply.len = 0;
    }

    if (json) {
        cJSON_Delete(json);
    }
    return result;
}

static inline void pipeline_send (FILE *dest, char *msg) {

    /* Sometimes we don't need to send a message */
    if (!msg || !dest) {
        log_debug("NULL msg or invalid destination stream.");
        return;
    }

    log_debug("Sending message:\n`%s`", msg);
    (void) fprintf(dest, "%s", msg);
    (void) fflush(dest);
}

/* Initialise reading from FILE */
int init_pipeline (FILE *to_read, FILE *to_send) {

    /* Ensure we have something to read */
    if (!to_read || !to_send) {
        log_err("Invalid FILE stream.");
        return -1;
    }

    msg_t message;
    message.method = UNKNOWN;
    int io_result;
    int lsp_result;
    int await_shutdown = 0;

    LspState *state = malloc(sizeof(LspState));

    while (true) {

        io_result = pipeline_read(to_read, &message);

        if (io_result < 0) {
            log_debug("Pipeline IO reading has failed, returning.");

            if (message.content) {
                free(message.content);
            }
            return -1;
        }

        log_debug("Content read: `%s`", message.content);
        log_debug("Length: `%llu`", message.len);

        log_debug("Passing message to dispatcher.");
        lsp_result =
            pipeline_dispatcher(to_send, &message, state);

        if (message.content) {
            free(message.content); /* Free the content after processing */
        }

        if (lsp_result < 0) {
            log_err("Dispatcher has encountered a problem, returning.");
            return lsp_result;
        }

        /* React to return codes */
        switch (lsp_result) {

            /* Default value for success */
            case (0):
                {
                    break;
                }

            /* 998 is for when we are in shutdown mode and we receive a message
               with an irrelevant method  */
            case (998):
                {
                    if (await_shutdown) {
                        break;
                    }
                    await_shutdown = 1;
                    break;
                }

                /* Handling exit requests... */
                /* We are on a path to exiting so lets close our streams */
                (void) fflush(to_read);
                (void) fflush(to_send);
                (void) fclose(to_read);
                (void) fclose(to_send);
            case (999):
                {
                    log_info("Exiting successfully.\nBye bye.");
                    return 0;
                    break;
                }
                /* Handling forceful exit request */
            case (1000):
                {
                    log_err("Received abrupt exit request.\nBye bye.");
                    return -1;
                    break;
                }
            default:
                {
                    COMPLAIN_UNREACHABLE(
                        "We should never be able to reach this.");
                    break;
                }
        }
    }
}
