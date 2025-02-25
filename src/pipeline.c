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

#define buffer_size 512

/* Parse Content Length
 * ********************************************************/
/* Takes a Content Length header line, and tries to parse the length value from
it.  */
u64 pipeline_parse_content_len (char *text) {

    log_debug("Parsing content length from string `%s`", text);

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
bool is_header_break_line (char *line) {
    const char *header_break = "\r\n";

    if (strcmp(header_break, (line)) == 0) {
        return true;
    }
    return false;
}

/* Reads from fd and assigns the message to *out */
int pipeline_read (FILE *to_read, msg_t *out) {

    if (!to_read) {
        log_debug("File to read is NULL.");
        return -1;
    }

    if (!out) {
        log_debug("output struct is NULL.");
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

/* Takes a message, and then acts on it. */
int pipeline_dispatcher (msg_t *message) {

    if (!message) {
        log_debug("Uninitialised message struct, returning.");
        return -1;
    }
    if (!message->content) {
        log_debug("Uninitialised message content, returning.");
        return -1;
    }
    if (!message->len) {
        log_debug("Bad message length, returning.");
        return -1;
    }

    cJSON *content_json = cJSON_ParseWithLength(message->content, message->len);

    if (!content_json) {
        log_debug("Failed to create JSON object from message: `%s`",
                  message->content);
        return -1;
    }

    cJSON *method = cJSON_GetObjectItem(content_json, "method");
    if (!method) {
        log_debug("Could not retrieve `method` item from JSON.");
        cJSON_Delete(content_json);
        return -1;
    }
    char *method_str = cJSON_GetStringValue(method);

    if (!method_str) {
        log_debug("Method string is not initialised.");
        cJSON_Delete(content_json);
        return -1;
    }

    int methodtype = pipeline_determine_method_type(method_str);
    if (methodtype < 0) {
        log_debug("Received unsupported method type: `%s`", method_str);
        cJSON_Delete(content_json);
        return -1;
    }

    int result = -1;

    log_debug("Message type: `%s`", method_str);

    switch (methodtype) {

        case (exit_):
            log_warn("exit has not been implemented yet.");
            result = 0;
            break;
        default:
            log_debug("Cannot handle method type: `%s`", method_str);
    }

    if (content_json) {
        cJSON_Delete(content_json);
    }
    return result;
}

/* Initialise reading from FILE */
int init_pipeline (FILE *to_read) {

    /* Ensure we have something to read */
    if (!to_read) {
        return -1;
    }

    msg_t message;
    int result;

    while (true) {

        result = pipeline_read(to_read, &message);

        if (result != 0) {
            log_debug("Pipeline reading has failed, returning.");
            return -1;
        }

        log_debug("Content read: `%s`", message.content);
        log_debug("Length: `%llu`", message.len);

        log_debug("Passing message to dispatcher.");
        result = pipeline_dispatcher(&message);

        if (message.content) {
            free(message.content); /* Free the content after processing */
        }

        if (result != 0) {
            fclose(to_read);
            return result;
        }
    }
}
