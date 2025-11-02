/*
 * API Handler: POST /api/logging/event
 */

#include "keyserver.h"
#include "http_utils.h"
#include "rate_limit.h"
#include "db_logging.h"
#include <string.h>
#include <json-c/json.h>

enum MHD_Result api_log_event_handler(struct MHD_Connection *connection, PGconn *db_conn,
                                       const char *upload_data, size_t upload_data_size) {
    char client_ip[46];
    char error_msg[512];

    // Get client IP
    if (http_get_client_ip(connection, client_ip, sizeof(client_ip)) != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to get client IP");
    }

    // Rate limiting (reuse register rate limit for now)
    if (!rate_limit_check(client_ip, RATE_LIMIT_TYPE_REGISTER)) {
        LOG_WARN("Rate limit exceeded for log_event: %s", client_ip);
        return http_send_error(connection, HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    // Parse JSON payload
    json_object *payload = http_parse_json_post(upload_data, upload_data_size);
    if (!payload) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid JSON");
    }

    // Extract fields
    json_object *field;

    // Required: event_type
    if (!json_object_object_get_ex(payload, "event_type", &field)) {
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing field: event_type");
    }
    const char *event_type_str = json_object_get_string(field);

    // Required: message
    if (!json_object_object_get_ex(payload, "message", &field)) {
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing field: message");
    }
    const char *message = json_object_get_string(field);

    // Optional fields with defaults
    const char *severity_str = "info";
    if (json_object_object_get_ex(payload, "severity", &field)) {
        severity_str = json_object_get_string(field);
    }

    const char *identity = "";
    if (json_object_object_get_ex(payload, "identity", &field)) {
        identity = json_object_get_string(field);
    }

    const char *platform = "";
    if (json_object_object_get_ex(payload, "platform", &field)) {
        platform = json_object_get_string(field);
    }

    const char *app_version = "";
    if (json_object_object_get_ex(payload, "app_version", &field)) {
        app_version = json_object_get_string(field);
    }

    const char *user_agent = "";
    if (json_object_object_get_ex(payload, "user_agent", &field)) {
        user_agent = json_object_get_string(field);
    }

    int64_t client_timestamp = 0;
    if (json_object_object_get_ex(payload, "client_timestamp", &field)) {
        client_timestamp = json_object_get_int64(field);
    }

    int64_t message_id = 0;
    if (json_object_object_get_ex(payload, "message_id", &field)) {
        message_id = json_object_get_int64(field);
    }

    int group_id = 0;
    if (json_object_object_get_ex(payload, "group_id", &field)) {
        group_id = json_object_get_int(field);
    }

    // Extract details JSON (optional)
    char *details_json = NULL;
    if (json_object_object_get_ex(payload, "details", &field)) {
        const char *details_str = json_object_to_json_string(field);
        if (details_str) {
            details_json = strdup(details_str);
        }
    }

    // Build log event structure
    log_event_t event;
    memset(&event, 0, sizeof(event));

    event.event_type = string_to_event_type(event_type_str);
    event.severity = string_to_severity_level(severity_str);
    strncpy(event.identity, identity, sizeof(event.identity) - 1);
    strncpy(event.message, message, sizeof(event.message) - 1);
    event.details_json = details_json;
    strncpy(event.client_ip, client_ip, sizeof(event.client_ip) - 1);
    event.user_agent = (char*)user_agent;
    strncpy(event.platform, platform, sizeof(event.platform) - 1);
    strncpy(event.app_version, app_version, sizeof(event.app_version) - 1);
    event.client_timestamp = client_timestamp;
    event.message_id = message_id;
    event.group_id = group_id;

    // Insert into database
    int result = db_log_event(db_conn, &event);

    // Cleanup
    if (details_json) free(details_json);
    json_object_put(payload);

    if (result != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to log event");
    }

    // Success response
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "message", json_object_new_string("Event logged successfully"));

    return http_send_json_response(connection, HTTP_OK, response);
}
