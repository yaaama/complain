#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#define buffer_size 512

u64 pipeline_parse_content_len (char *text) {

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
    while (isspace(*beginning)) {
        ++beginning;
    }

    char *end;

    const int base = 10; /* We are searching in base 10 */
    i64 length = strtol(beginning, &end, base);

    /* Check for parsing errors/invalid length */
    if (beginning == end || length <= 0) {
        return 0;
    }

    while (isspace(*end)) {
        ++end;
    }

    /* If the end characters are neither of these, then something is wrong */
    if (*end != '\0' && *end != '\r' && *end != '\n') {
        return 0;
    }

    return length;
}

/* Searches for the break \r\n after the content length header */
bool pipeline_found_content_len (char *text) {
    const char *header_break = "\r\n";
    const char *newline = "\n";

    if (strcmp(header_break, text) == 0 || strcmp(newline, text) == 0) {
        return false;
    }

    return true;
}

int pipeline_read (FILE *to_read, char out[]) {


    /* We have received some data... */
    if (fgets(out, buffer_size, to_read) == NULL) {
        return -1;
    }

    if (!pipeline_found_content_len(out)) {
        return -1;
    }

    u64 content_len = pipeline_parse_content_len(out);

    if (!(content_len > 0)) {
        return -1;
    }

    char content[content_len + 1];

    u64 bytes = fread(content, 1, content_len, to_read);

    /* Check if we have read the correct size */
    if (bytes != content_len) {
      return -1;
    }

  return 0;

}

/* Initialise reading from FILE */
int init_pipeline (FILE *to_read) {

    /* Ensure we have something to read */
    if (!to_read) {
        return -1;
    }

  char out[buffer_size];

    while (true) {
      if (!pipeline_read(to_read, out)) {
        exit(1);
      }
    }
}
