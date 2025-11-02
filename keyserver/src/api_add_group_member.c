/**
 * POST /api/groups/:id/members
 * Body: {"member": "alice", "role": "member"}
 */

#include "keyserver.h"
#include "http_utils.h"
#include "db_messages.h"
#include "rate_limit.h"
#include <json-c/json.h>
#include <microhttpd.h>
#include <string.h>
#include <stdlib.h>

enum MHD_Result api_add_group_member_handler(
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

    // Get client IP

    // Rate limit
    if (!rate_limit_check(client_ip, RATE_LIMIT_TYPE_REGISTER)) {
        return http_send_error(connection, HTTP_TOO_MANY_REQUESTS, "Rate limit exceeded");
    }

    // Parse group_id from URL (/api/groups/123/members)
    const char *id_start = strstr(url, "/api/groups/");
    if (!id_start) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid URL");
    }
    id_start += strlen("/api/groups/");

    const char *id_end = strstr(id_start, "/members");
    if (!id_end) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing /members in URL");
    }

    char group_id_str[32] = {0};
    size_t id_len = id_end - id_start;
    if (id_len >= sizeof(group_id_str)) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid group_id");
    }

    strncpy(group_id_str, id_start, id_len);
    int group_id = atoi(group_id_str);

    if (group_id <= 0) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid group_id");
    }

    // Parse JSON
    json_object *payload = http_parse_json_post(upload_data, upload_data_size);
    if (!payload) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid JSON");
    }

    // Extract fields
    group_member_t member = {0};
    member.group_id = group_id;
    member.joined_at = time(NULL);
    member.role = GROUP_ROLE_MEMBER;

    json_object *obj;
    if (json_object_object_get_ex(payload, "member", &obj)) {
        strncpy(member.member, json_object_get_string(obj), 32);
    }

    if (json_object_object_get_ex(payload, "role", &obj)) {
        member.role = group_role_from_string(json_object_get_string(obj));
    }

    if (strlen(member.member) == 0) {
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing required field: member");
    }

    // Add member
    int result = db_add_group_member(db_conn, group_id, &member);

    json_object_put(payload);

    if (result != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to add group member");
    }

    // Build response
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "group_id", json_object_new_int(group_id));
    json_object_object_add(response, "member", json_object_new_string(member.member));

    enum MHD_Result ret = http_send_json_response(connection, HTTP_OK, response);
    json_object_put(response);

    return ret;
}
