#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <string.h>

#include "../src/lsp.h"
#include "../src/pipeline.h"


Test (test_lsp, test_initialize) {

    msg_t* initialize = malloc(sizeof(msg_t));
    initialize->content =
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

    unsigned long len = strlen(initialize->content);
    initialize->len = len;

    /* initialize->len = 356; */

    pipeline_dispatcher(stdout, initialize, false);


}
