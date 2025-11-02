/**
 * GET /api/messages/group/:groupId?limit=50&offset=0
 */

#include "keyserver.h"
#include "http_utils.h"
#include "db_messages.h"
#include "rate_limit.h"
#include <json-c/json.h>
#include <microhttpd.h>
#include <string.h>
#include <stdlib.h>

enum MHD_Result api_load_group_messages_handler(
    struct MHD_Connection *connection,
    PGconn *db_conn,
    const char *url
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

    // Parse group_id from URL (/api/messages/group/123)
    const char *group_id_str = strrchr(url, '/');
    if (!group_id_str || strlen(group_id_str) <= 1) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing group_id in URL");
    }
    group_id_str++; // Skip the '/'

    int group_id = atoi(group_id_str);
    if (group_id <= 0) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid group_id");
    }

    // Parse query parameters
    int limit = 50;
    int offset = 0;

    const char *limit_param = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "limit");
    const char *offset_param = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "offset");

    if (limit_param) limit = atoi(limit_param);
    if (offset_param) offset = atoi(offset_param);

    // Validate limits
    if (limit < 1 || limit > 1000) limit = 50;
    if (offset < 0) offset = 0;

    // Load group messages from database
    int count = 0;
    message_t *messages = db_load_group_messages(db_conn, group_id, limit, offset, &count);

    // Build JSON response
    json_object *response = json_object_new_object();
    json_object *messages_array = json_object_new_array();

    if (messages && count > 0) {
        for (int i = 0; i < count; i++) {
            json_object *msg_obj = json_object_new_object();
            json_object_object_add(msg_obj, "id", json_object_new_int64(messages[i].id));
            json_object_object_add(msg_obj, "sender", json_object_new_string(messages[i].sender));
            json_object_object_add(msg_obj, "recipient", json_object_new_string(messages[i].recipient));

            // Encode ciphertext as base64
            char *ciphertext_base64 = http_base64_encode(messages[i].ciphertext,
                                                         messages[i].ciphertext_len);
            if (ciphertext_base64) {
                json_object_object_add(msg_obj, "ciphertext",
                                      json_object_new_string(ciphertext_base64));
                free(ciphertext_base64);
            }

            json_object_object_add(msg_obj, "ciphertext_len",
                                  json_object_new_int(messages[i].ciphertext_len));
            json_object_object_add(msg_obj, "created_at",
                                  json_object_new_int64(messages[i].created_at));
            json_object_object_add(msg_obj, "status",
                                  json_object_new_string(messages[i].status));

            if (messages[i].delivered_at > 0) {
                json_object_object_add(msg_obj, "delivered_at",
                                      json_object_new_int64(messages[i].delivered_at));
            }

            if (messages[i].read_at > 0) {
                json_object_object_add(msg_obj, "read_at",
                                      json_object_new_int64(messages[i].read_at));
            }

            json_object_object_add(msg_obj, "group_id",
                                  json_object_new_int(messages[i].group_id));

            json_object_array_add(messages_array, msg_obj);
        }
    }

    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "group_id", json_object_new_int(group_id));
    json_object_object_add(response, "count", json_object_new_int(count));
    json_object_object_add(response, "messages", messages_array);

    enum MHD_Result ret = http_send_json_response(connection, HTTP_OK, response);

    json_object_put(response);
    if (messages) db_free_messages(messages, count);

    return ret;
}
