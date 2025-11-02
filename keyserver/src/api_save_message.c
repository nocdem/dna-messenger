/**
 * POST /api/messages - Save message
 */

#include "keyserver.h"
#include "http_utils.h"
#include "rate_limit.h"
#include "db_messages.h"
#include <string.h>
#include <json-c/json.h>

enum MHD_Result api_save_message_handler(
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
    message_t message = {0};
    json_object *obj;

    if (json_object_object_get_ex(payload, "sender", &obj)) {
        strncpy(message.sender, json_object_get_string(obj), 32);
    }

    if (json_object_object_get_ex(payload, "recipient", &obj)) {
        strncpy(message.recipient, json_object_get_string(obj), 32);
    }

    if (json_object_object_get_ex(payload, "ciphertext", &obj)) {
        const char *ciphertext_base64 = json_object_get_string(obj);
        // Decode base64
        size_t decoded_len;
        message.ciphertext = http_base64_decode(ciphertext_base64, strlen(ciphertext_base64),
                                                &decoded_len);
        message.ciphertext_len = (int)decoded_len;
    }

    if (json_object_object_get_ex(payload, "status", &obj)) {
        strncpy(message.status, json_object_get_string(obj), 19);
    } else {
        strcpy(message.status, "pending");
    }

    if (json_object_object_get_ex(payload, "group_id", &obj)) {
        message.group_id = json_object_get_int(obj);
    }

    message.created_at = time(NULL);

    // Validate required fields
    if (strlen(message.sender) == 0 || strlen(message.recipient) == 0 || !message.ciphertext) {
        if (message.ciphertext) free(message.ciphertext);
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST,
                              "Missing required fields: sender, recipient, ciphertext");
    }

    // Save to database
    int64_t message_id = db_save_message(db_conn, &message);

    free(message.ciphertext);
    json_object_put(payload);

    if (message_id < 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to save message");
    }

    // Build response
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "message_id", json_object_new_int64(message_id));

    enum MHD_Result ret = http_send_json_response(connection, HTTP_OK, response);
    json_object_put(response);

    return ret;
}
