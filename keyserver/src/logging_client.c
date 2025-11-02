/*
 * Logging API Client Library - Implementation
 */

#include "logging_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <json-c/json.h>

// Response buffer for curl
struct response_buffer {
    char *data;
    size_t size;
};

// Callback for curl write
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct response_buffer *buf = (struct response_buffer *)userp;

    char *ptr = realloc(buf->data, buf->size + realsize + 1);
    if (ptr == NULL) {
        return 0;  // Out of memory
    }

    buf->data = ptr;
    memcpy(&(buf->data[buf->size]), contents, realsize);
    buf->size += realsize;
    buf->data[buf->size] = 0;

    return realsize;
}

// Initialize client configuration
void logging_client_init(logging_client_config_t *config, const char *api_base_url) {
    memset(config, 0, sizeof(logging_client_config_t));
    strncpy(config->api_base_url, api_base_url, sizeof(config->api_base_url) - 1);
    config->timeout_seconds = 5;
}

// Get current timestamp in ISO format
void logging_client_get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = gmtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

// Log an event
int logging_client_log_event(
    const logging_client_config_t *config,
    const char *event_type,
    const char *severity,
    const char *message,
    const char *details_json,
    int64_t message_id,
    int group_id
) {
    CURL *curl;
    CURLcode res;
    int return_code = -1;

    curl = curl_easy_init();
    if (!curl) {
        return -1;
    }

    // Build URL
    char url[512];
    snprintf(url, sizeof(url), "%s/api/logging/event", config->api_base_url);

    // Build JSON payload
    json_object *payload = json_object_new_object();
    json_object_object_add(payload, "event_type", json_object_new_string(event_type));
    json_object_object_add(payload, "severity", json_object_new_string(severity ? severity : "info"));
    json_object_object_add(payload, "message", json_object_new_string(message));

    if (config->identity[0] != '\0') {
        json_object_object_add(payload, "identity", json_object_new_string(config->identity));
    }

    if (config->platform[0] != '\0') {
        json_object_object_add(payload, "platform", json_object_new_string(config->platform));
    }

    if (config->app_version[0] != '\0') {
        json_object_object_add(payload, "app_version", json_object_new_string(config->app_version));
    }

    if (details_json) {
        json_object *details = json_tokener_parse(details_json);
        if (details) {
            json_object_object_add(payload, "details", details);
        }
    }

    if (message_id > 0) {
        json_object_object_add(payload, "message_id", json_object_new_int64(message_id));
    }

    if (group_id > 0) {
        json_object_object_add(payload, "group_id", json_object_new_int(group_id));
    }

    // Add client timestamp
    json_object_object_add(payload, "client_timestamp", json_object_new_int64((int64_t)time(NULL)));

    const char *json_string = json_object_to_json_string(payload);

    // Setup curl
    struct response_buffer response;
    response.data = malloc(1);
    response.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config->timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform request
    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 200) {
            return_code = 0;  // Success
        }
    }

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    json_object_put(payload);
    free(response.data);

    return return_code;
}

// Log a message
int logging_client_log_message(
    const logging_client_config_t *config,
    int64_t message_id,
    const char *sender,
    const char *recipient,
    int group_id,
    const char *status,
    int plaintext_size,
    int ciphertext_size,
    const char *error_code,
    const char *error_message
) {
    CURL *curl;
    CURLcode res;
    int return_code = -1;

    curl = curl_easy_init();
    if (!curl) {
        return -1;
    }

    // Build URL
    char url[512];
    snprintf(url, sizeof(url), "%s/api/logging/message", config->api_base_url);

    // Build JSON payload
    json_object *payload = json_object_new_object();

    if (message_id > 0) {
        json_object_object_add(payload, "message_id", json_object_new_int64(message_id));
    }

    json_object_object_add(payload, "sender", json_object_new_string(sender));
    json_object_object_add(payload, "recipient", json_object_new_string(recipient));

    if (group_id > 0) {
        json_object_object_add(payload, "group_id", json_object_new_int(group_id));
    }

    json_object_object_add(payload, "status", json_object_new_string(status));
    json_object_object_add(payload, "plaintext_size", json_object_new_int(plaintext_size));
    json_object_object_add(payload, "ciphertext_size", json_object_new_int(ciphertext_size));

    if (config->platform[0] != '\0') {
        json_object_object_add(payload, "platform", json_object_new_string(config->platform));
    }

    if (error_code) {
        json_object_object_add(payload, "error_code", json_object_new_string(error_code));
    }

    if (error_message) {
        json_object_object_add(payload, "error_message", json_object_new_string(error_message));
    }

    const char *json_string = json_object_to_json_string(payload);

    // Setup curl
    struct response_buffer response;
    response.data = malloc(1);
    response.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config->timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform request
    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 200) {
            return_code = 0;  // Success
        }
    }

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    json_object_put(payload);
    free(response.data);

    return return_code;
}

