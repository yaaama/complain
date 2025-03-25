
#include "lsp.h"

#include <assert.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "logging.h"

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

/** Creates a message with the minimal set of features to adhere to the LSP
 *  specification.
 **/
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

static int detect_sync_capabilities (cJSON *syncCapabilitiesJSON,
                                     LspClient *client) {

    if (!cJSON_IsObject(syncCapabilitiesJSON)) {
        log_warn("Client doesn't have any synchronisation capabilities.");
        return 0;
    }

    cJSON *synchronizationObj =
        cJSON_GetObjectItem(syncCapabilitiesJSON, "synchronization");

    cJSON *syncWillSave = cJSON_GetObjectItem(synchronizationObj, "willSave");
    cJSON *syncDidSave = cJSON_GetObjectItem(synchronizationObj, "didSave");
    cJSON *syncwillSaveWaitUntil =
        cJSON_GetObjectItem(synchronizationObj, "willSaveWaitUntil");

    if (cJSON_IsTrue(syncWillSave)) {
        client->capability |= CLIENT_SUPP_DOC_willSave;
    }
    if (cJSON_IsTrue(syncDidSave)) {
        client->capability |= CLIENT_SUPP_DOC_didSave;
    }
    if (cJSON_IsTrue(syncwillSaveWaitUntil)) {
        client->capability |= CLIENT_SUPP_DOC_willSaveWaitUntil;
    }

    cJSON *syncChangeKind = cJSON_GetObjectItem(synchronizationObj, "change");

    /* Now lets check for change syncing */

    if (!cJSON_IsNumber(syncChangeKind)) {
        log_info("TextDocumentSyncKind is not specified.");
        return 0;
    }

    client->capability |= CLIENT_SUPP_DOC_SYNC;

    int syncKind = 0;
    syncKind = syncChangeKind->valueint;

    if (syncKind < 0 || syncKind > 3) {
        log_info("Unsupported `textDocumentSyncKind` value of `%s`", syncKind);
        return -1;
    }

    switch (syncKind) {
        case 0:
            log_info("Client supports syncKind: `None`");
            break;
        case 1:
            log_info("Client supports syncKind: `Full`");
            client->capability |= CLIENT_SUPP_SYNC_FULL;
            break;
        case 2:
            log_info("Client supports syncKind: `Incremental`");
            client->capability |= CLIENT_SUPP_SYNC_INCREMENTAL;
            break;
        default:
            /* UNREACHABLE */
            COMPLAIN_UNREACHABLE(
                "Should be unreachable due to range checking.");
            break;
    }

    client->capability |= syncKind;

    return 0;
}

/** Creates a "server capabilities object"
 *
  "textDocumentSync": {
    "change": 2,
    "openClose": true,
    "save": true
    },

 * NOTE: Currently we only support 'textDocumentSync'
 */
static inline cJSON *server_capabilities (void) {

    cJSON *capabilities = cJSON_CreateObject();
    cJSON *doc_sync = cJSON_CreateObject();

    /* Tell client we want a full copy of the document per sync */
    cJSON_AddNumberToObject(doc_sync, "change", 2);
    cJSON_AddBoolToObject(doc_sync, "openClose", true);
    cJSON_AddBoolToObject(doc_sync, "didSave", true);
    cJSON_AddItemToObject(capabilities, "textDocumentSync", doc_sync);

    return capabilities;
}

