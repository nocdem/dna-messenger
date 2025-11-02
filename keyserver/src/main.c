/*
 * DNA Keyserver - Main Entry Point
 */

#include "keyserver.h"
#include "config.h"
#include "db.h"
#include "rate_limit.h"
#include "http_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>
#include <microhttpd.h>

// API handler declarations
enum MHD_Result api_health_handler(struct MHD_Connection *connection, PGconn *db_conn);
enum MHD_Result api_list_handler(struct MHD_Connection *connection, PGconn *db_conn, const char *url);
enum MHD_Result api_lookup_handler(struct MHD_Connection *connection, PGconn *db_conn, const char *identity);
enum MHD_Result api_register_handler(struct MHD_Connection *connection, PGconn *db_conn,
                                      const char *upload_data, size_t upload_data_size);
enum MHD_Result api_update_handler(struct MHD_Connection *connection, PGconn *db_conn,
                                    const char *upload_data, size_t upload_data_size);

// Logging API handler declarations
enum MHD_Result api_log_event_handler(struct MHD_Connection *connection, PGconn *db_conn,
                                       const char *upload_data, size_t upload_data_size);
enum MHD_Result api_log_message_handler(struct MHD_Connection *connection, PGconn *db_conn,
                                         const char *upload_data, size_t upload_data_size);
enum MHD_Result api_log_connection_handler(struct MHD_Connection *connection, PGconn *db_conn,
                                            const char *upload_data, size_t upload_data_size);
enum MHD_Result api_log_stats_handler(struct MHD_Connection *connection, PGconn *db_conn, const char *url);

// Messages API handler declarations
enum MHD_Result api_save_message_handler(struct MHD_Connection *connection, PGconn *db_conn,
                                         const char *upload_data, size_t upload_data_size);
enum MHD_Result api_load_conversation_handler(struct MHD_Connection *connection, PGconn *db_conn, const char *url);
enum MHD_Result api_load_group_messages_handler(struct MHD_Connection *connection, PGconn *db_conn, const char *url);
enum MHD_Result api_update_message_status_handler(struct MHD_Connection *connection, PGconn *db_conn,
                                                  const char *url, const char *upload_data, size_t upload_data_size);

// Contacts API handler declarations
enum MHD_Result api_save_contact_handler(struct MHD_Connection *connection, PGconn *db_conn,
                                         const char *upload_data, size_t upload_data_size);
enum MHD_Result api_load_contact_handler(struct MHD_Connection *connection, PGconn *db_conn, const char *url);
enum MHD_Result api_load_all_contacts_handler(struct MHD_Connection *connection, PGconn *db_conn);
enum MHD_Result api_delete_contact_handler(struct MHD_Connection *connection, PGconn *db_conn, const char *url);

// Groups API handler declarations
enum MHD_Result api_create_group_handler(struct MHD_Connection *connection, PGconn *db_conn,
                                         const char *upload_data, size_t upload_data_size);
enum MHD_Result api_load_group_handler(struct MHD_Connection *connection, PGconn *db_conn, const char *url);
enum MHD_Result api_load_user_groups_handler(struct MHD_Connection *connection, PGconn *db_conn);
enum MHD_Result api_add_group_member_handler(struct MHD_Connection *connection, PGconn *db_conn,
                                             const char *url, const char *upload_data, size_t upload_data_size);
enum MHD_Result api_remove_group_member_handler(struct MHD_Connection *connection, PGconn *db_conn, const char *url);
enum MHD_Result api_delete_group_handler(struct MHD_Connection *connection, PGconn *db_conn, const char *url);

// Global state
static struct MHD_Daemon *http_daemon = NULL;
static PGconn *db_conn = NULL;
static volatile sig_atomic_t running = 1;

// Logging
void log_message(const char *level, const char *fmt, ...) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(stderr, "[%s] %s - ", level, timestamp);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}

// POST data handler structure
struct post_data {
    char *data;
    size_t size;
};

