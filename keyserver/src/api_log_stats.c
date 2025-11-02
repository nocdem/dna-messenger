/*
 * API Handler: GET /api/logging/stats
 */

#include "keyserver.h"
#include "http_utils.h"
#include "rate_limit.h"
#include "db_logging.h"
#include <string.h>
#include <json-c/json.h>

enum MHD_Result api_log_stats_handler(struct MHD_Connection *connection, PGconn *db_conn, const char *url) {
    char client_ip[46];

    // Get client IP
    if (http_get_client_ip(connection, client_ip, sizeof(client_ip)) != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to get client IP");
    }

    // Rate limiting (reuse list rate limit)
    if (!rate_limit_check(client_ip, RATE_LIMIT_TYPE_LIST)) {
        LOG_WARN("Rate limit exceeded for log_stats: %s", client_ip);
        return http_send_error(connection, HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    // Parse query parameters (start_time and end_time)
    const char *start_time = NULL;
    const char *end_time = NULL;

    // Simple query parameter parsing
    const char *query_start = strchr(url, '?');
    if (query_start) {
        // Parse query string (simplified)
        char query[512];
        strncpy(query, query_start + 1, sizeof(query) - 1);

        char *token = strtok(query, "&");
        while (token) {
            char *eq = strchr(token, '=');
            if (eq) {
                *eq = '\0';
                const char *key = token;
                const char *value = eq + 1;

                if (strcmp(key, "start_time") == 0) {
                    start_time = value;
                } else if (strcmp(key, "end_time") == 0) {
                    end_time = value;
                }
            }
            token = strtok(NULL, "&");
        }
    }

    // Default to last 24 hours if not provided
    if (!start_time || !end_time) {
        return http_send_error(connection, HTTP_BAD_REQUEST,
                             "Missing required query parameters: start_time and end_time");
    }

    // Query statistics
    log_stats_t stats;
    memset(&stats, 0, sizeof(stats));

    int result = db_get_stats(db_conn, start_time, end_time, &stats);

    if (result == -2) {
        // Stats not found, compute them
        LOG_INFO("Stats not found, computing for period: %s to %s", start_time, end_time);
        if (db_compute_stats(db_conn, start_time, end_time) != 0) {
            return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to compute statistics");
        }
        // Try again
        result = db_get_stats(db_conn, start_time, end_time, &stats);
    }

    if (result != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to get statistics");
    }

    // Build JSON response
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "period_start", json_object_new_string(start_time));
    json_object_object_add(response, "period_end", json_object_new_string(end_time));

    json_object *stats_obj = json_object_new_object();
    json_object_object_add(stats_obj, "total_events", json_object_new_int64(stats.total_events));
    json_object_object_add(stats_obj, "total_messages", json_object_new_int64(stats.total_messages));
    json_object_object_add(stats_obj, "total_connections", json_object_new_int64(stats.total_connections));
    json_object_object_add(stats_obj, "messages_sent", json_object_new_int64(stats.messages_sent));
    json_object_object_add(stats_obj, "messages_delivered", json_object_new_int64(stats.messages_delivered));
    json_object_object_add(stats_obj, "messages_failed", json_object_new_int64(stats.messages_failed));
    json_object_object_add(stats_obj, "connections_success", json_object_new_int64(stats.connections_success));
    json_object_object_add(stats_obj, "connections_failed", json_object_new_int64(stats.connections_failed));
    json_object_object_add(stats_obj, "errors_count", json_object_new_int64(stats.errors_count));
    json_object_object_add(stats_obj, "warnings_count", json_object_new_int64(stats.warnings_count));

    json_object_object_add(response, "statistics", stats_obj);

    return http_send_json_response(connection, HTTP_OK, response);
}
