/**
 * POST /api/contacts - Save or update contact
 */

#include "keyserver.h"
#include "http_utils.h"
#include "db_messages.h"
#include "rate_limit.h"
#include <json-c/json.h>
#include <microhttpd.h>
#include <string.h>
#include <stdlib.h>

enum MHD_Result api_save_contact_handler(
    struct MHD_Connection *connection,
    PGconn *db_conn,
    const char *upload_data,
    size_t upload_data_size
) {
    char client_ip[46];

    // Get client IP
    if (http_get_client_ip(connection, client_ip, sizeof(client_ip)) != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to get client IP");
    }

    

    // Rate limit check
    if (!rate_limit_check(client_ip, RATE_LIMIT_TYPE_REGISTER)) {
        return http_send_error(connection, HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    // Parse JSON
    json_object *payload = http_parse_json_post(upload_data, upload_data_size);
    if (!payload) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid JSON");
    }

    // Extract fields
    contact_t contact = {0};
    json_object *obj;

    if (json_object_object_get_ex(payload, "identity", &obj)) {
        strncpy(contact.identity, json_object_get_string(obj), 32);
    }

    if (json_object_object_get_ex(payload, "signing_pubkey", &obj)) {
        const char *pubkey_base64 = json_object_get_string(obj);
        size_t decoded_len;
        contact.signing_pubkey = http_base64_decode(pubkey_base64, strlen(pubkey_base64),
                                                     &decoded_len);
        contact.signing_pubkey_len = (int)decoded_len;
    }

    if (json_object_object_get_ex(payload, "encryption_pubkey", &obj)) {
        const char *pubkey_base64 = json_object_get_string(obj);
        size_t decoded_len;
        contact.encryption_pubkey = http_base64_decode(pubkey_base64, strlen(pubkey_base64),
                                                        &decoded_len);
        contact.encryption_pubkey_len = (int)decoded_len;
    }

    if (json_object_object_get_ex(payload, "fingerprint", &obj)) {
        strncpy(contact.fingerprint, json_object_get_string(obj), 64);
    }

    contact.created_at = time(NULL);

    // Validate required fields
    if (strlen(contact.identity) == 0 || !contact.signing_pubkey || !contact.encryption_pubkey) {
        if (contact.signing_pubkey) free(contact.signing_pubkey);
        if (contact.encryption_pubkey) free(contact.encryption_pubkey);
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST,
                              "Missing required fields: identity, signing_pubkey, encryption_pubkey");
    }

    // Save to database
    int result = db_save_contact(db_conn, &contact);

    free(contact.signing_pubkey);
    free(contact.encryption_pubkey);
    json_object_put(payload);

    if (result != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to save contact");
    }

    // Build response
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "identity", json_object_new_string(contact.identity));

    enum MHD_Result ret = http_send_json_response(connection, HTTP_OK, response);
    json_object_put(response);

    return ret;
}
