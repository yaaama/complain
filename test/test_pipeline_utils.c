
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <string.h>

#include "../src/pipeline.h"

/* Test parsing content length */
Test (pipeline_utils, content_length_header) {
    char valid_header[] = "Content-Length: 123\r\n";
    char invalid_header[] = "Invalid-Header: 456\r\n";
    char empty_header[] = "";
    char malformed_header[] = "Content-Length: abc\r\n";
    char zero_length[] = "Content-Length: 0\r\n";

    cr_assert_eq(pipeline_parse_content_len(valid_header), 123,
                 "Failed to parse valid content length");
    cr_assert_eq(pipeline_parse_content_len(invalid_header), 0,
                 "Should return 0 for invalid header");
    cr_assert_eq(pipeline_parse_content_len(empty_header), 0,
                 "Should return 0 for empty header");
    cr_assert_eq(pipeline_parse_content_len(malformed_header), 0,
                 "Should return 0 for malformed header");
    cr_assert_eq(pipeline_parse_content_len(zero_length), 0,
                 "Should return 0 for zero length");
}

/* Test method type determination */
Test (pipeline_utils, determine_message_method) {

    char *valid_methods[] = {
        "exit",
        "initialize",
        "initialized",
        "shutdown",
        "textDocument/completion",
        "textDocument/didOpen",
        "textDocument/didChange",
        "textDocument/didClose",

    };

    enum method_type expected_types[] = {
        exit_,
        initialize,
        initialized,
        shutdown,
        textDocument_completion,
        textDocument_didOpen,
        textDocument_didChange,
        textDocument_didClose,
    };

    for (size_t i = 0; i < sizeof(valid_methods) / sizeof(valid_methods[0]);
         i++) {
        int result = pipeline_determine_method_type(valid_methods[i]);
        cr_assert_eq(result, expected_types[i],
                     "Method type determination failed for %s",
                     valid_methods[i]);
    }

    /* Invalid method */
    cr_assert_eq(pipeline_determine_method_type("invalid_method"), -1,
                 "Should return -1 for invalid method");

    /* Test NULL */
    cr_assert_eq(pipeline_determine_method_type(NULL), -1,
                 "Should return -1 for NULL input");
}

/* Test string trimming */
Test (pipeline_utils, string_trimming) {
    char str1[] = "  hello  ";
    char str2[] = "\t\ntest\r\n";
    char str3[] = "no_spaces";

    char *result1 = trim_leading_ws(str1);
    cr_assert_str_eq(result1, "hello  ",
                     "Leading whitespace not trimmed correctly");

    trim_trailing_ws(str1, strlen(str1));
    cr_assert_str_eq(str1, "  hello",
                     "Trailing whitespace not trimmed correctly");

    char *result2 = trim_leading_ws(str2);
    trim_trailing_ws(str2, strlen(str2));
    cr_assert_str_eq(result2, "test", "Mixed whitespace not trimmed correctly");

    char *result3 = trim_leading_ws(str3);
    trim_trailing_ws(str3, strlen(str3));
    cr_assert_str_eq(result3, "no_spaces",
                     "String without spaces modified incorrectly");
}
