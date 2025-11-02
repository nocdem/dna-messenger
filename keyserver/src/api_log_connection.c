/*
 * API Handler: POST /api/logging/connection
 */

#include "keyserver.h"
#include "http_utils.h"
#include "rate_limit.h"
#include "db_logging.h"
#include <string.h>
#include <json-c/json.h>

enum MHD_Result api_log_connection_handler(struct MHD_Connection *connection, PGconn *db_conn,
                                            const char *upload_data, size_t upload_data_size) {
    char client_ip[46];

    // Get client IP
    if (http_get_client_ip(connection, client_ip, sizeof(client_ip)) != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to get client IP");
    }

    // Rate limiting
    if (!rate_limit_check(client_ip, RATE_LIMIT_TYPE_REGISTER)) {
        LOG_WARN("Rate limit exceeded for log_connection: %s", client_ip);
        return http_send_error(connection, HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    // Parse JSON payload
    json_object *payload = http_parse_json_post(upload_data, upload_data_size);
    if (!payload) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid JSON");
    }

    // Extract required fields
    json_object *field;

    // Required: connection_type
    if (!json_object_object_get_ex(payload, "connection_type", &field)) {
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing field: connection_type");
    }
    const char *connection_type = json_object_get_string(field);

    // Required: host
    if (!json_object_object_get_ex(payload, "host", &field)) {
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing field: host");
    }
    const char *host = json_object_get_string(field);

    // Required: port
    if (!json_object_object_get_ex(payload, "port", &field)) {
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing field: port");
    }
    int port = json_object_get_int(field);

    // Required: success
    if (!json_object_object_get_ex(payload, "success", &field)) {
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing field: success");
    }
    bool success = json_object_get_boolean(field);

    // Optional fields
    const char *identity = "";
    if (json_object_object_get_ex(payload, "identity", &field)) {
        identity = json_object_get_string(field);
    }

    int response_time_ms = 0;
    if (json_object_object_get_ex(payload, "response_time_ms", &field)) {
        response_time_ms = json_object_get_int(field);
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

    const char *app_version = "";
    if (json_object_object_get_ex(payload, "app_version", &field)) {
        app_version = json_object_get_string(field);
    }

    // Build log connection structure
    log_connection_t log_conn;
    memset(&log_conn, 0, sizeof(log_conn));

    strncpy(log_conn.identity, identity, sizeof(log_conn.identity) - 1);
    strncpy(log_conn.connection_type, connection_type, sizeof(log_conn.connection_type) - 1);
    strncpy(log_conn.host, host, sizeof(log_conn.host) - 1);
    log_conn.port = port;
    log_conn.success = success;
    log_conn.response_time_ms = response_time_ms;
    strncpy(log_conn.error_code, error_code, sizeof(log_conn.error_code) - 1);
    strncpy(log_conn.error_message, error_message, sizeof(log_conn.error_message) - 1);
    strncpy(log_conn.client_ip, client_ip, sizeof(log_conn.client_ip) - 1);
    strncpy(log_conn.platform, platform, sizeof(log_conn.platform) - 1);
    strncpy(log_conn.app_version, app_version, sizeof(log_conn.app_version) - 1);

    // Insert into database
    int result = db_log_connection(db_conn, &log_conn);

    json_object_put(payload);

    if (result != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to log connection");
    }

    // Success response
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "message", json_object_new_string("Connection logged successfully"));

    return http_send_json_response(connection, HTTP_OK, response);
}
