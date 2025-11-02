/**
 * Database operations for messages, contacts, and groups
 * Part of DNA Messenger REST API
 */

#ifndef DB_MESSAGES_H
#define DB_MESSAGES_H

#include <libpq-fe.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// MESSAGE STRUCTURES AND OPERATIONS
// ============================================================================

typedef struct {
    int64_t id;
    char sender[33];
    char recipient[33];
    uint8_t *ciphertext;
    int ciphertext_len;
    time_t created_at;
    char status[20];
    time_t delivered_at;
    time_t read_at;
    int group_id;
} message_t;

/**
 * Save message to database
 * Returns message ID on success, -1 on error
 */
int64_t db_save_message(PGconn *conn, const message_t *message);

/**
 * Load conversation between two users
 * Returns array of messages, caller must free
 */
message_t *db_load_conversation(PGconn *conn, const char *user1, const char *user2,
                                int limit, int offset, int *count);

/**
 * Load group messages
 * Returns array of messages, caller must free
 */
message_t *db_load_group_messages(PGconn *conn, int group_id,
                                  int limit, int offset, int *count);

/**
 * Update message status
 */
int db_update_message_status(PGconn *conn, int64_t message_id, const char *status);

/**
 * Free message array
 */
void db_free_messages(message_t *messages, int count);

// ============================================================================
// CONTACT STRUCTURES AND OPERATIONS
// ============================================================================

typedef struct {
    int id;
    char identity[33];
    uint8_t *signing_pubkey;
    int signing_pubkey_len;
    uint8_t *encryption_pubkey;
    int encryption_pubkey_len;
    char fingerprint[65];
    time_t created_at;
} contact_t;

/**
 * Save or update contact
 */
int db_save_contact(PGconn *conn, const contact_t *contact);

/**
 * Load contact by identity
 * Returns contact or NULL if not found, caller must free
 */
contact_t *db_load_contact(PGconn *conn, const char *identity);

/**
 * Load all contacts
 * Returns array of contacts, caller must free
 */
contact_t *db_load_all_contacts(PGconn *conn, int *count);

/**
 * Delete contact
 */
int db_delete_contact(PGconn *conn, const char *identity);

/**
 * Free contact
 */
void db_free_contact(contact_t *contact);

/**
 * Free contact array
 */
void db_free_contacts(contact_t *contacts, int count);

// ============================================================================
// GROUP STRUCTURES AND OPERATIONS
// ============================================================================

typedef enum {
    GROUP_ROLE_CREATOR,
    GROUP_ROLE_ADMIN,
    GROUP_ROLE_MEMBER
} group_role_t;

typedef struct {
    int group_id;
    char member[33];
    group_role_t role;
    time_t joined_at;
} group_member_t;

typedef struct {
    int id;
    char name[128];
    char description[512];
    char creator[33];
    time_t created_at;
    time_t updated_at;
    group_member_t *members;
    int member_count;
} group_t;

/**
 * Create group
 * Returns group ID on success, -1 on error
 */
int db_create_group(PGconn *conn, const group_t *group);

/**
 * Load group by ID
 * Returns group or NULL if not found, caller must free
 */
group_t *db_load_group(PGconn *conn, int group_id);

/**
 * Load user's groups
 * Returns array of groups, caller must free
 */
group_t *db_load_user_groups(PGconn *conn, const char *user_identity, int *count);

/**
 * Add member to group
 */
int db_add_group_member(PGconn *conn, int group_id, const group_member_t *member);

/**
 * Remove member from group
 */
int db_remove_group_member(PGconn *conn, int group_id, const char *member_identity);

/**
 * Delete group
 */
int db_delete_group(PGconn *conn, int group_id);

/**
 * Free group
 */
void db_free_group(group_t *group);

/**
 * Free group array
 */
void db_free_groups(group_t *groups, int count);

/**
 * Helper: Convert string to group role enum
 */
group_role_t group_role_from_string(const char *role);

/**
 * Helper: Convert group role enum to string
 */
const char *group_role_to_string(group_role_t role);

#ifdef __cplusplus
}
#endif

#endif // DB_MESSAGES_H