// Request handler
static enum MHD_Result answer_to_connection(void *cls, struct MHD_Connection *connection,
                                             const char *url, const char *method,
                                             const char *version, const char *upload_data,
                                             size_t *upload_data_size, void **con_cls) {
    (void)cls;
    (void)version;

    // Handle POST data accumulation
    if (strcmp(method, "POST") == 0) {
        struct post_data *pd = *con_cls;

        if (pd == NULL) {
            // First call - allocate structure
            pd = calloc(1, sizeof(struct post_data));
            if (!pd) return MHD_NO;
            *con_cls = pd;
            return MHD_YES;
        }

        if (*upload_data_size > 0) {
            // Accumulate POST data
            char *new_data = realloc(pd->data, pd->size + *upload_data_size + 1);
            if (!new_data) {
                return MHD_NO;
            }
            pd->data = new_data;
            memcpy(pd->data + pd->size, upload_data, *upload_data_size);
            pd->size += *upload_data_size;
            pd->data[pd->size] = '\0';

            *upload_data_size = 0;
            return MHD_YES;
        }

        // All POST data received, process request
        enum MHD_Result ret;

        // Route: POST /api/keyserver/register
        if (strcmp(url, "/api/keyserver/register") == 0) {
            ret = api_register_handler(connection, db_conn, pd->data, pd->size);
        }
        // Route: POST /api/keyserver/update
        else if (strcmp(url, "/api/keyserver/update") == 0) {
            ret = api_update_handler(connection, db_conn, pd->data, pd->size);
        }
        // Route: POST /api/logging/event
        else if (strcmp(url, "/api/logging/event") == 0) {
            ret = api_log_event_handler(connection, db_conn, pd->data, pd->size);
        }
        // Route: POST /api/logging/message
        else if (strcmp(url, "/api/logging/message") == 0) {
            ret = api_log_message_handler(connection, db_conn, pd->data, pd->size);
        }
        // Route: POST /api/logging/connection
        else if (strcmp(url, "/api/logging/connection") == 0) {
            ret = api_log_connection_handler(connection, db_conn, pd->data, pd->size);
        }
        // Route: POST /api/messages
        else if (strcmp(url, "/api/messages") == 0) {
            ret = api_save_message_handler(connection, db_conn, pd->data, pd->size);
        }
        // Route: POST /api/contacts
        else if (strcmp(url, "/api/contacts") == 0) {
            ret = api_save_contact_handler(connection, db_conn, pd->data, pd->size);
        }
        // Route: POST /api/groups
        else if (strcmp(url, "/api/groups") == 0) {
            ret = api_create_group_handler(connection, db_conn, pd->data, pd->size);
        }
        // Route: POST /api/groups/:id/members
        else if (strstr(url, "/api/groups/") && strstr(url, "/members") && !strrchr(url+12, '/')) {
            ret = api_add_group_member_handler(connection, db_conn, url, pd->data, pd->size);
        } else {
            ret = http_send_error(connection, HTTP_NOT_FOUND, "Not found");
        }

        // Cleanup
        if (pd->data) free(pd->data);
        free(pd);
        *con_cls = NULL;

        return ret;
    }

    // GET requests
    if (strcmp(method, "GET") == 0) {
        // Route: GET /api/keyserver/health
        if (strcmp(url, "/api/keyserver/health") == 0) {
            return api_health_handler(connection, db_conn);
        }

        // Route: GET /api/keyserver/list
        if (strcmp(url, "/api/keyserver/list") == 0 ||
            strncmp(url, "/api/keyserver/list?", 20) == 0) {
            return api_list_handler(connection, db_conn, url);
        }

        // Route: GET /api/keyserver/lookup/<dna>
        if (strncmp(url, "/api/keyserver/lookup/", 22) == 0) {
            const char *dna = url + 22;
            return api_lookup_handler(connection, db_conn, dna);
        }

        // Route: GET /api/logging/stats
        if (strcmp(url, "/api/logging/stats") == 0 ||
            strncmp(url, "/api/logging/stats?", 19) == 0) {
            return api_log_stats_handler(connection, db_conn, url);
        }

        // Route: GET /api/messages/conversation
        if (strncmp(url, "/api/messages/conversation", 26) == 0) {
            return api_load_conversation_handler(connection, db_conn, url);
        }

        // Route: GET /api/messages/group/:id
        if (strncmp(url, "/api/messages/group/", 20) == 0) {
            return api_load_group_messages_handler(connection, db_conn, url);
        }

        // Route: GET /api/contacts/:identity
        if (strncmp(url, "/api/contacts/", 14) == 0 && strchr(url+14, '/') == NULL) {
            return api_load_contact_handler(connection, db_conn, url);
        }

        // Route: GET /api/contacts
        if (strcmp(url, "/api/contacts") == 0) {
            return api_load_all_contacts_handler(connection, db_conn);
        }

        // Route: GET /api/groups/:id
        if (strncmp(url, "/api/groups/", 12) == 0 && strchr(url+12, '/') == NULL && strchr(url+12, '?') == NULL) {
            return api_load_group_handler(connection, db_conn, url);
        }

        // Route: GET /api/groups?member=X
        if (strncmp(url, "/api/groups?", 12) == 0) {
            return api_load_user_groups_handler(connection, db_conn);
        }
    }

    // PATCH requests
    if (strcmp(method, "PATCH") == 0) {
        // Route: PATCH /api/messages/:id/status
        if (strstr(url, "/api/messages/") && strstr(url, "/status")) {
            return api_update_message_status_handler(connection, db_conn, url, NULL, 0);
        }
    }

    // DELETE requests
    if (strcmp(method, "DELETE") == 0) {
        // Route: DELETE /api/contacts/:identity
        if (strncmp(url, "/api/contacts/", 14) == 0) {
            return api_delete_contact_handler(connection, db_conn, url);
        }

        // Route: DELETE /api/groups/:id/members/:identity
        if (strstr(url, "/api/groups/") && strstr(url, "/members/")) {
            return api_remove_group_member_handler(connection, db_conn, url);
        }

        // Route: DELETE /api/groups/:id
        if (strncmp(url, "/api/groups/", 12) == 0 && !strstr(url, "/members")) {
            return api_delete_group_handler(connection, db_conn, url);
        }
    }

    // 404 Not Found
    return http_send_error(connection, HTTP_NOT_FOUND, "Not found");
}

