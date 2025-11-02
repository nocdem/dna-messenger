/**
 * GET /api/contacts
 */

#include "keyserver.h"
#include "http_utils.h"
#include "db_messages.h"
#include "rate_limit.h"
#include <json-c/json.h>
#include <microhttpd.h>

enum MHD_Result api_load_all_contacts_handler(
    struct MHD_Connection *connection,
    PGconn *db_conn
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

    // Load all contacts
    int count = 0;
    contact_t *contacts = db_load_all_contacts(db_conn, &count);

    // Build JSON response
    json_object *response = json_object_new_object();
    json_object *contacts_array = json_object_new_array();

    if (contacts && count > 0) {
        for (int i = 0; i < count; i++) {
            json_object *contact_obj = json_object_new_object();
            json_object_object_add(contact_obj, "id", json_object_new_int(contacts[i].id));
            json_object_object_add(contact_obj, "identity", json_object_new_string(contacts[i].identity));

            // Encode public keys
            char *signing_base64 = http_base64_encode(contacts[i].signing_pubkey, contacts[i].signing_pubkey_len);
            if (signing_base64) {
                json_object_object_add(contact_obj, "signing_pubkey", json_object_new_string(signing_base64));
                free(signing_base64);
            }

            char *encryption_base64 = http_base64_encode(contacts[i].encryption_pubkey, contacts[i].encryption_pubkey_len);
            if (encryption_base64) {
                json_object_object_add(contact_obj, "encryption_pubkey", json_object_new_string(encryption_base64));
                free(encryption_base64);
            }

            json_object_object_add(contact_obj, "fingerprint", json_object_new_string(contacts[i].fingerprint));
            json_object_object_add(contact_obj, "created_at", json_object_new_int64(contacts[i].created_at));

            json_object_array_add(contacts_array, contact_obj);
        }
    }

    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "count", json_object_new_int(count));
    json_object_object_add(response, "contacts", contacts_array);

    enum MHD_Result ret = http_send_json_response(connection, HTTP_OK, response);

    json_object_put(response);
    if (contacts) db_free_contacts(contacts, count);

    return ret;
}
