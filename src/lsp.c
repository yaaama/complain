
#include "lsp.h"

#include <assert.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "logging.h"

#define CLIENT_SUPP_COMPLETION (1 << 0)
#define CLIENT_SUPP_DOC_SYNC (1 << 1)


LspClient client = {0};

/* Checks the message to see if it has:
 * 1. jsonrpc object
 * 2. method object
 */
bool clients_json_valid (cJSON *message) {

    assert(message);

    bool validity = true;

    if (!cJSON_HasObjectItem(message, "jsonrpc")) {
        log_err("JSON received does not contain `jsonrpc` object.");
        validity = false;
    }
    if (!cJSON_HasObjectItem(message, "method")) {
        log_err("JSON received does not contain `method` object.");
        validity = false;
    }

    return validity;
}

/* Creates a message with the minimal set of features to adhere to the LSP
 * specification */
static inline cJSON *base_response (double id) {
    assert(id > 0);

    /* response object */
    cJSON *response = cJSON_CreateObject();
    cJSON *r_jsonrpc = cJSON_CreateString("2.0");
    cJSON *r_id = cJSON_CreateNumber(id);
    cJSON_AddItemToObject(response, "jsonrpc", r_jsonrpc);
    cJSON_AddItemToObject(response, "id", r_id);
    return response;
}

/* Creates a "server capabilities object"*/
static inline cJSON *server_capabilities (void) {

    cJSON *r_result_capabilities = cJSON_CreateObject();
    cJSON *r_text_sync_capabilities = cJSON_CreateObject();

    /* Tell client we want a full copy of the document per sync */
    cJSON_AddNumberToObject(r_text_sync_capabilities, "Full", 1);
    return r_result_capabilities;
}

char *lsp_initialize (cJSON *message) {
    log_debug("");

    /* Read necessary information from the init message */

    cJSON *id_json = cJSON_GetObjectItem(message, "id");
    double id = 0;
    if (!cJSON_IsNumber(id_json)) {
        log_err("`id` does not exist in this message.");
        return NULL;
    }

    /* Get id value */
    id = cJSON_GetNumberValue(id_json);

    /* Exit if id is invalid */
    if (id < 0) {
        log_err("Id received was less than 0.");
        return NULL;
    }

    if (!cJSON_HasObjectItem(message, "params")) {
        log_err("JSON received does not contain `params` object.");
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

    /* Client Capabilities
     capabilities->textDocument->(synchronization,completion)*/
    cJSON *client_capabilities = cJSON_GetObjectItem(params, "capabilities");
    cJSON *text_document_capabilities =
        cJSON_GetObjectItem(client_capabilities, "textDocument");

    cJSON *completion_capabilities =
        cJSON_GetObjectItem(text_document_capabilities, "completion");
    cJSON *sync_capabilities =
        cJSON_GetObjectItem(text_document_capabilities, "synchronization");

    bool has_textdoc_capabilities = true;
    if (!cJSON_IsObject(text_document_capabilities)) {
        log_warn("Client has no text documentation capabilities.");
        client.capability = 0;
        has_textdoc_capabilities = false;
    }

    if (has_textdoc_capabilities) {

        if (!cJSON_IsObject(sync_capabilities)) {
            log_warn("Client doesn't have any synchronisation capabilities.");
        } else {
            client.capability |= CLIENT_SUPP_DOC_SYNC;
        }

        if (!cJSON_IsObject(completion_capabilities)) {
            log_warn("Client has no completion capabilities.");
        } else {
            client.capability |= CLIENT_SUPP_COMPLETION;
        }
    }

    /* We must wait for 'initialized' notification */
    client.initialized = false;
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

    cJSON *response = base_response(id);

    /* result subobject */
    cJSON *r_result = cJSON_CreateObject();

    cJSON *r_result_capabilities = server_capabilities();
    cJSON_AddItemToObject(r_result, "capabilities", r_result_capabilities);
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

    /* textDocument Object */
    cJSON *textDocJSON = cJSON_GetObjectItem(message, "textDocument");

    if (!cJSON_IsObject(textDocJSON)) {
        log_warn("Text document is empty. Returning.");
        /* TODO Return LSP error message here */
        return -1;
    }

    cJSON *uriJSON = cJSON_GetObjectItem(textDocJSON, "uri");
    cJSON *langIdJSON = cJSON_GetObjectItem(textDocJSON, "languageId");
    cJSON *verJSON = cJSON_GetObjectItem(textDocJSON, "version");
    cJSON *textJSON = cJSON_GetObjectItem(textDocJSON, "text");

    if (!cJSON_IsString(uriJSON)) {
        log_warn("uri is not a valid string.");
        return -1;
    }
    if (!cJSON_IsString(langIdJSON)) {
        log_warn("languageId is not a valid string.");
        return -1;
    }

    if (!cJSON_IsNumber(verJSON)) {
        log_warn("version is not a valid number.");
        return -1;
    }

    if (!cJSON_IsString(textJSON)) {
        log_warn("text is not a valid string.");
        return -1;
    }

    char *uri = cJSON_GetStringValue(uriJSON);
    char *langId = cJSON_GetStringValue(langIdJSON);

    double ver = cJSON_GetNumberValue(verJSON);
    char *text = cJSON_GetStringValue(textJSON);

    assert(ver > 0);

    Document doc = {
        .open = true, .uri = uri, .text = text, .version = (u64)ver};

    log_info(
        "Successful textDocument_didOpen parsing. Document info:\n"
        "langId: `%s`\n"
        "uri: `%s`\n"
        "version:`%llu`\n"
        "text:`%s`\n",
        langId, doc.uri, doc.version, doc.text);

    return 0;
}

int lsp_textDocument_didChange (cJSON *message) {
    log_debug("didChange");

    cJSON *paramsJSON = cJSON_GetObjectItem(message, "params");
    cJSON *changesJSON = cJSON_GetObjectItem(message, "contentChanges");

    /* Parse incremental changes */
    cJSON *rangeJSON;

    /* TODO We must create a change object */
    DocChange *change = malloc(sizeof(DocChange));

    /* We need a dynamic array here... brb */
    DocChange changes[1];

    /* Iterate for each item in `contentChanges` */
    cJSON_ArrayForEach(rangeJSON, changesJSON) {

        cJSON *rangeStartJSON = cJSON_GetObjectItem(rangeJSON, "start");
        cJSON *startLineJSON = cJSON_GetObjectItem(rangeStartJSON, "line");
        cJSON *startCharJSON = cJSON_GetObjectItem(rangeStartJSON, "character");

        cJSON *rangeEndJSON = cJSON_GetObjectItem(rangeJSON, "end");
        cJSON *endLineJSON = cJSON_GetObjectItem(rangeEndJSON, "line");
        cJSON *endCharJSON = cJSON_GetObjectItem(rangeEndJSON, "character");

        cJSON *rangeLenJSON = cJSON_GetObjectItem(rangeJSON, "rangeLength");
        cJSON *rangeTextJSON = cJSON_GetObjectItem(rangeJSON, "text");

        if (!cJSON_IsNumber(rangeLenJSON)) {
            log_warn("Received a bad `rangeLength`.");
            return -1;
        }

        if (!cJSON_IsString(rangeTextJSON)) {
            log_warn("Received invalid `text`.");
            return -1;
        }

        /* TODO Finish checking for valid JSON values */
    }

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
