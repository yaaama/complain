#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#define buffer_size 512

typedef struct msg_t {
    char *content;
    u64 len;
} msg_t;

static inline void trim_leading_ws(char *str);
static inline void trim_trailing_ws(char *str, u64 len);

static inline void trim_leading_ws (char *str) {
    while (isspace(*str)) {
        ++str;
    }
}

static inline void trim_trailing_ws (char *str, u64 len) {
    char *end = str + len;

    while ((end > str) && (isspace(*end))) {
        --end;
    }
    *end = '\0';
}

u64 pipeline_parse_content_len (char *text) {

    /* printf("Parsing content length from: `%s`\n", text); */

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
    trim_leading_ws(beginning);

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
    printf("pipeline_read\n");

    char line[buffer_size];
    char prev_line[buffer_size];

    while (true) {

        if (fgets(line, buffer_size, to_read) == NULL) {
            return 0;
        }

        if (is_header_break_line(line)) {
            puts("Found header break!");
            break;
        }

        memcpy(prev_line, line, buffer_size);
    }

    printf("Previous line is: `%s`\n", prev_line);
    printf("Curr Line: `%s`\n", line);

    /* Try parsing the content length number */
    u64 content_len = pipeline_parse_content_len(prev_line);
    printf("Content length: `%llu`\n", content_len);

    if (!(content_len > 0)) {
        puts("Content length not bigger than 0.");
        return -1;
    }

    out->content = malloc(sizeof (char) * (content_len + 1));

    u64 read_len = fread(out->content, 1, content_len, to_read);

    /* Check if we have read the correct size */
    if (read_len != content_len) {
        printf("Bad content length.\n");
        return -1;
    }

    out->len = content_len;
    *((out->content) + content_len) = '\0';

    return 0;
}

/* Initialise reading from FILE */
int init_pipeline (FILE *to_read) {

    /* Ensure we have something to read */
    if (!to_read) {
        return -1;
    }

    msg_t out;

    while (true) {
        if (!pipeline_read(to_read, &out)) {
            return -1;
        }
        printf("Content: `%s`", (out.content));
        printf("Length: `%llu`\n", out.len);
    }
}


#include <assert.h>

void test_pipeline_parse_content_len (void) {
    // Test valid cases
    assert(pipeline_parse_content_len("Content-Length: 123") == 123);
    assert(pipeline_parse_content_len("Content-Length:456") == 456);
    assert(pipeline_parse_content_len("Content-Length:   789") == 789);

    // Test invalid cases
    assert(pipeline_parse_content_len("Wrong-Header: 123") == 0);
    assert(pipeline_parse_content_len("Content-Length: abc") == 0);
    assert(pipeline_parse_content_len("Content-Length: -123") == 0);
    assert(pipeline_parse_content_len("Content-Length: 123abc") == 0);
}

void test_pipeline_found_content_len (void) {
    assert(is_header_break_line("\r\n"));
    assert(is_header_break_line("\n") == false);
    assert(is_header_break_line("Content-Length: 123") == false);
}

void test_pipeline_read (void) {
    // Create a temporary file for testing
    FILE *temp = tmpfile();
    if (!temp) {
        return;
    }

    // Write test data
    fprintf(temp, "Content-Length: 13\r\n\r\nHello, World!\r\n");
    rewind(temp);

    msg_t msg;

    assert(pipeline_read(temp, &msg) == 0);
    printf("Msg Len: %llu\n", msg.len);
    assert(msg.len == 13);
    assert(strncmp(msg.content, "Hello, World!", 13) == 0);

    free(msg.content);
    fclose(temp);
}
int main (void) {

    test_pipeline_parse_content_len();
    test_pipeline_found_content_len();
    test_pipeline_read();

    printf("All tests passed!\n");
    return 0;
}
