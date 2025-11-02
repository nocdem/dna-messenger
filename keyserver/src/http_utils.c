/*
 * HTTP Response Utilities
 */

#include "http_utils.h"
#include "keyserver.h"
#include <string.h>
#include <arpa/inet.h>

enum MHD_Result http_send_json_response(struct MHD_Connection *connection,
                                         int status_code, json_object *json_obj) {
    const char *json_str = json_object_to_json_string_ext(json_obj,
                                                          JSON_C_TO_STRING_PLAIN);

    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(json_str),
        (void*)json_str,
        MHD_RESPMEM_MUST_COPY
    );

    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");

    enum MHD_Result ret = MHD_queue_response(connection, status_code, response);

    MHD_destroy_response(response);
    json_object_put(json_obj);

    return ret;
}

enum MHD_Result http_send_error(struct MHD_Connection *connection,
                                 int status_code, const char *error_msg) {
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(false));
    json_object_object_add(response, "error", json_object_new_string(error_msg));

    return http_send_json_response(connection, status_code, response);
}

enum MHD_Result http_send_success(struct MHD_Connection *connection, const char *message) {
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(true));
    json_object_object_add(response, "message", json_object_new_string(message));

    return http_send_json_response(connection, HTTP_OK, response);
}

int http_get_client_ip(struct MHD_Connection *connection,
                      char *ip_buf, size_t ip_len) {
    const union MHD_ConnectionInfo *info;

    info = MHD_get_connection_info(connection,
                                   MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    if (!info) {
        return -1;
    }

    struct sockaddr *addr = (struct sockaddr*)info->client_addr;

    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *addr_in = (struct sockaddr_in*)addr;
        inet_ntop(AF_INET, &addr_in->sin_addr, ip_buf, ip_len);
        return 0;
    } else if (addr->sa_family == AF_INET6) {
        struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6*)addr;
        inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip_buf, ip_len);
        return 0;
    }

    return -1;
}

json_object* http_parse_json_post(const char *upload_data,
                                  size_t upload_data_size) {
    if (!upload_data || upload_data_size == 0) {
        return NULL;
    }

    // Ensure null termination
    char *json_str = malloc(upload_data_size + 1);
    if (!json_str) {
        return NULL;
    }

    memcpy(json_str, upload_data, upload_data_size);
    json_str[upload_data_size] = '\0';

    json_object *obj = json_tokener_parse(json_str);
    free(json_str);

    return obj;
}

// Base64 encoding table
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* http_base64_encode(const unsigned char *data, size_t len) {
    if (!data || len == 0) return NULL;

    size_t out_len = 4 * ((len + 2) / 3);
    char *encoded = malloc(out_len + 1);
    if (!encoded) return NULL;

    size_t i = 0, j = 0;
    while (i < len) {
        uint32_t octet_a = i < len ? data[i++] : 0;
        uint32_t octet_b = i < len ? data[i++] : 0;
        uint32_t octet_c = i < len ? data[i++] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        encoded[j++] = base64_table[(triple >> 18) & 0x3F];
        encoded[j++] = base64_table[(triple >> 12) & 0x3F];
        encoded[j++] = base64_table[(triple >> 6) & 0x3F];
        encoded[j++] = base64_table[triple & 0x3F];
    }

    // Add padding
    size_t padding = (3 - (len % 3)) % 3;
    for (size_t p = 0; p < padding; p++) {
        encoded[out_len - 1 - p] = '=';
    }

    encoded[out_len] = '\0';
    return encoded;
}

unsigned char* http_base64_decode(const char *str, size_t str_len, size_t *out_len) {
    if (!str || str_len == 0 || !out_len) return NULL;

    // Remove padding
    while (str_len > 0 && str[str_len - 1] == '=') {
        str_len--;
    }

    size_t decoded_len = (str_len * 3) / 4;
    unsigned char *decoded = malloc(decoded_len + 1);
    if (!decoded) return NULL;

    size_t i = 0, j = 0;
    while (i < str_len) {
        uint32_t sextet_a = i < str_len ? strchr(base64_table, str[i++]) - base64_table : 0;
        uint32_t sextet_b = i < str_len ? strchr(base64_table, str[i++]) - base64_table : 0;
        uint32_t sextet_c = i < str_len ? strchr(base64_table, str[i++]) - base64_table : 0;
        uint32_t sextet_d = i < str_len ? strchr(base64_table, str[i++]) - base64_table : 0;

        uint32_t triple = (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + sextet_d;

        if (j < decoded_len) decoded[j++] = (triple >> 16) & 0xFF;
        if (j < decoded_len) decoded[j++] = (triple >> 8) & 0xFF;
        if (j < decoded_len) decoded[j++] = triple & 0xFF;
    }

    *out_len = j;
    return decoded;
}
