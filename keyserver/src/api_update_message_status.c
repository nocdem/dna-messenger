/**
 * PATCH /api/messages/:id/status
 * Body: {"status": "delivered"}
 */

#include "keyserver.h"
#include "http_utils.h"
#include "db_messages.h"
#include "rate_limit.h"
#include <json-c/json.h>
#include <microhttpd.h>
#include <string.h>
#include <stdlib.h>

enum MHD_Result api_update_message_status_handler(
    struct MHD_Connection *connection,
    PGconn *db_conn,
    const char *url,
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

    // Parse message_id from URL (/api/messages/123/status)
    // Find the number after /api/messages/ and before /status
    const char *id_start = strstr(url, "/api/messages/");
    if (!id_start) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid URL");
    }
    id_start += strlen("/api/messages/");

    const char *id_end = strstr(id_start, "/status");
    if (!id_end) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing /status in URL");
    }

    char message_id_str[32] = {0};
    size_t id_len = id_end - id_start;
    if (id_len >= sizeof(message_id_str)) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid message_id");
    }

    strncpy(message_id_str, id_start, id_len);
    int64_t message_id = atoll(message_id_str);

    if (message_id <= 0) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid message_id");
    }

    // Parse JSON body
    json_object *payload = http_parse_json_post(upload_data, upload_data_size);
    if (!payload) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid JSON");
    }

    // Extract status
    json_object *status_obj;
    if (!json_object_object_get_ex(payload, "status", &status_obj)) {
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing status field");
    }

    const char *status = json_object_get_string(status_obj);
    if (!status || strlen(status) == 0) {
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid status value");
    }

    // Validate status value
    if (strcmp(status, "pending") != 0 &&
        strcmp(status, "sent") != 0 &&
        strcmp(status, "delivered") != 0 &&
        strcmp(status, "read") != 0 &&
        strcmp(status, "failed") != 0) {
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST,
                              "Invalid status. Must be: pending, sent, delivered, read, or failed");
    }

    // Update status in database
    int result = db_update_message_status(db_conn, message_id, status);

    json_object_put(payload);

    if (result != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to update message status");
    }

    // Build response
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "message_id", json_object_new_int64(message_id));
    json_object_object_add(response, "status", json_object_new_string(status));

    enum MHD_Result ret = http_send_json_response(connection, HTTP_OK, response);
    json_object_put(response);

    return ret;
}