int lsp_initialize (LspState *state, cJSON *message) {
    log_debug("");

    /* Read necessary information from the init message */
    int error_code = 0;
    double msg_id = -1;

    cJSON *id_json = cJSON_GetObjectItem(message, "id");
    if (!cJSON_IsNumber(id_json)) {
        log_err("`id` is not a valid number.");
        error_code = RPC_InvalidRequest;
        goto failed;
    }

    /* Get id value */
    msg_id = cJSON_GetNumberValue(id_json);

    /* Exit if id is invalid */
    if (msg_id < 0) {
        log_err("Id received was less than 0.");
        error_code = RPC_InvalidRequest;
        goto failed;
    }

    if (!cJSON_HasObjectItem(message, "params")) {
        log_err("JSON received does not contain `params` object.");
        error_code = RPC_InvalidParams;
        goto failed;
    }

    cJSON *params = cJSON_GetObjectItem(message, "params");

    /* Process ID */
    size_t process_id = 0;
    cJSON *json_processID = cJSON_GetObjectItem(params, "processId");

    if (cJSON_IsNumber(json_processID) && json_processID->valueint > 0) {
        process_id = json_processID->valueint;
    }
    if (process_id == 0) {
        log_warn("Process ID is invalid.");
        goto failed;
    }

    cJSON *root_uri = cJSON_GetObjectItem(params, "rootUri");

    if ((!cJSON_IsString(root_uri)) || (root_uri->valuestring == NULL)) {
        log_err("Root URI not received.");
        error_code = RPC_InvalidParams;
        goto failed;
    }

    /* Extract the rootUri into a new string */

    size_t uri_len = strlen(root_uri->valuestring);
    log_debug("Root URI `%s`, URI Length: `%zu`", root_uri->valuestring,
              uri_len);

    if (uri_len == 0) {
        log_err("Invalid URI, length is 0.");
        error_code = RPC_InvalidParams;
        goto failed;
    }
    char *uri = malloc(sizeof(char) * (uri_len + 1));

    if (!uri) {
        log_err("Could not allocate mem, exiting.");
        abort();
    }

    /* copy over valuestring from json object to newly allocated string */
    memcpy(uri, root_uri->valuestring, uri_len);

    /* null terminate */
    uri[uri_len] = '\0';
    log_debug("uri length: `%zu`, uri: `%s`", uri_len, uri);

    /* Client Capabilities
     capabilities.textDocument.(synchronization, completion)*/
    cJSON *client_capabilities = cJSON_GetObjectItem(params, "capabilities");
    cJSON *text_document_capabilities =
        cJSON_GetObjectItem(client_capabilities, "textDocument");
    bool has_textdoc_capabilities = true;

    cJSON *completion_capabilities =
        cJSON_GetObjectItem(text_document_capabilities, "completion");
    cJSON *sync_capabilities =
        cJSON_GetObjectItem(text_document_capabilities, "synchronization");

    if (!cJSON_IsObject(text_document_capabilities)) {
        log_warn("Client has no text documentation capabilities.");
        state->client.capability = 0;
        has_textdoc_capabilities = false;
    }

    if (has_textdoc_capabilities) {

        /* Detect sync capabilities */
        if (!cJSON_IsObject(sync_capabilities)) {
            log_warn("No sync capabilities");
        } else {
            detect_sync_capabilities(sync_capabilities, &state->client);
        }

        if (cJSON_IsObject(completion_capabilities)) {
            state->client.capability |= CLIENT_SUPP_COMPLETION;
        } else {
            log_warn("Client has no completion capabilities.");
        }
    }

    /* We must wait for 'initialized' notification */
    state->client.shutdown_requested = false;
    state->client.initialized = false;
    state->client.root_uri = uri;
    state->client.processID = process_id;

    /* For debugging sake */
#ifndef NDEBUG
    char *debug_printing = cJSON_Print(text_document_capabilities);
    log_debug(
        "Initialised with values:\nprocess id: %d,\ntextDoc capabilities: "
        "%s\n",
        process_id, debug_printing);
    free(debug_printing);
#endif  // NDEBUG
    /* end */

    /* Prepare our response: */

    cJSON *response = base_response(msg_id);

    /* result subobject */
    cJSON *result_object = cJSON_CreateObject();

    cJSON *server_capability = server_capabilities();
    cJSON_AddItemToObject(result_object, "capabilities", server_capability);
    cJSON_AddItemToObject(response, "result", result_object);

    char *str_response = cJSON_Print(response);
    log_debug("Our response: `%s`", str_response);
    size_t str_response_len = strlen(str_response);

    state->reply.msg_len = str_response_len;

    char temp_header[128] = {0};
    sprintf(temp_header, "Content-Length: %zu\r\n\r\n", str_response_len);

    u32 header_len = strlen(temp_header);
    state->reply.header = malloc(sizeof(char) * header_len + 1);
    memcpy(state->reply.header, temp_header, header_len + 1);

    state->reply.msg = malloc(sizeof(char) * str_response_len + 1);
    sprintf(state->reply.msg, "%s", str_response);

    state->has_msg = true;
    free(str_response);
    cJSON_Delete(response);

    return 0;

failed:
    {
        state->has_err = true;
        state->error.code = error_code;
        return -1;
    }
}

int lsp_initialized (LspState *state, cJSON *message) {

    if (!cJSON_IsObject(message)) {
        log_err("Message was not a valid object.");
        return -1;
    };

    log_debug("Received initialized notification from client.");
    if (state->client.initialized == true) {
        log_warn("We are already initialised.");
        return 0;
    }
    state->client.initialized = true;
    return 0;
}

int lsp_exit (LspState *state, cJSON *message) {

    assert(state);
    if (state->client.shutdown_requested == false) {
        log_warn("Abrupt shutdown is in process now.");
        return 1000;
    }
    log_info("Exit requested.");
    return 999;
}

int lsp_shutdown (LspState *state, cJSON *message) {

    assert(state);
    if (state->client.shutdown_requested) {
        return 998;
    }
    state->client.shutdown_requested = true;
    log_info("shutdown_requested set to TRUE.");
    return 998;
}

