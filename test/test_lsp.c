#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <string.h>

#include "../src/lsp.h"
#include "../src/pipeline.h"

Test (test_lsp, test_initialize) {

    msg_t* initialize = malloc(sizeof(msg_t));
    char* content =
        "{\n"
        "  \"jsonrpc\": \"2.0\",\n"
        "  \"id\": 1,\n"
        "  \"method\": \"initialize\",\n"
        "  \"params\": {\n"
        "    \"processId\": 12345,\n"
        "    \"rootUri\": \"file:///home/user/project\",\n"
        "    \"capabilities\": {\n"
        "      \"textDocument\": {\n"
        "        \"completion\": {\n"
        "          \"dynamicRegistration\": true,\n"
        "          \"completionItem\": {\n"
        "            \"snippetSupport\": true\n"
        "          }\n"
        "        }\n"
        "      }\n"
        "    }\n"
        "  }\n"
        "}";

    size_t content_len = strlen(content);
    initialize->content = malloc(content_len + 1);
    if (initialize->content == NULL) {
        /* Handle allocation failure */
        free(initialize);
        return;
    }

    strcpy(initialize->content, content);
    initialize->len = strlen(initialize->content);
    LspState state;
    state.client.shutdown_requested = false;

    pipeline_dispatcher(stdout, initialize, &state);

    free(initialize->content);
    free(initialize);
    free(state.client.root_uri);
}


