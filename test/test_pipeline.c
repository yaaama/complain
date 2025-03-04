#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <string.h>

#include "../src/pipeline.h"

/* Test fixtures */
/* static void setup (void) {
 *     // Setup code that runs before each test
 * }
 *
 * static void teardown (void) {
 *     // Cleanup code that runs after each test
 * } */

/* Test parsing content length */
Test (pipeline_tests, test_content_length_parsing) {
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
Test (pipeline_tests, test_method_type_determination) {

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
Test (pipeline_tests, test_string_trimming) {
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

/* Test message reading from file */
Test (pipeline_tests, test_pipeline_read) {
    /* Create a temporary file with test content */
    FILE *temp = tmpfile();
    cr_assert_not_null(temp, "Failed to create temporary file");

    const char *test_content = "Content-Length: 13\r\n\r\nHello, World!";
    fputs(test_content, temp);
    fseek(temp, 0, SEEK_SET);

    msg_t message;
    int result = pipeline_read(temp, &message);

    cr_assert_eq(result, 0, "pipeline_read failed");
    cr_assert_eq(message.len, 13, "Incorrect message length");
    cr_assert_str_eq(message.content, "Hello, World!",
                     "Incorrect message content");

    free(message.content);
    fclose(temp);
}

/* Test dispatcher */
Test (pipeline_tests, test_pipeline_dispatcher_exit) {
    const char *json_str = "{\"method\":\"exit\", \"id\":1}";
    msg_t message = {0};
    message.len = strlen(json_str);
    message.content = calloc(sizeof(char), message.len + 1);
    strcpy(message.content, json_str);
    message.content[message.len] = '\0';

    int result = pipeline_dispatcher(stderr, &message, true);
    cr_assert_eq(result, 999, "Dispatcher failed for exit with a shutdown");

    result = pipeline_dispatcher(stderr, &message, false);
    cr_assert_eq(result, 1000, "Dispatcher failed for early exit method");

}

Test (pipeline_tests, test_pipeline_dispatcher_shutdown) {
    msg_t message;
    char *sdn_str = "{\"method\":\"shutdown\", \"id\":1}";
    char *exit_str = "{\"method\":\"exit\", \"id\":1}";
    char *any_str = "{\"method\":\"initialize\", \"id\":1}";
    char *curr_str = NULL;

    /* Shutdown method */
     curr_str = sdn_str;

    message.len = strlen(curr_str);
    message.content = calloc(sizeof(char), message.len + 1);
    strcpy(message.content, curr_str);
    message.content[message.len] = '\0';
    int result = pipeline_dispatcher(stderr, &message, false);
    cr_assert_eq(result, 998, "Dispatcher failed for shutdown method.");


    result = pipeline_dispatcher(stderr, &message, true);
    cr_assert_eq(result, 998, "Dispatcher failed for repeated shutdown method.");

    /* Exit Method */
    free(message.content);
    curr_str = exit_str;

    message.len = strlen(curr_str);
    message.content = calloc(sizeof(char), message.len + 1);
    strcpy(message.content, curr_str);
    message.content[message.len] = '\0';

    result = pipeline_dispatcher(stderr, &message, true);
    cr_assert_eq(result, 999, "Dispatcher failed to handle exit method whilst shutting down.");


    free(message.content);

    /* Any Method whilst in shutdown */
    curr_str = any_str;

    message.len = strlen(curr_str);
    message.content = calloc(sizeof(char), message.len + 1);
    strcpy(message.content, curr_str);
    message.content[message.len] = '\0';
    result = pipeline_dispatcher(stderr, &message, true);
    cr_assert_eq(result, 998, "Dispatcher failed for irrelevant method whilst shutting down.");

}


/* Test error cases */
Test (pipeline_tests, test_error_cases) {
    /* Test NULL file pointer */
    cr_assert_eq(init_pipeline(NULL,NULL), -1, "Should fail with NULL file pointer");

    /* Test NULL message */
    cr_assert_eq(pipeline_dispatcher(stderr, NULL, false), -1,
                 "Should fail with NULL message");

    /* Test pipeline_read with NULL parameters */
    msg_t msg;
    cr_assert_eq(pipeline_read(NULL, &msg), -1,
                 "Should fail with NULL file pointer");

    cr_assert_eq(pipeline_read(tmpfile(), NULL), -1,
                 "Should fail with NULL message struct");

    /* Test invalid JSON */
    /* msg_t message2;
     * const char *invalid_json = "{\"method\":\"invalid\", \"id\":3}";
     * strcpy(message2.content, invalid_json);
     * message2.len = strlen(invalid_json);
     * puts("Dispatcher try 2.");
     * int result2 = pipeline_dispatcher(&message2);
     * cr_expect_eq(result2, -1, "Dispatcher should fail for invalid method"); */
}
