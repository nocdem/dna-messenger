/**
 * GET /api/messages/conversation?user1=X&user2=Y&limit=50&offset=0
 */

#include "keyserver.h"
#include "http_utils.h"
#include "db_messages.h"
#include "rate_limit.h"
#include <json-c/json.h>
#include <microhttpd.h>
#include <string.h>
#include <stdlib.h>

enum MHD_Result api_load_conversation_handler(
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

    // Parse query parameters
    char user1[33] = {0};
    char user2[33] = {0};
    int limit = 50;
    int offset = 0;

    const char *user1_param = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "user1");
    const char *user2_param = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "user2");
    const char *limit_param = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "limit");
    const char *offset_param = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "offset");

    if (user1_param) strncpy(user1, user1_param, 32);
    if (user2_param) strncpy(user2, user2_param, 32);
    if (limit_param) limit = atoi(limit_param);
    if (offset_param) offset = atoi(offset_param);

    // Validate required parameters
    if (strlen(user1) == 0 || strlen(user2) == 0) {
        return http_send_error(connection, HTTP_BAD_REQUEST,
                              "Missing required parameters: user1, user2");
    }

    // Validate limits
    if (limit < 1 || limit > 1000) limit = 50;
    if (offset < 0) offset = 0;

    // Load conversation from database
    int count = 0;
    message_t *messages = db_load_conversation(db_conn, user1, user2, limit, offset, &count);

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

            if (messages[i].group_id > 0) {
                json_object_object_add(msg_obj, "group_id",
                                      json_object_new_int(messages[i].group_id));
            }

            json_object_array_add(messages_array, msg_obj);
        }
    }

    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "count", json_object_new_int(count));
    json_object_object_add(response, "messages", messages_array);

    enum MHD_Result ret = http_send_json_response(connection, HTTP_OK, response);

    json_object_put(response);
    if (messages) db_free_messages(messages, count);

    return ret;
}
