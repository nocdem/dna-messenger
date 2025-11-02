/**
 * GET /api/groups/:id
 */

#include "keyserver.h"
#include "http_utils.h"
#include "db_messages.h"
#include "rate_limit.h"
#include <json-c/json.h>
#include <microhttpd.h>
#include <string.h>
#include <stdlib.h>

enum MHD_Result api_load_group_handler(
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

    // Parse group_id from URL
    const char *group_id_str = strrchr(url, '/');
    if (!group_id_str || strlen(group_id_str) <= 1) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing group_id in URL");
    }
    group_id_str++;

    int group_id = atoi(group_id_str);
    if (group_id <= 0) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid group_id");
    }

    // Load group
    group_t *group = db_load_group(db_conn, group_id);
    if (!group) {
        return http_send_error(connection, HTTP_NOT_FOUND, "Group not found");
    }

    // Build JSON
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));

    json_object *group_obj = json_object_new_object();
    json_object_object_add(group_obj, "id", json_object_new_int(group->id));
    json_object_object_add(group_obj, "name", json_object_new_string(group->name));
    json_object_object_add(group_obj, "description", json_object_new_string(group->description));
    json_object_object_add(group_obj, "creator", json_object_new_string(group->creator));
    json_object_object_add(group_obj, "created_at", json_object_new_int64(group->created_at));
    json_object_object_add(group_obj, "updated_at", json_object_new_int64(group->updated_at));

    // Add members
    json_object *members_array = json_object_new_array();
    for (int i = 0; i < group->member_count; i++) {
        json_object *member_obj = json_object_new_object();
        json_object_object_add(member_obj, "member", json_object_new_string(group->members[i].member));
        json_object_object_add(member_obj, "role", json_object_new_string(group_role_to_string(group->members[i].role)));
        json_object_object_add(member_obj, "joined_at", json_object_new_int64(group->members[i].joined_at));
        json_object_array_add(members_array, member_obj);
    }
    json_object_object_add(group_obj, "members", members_array);

    json_object_object_add(response, "group", group_obj);

    enum MHD_Result ret = http_send_json_response(connection, HTTP_OK, response);

    json_object_put(response);
    db_free_group(group);

    return ret;
}
