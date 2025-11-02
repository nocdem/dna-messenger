/*
 * Database Layer - Logging Operations
 */

#include "db_logging.h"
#include "keyserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper: Convert event type enum to string
const char* event_type_to_string(event_type_t type) {
    switch (type) {
        case EVENT_MESSAGE_SENT: return "message_sent";
        case EVENT_MESSAGE_RECEIVED: return "message_received";
        case EVENT_MESSAGE_FAILED: return "message_failed";
        case EVENT_CONNECTION_SUCCESS: return "connection_success";
        case EVENT_CONNECTION_FAILED: return "connection_failed";
        case EVENT_AUTH_SUCCESS: return "auth_success";
        case EVENT_AUTH_FAILED: return "auth_failed";
        case EVENT_KEY_GENERATED: return "key_generated";
        case EVENT_KEY_EXPORTED: return "key_exported";
        case EVENT_GROUP_CREATED: return "group_created";
        case EVENT_GROUP_JOINED: return "group_joined";
        case EVENT_GROUP_LEFT: return "group_left";
        case EVENT_CONTACT_ADDED: return "contact_added";
        case EVENT_CONTACT_REMOVED: return "contact_removed";
        case EVENT_APP_STARTED: return "app_started";
        case EVENT_APP_STOPPED: return "app_stopped";
        case EVENT_ERROR: return "error";
        case EVENT_WARNING: return "warning";
        case EVENT_INFO: return "info";
        case EVENT_DEBUG: return "debug";
        default: return "info";
    }
}

// Helper: Convert severity level enum to string
const char* severity_level_to_string(severity_level_t level) {
    switch (level) {
        case SEVERITY_DEBUG: return "debug";
        case SEVERITY_INFO: return "info";
        case SEVERITY_WARNING: return "warning";
        case SEVERITY_ERROR: return "error";
        case SEVERITY_CRITICAL: return "critical";
        default: return "info";
    }
}

// Helper: Convert string to event type enum
event_type_t string_to_event_type(const char *str) {
    if (strcmp(str, "message_sent") == 0) return EVENT_MESSAGE_SENT;
    if (strcmp(str, "message_received") == 0) return EVENT_MESSAGE_RECEIVED;
    if (strcmp(str, "message_failed") == 0) return EVENT_MESSAGE_FAILED;
    if (strcmp(str, "connection_success") == 0) return EVENT_CONNECTION_SUCCESS;
    if (strcmp(str, "connection_failed") == 0) return EVENT_CONNECTION_FAILED;
    if (strcmp(str, "auth_success") == 0) return EVENT_AUTH_SUCCESS;
    if (strcmp(str, "auth_failed") == 0) return EVENT_AUTH_FAILED;
    if (strcmp(str, "key_generated") == 0) return EVENT_KEY_GENERATED;
    if (strcmp(str, "key_exported") == 0) return EVENT_KEY_EXPORTED;
    if (strcmp(str, "group_created") == 0) return EVENT_GROUP_CREATED;
    if (strcmp(str, "group_joined") == 0) return EVENT_GROUP_JOINED;
    if (strcmp(str, "group_left") == 0) return EVENT_GROUP_LEFT;
    if (strcmp(str, "contact_added") == 0) return EVENT_CONTACT_ADDED;
    if (strcmp(str, "contact_removed") == 0) return EVENT_CONTACT_REMOVED;
    if (strcmp(str, "app_started") == 0) return EVENT_APP_STARTED;
    if (strcmp(str, "app_stopped") == 0) return EVENT_APP_STOPPED;
    if (strcmp(str, "error") == 0) return EVENT_ERROR;
    if (strcmp(str, "warning") == 0) return EVENT_WARNING;
    if (strcmp(str, "debug") == 0) return EVENT_DEBUG;
    return EVENT_INFO;
}

// Helper: Convert string to severity level enum
severity_level_t string_to_severity_level(const char *str) {
    if (strcmp(str, "debug") == 0) return SEVERITY_DEBUG;
    if (strcmp(str, "info") == 0) return SEVERITY_INFO;
    if (strcmp(str, "warning") == 0) return SEVERITY_WARNING;
    if (strcmp(str, "error") == 0) return SEVERITY_ERROR;
    if (strcmp(str, "critical") == 0) return SEVERITY_CRITICAL;
    return SEVERITY_INFO;
}

