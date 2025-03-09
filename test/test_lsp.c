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

    initialize->content = malloc(strlen(content) + 10);
    if (initialize->content == NULL) {
        /* Handle allocation failure */
        free(initialize);
        return;
    }

    strcpy(initialize->content, content);
    initialize->len = strlen(initialize->content);

    pipeline_dispatcher(stdout, initialize, false);

    free(initialize->content);
    free(initialize);
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

    pipeline_dispatcher(stdout, didOpen, false);

    free(didOpen->content);
    free(didOpen);
}
