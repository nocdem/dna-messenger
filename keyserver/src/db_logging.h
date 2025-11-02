/*
 * Database Layer - Logging Operations
 */

#ifndef DB_LOGGING_H
#define DB_LOGGING_H

#include <libpq-fe.h>
#include <stdint.h>
#include <stdbool.h>

// Event types
typedef enum {
    EVENT_MESSAGE_SENT,
    EVENT_MESSAGE_RECEIVED,
    EVENT_MESSAGE_FAILED,
    EVENT_CONNECTION_SUCCESS,
    EVENT_CONNECTION_FAILED,
    EVENT_AUTH_SUCCESS,
    EVENT_AUTH_FAILED,
    EVENT_KEY_GENERATED,
    EVENT_KEY_EXPORTED,
    EVENT_GROUP_CREATED,
    EVENT_GROUP_JOINED,
    EVENT_GROUP_LEFT,
    EVENT_CONTACT_ADDED,
    EVENT_CONTACT_REMOVED,
    EVENT_APP_STARTED,
    EVENT_APP_STOPPED,
    EVENT_ERROR,
    EVENT_WARNING,
    EVENT_INFO,
    EVENT_DEBUG
} event_type_t;

// Severity levels
typedef enum {
    SEVERITY_DEBUG,
    SEVERITY_INFO,
    SEVERITY_WARNING,
    SEVERITY_ERROR,
    SEVERITY_CRITICAL
} severity_level_t;

// Log event structure
typedef struct {
    event_type_t event_type;
    severity_level_t severity;
    char identity[33];          // Nullable (use empty string for system events)
    char message[1024];
    char *details_json;         // JSONB field (nullable)
    char client_ip[46];         // Nullable
    char *user_agent;           // Nullable
    char platform[51];          // 'android', 'ios', 'desktop', 'keyserver'
    char app_version[51];       // Nullable
    int64_t client_timestamp;   // Unix timestamp (0 if not provided)
    int64_t message_id;         // Reference to messages.id (0 if not applicable)
    int group_id;               // Reference to groups.id (0 if not applicable)
} log_event_t;

// Message log structure
typedef struct {
    int64_t message_id;         // Unique message ID
    char sender[33];
    char recipient[33];
    int group_id;               // 0 if not a group message
    char status[21];            // 'sent', 'delivered', 'read', 'failed'
    int plaintext_size;
    int ciphertext_size;
    char encrypted_at[32];      // ISO timestamp (nullable)
    char sent_at[32];           // ISO timestamp (nullable)
    char delivered_at[32];      // ISO timestamp (nullable)
    char read_at[32];           // ISO timestamp (nullable)
    char error_code[51];        // Nullable
    char error_message[512];    // Nullable
    char client_ip[46];         // Nullable
    char platform[51];          // Nullable
} log_message_t;

// Connection log structure
typedef struct {
    char identity[33];          // Nullable
    char connection_type[51];   // 'database', 'keyserver', 'rpc', 'peer'
    char host[256];
    int port;
    bool success;
    int response_time_ms;       // 0 if not measured
    char error_code[51];        // Nullable
    char error_message[512];    // Nullable
    char client_ip[46];         // Nullable
    char platform[51];          // Nullable
    char app_version[51];       // Nullable
} log_connection_t;

// Statistics structure
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
} log_stats_t;

// Database functions
int db_log_event(PGconn *conn, const log_event_t *event);
int db_log_message(PGconn *conn, const log_message_t *message);
int db_log_connection(PGconn *conn, const log_connection_t *connection);

// Query functions
int db_query_events(PGconn *conn, const char *identity, event_type_t event_type,
                   int limit, int offset, log_event_t **events, int *count);
int db_query_messages(PGconn *conn, const char *identity, const char *status,
                     int limit, int offset, log_message_t **messages, int *count);
int db_query_connections(PGconn *conn, const char *identity, bool success_only,
                        int limit, int offset, log_connection_t **connections, int *count);

// Statistics functions
int db_get_stats(PGconn *conn, const char *start_time, const char *end_time,
                log_stats_t *stats);
int db_compute_stats(PGconn *conn, const char *start_time, const char *end_time);

// Cleanup
int db_cleanup_old_logs(PGconn *conn);

// Helper functions
const char* event_type_to_string(event_type_t type);
const char* severity_level_to_string(severity_level_t level);
event_type_t string_to_event_type(const char *str);
severity_level_t string_to_severity_level(const char *str);

#endif // DB_LOGGING_H
