#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <string.h>

#include "../src/lsp.h"
#include "../src/pipeline.h"

/* Test fixtures */
/* static void setup (void) {
 *     // Setup code that runs before each test
 * }
 *
 * static void teardown (void) {
 *     // Cleanup code that runs after each test
 * } */


/* Test message reading from file */
Test (pipeline_utils, pipeline_read) {
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
Test (pipeline_utils, pipeline_dispatcher_exit) {
    const char *json_str = "{\"method\":\"exit\", \"id\":1}";
    msg_t message = {0};
    message.len = strlen(json_str);
    message.content = calloc(sizeof(char), message.len + 1);
    strcpy(message.content, json_str);
    message.content[message.len] = '\0';

    LspState state;
    state.client.shutdown_requested = true;

    int result = pipeline_dispatcher(stderr, &message, &state);
    cr_assert_eq(result, 999, "Dispatcher failed for exit with a shutdown");

    state.client.shutdown_requested = false;

    result = pipeline_dispatcher(stderr, &message, &state);
    cr_assert_eq(result, 1000, "Dispatcher failed for early exit method");

    free(message.content);
}

Test (pipeline_utils, pipeline_dispatcher_shutdown) {
    msg_t message;
    char *sdn_str = "{\"method\":\"shutdown\", \"id\":1}";
    char *exit_str = "{\"method\":\"exit\", \"id\":1}";
    char *any_str = "{\"method\":\"initialize\", \"id\":1}";
    char *curr_str = NULL;

    /* Shutdown method */
    curr_str = sdn_str;

    LspState state;
    state.client.shutdown_requested = false;

    message.len = strlen(curr_str);
    message.content = calloc(sizeof(char), message.len + 1);
    strcpy(message.content, curr_str);
    message.content[message.len] = '\0';
    int result = pipeline_dispatcher(stderr, &message, &state);
    cr_assert_eq(result, 998, "Dispatcher failed for shutdown method.");

    result = pipeline_dispatcher(stderr, &message, &state);
    cr_assert_eq(result, 998,
                 "Dispatcher failed for repeated shutdown method.");

    /* Exit Method */
    free(message.content);
    curr_str = exit_str;

    message.len = strlen(curr_str);
    message.content = calloc(sizeof(char), message.len + 1);
    strcpy(message.content, curr_str);
    message.content[message.len] = '\0';

    state.client.shutdown_requested = true;
    result = pipeline_dispatcher(stderr, &message, &state);
    cr_assert_eq(
        result, 999,
        "Dispatcher failed to handle exit method whilst shutting down.");

    free(message.content);

    /* Any Method whilst in shutdown */
    curr_str = any_str;

    message.len = strlen(curr_str);
    message.content = calloc(sizeof(char), message.len + 1);
    strcpy(message.content, curr_str);
    message.content[message.len] = '\0';
    result = pipeline_dispatcher(stderr, &message, &state);
    cr_assert_eq(
        result, 998,
        "Dispatcher failed for irrelevant method whilst shutting down.");

    free(message.content);
}

/* Test error cases */
Test (pipeline_utils, test_error_cases) {
    /* Test NULL file pointer */
    cr_assert_eq(init_pipeline(NULL, NULL), -1,
                 "Should fail with NULL file pointer");

    LspState state;
    int err = RPC_ParseError;

    /* Test NULL message */
    cr_assert_eq(pipeline_dispatcher(stderr, NULL, &state), err,
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
     * cr_expect_eq(result2, -1, "Dispatcher should fail for invalid method");
     */
}
