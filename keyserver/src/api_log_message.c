/*
 * API Handler: POST /api/logging/message
 */

#include "keyserver.h"
#include "http_utils.h"
#include "rate_limit.h"
#include "db_logging.h"
#include <string.h>
#include <json-c/json.h>

enum MHD_Result api_log_message_handler(struct MHD_Connection *connection, PGconn *db_conn,
                                         const char *upload_data, size_t upload_data_size) {
    char client_ip[46];

    // Get client IP
    if (http_get_client_ip(connection, client_ip, sizeof(client_ip)) != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to get client IP");
    }

    // Rate limiting
    if (!rate_limit_check(client_ip, RATE_LIMIT_TYPE_REGISTER)) {
        LOG_WARN("Rate limit exceeded for log_message: %s", client_ip);
        return http_send_error(connection, HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    // Parse JSON payload
    json_object *payload = http_parse_json_post(upload_data, upload_data_size);
    if (!payload) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid JSON");
    }

    // Extract required fields
    json_object *field;

    // Required: sender
    if (!json_object_object_get_ex(payload, "sender", &field)) {
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing field: sender");
    }
    const char *sender = json_object_get_string(field);

    // Required: recipient
    if (!json_object_object_get_ex(payload, "recipient", &field)) {
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing field: recipient");
    }
    const char *recipient = json_object_get_string(field);

    // Required: status
    if (!json_object_object_get_ex(payload, "status", &field)) {
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing field: status");
    }
    const char *status = json_object_get_string(field);

    // Optional fields
    int64_t message_id = 0;
    if (json_object_object_get_ex(payload, "message_id", &field)) {
        message_id = json_object_get_int64(field);
    }

    int group_id = 0;
    if (json_object_object_get_ex(payload, "group_id", &field)) {
        group_id = json_object_get_int(field);
    }

    int plaintext_size = 0;
    if (json_object_object_get_ex(payload, "plaintext_size", &field)) {
        plaintext_size = json_object_get_int(field);
    }

    int ciphertext_size = 0;
    if (json_object_object_get_ex(payload, "ciphertext_size", &field)) {
        ciphertext_size = json_object_get_int(field);
    }

    const char *encrypted_at = "";
    if (json_object_object_get_ex(payload, "encrypted_at", &field)) {
        encrypted_at = json_object_get_string(field);
    }

    const char *sent_at = "";
    if (json_object_object_get_ex(payload, "sent_at", &field)) {
        sent_at = json_object_get_string(field);
    }

    const char *delivered_at = "";
    if (json_object_object_get_ex(payload, "delivered_at", &field)) {
        delivered_at = json_object_get_string(field);
    }

    const char *read_at = "";
    if (json_object_object_get_ex(payload, "read_at", &field)) {
        read_at = json_object_get_string(field);
    }

    const char *error_code = "";
    if (json_object_object_get_ex(payload, "error_code", &field)) {
        error_code = json_object_get_string(field);
    }

    const char *error_message = "";
    if (json_object_object_get_ex(payload, "error_message", &field)) {
        error_message = json_object_get_string(field);
    }

    const char *platform = "";
    if (json_object_object_get_ex(payload, "platform", &field)) {
        platform = json_object_get_string(field);
    }

    // Build log message structure
    log_message_t log_msg;
    memset(&log_msg, 0, sizeof(log_msg));

    log_msg.message_id = message_id;
    strncpy(log_msg.sender, sender, sizeof(log_msg.sender) - 1);
    strncpy(log_msg.recipient, recipient, sizeof(log_msg.recipient) - 1);
    log_msg.group_id = group_id;
    strncpy(log_msg.status, status, sizeof(log_msg.status) - 1);
    log_msg.plaintext_size = plaintext_size;
    log_msg.ciphertext_size = ciphertext_size;
    strncpy(log_msg.encrypted_at, encrypted_at, sizeof(log_msg.encrypted_at) - 1);
    strncpy(log_msg.sent_at, sent_at, sizeof(log_msg.sent_at) - 1);
    strncpy(log_msg.delivered_at, delivered_at, sizeof(log_msg.delivered_at) - 1);
    strncpy(log_msg.read_at, read_at, sizeof(log_msg.read_at) - 1);
    strncpy(log_msg.error_code, error_code, sizeof(log_msg.error_code) - 1);
    strncpy(log_msg.error_message, error_message, sizeof(log_msg.error_message) - 1);
    strncpy(log_msg.client_ip, client_ip, sizeof(log_msg.client_ip) - 1);
    strncpy(log_msg.platform, platform, sizeof(log_msg.platform) - 1);

    // Insert into database
    int result = db_log_message(db_conn, &log_msg);

    json_object_put(payload);

    if (result != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to log message");
    }

    // Success response
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "message", json_object_new_string("Message logged successfully"));

    return http_send_json_response(connection, HTTP_OK, response);
}
