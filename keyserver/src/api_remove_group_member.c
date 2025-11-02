/**
 * DELETE /api/groups/:groupId/members/:identity
 */

#include "keyserver.h"
#include "http_utils.h"
#include "db_messages.h"
#include "rate_limit.h"
#include <json-c/json.h>
#include <microhttpd.h>
#include <string.h>
#include <stdlib.h>

enum MHD_Result api_remove_group_member_handler(
    struct MHD_Connection *connection,
    PGconn *db_conn,
    const char *url
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

    // Parse URL: /api/groups/123/members/alice
    const char *id_start = strstr(url, "/api/groups/");
    if (!id_start) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid URL");
    }
    id_start += strlen("/api/groups/");

    const char *id_end = strstr(id_start, "/members/");
    if (!id_end) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing /members/ in URL");
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

    // Get member identity
    const char *member_identity = id_end + strlen("/members/");
    if (strlen(member_identity) == 0) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing member identity");
    }

    // Remove member
    int result = db_remove_group_member(db_conn, group_id, member_identity);
    if (result != 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to remove group member");
    }

    // Build response
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "group_id", json_object_new_int(group_id));
    json_object_object_add(response, "member", json_object_new_string(member_identity));

    enum MHD_Result ret = http_send_json_response(connection, HTTP_OK, response);
    json_object_put(response);

    return ret;
}
