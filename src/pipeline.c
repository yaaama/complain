#include "pipeline.h"

#include <assert.h>
#include <cjson/cJSON.h>
#include <ctype.h>
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


/* Parse Content Length ********************************************************/
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
    while (isspace(*end)) {
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

int pipeline_read (FILE *to_read, msg_t *out) {

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

    u64 read_len = fread(out->content, 1, content_len, to_read);

    /* Check if we have read the correct size */
    if (read_len != content_len) {
        log_debug("Bytes read (`%llu`) does not match content length (`%llu`)",
                  read_len, content_len);
        return -1;
    }

    out->len = content_len;
    *((out->content) + content_len) = '\0';

    return 0;
}


/* Lookup table entry */
struct method_entry {
    const char *str;
    enum method_type type;
    unsigned int hash;
};

int compare_method_entries (const void *a, const void *b) {
    const struct method_entry *entry_a = (const struct method_entry *)a;
    const struct method_entry *entry_b = (const struct method_entry *)b;

    if (entry_a->hash < entry_b->hash) {
        return -1;
    }
    if (entry_a->hash > entry_b->hash) {
        return 1;
    }
    return 0;
}

/* Static lookup table - sorted by hash for binary search */
static struct method_entry method_table[] = {
    {"exit", exit_, 0},
    {"initialise", initialise, 0},
    {"initialised", initialised, 0},
    {"shutdown", shutdown, 0},
    {"textDocument_completion", textDocument_completion, 0},
    {"textDocument_didOpen", textDocument_didOpen, 0}};

static bool is_initialised = false;

/* Initialize hashes when program starts */
void init_method_table (void) {

    if (is_initialised) {
        return;
    }

    for (size_t i = 0; i < ARRAY_LENGTH(method_table); i++) {
        method_table[i].hash = hash_string(method_table[i].str);
    }
    qsort(method_table, ARRAY_LENGTH(method_table), sizeof(struct method_entry),
          compare_method_entries);
    is_initialised = true;
}

/* Retrieves method */
int pipeline_determine_method_type (char *method_str) {

    u32 hash = hash_string(method_str);

    /* Binary search in the table */
    s32 left = 0;
    s32 right = ARRAY_LENGTH(method_table) - 1;

    while (left <= right) {
        assert(left < INT_MAX - right);
        s32 mid = (left + right) / 2;
        assert(mid < INT_MAX && mid >= 0);

        if (hash < method_table[mid].hash) {
            right = mid - 1;
        } else if (hash > method_table[mid].hash) {
            left = mid + 1;
        } else {
            /* Hash matches - verify string */
            if (strcmp(method_str, method_table[mid].str) == 0) {
                return method_table[mid].type;
            }
            /* Hash collision - search linearly from here */
            u32 i = mid - 1;
            while (i >= 0 && method_table[i].hash == hash) {
                if (strcmp(method_str, method_table[i].str) == 0) {
                    return method_table[i].type;
                }
                --i;
            }
            i = mid + 1;
            while (i < ARRAY_LENGTH(method_table) &&
                   method_table[i].hash == hash) {
                if (strcmp(method_str, method_table[i].str) == 0) {
                    return method_table[i].type;
                }
                ++i;
            }
            return -1; /* No match found */
        }
    }
    return -1; /* No match found */
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
        goto fail_cleanup;
    }
    char *method_str = cJSON_GetStringValue(method);

    if (!method_str) {
        log_debug("Method string is not initialised.");
        goto fail_cleanup;
    }

    u32 methodtype = pipeline_determine_method_type(method_str);
    int result = 0;

    log_debug("Message type: `%s`", method_table[methodtype]);

    switch (methodtype) {

        case (exit_):
            break;
        default:
            log_debug("Cannot handle method type: `%s`", method_str);
            goto fail_cleanup;
    }

fail_cleanup:
    {
        cJSON_Delete(content_json);
        result = -1;
    }

    return result;
}

/* Initialise reading from FILE */
int init_pipeline (FILE *to_read) {

    /* Ensure we have something to read */
    if (!to_read) {
        return -1;
    }

    init_method_table();

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

        free(message.content); /* Free the content after processing */

        if (result != 0) {
            return result;
        }
    }
}
