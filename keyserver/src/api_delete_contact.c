/**
 * DELETE /api/contacts/:identity
 */

#include "keyserver.h"
#include "http_utils.h"
#include "db_messages.h"
#include "rate_limit.h"
#include <json-c/json.h>
#include <microhttpd.h>
#include <string.h>

enum MHD_Result api_delete_contact_handler(
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

    // Parse identity from URL
    const char *identity = strrchr(url, '/');
    if (!identity || strlen(identity) <= 1) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing identity in URL");
    }
    identity++; // Skip '/'

    // Delete contact
    int result = db_delete_contact(db_conn, identity);
    if (result != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to delete contact");
    }

    // Build response
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "identity", json_object_new_string(identity));

    enum MHD_Result ret = http_send_json_response(connection, HTTP_OK, response);
    json_object_put(response);

    return ret;
}
