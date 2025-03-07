
#include "lsp.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "common.h"

#define CLIENT_SUPP_COMPLETION (1 << 0)


typedef struct LspClient {
    u32 capability;
    char *root_uri;
    u32 processID;
    bool initialized;

} LspClient;

char *lsp_initialize (cJSON *message) {
    log_debug("");

    /* Read necessary information from the init message */

    cJSON *id = cJSON_GetObjectItem(message, "id");
    double id_val = 0;
    if (cJSON_IsNumber(id)) {
        id_val = cJSON_GetNumberValue(id);
    }
    /* Exit if id is invalid */
    if (id_val < 0) {
        log_err("Id received was less than 0.");
        return NULL;
    }

    cJSON *params = cJSON_GetObjectItem(message, "params");
    if (!params) {
        log_err("No `params` in clients `initialize` message found.");
        return NULL;
    }

    cJSON *root_uri = cJSON_GetObjectItem(params, "rootUri");

    if (!cJSON_IsString(root_uri)) {
        log_err("Root URI not received.");
        return NULL;
    }

    /* Extract the rootUri into a new string */
    log_debug("Root URI `%s`", root_uri->valuestring);

    size_t uri_len = strlen(root_uri->valuestring);
    log_debug("URI Length: %d", uri_len) if (uri_len == 0) {
        log_err("Something went wrong here.");
        exit(1);
    }
    char *uri = malloc(sizeof(char) * (uri_len + 1));
    if (!uri) {
        log_err("Could not allocate mem, exiting.");
        exit(1);
    }
    /* copy over valuestring from json object to newly allocated string */
    memcpy(uri, root_uri->valuestring, uri_len);

    /* null terminate */
    uri[uri_len] = '\0';
    log_debug("uri length: `%zu`, uri: `%s`", uri_len, uri);


    /* Process ID */
    size_t process_id = 0;
    cJSON *json_processID = cJSON_GetObjectItem(params, "processId");
    if (cJSON_IsNumber(json_processID)) {
        process_id = json_processID->valueint;
    }

    /* Capabilities */
    cJSON *client_capabilities = cJSON_GetObjectItem(params, "capabilities");
    cJSON *text_document_capabilities =
        cJSON_GetObjectItem(client_capabilities, "textDocument");
    cJSON *completion_capabilities =
        cJSON_GetObjectItem(text_document_capabilities, "completion");

    if (!completion_capabilities) {
        log_warn("Client has no completion capabilities.");
    }

    /* Populate our client structure */
    client.initialized = false; /* We must wait for 'initialized' notification */
    client.capability |= CLIENT_SUPP_COMPLETION;
    client.root_uri = uri;
    client.processID = process_id;

    /* For debugging sake */
    char *debug_printing = cJSON_Print(text_document_capabilities);
    log_debug(
        "Initialised with values:\nprocess id: %d,\ntextDoc capabilities: %s\n",
        process_id, debug_printing);
    free(debug_printing);
    /* end */

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
    /* XXX Padding with extra bytes to be sure header will fit into string */
    char *final_message = malloc(str_response_len + 64);

    /* TODO Clean up better */
    if (!final_message) {
        log_err("Out of memory.");
        exit(1);
    }

    sprintf(final_message, "Content-Length: %zu\r\n\r\n%s", str_response_len,
            str_response);

    free(str_response);
    cJSON_Delete(response);

    return final_message;
}

int lsp_initialized (cJSON *message) {

    if (!cJSON_IsObject(message)) {
        log_err("Message was not a valid object.");
        return -1;
    };

    log_debug("Received initialized notification from client.");
    if (client.initialized == true) {
        log_warn("We are already initialised.");
        return 0;
    }
    client.initialized = true;
    return 0;
}

int lsp_exit (cJSON *message) {
    log_info("Exit requested.");
    return 0;
}

int lsp_shutdown (cJSON *message) {
    log_info("Shutdown requested.");
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