// Signal handler
static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

// Cleanup on request completion
static void request_completed(void *cls, struct MHD_Connection *connection,
                             void **con_cls, enum MHD_RequestTerminationCode toe) {
    (void)cls;
    (void)connection;
    (void)toe;

    struct post_data *pd = *con_cls;
    if (pd) {
        if (pd->data) free(pd->data);
        free(pd);
    }
}

int main(int argc, char *argv[]) {
    printf("====================================\n");
    printf(" DNA Keyserver v%s\n", KEYSERVER_VERSION);
    printf("====================================\n\n");

    // Load configuration
    config_init_defaults(&g_config);

    if (argc > 1) {
        if (config_load(argv[1], &g_config) != 0) {
            fprintf(stderr, "Using default configuration\n");
        } else {
            LOG_INFO("Loaded configuration from: %s", argv[1]);
        }
    } else {
        LOG_INFO("Using default configuration (no config file specified)");
    }

    config_print(&g_config);
    printf("\n");

    // Connect to database
    LOG_INFO("Connecting to PostgreSQL...");
    db_conn = db_connect(&g_config);
    if (!db_conn) {
        LOG_ERROR("Failed to connect to database");
        return 1;
    }

    // Initialize rate limiter
    rate_limit_init();
    LOG_INFO("Rate limiter initialized");

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Start HTTP server
    LOG_INFO("Starting HTTP server on %s:%d", g_config.bind_address, g_config.port);

    http_daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG,
        g_config.port,
        NULL, NULL,
        &answer_to_connection, NULL,
        MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
        MHD_OPTION_CONNECTION_LIMIT, (unsigned int)g_config.max_connections,
        MHD_OPTION_END
    );

    if (!http_daemon) {
        LOG_ERROR("Failed to start HTTP server");
        db_disconnect(db_conn);
        return 1;
    }

    printf("\n");
    printf("====================================\n");
    printf(" Keyserver ONLINE\n");
    printf("====================================\n");
    printf("Endpoints:\n");
    printf("  POST /api/keyserver/register\n");
    printf("  POST /api/keyserver/update\n");
    printf("  GET  /api/keyserver/lookup/<dna>\n");
    printf("  GET  /api/keyserver/list\n");
    printf("  GET  /api/keyserver/health\n");
    printf("\n");
    printf("Press Ctrl+C to stop\n");
    printf("====================================\n\n");

    // Main loop
    while (running) {
        sleep(1);
    }

    // Cleanup
    LOG_INFO("Shutting down...");

    if (http_daemon) {
        MHD_stop_daemon(http_daemon);
    }

    rate_limit_cleanup();
    db_disconnect(db_conn);

    LOG_INFO("Keyserver stopped");
    return 0;
}