// Log a connection
int logging_client_log_connection(
    const logging_client_config_t *config,
    const char *connection_type,
    const char *host,
    int port,
    bool success,
    int response_time_ms,
    const char *error_code,
    const char *error_message
) {
    CURL *curl;
    CURLcode res;
    int return_code = -1;

    curl = curl_easy_init();
    if (!curl) {
        return -1;
    }

    // Build URL
    char url[512];
    snprintf(url, sizeof(url), "%s/api/logging/connection", config->api_base_url);

    // Build JSON payload
    json_object *payload = json_object_new_object();

    if (config->identity[0] != '\0') {
        json_object_object_add(payload, "identity", json_object_new_string(config->identity));
    }

    json_object_object_add(payload, "connection_type", json_object_new_string(connection_type));
    json_object_object_add(payload, "host", json_object_new_string(host));
    json_object_object_add(payload, "port", json_object_new_int(port));
    json_object_object_add(payload, "success", json_object_new_boolean(success));

    if (response_time_ms > 0) {
        json_object_object_add(payload, "response_time_ms", json_object_new_int(response_time_ms));
    }

    if (config->platform[0] != '\0') {
        json_object_object_add(payload, "platform", json_object_new_string(config->platform));
    }

    if (config->app_version[0] != '\0') {
        json_object_object_add(payload, "app_version", json_object_new_string(config->app_version));
    }

    if (error_code) {
        json_object_object_add(payload, "error_code", json_object_new_string(error_code));
    }

    if (error_message) {
        json_object_object_add(payload, "error_message", json_object_new_string(error_message));
    }

    const char *json_string = json_object_to_json_string(payload);

    // Setup curl
    struct response_buffer response;
    response.data = malloc(1);
    response.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config->timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform request
    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 200) {
            return_code = 0;  // Success
        }
    }

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    json_object_put(payload);
    free(response.data);

    return return_code;
}

// Get statistics
int logging_client_get_stats(
    const logging_client_config_t *config,
    const char *start_time,
    const char *end_time,
    logging_stats_t *stats
) {
    CURL *curl;
    CURLcode res;
    int return_code = -1;

    curl = curl_easy_init();
    if (!curl) {
        return -1;
    }

    // Build URL with query parameters
    char url[768];
    snprintf(url, sizeof(url), "%s/api/logging/stats?start_time=%s&end_time=%s",
             config->api_base_url, start_time, end_time);

    // Setup curl
    struct response_buffer response;
    response.data = malloc(1);
    response.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config->timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // Perform request
    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (response_code == 200 && response.data) {
            // Parse JSON response
            json_object *root = json_tokener_parse(response.data);
            if (root) {
                json_object *stats_obj;
                if (json_object_object_get_ex(root, "statistics", &stats_obj)) {
                    json_object *field;

                    if (json_object_object_get_ex(stats_obj, "total_events", &field)) {
                        stats->total_events = json_object_get_int64(field);
                    }
                    if (json_object_object_get_ex(stats_obj, "total_messages", &field)) {
                        stats->total_messages = json_object_get_int64(field);
                    }
                    if (json_object_object_get_ex(stats_obj, "total_connections", &field)) {
                        stats->total_connections = json_object_get_int64(field);
                    }
                    if (json_object_object_get_ex(stats_obj, "messages_sent", &field)) {
                        stats->messages_sent = json_object_get_int64(field);
                    }
                    if (json_object_object_get_ex(stats_obj, "messages_delivered", &field)) {
                        stats->messages_delivered = json_object_get_int64(field);
                    }
                    if (json_object_object_get_ex(stats_obj, "messages_failed", &field)) {
                        stats->messages_failed = json_object_get_int64(field);
                    }
                    if (json_object_object_get_ex(stats_obj, "connections_success", &field)) {
                        stats->connections_success = json_object_get_int64(field);
                    }
                    if (json_object_object_get_ex(stats_obj, "connections_failed", &field)) {
                        stats->connections_failed = json_object_get_int64(field);
                    }
                    if (json_object_object_get_ex(stats_obj, "errors_count", &field)) {
                        stats->errors_count = json_object_get_int64(field);
                    }
                    if (json_object_object_get_ex(stats_obj, "warnings_count", &field)) {
                        stats->warnings_count = json_object_get_int64(field);
                    }

                    return_code = 0;  // Success
                }
                json_object_put(root);
            }
        }
    }

    // Cleanup
    curl_easy_cleanup(curl);
    free(response.data);

    return return_code;
}