Test (test_lsp, test_initialize_w_sync) {

    const char* content =
        "{\n"
        "    \"jsonrpc\": \"2.0\",\n"
        "    \"id\": 1,\n"
        "    \"method\": \"initialize\",\n"
        "    \"params\": {\n"
        "        \"processId\": 12345,\n"
        "        \"clientInfo\": {\n"
        "            \"name\": \"Visual Studio Code\",\n"
        "            \"version\": \"1.60.0\"\n"
        "        },\n"
        "        \"rootPath\": \"/home/user/project\",\n"
        "        \"rootUri\": \"file:///home/user/project\",\n"
        "        \"capabilities\": {\n"
        "            \"workspace\": {\n"
        "                \"applyEdit\": true,\n"
        "                \"didChangeConfiguration\": {\n"
        "                    \"dynamicRegistration\": true\n"
        "                },\n"
        "                \"didChangeWatchedFiles\": {\n"
        "                    \"dynamicRegistration\": true\n"
        "                }\n"
        "            },\n"
        "            \"textDocument\": {\n"
        "                \"synchronization\": {\n"
        "                    \"willSave\": true,\n"
        "                    \"willSaveWaitUntil\": true,\n"
        "                    \"didSave\": true,\n"
        "                    \"change\": 2\n"
        "                },\n"
        "                \"completion\": {\n"
        "                    \"dynamicRegistration\": true,\n"
        "                    \"completionItem\": {\n"
        "                        \"snippetSupport\": true,\n"
        "                        \"commitCharactersSupport\": true,\n"
        "                        \"documentationFormat\": [\"markdown\", "
        "\"plaintext\"],\n"
        "                        \"deprecatedSupport\": true\n"
        "                    },\n"
        "                    \"contextSupport\": true\n"
        "                }\n"
        "            }\n"
        "        },\n"
        "        \"trace\": \"verbose\"\n"
        "    }\n"
        "}";

    msg_t* initialize = malloc(sizeof(msg_t));
    initialize->content = malloc(strlen(content) + 10);
    if (initialize->content == NULL) {
        /* Handle allocation failure */
        free(initialize);
        return;
    }

    strcpy(initialize->content, content);
    initialize->len = strlen(initialize->content);
    LspState state;
    state.client.shutdown_requested = false;

    pipeline_dispatcher(stdout, initialize, &state);

    free(initialize->content);
    free(initialize);
    free(state.client.root_uri);
    free(state.error.msg);
}
Test (test_lsp, test_initialized) {

    const char* content =
        "{\n"
        "    \"jsonrpc\": \"2.0\",\n"
        "    \"id\": 1,\n"
        "    \"method\": \"initialize\",\n"
        "    \"params\": {\n"
        "        \"processId\": 12345,\n"
        "        \"clientInfo\": {\n"
        "            \"name\": \"Visual Studio Code\",\n"
        "            \"version\": \"1.60.0\"\n"
        "        },\n"
        "        \"rootPath\": \"/home/user/project\",\n"
        "        \"rootUri\": \"file:///home/user/project\",\n"
        "        \"capabilities\": {\n"
        "            \"workspace\": {\n"
        "                \"applyEdit\": true,\n"
        "                \"didChangeConfiguration\": {\n"
        "                    \"dynamicRegistration\": true\n"
        "                },\n"
        "                \"didChangeWatchedFiles\": {\n"
        "                    \"dynamicRegistration\": true\n"
        "                }\n"
        "            },\n"
        "            \"textDocument\": {\n"
        "                \"synchronization\": {\n"
        "                    \"willSave\": true,\n"
        "                    \"willSaveWaitUntil\": true,\n"
        "                    \"didSave\": true,\n"
        "                    \"change\": 2\n"
        "                },\n"
        "                \"completion\": {\n"
        "                    \"dynamicRegistration\": true,\n"
        "                    \"completionItem\": {\n"
        "                        \"snippetSupport\": true,\n"
        "                        \"commitCharactersSupport\": true,\n"
        "                        \"documentationFormat\": [\"markdown\", "
        "\"plaintext\"],\n"
        "                        \"deprecatedSupport\": true\n"
        "                    },\n"
        "                    \"contextSupport\": true\n"
        "                }\n"
        "            }\n"
        "        },\n"
        "        \"trace\": \"verbose\"\n"
        "    }\n"
        "}";

    msg_t* initialize = malloc(sizeof(msg_t));
    initialize->content = malloc(strlen(content) + 10);
    if (initialize->content == NULL) {
        /* Handle allocation failure */
        free(initialize);
        return;
    }

    strcpy(initialize->content, content);
    initialize->len = strlen(initialize->content);
    LspState state = {0};
    state.client.shutdown_requested = false;

    fprintf(stderr, "Sendine initialize method!\n");
    /* Send our initialise method */
    pipeline_dispatcher(stdout, initialize, &state);


    free(initialize->content);
    free(initialize);

    char *initialized_msg =  "{\n  \"jsonrpc\": \"2.0\",\n  \"method\": \"initialized\",\n  \"params\": {}\n}\n";

    initialize = malloc(sizeof(msg_t));
    initialize->content = malloc(strlen(initialized_msg) + 1);
    if (initialize->content == NULL) {
        /* Handle allocation failure */
        free(initialize);
        return;
    }

    strcpy(initialize->content, initialized_msg);
    initialize->len = strlen(initialize->content);

    fprintf(stderr, "Sendine client initialized method!\n");
    pipeline_dispatcher(stdout, initialize, &state);

    free(initialize->content);
    free(initialize);
    free(state.client.root_uri);
    free(state.error.msg);
}


Test (test_lsp, test_doc_DidOpen) {

    msg_t* didOpen = malloc(sizeof(msg_t));
    char* content =
        "{\r\n"
        "  \"jsonrpc\": \"2.0"
        "\",\r\n"
        "  \"id\": 3,\r\n"
        "  \"method\": \"text"
        "Document/didOpen\","
        "\r\n"
        "  \"textDocument\": "
        "{\r\n"
        "    \"uri\": \"/abc/"
        "uri/to/no/where/\","
        "\r\n"
        "    \"languageId\": "
        "\"markdown doc\",\r"
        "\n"
        "    \"version\": 900"
        "0,\r\n"
        "    \"text\": \"blah"
        "blahline2and line 3!"
        "\"\r\n"
        "  }\r\n"
        "}";

    didOpen->content = malloc(strlen(content) + 10);

    if (didOpen->content == NULL) {
        free(didOpen);
        return;
    }

    strcpy(didOpen->content, content);
    didOpen->len = strlen(didOpen->content);

    LspState state;
    state.client.shutdown_requested = false;
    pipeline_dispatcher(stdout, didOpen, &state);
    puts("Finished pipeline dispatcher");

    free(didOpen->content);
    free(didOpen);
}
