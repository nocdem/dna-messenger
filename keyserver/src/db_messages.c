/**
 * Database operations for messages, contacts, and groups
 */

#include "db_messages.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// MESSAGE OPERATIONS
// ============================================================================

int64_t db_save_message(PGconn *conn, const message_t *message) {
    const char *sql =
        "INSERT INTO messages "
        "(sender, recipient, ciphertext, ciphertext_len, created_at, status, group_id) "
        "VALUES ($1, $2, $3, $4, to_timestamp($5), $6, NULLIF($7, 0)) "
        "RETURNING id";

    char created_at_str[32];
    snprintf(created_at_str, sizeof(created_at_str), "%ld", (long)message->created_at);

    char ciphertext_len_str[32];
    snprintf(ciphertext_len_str, sizeof(ciphertext_len_str), "%d", message->ciphertext_len);

    char group_id_str[32];
    snprintf(group_id_str, sizeof(group_id_str), "%d", message->group_id);

    const char *paramValues[7] = {
        message->sender,
        message->recipient,
        (const char *)message->ciphertext,
        ciphertext_len_str,
        created_at_str,
        message->status,
        group_id_str
    };

    int paramLengths[7] = {
        0, 0, message->ciphertext_len, 0, 0, 0, 0
    };

    int paramFormats[7] = {
        0, 0, 1, 0, 0, 0, 0  // Binary format for ciphertext
    };

    PGresult *res = PQexecParams(conn, sql, 7, NULL, paramValues,
                                 paramLengths, paramFormats, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Insert message failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    int64_t message_id = atoll(PQgetvalue(res, 0, 0));
    PQclear(res);
    return message_id;
}

message_t *db_load_conversation(PGconn *conn, const char *user1, const char *user2,
                                int limit, int offset, int *count) {
    const char *sql =
        "SELECT id, sender, recipient, ciphertext, ciphertext_len, "
        "EXTRACT(EPOCH FROM created_at)::bigint, status, "
        "EXTRACT(EPOCH FROM delivered_at)::bigint, "
        "EXTRACT(EPOCH FROM read_at)::bigint, "
        "COALESCE(group_id, 0) "
        "FROM messages "
        "WHERE ((sender = $1 AND recipient = $2) OR (sender = $2 AND recipient = $1)) "
        "  AND group_id IS NULL "
        "ORDER BY created_at DESC "
        "LIMIT $3 OFFSET $4";

    char limit_str[32], offset_str[32];
    snprintf(limit_str, sizeof(limit_str), "%d", limit);
    snprintf(offset_str, sizeof(offset_str), "%d", offset);

    const char *paramValues[4] = {user1, user2, limit_str, offset_str};

    PGresult *res = PQexecParams(conn, sql, 4, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Load conversation failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        *count = 0;
        return NULL;
    }

    int nrows = PQntuples(res);
    *count = nrows;

    if (nrows == 0) {
        PQclear(res);
        return NULL;
    }

    message_t *messages = calloc(nrows, sizeof(message_t));
    if (!messages) {
        PQclear(res);
        *count = 0;
        return NULL;
    }

    for (int i = 0; i < nrows; i++) {
        messages[i].id = atoll(PQgetvalue(res, i, 0));
        strncpy(messages[i].sender, PQgetvalue(res, i, 1), 32);
        strncpy(messages[i].recipient, PQgetvalue(res, i, 2), 32);

        int ciphertext_len = atoi(PQgetvalue(res, i, 4));
        messages[i].ciphertext_len = ciphertext_len;
        messages[i].ciphertext = malloc(ciphertext_len);
        if (messages[i].ciphertext) {
            memcpy(messages[i].ciphertext, PQgetvalue(res, i, 3), ciphertext_len);
        }

        messages[i].created_at = atoll(PQgetvalue(res, i, 5));
        strncpy(messages[i].status, PQgetvalue(res, i, 6), 19);
        messages[i].delivered_at = PQgetisnull(res, i, 7) ? 0 : atoll(PQgetvalue(res, i, 7));
        messages[i].read_at = PQgetisnull(res, i, 8) ? 0 : atoll(PQgetvalue(res, i, 8));
        messages[i].group_id = atoi(PQgetvalue(res, i, 9));
    }

    PQclear(res);
    return messages;
}

message_t *db_load_group_messages(PGconn *conn, int group_id,
                                  int limit, int offset, int *count) {
    const char *sql =
        "SELECT id, sender, recipient, ciphertext, ciphertext_len, "
        "EXTRACT(EPOCH FROM created_at)::bigint, status, "
        "EXTRACT(EPOCH FROM delivered_at)::bigint, "
        "EXTRACT(EPOCH FROM read_at)::bigint, "
        "group_id "
        "FROM messages "
        "WHERE group_id = $1 "
        "ORDER BY created_at DESC "
        "LIMIT $2 OFFSET $3";

    char group_id_str[32], limit_str[32], offset_str[32];
    snprintf(group_id_str, sizeof(group_id_str), "%d", group_id);
    snprintf(limit_str, sizeof(limit_str), "%d", limit);
    snprintf(offset_str, sizeof(offset_str), "%d", offset);

    const char *paramValues[3] = {group_id_str, limit_str, offset_str};

    PGresult *res = PQexecParams(conn, sql, 3, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Load group messages failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        *count = 0;
        return NULL;
    }

    int nrows = PQntuples(res);
    *count = nrows;

    if (nrows == 0) {
        PQclear(res);
        return NULL;
    }

    message_t *messages = calloc(nrows, sizeof(message_t));
    if (!messages) {
        PQclear(res);
        *count = 0;
        return NULL;
    }

    for (int i = 0; i < nrows; i++) {
        messages[i].id = atoll(PQgetvalue(res, i, 0));
        strncpy(messages[i].sender, PQgetvalue(res, i, 1), 32);
        strncpy(messages[i].recipient, PQgetvalue(res, i, 2), 32);

        int ciphertext_len = atoi(PQgetvalue(res, i, 4));
        messages[i].ciphertext_len = ciphertext_len;
        messages[i].ciphertext = malloc(ciphertext_len);
        if (messages[i].ciphertext) {
            memcpy(messages[i].ciphertext, PQgetvalue(res, i, 3), ciphertext_len);
        }

        messages[i].created_at = atoll(PQgetvalue(res, i, 5));
        strncpy(messages[i].status, PQgetvalue(res, i, 6), 19);
        messages[i].delivered_at = PQgetisnull(res, i, 7) ? 0 : atoll(PQgetvalue(res, i, 7));
        messages[i].read_at = PQgetisnull(res, i, 8) ? 0 : atoll(PQgetvalue(res, i, 8));
        messages[i].group_id = atoi(PQgetvalue(res, i, 9));
    }

    PQclear(res);
    return messages;
}

int db_update_message_status(PGconn *conn, int64_t message_id, const char *status) {
    const char *sql;

    if (strcasecmp(status, "delivered") == 0) {
        sql = "UPDATE messages SET status = $1, delivered_at = NOW() WHERE id = $2";
    } else if (strcasecmp(status, "read") == 0) {
        sql = "UPDATE messages SET status = $1, read_at = NOW() WHERE id = $2";
    } else {
        sql = "UPDATE messages SET status = $1 WHERE id = $2";
    }

    char message_id_str[32];
    snprintf(message_id_str, sizeof(message_id_str), "%ld", (long)message_id);

    const char *paramValues[2] = {status, message_id_str};

    PGresult *res = PQexecParams(conn, sql, 2, NULL, paramValues, NULL, NULL, 0);

    int success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    if (!success) {
        fprintf(stderr, "Update message status failed: %s\n", PQerrorMessage(conn));
    }

    PQclear(res);
    return success ? 0 : -1;
}

void db_free_messages(message_t *messages, int count) {
    if (!messages) return;

    for (int i = 0; i < count; i++) {
        if (messages[i].ciphertext) {
            free(messages[i].ciphertext);
        }
    }
    free(messages);
}

// ============================================================================
// CONTACT OPERATIONS
// ============================================================================

int db_save_contact(PGconn *conn, const contact_t *contact) {
    const char *sql =
        "INSERT INTO keyserver "
        "(identity, signing_pubkey, signing_pubkey_len, encryption_pubkey, "
        " encryption_pubkey_len, fingerprint, created_at) "
        "VALUES ($1, $2, $3, $4, $5, $6, to_timestamp($7)) "
        "ON CONFLICT (identity) DO UPDATE SET "
        "  signing_pubkey = EXCLUDED.signing_pubkey, "
        "  signing_pubkey_len = EXCLUDED.signing_pubkey_len, "
        "  encryption_pubkey = EXCLUDED.encryption_pubkey, "
        "  encryption_pubkey_len = EXCLUDED.encryption_pubkey_len, "
        "  fingerprint = EXCLUDED.fingerprint";

    char signing_len_str[32], encryption_len_str[32], created_at_str[32];
    snprintf(signing_len_str, sizeof(signing_len_str), "%d", contact->signing_pubkey_len);
    snprintf(encryption_len_str, sizeof(encryption_len_str), "%d", contact->encryption_pubkey_len);
    snprintf(created_at_str, sizeof(created_at_str), "%ld", (long)contact->created_at);

    const char *paramValues[7] = {
        contact->identity,
        (const char *)contact->signing_pubkey,
        signing_len_str,
        (const char *)contact->encryption_pubkey,
        encryption_len_str,
        contact->fingerprint,
        created_at_str
    };

    int paramLengths[7] = {
        0,
        contact->signing_pubkey_len,
        0,
        contact->encryption_pubkey_len,
        0, 0, 0
    };

    int paramFormats[7] = {0, 1, 0, 1, 0, 0, 0};

    PGresult *res = PQexecParams(conn, sql, 7, NULL, paramValues,
                                 paramLengths, paramFormats, 0);

    int success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    if (!success) {
        fprintf(stderr, "Save contact failed: %s\n", PQerrorMessage(conn));
    }

    PQclear(res);
    return success ? 0 : -1;
}

contact_t *db_load_contact(PGconn *conn, const char *identity) {
    const char *sql =
        "SELECT id, identity, signing_pubkey, signing_pubkey_len, "
        "encryption_pubkey, encryption_pubkey_len, fingerprint, "
        "EXTRACT(EPOCH FROM created_at)::bigint "
        "FROM keyserver WHERE identity = $1";

    const char *paramValues[1] = {identity};

    PGresult *res = PQexecParams(conn, sql, 1, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return NULL;
    }

    contact_t *contact = calloc(1, sizeof(contact_t));
    if (!contact) {
        PQclear(res);
        return NULL;
    }

    contact->id = atoi(PQgetvalue(res, 0, 0));
    strncpy(contact->identity, PQgetvalue(res, 0, 1), 32);

    int signing_len = atoi(PQgetvalue(res, 0, 3));
    contact->signing_pubkey_len = signing_len;
    contact->signing_pubkey = malloc(signing_len);
    if (contact->signing_pubkey) {
        memcpy(contact->signing_pubkey, PQgetvalue(res, 0, 2), signing_len);
    }

    int encryption_len = atoi(PQgetvalue(res, 0, 5));
    contact->encryption_pubkey_len = encryption_len;
    contact->encryption_pubkey = malloc(encryption_len);
    if (contact->encryption_pubkey) {
        memcpy(contact->encryption_pubkey, PQgetvalue(res, 0, 4), encryption_len);
    }

    strncpy(contact->fingerprint, PQgetvalue(res, 0, 6), 64);
    contact->created_at = atoll(PQgetvalue(res, 0, 7));

    PQclear(res);
    return contact;
}

contact_t *db_load_all_contacts(PGconn *conn, int *count) {
    const char *sql =
        "SELECT id, identity, signing_pubkey, signing_pubkey_len, "
        "encryption_pubkey, encryption_pubkey_len, fingerprint, "
        "EXTRACT(EPOCH FROM created_at)::bigint "
        "FROM keyserver ORDER BY identity";

    PGresult *res = PQexec(conn, sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Load all contacts failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        *count = 0;
        return NULL;
    }

    int nrows = PQntuples(res);
    *count = nrows;

    if (nrows == 0) {
        PQclear(res);
        return NULL;
    }

    contact_t *contacts = calloc(nrows, sizeof(contact_t));
    if (!contacts) {
        PQclear(res);
        *count = 0;
        return NULL;
    }

    for (int i = 0; i < nrows; i++) {
        contacts[i].id = atoi(PQgetvalue(res, i, 0));
        strncpy(contacts[i].identity, PQgetvalue(res, i, 1), 32);

        int signing_len = atoi(PQgetvalue(res, i, 3));
        contacts[i].signing_pubkey_len = signing_len;
        contacts[i].signing_pubkey = malloc(signing_len);
        if (contacts[i].signing_pubkey) {
            memcpy(contacts[i].signing_pubkey, PQgetvalue(res, i, 2), signing_len);
        }

        int encryption_len = atoi(PQgetvalue(res, i, 5));
        contacts[i].encryption_pubkey_len = encryption_len;
        contacts[i].encryption_pubkey = malloc(encryption_len);
        if (contacts[i].encryption_pubkey) {
            memcpy(contacts[i].encryption_pubkey, PQgetvalue(res, i, 4), encryption_len);
        }

        strncpy(contacts[i].fingerprint, PQgetvalue(res, i, 6), 64);
        contacts[i].created_at = atoll(PQgetvalue(res, i, 7));
    }

    PQclear(res);
    return contacts;
}

int db_delete_contact(PGconn *conn, const char *identity) {
    const char *sql = "DELETE FROM keyserver WHERE identity = $1";
    const char *paramValues[1] = {identity};

    PGresult *res = PQexecParams(conn, sql, 1, NULL, paramValues, NULL, NULL, 0);

    int success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    if (!success) {
        fprintf(stderr, "Delete contact failed: %s\n", PQerrorMessage(conn));
    }

    PQclear(res);
    return success ? 0 : -1;
}

void db_free_contact(contact_t *contact) {
    if (!contact) return;

    if (contact->signing_pubkey) free(contact->signing_pubkey);
    if (contact->encryption_pubkey) free(contact->encryption_pubkey);
    free(contact);
}

void db_free_contacts(contact_t *contacts, int count) {
    if (!contacts) return;

    for (int i = 0; i < count; i++) {
        if (contacts[i].signing_pubkey) free(contacts[i].signing_pubkey);
        if (contacts[i].encryption_pubkey) free(contacts[i].encryption_pubkey);
    }
    free(contacts);
}

// ============================================================================
// GROUP OPERATIONS
// ============================================================================

group_role_t group_role_from_string(const char *role) {
    if (strcasecmp(role, "creator") == 0) return GROUP_ROLE_CREATOR;
    if (strcasecmp(role, "admin") == 0) return GROUP_ROLE_ADMIN;
    return GROUP_ROLE_MEMBER;
}

const char *group_role_to_string(group_role_t role) {
    switch (role) {
        case GROUP_ROLE_CREATOR: return "creator";
        case GROUP_ROLE_ADMIN: return "admin";
        case GROUP_ROLE_MEMBER: return "member";
        default: return "member";
    }
}

int db_create_group(PGconn *conn, const group_t *group) {
    const char *sql =
        "INSERT INTO groups (name, description, creator, created_at, updated_at) "
        "VALUES ($1, $2, $3, to_timestamp($4), to_timestamp($5)) "
        "RETURNING id";

    char created_at_str[32], updated_at_str[32];
    snprintf(created_at_str, sizeof(created_at_str), "%ld", (long)group->created_at);
    snprintf(updated_at_str, sizeof(updated_at_str), "%ld", (long)group->updated_at);

    const char *paramValues[5] = {
        group->name,
        group->description,
        group->creator,
        created_at_str,
        updated_at_str
    };

    PGresult *res = PQexecParams(conn, sql, 5, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Create group failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    int group_id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    // Add creator as member
    group_member_t creator_member = {
        .group_id = group_id,
        .role = GROUP_ROLE_CREATOR,
        .joined_at = group->created_at
    };
    strncpy(creator_member.member, group->creator, 32);

    if (db_add_group_member(conn, group_id, &creator_member) != 0) {
        return -1;
    }

    // Add other members
    for (int i = 0; i < group->member_count; i++) {
        if (db_add_group_member(conn, group_id, &group->members[i]) != 0) {
            return -1;
        }
    }

    return group_id;
}

group_t *db_load_group(PGconn *conn, int group_id) {
    const char *sql =
        "SELECT id, name, description, creator, "
        "EXTRACT(EPOCH FROM created_at)::bigint, "
        "EXTRACT(EPOCH FROM updated_at)::bigint "
        "FROM groups WHERE id = $1";

    char group_id_str[32];
    snprintf(group_id_str, sizeof(group_id_str), "%d", group_id);
    const char *paramValues[1] = {group_id_str};

    PGresult *res = PQexecParams(conn, sql, 1, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return NULL;
    }

    group_t *group = calloc(1, sizeof(group_t));
    if (!group) {
        PQclear(res);
        return NULL;
    }

    group->id = atoi(PQgetvalue(res, 0, 0));
    strncpy(group->name, PQgetvalue(res, 0, 1), 127);
    strncpy(group->description, PQgetvalue(res, 0, 2), 511);
    strncpy(group->creator, PQgetvalue(res, 0, 3), 32);
    group->created_at = atoll(PQgetvalue(res, 0, 4));
    group->updated_at = atoll(PQgetvalue(res, 0, 5));

    PQclear(res);

    // Load members
    const char *member_sql =
        "SELECT group_id, member, role, EXTRACT(EPOCH FROM joined_at)::bigint "
        "FROM group_members WHERE group_id = $1 ORDER BY joined_at";

    res = PQexecParams(conn, member_sql, 1, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        int nrows = PQntuples(res);
        group->member_count = nrows;

        if (nrows > 0) {
            group->members = calloc(nrows, sizeof(group_member_t));
            if (group->members) {
                for (int i = 0; i < nrows; i++) {
                    group->members[i].group_id = atoi(PQgetvalue(res, i, 0));
                    strncpy(group->members[i].member, PQgetvalue(res, i, 1), 32);
                    group->members[i].role = group_role_from_string(PQgetvalue(res, i, 2));
                    group->members[i].joined_at = atoll(PQgetvalue(res, i, 3));
                }
            }
        }
    }

    PQclear(res);
    return group;
}

group_t *db_load_user_groups(PGconn *conn, const char *user_identity, int *count) {
    const char *sql =
        "SELECT DISTINCT g.id, g.name, g.description, g.creator, "
        "EXTRACT(EPOCH FROM g.created_at)::bigint, "
        "EXTRACT(EPOCH FROM g.updated_at)::bigint "
        "FROM groups g "
        "JOIN group_members gm ON g.id = gm.group_id "
        "WHERE gm.member = $1 "
        "ORDER BY g.updated_at DESC";

    const char *paramValues[1] = {user_identity};

    PGresult *res = PQexecParams(conn, sql, 1, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Load user groups failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        *count = 0;
        return NULL;
    }

    int nrows = PQntuples(res);
    *count = nrows;

    if (nrows == 0) {
        PQclear(res);
        return NULL;
    }

    group_t *groups = calloc(nrows, sizeof(group_t));
    if (!groups) {
        PQclear(res);
        *count = 0;
        return NULL;
    }

    for (int i = 0; i < nrows; i++) {
        groups[i].id = atoi(PQgetvalue(res, i, 0));
        strncpy(groups[i].name, PQgetvalue(res, i, 1), 127);
        strncpy(groups[i].description, PQgetvalue(res, i, 2), 511);
        strncpy(groups[i].creator, PQgetvalue(res, i, 3), 32);
        groups[i].created_at = atoll(PQgetvalue(res, i, 4));
        groups[i].updated_at = atoll(PQgetvalue(res, i, 5));

        // Load members for this group
        char group_id_str[32];
        snprintf(group_id_str, sizeof(group_id_str), "%d", groups[i].id);
        const char *member_params[1] = {group_id_str};

        const char *member_sql =
            "SELECT group_id, member, role, EXTRACT(EPOCH FROM joined_at)::bigint "
            "FROM group_members WHERE group_id = $1 ORDER BY joined_at";

        PGresult *member_res = PQexecParams(conn, member_sql, 1, NULL,
                                           member_params, NULL, NULL, 0);

        if (PQresultStatus(member_res) == PGRES_TUPLES_OK) {
            int member_count = PQntuples(member_res);
            groups[i].member_count = member_count;

            if (member_count > 0) {
                groups[i].members = calloc(member_count, sizeof(group_member_t));
                if (groups[i].members) {
                    for (int j = 0; j < member_count; j++) {
                        groups[i].members[j].group_id = atoi(PQgetvalue(member_res, j, 0));
                        strncpy(groups[i].members[j].member, PQgetvalue(member_res, j, 1), 32);
                        groups[i].members[j].role = group_role_from_string(PQgetvalue(member_res, j, 2));
                        groups[i].members[j].joined_at = atoll(PQgetvalue(member_res, j, 3));
                    }
                }
            }
        }

        PQclear(member_res);
    }

    PQclear(res);
    return groups;
}

int db_add_group_member(PGconn *conn, int group_id, const group_member_t *member) {
    const char *sql =
        "INSERT INTO group_members (group_id, member, role, joined_at) "
        "VALUES ($1, $2, $3, to_timestamp($4))";

    char group_id_str[32], joined_at_str[32];
    snprintf(group_id_str, sizeof(group_id_str), "%d", group_id);
    snprintf(joined_at_str, sizeof(joined_at_str), "%ld", (long)member->joined_at);

    const char *paramValues[4] = {
        group_id_str,
        member->member,
        group_role_to_string(member->role),
        joined_at_str
    };

    PGresult *res = PQexecParams(conn, sql, 4, NULL, paramValues, NULL, NULL, 0);

    int success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    if (!success) {
        fprintf(stderr, "Add group member failed: %s\n", PQerrorMessage(conn));
    }

    PQclear(res);
    return success ? 0 : -1;
}

int db_remove_group_member(PGconn *conn, int group_id, const char *member_identity) {
    const char *sql = "DELETE FROM group_members WHERE group_id = $1 AND member = $2";

    char group_id_str[32];
    snprintf(group_id_str, sizeof(group_id_str), "%d", group_id);

    const char *paramValues[2] = {group_id_str, member_identity};

    PGresult *res = PQexecParams(conn, sql, 2, NULL, paramValues, NULL, NULL, 0);

    int success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    if (!success) {
        fprintf(stderr, "Remove group member failed: %s\n", PQerrorMessage(conn));
    }

    PQclear(res);
    return success ? 0 : -1;
}

int db_delete_group(PGconn *conn, int group_id) {
    const char *sql = "DELETE FROM groups WHERE id = $1";

    char group_id_str[32];
    snprintf(group_id_str, sizeof(group_id_str), "%d", group_id);

    const char *paramValues[1] = {group_id_str};

    PGresult *res = PQexecParams(conn, sql, 1, NULL, paramValues, NULL, NULL, 0);

    int success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    if (!success) {
        fprintf(stderr, "Delete group failed: %s\n", PQerrorMessage(conn));
    }

    PQclear(res);
    return success ? 0 : -1;
}

void db_free_group(group_t *group) {
    if (!group) return;

    if (group->members) free(group->members);
    free(group);
}

void db_free_groups(group_t *groups, int count) {
    if (!groups) return;

    for (int i = 0; i < count; i++) {
        if (groups[i].members) free(groups[i].members);
    }
    free(groups);
}