// Log a general event
int db_log_event(PGconn *conn, const log_event_t *event) {
    const char *sql =
        "INSERT INTO logging_events "
        "(event_type, severity, identity, message, details, "
        " client_ip, user_agent, platform, app_version, "
        " client_timestamp, message_id, group_id) "
        "VALUES ($1::event_type, $2::severity_level, "
        "NULLIF($3, ''), $4, $5::jsonb, "
        "NULLIF($6, '')::inet, NULLIF($7, ''), NULLIF($8, ''), NULLIF($9, ''), "
        "NULLIF($10, '0')::bigint, NULLIF($11, '0')::bigint, NULLIF($12, '0')::integer)";

    char client_timestamp_str[32] = {0};
    char message_id_str[32] = {0};
    char group_id_str[32] = {0};

    if (event->client_timestamp > 0) {
        snprintf(client_timestamp_str, sizeof(client_timestamp_str), "%ld", event->client_timestamp);
    }
    if (event->message_id > 0) {
        snprintf(message_id_str, sizeof(message_id_str), "%ld", event->message_id);
    }
    if (event->group_id > 0) {
        snprintf(group_id_str, sizeof(group_id_str), "%d", event->group_id);
    }

    const char *paramValues[12] = {
        event_type_to_string(event->event_type),
        severity_level_to_string(event->severity),
        event->identity,
        event->message,
        event->details_json ? event->details_json : "{}",
        event->client_ip,
        event->user_agent,
        event->platform,
        event->app_version,
        client_timestamp_str,
        message_id_str,
        group_id_str
    };

    PGresult *res = PQexecParams(conn, sql, 12, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Failed to log event: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    return 0;
}

// Log a message event
int db_log_message(PGconn *conn, const log_message_t *message) {
    const char *sql =
        "INSERT INTO logging_messages "
        "(message_id, sender, recipient, group_id, status, "
        " plaintext_size, ciphertext_size, "
        " encrypted_at, sent_at, delivered_at, read_at, "
        " error_code, error_message, client_ip, platform) "
        "VALUES (NULLIF($1, '0')::bigint, $2, $3, NULLIF($4, '0')::integer, $5, "
        "$6, $7, "
        "NULLIF($8, '')::timestamp, NULLIF($9, '')::timestamp, "
        "NULLIF($10, '')::timestamp, NULLIF($11, '')::timestamp, "
        "NULLIF($12, ''), NULLIF($13, ''), NULLIF($14, '')::inet, NULLIF($15, ''))";

    char message_id_str[32] = {0};
    char group_id_str[32] = {0};
    char plaintext_size_str[32];
    char ciphertext_size_str[32];

    if (message->message_id > 0) {
        snprintf(message_id_str, sizeof(message_id_str), "%ld", message->message_id);
    }
    if (message->group_id > 0) {
        snprintf(group_id_str, sizeof(group_id_str), "%d", message->group_id);
    }
    snprintf(plaintext_size_str, sizeof(plaintext_size_str), "%d", message->plaintext_size);
    snprintf(ciphertext_size_str, sizeof(ciphertext_size_str), "%d", message->ciphertext_size);

    const char *paramValues[15] = {
        message_id_str,
        message->sender,
        message->recipient,
        group_id_str,
        message->status,
        plaintext_size_str,
        ciphertext_size_str,
        message->encrypted_at,
        message->sent_at,
        message->delivered_at,
        message->read_at,
        message->error_code,
        message->error_message,
        message->client_ip,
        message->platform
    };

    PGresult *res = PQexecParams(conn, sql, 15, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Failed to log message: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    return 0;
}

// Log a connection event
int db_log_connection(PGconn *conn, const log_connection_t *connection) {
    const char *sql =
        "INSERT INTO logging_connections "
        "(identity, connection_type, host, port, success, "
        " response_time_ms, error_code, error_message, "
        " client_ip, platform, app_version) "
        "VALUES (NULLIF($1, ''), $2, $3, $4, $5, "
        "NULLIF($6, '0')::integer, NULLIF($7, ''), NULLIF($8, ''), "
        "NULLIF($9, '')::inet, NULLIF($10, ''), NULLIF($11, ''))";

    char port_str[16];
    char response_time_str[32] = {0};

    snprintf(port_str, sizeof(port_str), "%d", connection->port);
    if (connection->response_time_ms > 0) {
        snprintf(response_time_str, sizeof(response_time_str), "%d", connection->response_time_ms);
    }

    const char *paramValues[11] = {
        connection->identity,
        connection->connection_type,
        connection->host,
        port_str,
        connection->success ? "true" : "false",
        response_time_str,
        connection->error_code,
        connection->error_message,
        connection->client_ip,
        connection->platform,
        connection->app_version
    };

    PGresult *res = PQexecParams(conn, sql, 11, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Failed to log connection: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    return 0;
}

// Query events (simplified version)
int db_query_events(PGconn *conn, const char *identity, event_type_t event_type,
                   int limit, int offset, log_event_t **events, int *count) {
    // TODO: Implement full query functionality
    // For now, return not implemented
    *events = NULL;
    *count = 0;
    return 0;
}

// Query messages (simplified version)
int db_query_messages(PGconn *conn, const char *identity, const char *status,
                     int limit, int offset, log_message_t **messages, int *count) {
    // TODO: Implement full query functionality
    *messages = NULL;
    *count = 0;
    return 0;
}

// Query connections (simplified version)
int db_query_connections(PGconn *conn, const char *identity, bool success_only,
                        int limit, int offset, log_connection_t **connections, int *count) {
    // TODO: Implement full query functionality
    *connections = NULL;
    *count = 0;
    return 0;
}

// Get statistics
int db_get_stats(PGconn *conn, const char *start_time, const char *end_time,
                log_stats_t *stats) {
    const char *sql =
        "SELECT "
        "total_events, total_messages, total_connections, "
        "messages_sent, messages_delivered, messages_failed, "
        "connections_success, connections_failed, "
        "errors_count, warnings_count "
        "FROM logging_stats "
        "WHERE period_start = $1::timestamp AND period_end = $2::timestamp "
        "ORDER BY computed_at DESC LIMIT 1";

    const char *paramValues[2] = {start_time, end_time};

    PGresult *res = PQexecParams(conn, sql, 2, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        LOG_ERROR("Failed to get stats: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        return -2;  // Not found
    }

    // Parse stats
    stats->total_events = atoll(PQgetvalue(res, 0, 0));
    stats->total_messages = atoll(PQgetvalue(res, 0, 1));
    stats->total_connections = atoll(PQgetvalue(res, 0, 2));
    stats->messages_sent = atoll(PQgetvalue(res, 0, 3));
    stats->messages_delivered = atoll(PQgetvalue(res, 0, 4));
    stats->messages_failed = atoll(PQgetvalue(res, 0, 5));
    stats->connections_success = atoll(PQgetvalue(res, 0, 6));
    stats->connections_failed = atoll(PQgetvalue(res, 0, 7));
    stats->errors_count = atoll(PQgetvalue(res, 0, 8));
    stats->warnings_count = atoll(PQgetvalue(res, 0, 9));

    PQclear(res);
    return 0;
}

// Compute statistics for a period
int db_compute_stats(PGconn *conn, const char *start_time, const char *end_time) {
    const char *sql = "SELECT compute_statistics($1::timestamp, $2::timestamp)";

    const char *paramValues[2] = {start_time, end_time};

    PGresult *res = PQexecParams(conn, sql, 2, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        LOG_ERROR("Failed to compute stats: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    return 0;
}

// Cleanup old logs
int db_cleanup_old_logs(PGconn *conn) {
    const char *sql = "SELECT cleanup_old_logs()";

    PGresult *res = PQexec(conn, sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        LOG_ERROR("Failed to cleanup old logs: %s", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    LOG_INFO("Old logs cleaned up successfully");
    return 0;
}
