/**
 * GET /api/groups?member=identity
 */

#include "keyserver.h"
#include "http_utils.h"
#include "db_messages.h"
#include "rate_limit.h"
#include <json-c/json.h>
#include <microhttpd.h>
#include <string.h>

enum MHD_Result api_load_user_groups_handler(
    struct MHD_Connection *connection,
    PGconn *db_conn
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

    // Get member parameter
    const char *member = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "member");
    if (!member || strlen(member) == 0) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing required parameter: member");
    }

    // Load groups
    int count = 0;
    group_t *groups = db_load_user_groups(db_conn, member, &count);

    // Build JSON
    json_object *response = json_object_new_object();
    json_object *groups_array = json_object_new_array();

    if (groups && count > 0) {
        for (int i = 0; i < count; i++) {
            json_object *group_obj = json_object_new_object();
            json_object_object_add(group_obj, "id", json_object_new_int(groups[i].id));
            json_object_object_add(group_obj, "name", json_object_new_string(groups[i].name));
            json_object_object_add(group_obj, "description", json_object_new_string(groups[i].description));
            json_object_object_add(group_obj, "creator", json_object_new_string(groups[i].creator));
            json_object_object_add(group_obj, "created_at", json_object_new_int64(groups[i].created_at));
            json_object_object_add(group_obj, "updated_at", json_object_new_int64(groups[i].updated_at));

            // Add members
            json_object *members_array = json_object_new_array();
            for (int j = 0; j < groups[i].member_count; j++) {
                json_object *member_obj = json_object_new_object();
                json_object_object_add(member_obj, "member", json_object_new_string(groups[i].members[j].member));
                json_object_object_add(member_obj, "role", json_object_new_string(group_role_to_string(groups[i].members[j].role)));
                json_object_object_add(member_obj, "joined_at", json_object_new_int64(groups[i].members[j].joined_at));
                json_object_array_add(members_array, member_obj);
            }
            json_object_object_add(group_obj, "members", members_array);

            json_object_array_add(groups_array, group_obj);
        }
    }

    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "count", json_object_new_int(count));
    json_object_object_add(response, "groups", groups_array);

    enum MHD_Result ret = http_send_json_response(connection, HTTP_OK, response);

    json_object_put(response);
    if (groups) db_free_groups(groups, count);

    return ret;
}
