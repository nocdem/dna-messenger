/**
 * GET /api/contacts/:identity
 */

#include "keyserver.h"
#include "http_utils.h"
#include "db_messages.h"
#include "rate_limit.h"
#include <json-c/json.h>
#include <microhttpd.h>
#include <string.h>
#include <stdlib.h>

enum MHD_Result api_load_contact_handler(
    struct MHD_Connection *connection,
    PGconn *db_conn,
    const char *url
) {
    char client_ip[46];

    // Get client IP
    if (http_get_client_ip(connection, client_ip, sizeof(client_ip)) != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to get client IP");
    }

    // Get client IP

    // Rate limit
    if (!rate_limit_check(client_ip, RATE_LIMIT_TYPE_REGISTER)) {
        return http_send_error(connection, HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    // Parse identity from URL (/api/contacts/alice)
    const char *identity = strrchr(url, '/');
    if (!identity || strlen(identity) <= 1) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing identity in URL");
    }
    identity++; // Skip '/'

    // Load contact
    contact_t *contact = db_load_contact(db_conn, identity);
    if (!contact) {
        return http_send_error(connection, HTTP_NOT_FOUND, "Contact not found");
    }

    // Build JSON response
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));

    json_object *contact_obj = json_object_new_object();
    json_object_object_add(contact_obj, "id", json_object_new_int(contact->id));
    json_object_object_add(contact_obj, "identity", json_object_new_string(contact->identity));

    // Encode public keys as base64
    char *signing_base64 = http_base64_encode(contact->signing_pubkey, contact->signing_pubkey_len);
    if (signing_base64) {
        json_object_object_add(contact_obj, "signing_pubkey", json_object_new_string(signing_base64));
        free(signing_base64);
    }

    char *encryption_base64 = http_base64_encode(contact->encryption_pubkey, contact->encryption_pubkey_len);
    if (encryption_base64) {
        json_object_object_add(contact_obj, "encryption_pubkey", json_object_new_string(encryption_base64));
        free(encryption_base64);
    }

    json_object_object_add(contact_obj, "fingerprint", json_object_new_string(contact->fingerprint));
    json_object_object_add(contact_obj, "created_at", json_object_new_int64(contact->created_at));

    json_object_object_add(response, "contact", contact_obj);

    enum MHD_Result ret = http_send_json_response(connection, HTTP_OK, response);

    json_object_put(response);
    db_free_contact(contact);

    return ret;
}
