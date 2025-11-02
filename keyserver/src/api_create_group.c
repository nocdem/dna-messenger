/**
 * POST /api/groups - Create group
 * Body: {"name": "Group", "description": "Desc", "creator": "alice", "members": [{"member": "bob", "role": "member"}]}
 */

#include "keyserver.h"
#include "http_utils.h"
#include "db_messages.h"
#include "rate_limit.h"
#include <json-c/json.h>
#include <microhttpd.h>
#include <string.h>
#include <stdlib.h>

enum MHD_Result api_create_group_handler(
    struct MHD_Connection *connection,
    PGconn *db_conn,
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

    // Parse JSON
    json_object *payload = http_parse_json_post(upload_data, upload_data_size);
    if (!payload) {
        return http_send_error(connection, HTTP_BAD_REQUEST, "Invalid JSON");
    }

    // Extract fields
    group_t group = {0};
    json_object *obj;

    if (json_object_object_get_ex(payload, "name", &obj)) {
        strncpy(group.name, json_object_get_string(obj), 127);
    }

    if (json_object_object_get_ex(payload, "description", &obj)) {
        strncpy(group.description, json_object_get_string(obj), 511);
    }

    if (json_object_object_get_ex(payload, "creator", &obj)) {
        strncpy(group.creator, json_object_get_string(obj), 32);
    }

    group.created_at = time(NULL);
    group.updated_at = time(NULL);

    // Parse members array
    if (json_object_object_get_ex(payload, "members", &obj) && json_object_is_type(obj, json_type_array)) {
        int member_count = json_object_array_length(obj);
        if (member_count > 0) {
            group.members = calloc(member_count, sizeof(group_member_t));
            if (group.members) {
                group.member_count = 0;
                for (int i = 0; i < member_count; i++) {
                    json_object *member_obj = json_object_array_get_idx(obj, i);
                    json_object *member_identity, *member_role;

                    if (json_object_object_get_ex(member_obj, "member", &member_identity)) {
                        strncpy(group.members[i].member, json_object_get_string(member_identity), 32);
                        group.members[i].group_id = 0; // Will be set by db_create_group
                        group.members[i].joined_at = time(NULL);

                        if (json_object_object_get_ex(member_obj, "role", &member_role)) {
                            group.members[i].role = group_role_from_string(json_object_get_string(member_role));
                        } else {
                            group.members[i].role = GROUP_ROLE_MEMBER;
                        }
                        group.member_count++;
                    }
                }
            }
        }
    }

    // Validate
    if (strlen(group.name) == 0 || strlen(group.creator) == 0) {
        if (group.members) free(group.members);
        json_object_put(payload);
        return http_send_error(connection, HTTP_BAD_REQUEST, "Missing required fields: name, creator");
    }

    // Create group
    int group_id = db_create_group(db_conn, &group);

    if (group.members) free(group.members);
    json_object_put(payload);

    if (group_id < 0) {
        return http_send_error(connection, HTTP_INTERNAL_ERROR, "Failed to create group");
    }

    // Build response
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "group_id", json_object_new_int(group_id));

    enum MHD_Result ret = http_send_json_response(connection, HTTP_OK, response);
    json_object_put(response);

    return ret;
}