int lsp_textDocument_didOpen (LspState *state, cJSON *message) {

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
        .open = true, .uri = uri, .text = text, .version = (u64) ver};

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

    /* message.params */
    cJSON *paramsJSON = cJSON_GetObjectItem(message, "params");
    if (!cJSON_IsObject(paramsJSON)) {
        log_warn("Invalid params object in didChange message");
        return -1;
    }

    /* message.params.textDocument */
    cJSON *textDocJSON = cJSON_GetObjectItem(paramsJSON, "textDocument");
    if (!cJSON_IsObject(textDocJSON)) {
        log_warn("Invalid textDocument object in didChange message");
        return -1;
    }
    /* message.params.textDocument.uri */
    cJSON *uriJSON = cJSON_GetObjectItem(textDocJSON, "uri");
    /* message.params.textDocument.version */
    cJSON *versionJSON = cJSON_GetObjectItem(textDocJSON, "version");
    if (!cJSON_IsString(uriJSON) || !cJSON_IsNumber(versionJSON)) {
        log_warn("Invalid uri or version in `didChange` message");
        return -1;
    }

    /* message.params.contentChanges[] */
    cJSON *changesJSON = cJSON_GetObjectItem(paramsJSON, "contentChanges");

    int change_count = cJSON_GetArraySize(changesJSON);

    if (change_count <= 0) {
        log_debug("No changes to apply.");
        return 0;
    }

    /* We must create a change object */
    DocChange *changes = (DocChange *) malloc(change_count * sizeof(DocChange));
    if (!changes) {
        log_err("Failed to allocate memory.");
        (void) abort;
    }

    /* Index of change */
    int i = 0;
    /* Parse incremental changes */
    cJSON *rangeJSON;
    /* Iterate for each item in `contentChanges` */
    cJSON_ArrayForEach(rangeJSON, changesJSON) {

        rangeJSON = cJSON_GetObjectItem(rangeJSON, "range");
        if (!cJSON_IsObject(rangeJSON)) {
            log_warn("Missing or invalid range object");
            goto failed_changes;
        }

        cJSON *rangeStartJSON = cJSON_GetObjectItem(rangeJSON, "start");
        cJSON *rangeEndJSON = cJSON_GetObjectItem(rangeJSON, "end");
        if (!cJSON_IsObject(rangeStartJSON) || !cJSON_IsObject(rangeEndJSON)) {
            log_warn("Invalid `start` / `end` objects in `range`");
            goto failed_changes;
        }

        cJSON *startLineJSON = cJSON_GetObjectItem(rangeStartJSON, "line");
        cJSON *startCharJSON = cJSON_GetObjectItem(rangeStartJSON, "character");
        cJSON *endLineJSON = cJSON_GetObjectItem(rangeEndJSON, "line");
        cJSON *endCharJSON = cJSON_GetObjectItem(rangeEndJSON, "character");
        if (!cJSON_IsNumber(startLineJSON) || !cJSON_IsNumber(startCharJSON) ||
            !cJSON_IsNumber(endLineJSON) || !cJSON_IsNumber(endCharJSON)) {
            log_warn("Invalid line or character values in range");
            goto failed_changes;
        }

        cJSON *rangeLenJSON = cJSON_GetObjectItem(rangeJSON, "rangeLength");
        cJSON *rangeTextJSON = cJSON_GetObjectItem(rangeJSON, "text");

        if (!cJSON_IsNumber(rangeLenJSON)) {
            log_warn("Received a bad `rangeLength`.");
            goto failed_changes;
        }

        if (!cJSON_IsString(rangeTextJSON)) {
            log_warn("Received invalid `text`.");
            goto failed_changes;
        }

        changes[i].start.line = startLineJSON->valueint;
        changes[i].start.pos = startLineJSON->valueint;
        changes[i].end.line = endLineJSON->valueint;
        changes[i].end.pos = endLineJSON->valueint;
        changes[i].range_len = rangeLenJSON->valueint;

        /* Allocate memory for the text and copy it */
        char *text = rangeTextJSON->valuestring;
        size_t textLen = strlen(text) + 1;
        changes[i].text = (char *) malloc(textLen);

        if (!changes[i].text) {
            log_err("Failed to allocate memory for change text");

            /* Clean up previously allocated text */
            for (int j = 0; j < i; j++) {
                free(changes[j].text);
            }
            free(changes);
            return -1;
        }
        /* Copy changes text to our object */
        memcpy(changes[i].text, text, textLen);
        ++i;
    }

    char *uri = uriJSON->valuestring;
    int version = versionJSON->valueint;

    /* Apply our document changes here... */
    /* apply_document_changes(uri, version, changes, changeCount) */

    /* Clean up allocated memory */
    for (int j = 0; j < change_count; j++) {
        free(changes[j].text);
    }
    free(changes);

    return 0;

failed_changes:
    {
        /* Clean up allocated memory for the changes we have made so far */
        for (int j = 0; j < i; j++) {
            free(changes[j].text);
        }
        free(changes);
        return -1;
    }
}

int lsp_textDocument_didClose (cJSON *message) {
    log_debug("didClose");
    return 0;
}

int lsp_textDocument_completion (cJSON *message) {
    log_debug("completion");
    return 0;
}
