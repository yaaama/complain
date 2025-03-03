
#include "lsp.h"
#include "pipeline.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"




char *lsp_initialize (cJSON *message) {
    log_debug("");

    /* Read necessary information from the init message */

    cJSON *id = cJSON_GetObjectItem(message,"id");
    double id_val = 0;
    if (cJSON_IsNumber(id) ) {
        id_val = cJSON_GetNumberValue(id);
    }
    /* Exit if id is invalid */
    if (id_val < 0) {
        log_err("Id received was less than 0.");
    }

    cJSON *root_uri = cJSON_GetObjectItem(message, "rootUri");

    cJSON *params = cJSON_GetObjectItem(message,"params");
    cJSON *client_capabilities = cJSON_GetObjectItem(params, "capabilities");
    size_t process_id = cJSON_GetObjectItem(params, "processId")->valueint;

    cJSON *text_document_capabilities =
        cJSON_GetObjectItem(client_capabilities, "textDocument");
    cJSON *completion_capabilities =
        cJSON_GetObjectItem(text_document_capabilities, "completion");


    log_debug(
        "Initialised with values:\nprocess id: %d,\ntextDoc capabilities: %s\n",
        process_id,  cJSON_Print(text_document_capabilities));


    /* Prepare our response: */
    /*
      response {
      ..result {
      ....capabilities { }
      ..},
      ..jsonrpc,
      ..id
      }
     */

    /* result object */
    cJSON *r_result = cJSON_CreateObject();
    cJSON *r_result_capabilities = cJSON_CreateObject();
    cJSON_AddItemToObject(r_result, "capabilities", r_result_capabilities);

    /* response */
    cJSON *response = cJSON_CreateObject();
    cJSON *r_jsonrpc = cJSON_CreateString("2.0");
    cJSON *r_id = cJSON_CreateNumber(id_val);

    cJSON_AddItemToObject(response, "jsonrpc", r_jsonrpc);
    cJSON_AddItemToObject(response, "id", r_id);
    cJSON_AddItemToObject(response, "result", r_result);

    char *str_response = cJSON_PrintUnformatted(response);
    size_t str_response_len = strlen(str_response);
    char *final_message = malloc(str_response_len + 64);

    sprintf(final_message, "Content-Length: %zu\r\n\r\n%s",
            str_response_len, str_response);

    free(str_response);
    cJSON_Delete(response);

    return final_message;
}

int lsp_initialized (cJSON *message) {
    log_debug("initialized");
    return 0;
}

int lsp_exit (cJSON *message) {
    log_debug("exit");
    return 0;
}

int lsp_shutdown (cJSON *message) {
    log_debug("shutdown");
    return 0;
}

int lsp_textDocument_didOpen (cJSON *message) {
    log_debug("didOpen");
    return 0;
}

int lsp_textDocument_didChange (cJSON *message) {
    log_debug("didChange");
    return 0;
}

int lsp_textDocument_didClose (cJSON *message) {
    log_debug("didClose");
    return 0;
}

int lsp_textDocument_completion (cJSON *message) {
    log_debug("completion");
    return 0;
}
