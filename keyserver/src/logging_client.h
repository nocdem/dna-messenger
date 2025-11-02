/*
 * Logging API Client Library
 *
 * Provides HTTP client functions to call the logging API endpoints
 * instead of directly inserting to database.
 */

#ifndef LOGGING_CLIENT_H
#define LOGGING_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Client configuration
typedef struct {
    char api_base_url[256];  // e.g., "http://localhost:8080"
    int timeout_seconds;
    char identity[33];        // Optional: Set for authenticated requests
    char platform[51];        // "android", "ios", "desktop", "keyserver"
    char app_version[51];
} logging_client_config_t;

// Initialize client configuration
void logging_client_init(logging_client_config_t *config, const char *api_base_url);

// Event logging
int logging_client_log_event(
    const logging_client_config_t *config,
    const char *event_type,        // "message_sent", "connection_success", etc.
    const char *severity,           // "debug", "info", "warning", "error", "critical"
    const char *message,
    const char *details_json,       // Optional: JSON string for additional data
    int64_t message_id,             // Optional: 0 if not applicable
    int group_id                    // Optional: 0 if not applicable
);

// Message logging
int logging_client_log_message(
    const logging_client_config_t *config,
    int64_t message_id,
    const char *sender,
    const char *recipient,
    int group_id,                   // 0 if not a group message
    const char *status,             // "sent", "delivered", "read", "failed"
    int plaintext_size,
    int ciphertext_size,
    const char *error_code,         // Optional: NULL if no error
    const char *error_message       // Optional: NULL if no error
);

// Connection logging
int logging_client_log_connection(
    const logging_client_config_t *config,
    const char *connection_type,    // "database", "keyserver", "rpc", "peer"
    const char *host,
    int port,
    bool success,
    int response_time_ms,           // 0 if not measured
    const char *error_code,         // Optional: NULL if no error
    const char *error_message       // Optional: NULL if no error
);

// Query statistics (GET request)
typedef struct {
    int64_t total_events;
    int64_t total_messages;
    int64_t total_connections;
    int64_t messages_sent;
    int64_t messages_delivered;
    int64_t messages_failed;
    int64_t connections_success;
    int64_t connections_failed;
    int64_t errors_count;
    int64_t warnings_count;
} logging_stats_t;

int logging_client_get_stats(
    const logging_client_config_t *config,
    const char *start_time,         // ISO timestamp: "2025-10-27 00:00:00"
    const char *end_time,           // ISO timestamp: "2025-10-28 00:00:00"
    logging_stats_t *stats
);

// Helper: Get current timestamp in ISO format
void logging_client_get_timestamp(char *buffer, size_t size);

#endif // LOGGING_CLIENT_H
