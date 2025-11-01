/*
 * DNA Messenger - CLI Messenger Module
 *
 * Storage Architecture:
 * - Private keys: ~/.dna/<identity>-dilithium.pqkey, <identity>-kyber512.pqkey (filesystem)
 * - Public keys: PostgreSQL keyserver table (shared, network-ready)
 * - Messages: PostgreSQL messages table (shared, network-ready)
 *
 * Phase 9.1b: Hybrid P2P Transport
 * - P2P direct messaging when both peers online (via DHT + TCP)
 * - PostgreSQL fallback for offline message delivery
 */

#ifndef MESSENGER_H
#define MESSENGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <libpq-fe.h>
#include "dna_api.h"
#include "p2p/p2p_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// MESSENGER CONTEXT
// ============================================================================

// Public key cache entry
typedef struct {
    char *identity;
    uint8_t *signing_pubkey;
    size_t signing_pubkey_len;
    uint8_t *encryption_pubkey;
    size_t encryption_pubkey_len;
} pubkey_cache_entry_t;

#define PUBKEY_CACHE_SIZE 100

/**
 * Messenger Context
 * Manages PostgreSQL connection and DNA API context
 */
typedef struct {
    char *identity;              // User's identity name (e.g., "alice")
    PGconn *pg_conn;             // PostgreSQL connection
    dna_context_t *dna_ctx;      // DNA API context

    // P2P Transport (Phase 9.1b: Hybrid P2P messaging)
    p2p_transport_t *p2p_transport;  // P2P transport layer (NULL if disabled)
    bool p2p_enabled;                 // Enable/disable P2P messaging

    // Public key cache (API fetch caching)
    pubkey_cache_entry_t cache[PUBKEY_CACHE_SIZE];
    int cache_count;
} messenger_context_t;

/**
 * Message Info
 * Represents a single message in a conversation
 */
typedef struct {
    int id;                      // Message ID
    char *sender;                // Sender identity
    char *recipient;             // Recipient identity
    char *timestamp;             // Timestamp string
    char *status;                // Message status: "sent", "delivered", "read"
    char *delivered_at;          // Delivery timestamp (NULL if not delivered)
    char *read_at;               // Read timestamp (NULL if not read)
    char *plaintext;             // Decrypted message text (NULL if not decrypted)
} message_info_t;

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Initialize messenger context
 *
 * Connects to PostgreSQL database (local or network)
 * Connection string: postgresql://dna:dna_password@localhost:5432/dna_messenger
 *
 * @param identity: User's identity name
 * @return: Messenger context, or NULL on error
 */
messenger_context_t* messenger_init(const char *identity);

/**
 * Free messenger context
 *
 * @param ctx: Messenger context to free
 */
void messenger_free(messenger_context_t *ctx);

// ============================================================================
// KEY GENERATION
// ============================================================================

/**
 * Generate new key pair for identity
 *
 * Creates:
 * - ~/.dna/<identity>-dilithium.pqkey (private signing key)
 * - ~/.dna/<identity>-kyber512.pqkey (private encryption key)
 * - Uploads public keys to PostgreSQL keyserver
 *
 * @param ctx: Messenger context
 * @param identity: Identity name (e.g., "alice")
 * @return: 0 on success, -1 on error
 */
int messenger_generate_keys(messenger_context_t *ctx, const char *identity);

/**
 * Restore key pair from BIP39 recovery seed
 *
 * Prompts user for 24-word mnemonic and optional passphrase.
 * Regenerates keys deterministically and uploads to keyserver.
 *
 * @param ctx: Messenger context
 * @param identity: Identity name (e.g., "alice")
 * @return: 0 on success, -1 on error
 */
int messenger_restore_keys(messenger_context_t *ctx, const char *identity);

/**
 * Restore key pair from file containing BIP39 recovery seed
 *
 * File format: "word1 word2 word3 ... word24 [passphrase]"
 * Passphrase is optional (on same line after 24 words).
 *
 * @param ctx: Messenger context
 * @param identity: Identity name (e.g., "alice")
 * @param seed_file: Path to file containing seed phrase
 * @return: 0 on success, -1 on error
 */
int messenger_restore_keys_from_file(messenger_context_t *ctx, const char *identity, const char *seed_file);

// ============================================================================
// PUBLIC KEY MANAGEMENT (keyserver table)
// ============================================================================

/**
 * Store public key in keyserver
 *
 * Called when generating or importing a key pair.
 *
 * @param ctx: Messenger context
 * @param identity: Key owner's identity
 * @param signing_pubkey: Dilithium3 public key
 * @param signing_pubkey_len: Signing key length
 * @param encryption_pubkey: Kyber512 public key
 * @param encryption_pubkey_len: Encryption key length
 * @return: 0 on success, -1 on error
 */
int messenger_store_pubkey(
    messenger_context_t *ctx,
    const char *identity,
    const uint8_t *signing_pubkey,
    size_t signing_pubkey_len,
    const uint8_t *encryption_pubkey,
    size_t encryption_pubkey_len
);

/**
 * Load public key from keyserver
 *
 * @param ctx: Messenger context
 * @param identity: Key owner's identity
 * @param signing_pubkey_out: Output signing key (caller must free)
 * @param signing_pubkey_len_out: Output signing key length
 * @param encryption_pubkey_out: Output encryption key (caller must free)
 * @param encryption_pubkey_len_out: Output encryption key length
 * @return: 0 on success, -1 on error
 */
int messenger_load_pubkey(
    messenger_context_t *ctx,
    const char *identity,
    uint8_t **signing_pubkey_out,
    size_t *signing_pubkey_len_out,
    uint8_t **encryption_pubkey_out,
    size_t *encryption_pubkey_len_out
);

/**
 * List all public keys in keyserver
 *
 * @param ctx: Messenger context
 * @return: 0 on success, -1 on error
 */
int messenger_list_pubkeys(messenger_context_t *ctx);

/**
 * Get contact list (identities from keyserver)
 *
 * Returns array of identity strings. Caller must free each string and the array.
 *
 * @param ctx: Messenger context
 * @param identities_out: Output array of identity strings (caller must free)
 * @param count_out: Number of identities returned
 * @return: 0 on success, -1 on error
 */
int messenger_get_contact_list(messenger_context_t *ctx, char ***identities_out, int *count_out);

/**
 * Delete public key from keyserver
 *
 * @param ctx: Messenger context
 * @param identity: Key owner's identity
 * @return: 0 on success, -1 on error
 */
int messenger_delete_pubkey(messenger_context_t *ctx, const char *identity);

// ============================================================================
// MESSAGE OPERATIONS (messages table)
// ============================================================================

/**
 * Send message to recipient(s)
 *
 * Encrypts message and stores in messages table.
 * Sender is automatically added as first recipient so they can decrypt their own sent messages.
 * In Phase 3, messages are stored in PostgreSQL (ready for network access).
 *
 * @param ctx: Messenger context
 * @param recipients: Array of recipient identities (1-254, sender is added automatically)
 * @param recipient_count: Number of recipients (1-254)
 * @param message: Plaintext message
 * @return: 0 on success, -1 on error
 */
int messenger_send_message(
    messenger_context_t *ctx,
    const char **recipients,
    size_t recipient_count,
    const char *message
);

/**
 * List messages for current user
 *
 * Shows messages where recipient = current identity
 *
 * @param ctx: Messenger context
 * @return: 0 on success, -1 on error
 */
int messenger_list_messages(messenger_context_t *ctx);

/**
 * List sent messages
 *
 * Shows messages where sender = current identity
 *
 * @param ctx: Messenger context
 * @return: 0 on success, -1 on error
 */
int messenger_list_sent_messages(messenger_context_t *ctx);

/**
 * Read and decrypt specific message
 *
 * @param ctx: Messenger context
 * @param message_id: Message ID
 * @return: 0 on success, -1 on error
 */
int messenger_read_message(messenger_context_t *ctx, int message_id);

/**
 * Decrypt a message and return plaintext
 *
 * @param ctx: Messenger context
 * @param message_id: Message ID
 * @param plaintext_out: Output plaintext (caller must free)
 * @param plaintext_len_out: Output plaintext length
 * @return: 0 on success, -1 on error
 */
int messenger_decrypt_message(messenger_context_t *ctx, int message_id,
                                char **plaintext_out, size_t *plaintext_len_out);

/**
 * Delete message
 *
 * @param ctx: Messenger context
 * @param message_id: Message ID
 * @return: 0 on success, -1 on error
 */
int messenger_delete_message(messenger_context_t *ctx, int message_id);

/**
 * Search messages by sender
 *
 * Shows all messages from a specific sender to current user
 *
 * @param ctx: Messenger context
 * @param sender: Sender's identity to search for
 * @return: 0 on success, -1 on error
 */
int messenger_search_by_sender(messenger_context_t *ctx, const char *sender);

/**
 * Show conversation with another user
 *
 * Shows all messages exchanged with another user (both sent and received),
 * sorted by timestamp
 *
 * @param ctx: Messenger context
 * @param other_identity: The other person's identity
 * @return: 0 on success, -1 on error
 */
int messenger_show_conversation(messenger_context_t *ctx, const char *other_identity);

/**
 * Get conversation with another user
 *
 * Returns array of messages exchanged with another user (both sent and received),
 * sorted by timestamp. Messages are NOT decrypted (plaintext field is NULL).
 * Caller must free the array using messenger_free_messages().
 *
 * @param ctx: Messenger context
 * @param other_identity: The other person's identity
 * @param messages_out: Output array of message_info_t (caller must free)
 * @param count_out: Number of messages returned
 * @return: 0 on success, -1 on error
 */
int messenger_get_conversation(messenger_context_t *ctx, const char *other_identity,
                                 message_info_t **messages_out, int *count_out);

/**
 * Free message array
 *
 * @param messages: Array of message_info_t to free
 * @param count: Number of messages in array
 */
void messenger_free_messages(message_info_t *messages, int count);

/**
 * Search messages by date range
 *
 * Shows messages (sent or received) within a specific date range
 *
 * @param ctx: Messenger context
 * @param start_date: Start date (format: "YYYY-MM-DD") or NULL for no start limit
 * @param end_date: End date (format: "YYYY-MM-DD") or NULL for no end limit
 * @param include_sent: Include sent messages
 * @param include_received: Include received messages
 * @return: 0 on success, -1 on error
 */
int messenger_search_by_date(messenger_context_t *ctx, const char *start_date,
                              const char *end_date, bool include_sent, bool include_received);

// ============================================================================
// MESSAGE STATUS / READ RECEIPTS
// ============================================================================

/**
 * Mark message as delivered
 *
 * Called when recipient fetches the message from server.
 * Updates status from 'sent' to 'delivered' and sets delivered_at timestamp.
 *
 * @param ctx: Messenger context
 * @param message_id: Message ID to mark as delivered
 * @return: 0 on success, -1 on error
 */
int messenger_mark_delivered(messenger_context_t *ctx, int message_id);

/**
 * Mark all messages in conversation as read
 *
 * Called when recipient opens the conversation.
 * Updates status from 'sent'/'delivered' to 'read' and sets read_at timestamp.
 *
 * @param ctx: Messenger context
 * @param sender_identity: The sender whose messages to mark as read
 * @return: 0 on success, -1 on error
 */
int messenger_mark_conversation_read(messenger_context_t *ctx, const char *sender_identity);

// ============================================================================
// GROUP MANAGEMENT
// ============================================================================

/**
 * Group Info
 * Represents a group with its metadata
 */
typedef struct {
    int id;                      // Group ID
    char *name;                  // Group name
    char *description;           // Optional description
    char *creator;               // Creator identity
    char *created_at;            // Creation timestamp
    int member_count;            // Number of members
} group_info_t;

/**
 * Create a new group
 *
 * @param ctx: Messenger context
 * @param name: Group name
 * @param description: Optional group description (can be NULL)
 * @param members: Array of member identities (excluding creator, who is added automatically)
 * @param member_count: Number of members
 * @param group_id_out: Output group ID
 * @return: 0 on success, -1 on error
 */
int messenger_create_group(
    messenger_context_t *ctx,
    const char *name,
    const char *description,
    const char **members,
    size_t member_count,
    int *group_id_out
);

/**
 * Get list of all groups current user belongs to
 *
 * @param ctx: Messenger context
 * @param groups_out: Output array of group_info_t (caller must free with messenger_free_groups)
 * @param count_out: Number of groups returned
 * @return: 0 on success, -1 on error
 */
int messenger_get_groups(
    messenger_context_t *ctx,
    group_info_t **groups_out,
    int *count_out
);

/**
 * Get group info by ID
 *
 * @param ctx: Messenger context
 * @param group_id: Group ID
 * @param group_out: Output group_info_t (caller must free fields)
 * @return: 0 on success, -1 on error
 */
int messenger_get_group_info(
    messenger_context_t *ctx,
    int group_id,
    group_info_t *group_out
);

/**
 * Get members of a specific group
 *
 * @param ctx: Messenger context
 * @param group_id: Group ID
 * @param members_out: Output array of identity strings (caller must free)
 * @param count_out: Number of members returned
 * @return: 0 on success, -1 on error
 */
int messenger_get_group_members(
    messenger_context_t *ctx,
    int group_id,
    char ***members_out,
    int *count_out
);

/**
 * Add member to group
 *
 * @param ctx: Messenger context
 * @param group_id: Group ID
 * @param member: Identity to add
 * @return: 0 on success, -1 on error
 */
int messenger_add_group_member(
    messenger_context_t *ctx,
    int group_id,
    const char *member
);

/**
 * Remove member from group
 *
 * @param ctx: Messenger context
 * @param group_id: Group ID
 * @param member: Identity to remove
 * @return: 0 on success, -1 on error
 */
int messenger_remove_group_member(
    messenger_context_t *ctx,
    int group_id,
    const char *member
);

/**
 * Leave a group
 *
 * @param ctx: Messenger context
 * @param group_id: Group ID
 * @return: 0 on success, -1 on error
 */
int messenger_leave_group(
    messenger_context_t *ctx,
    int group_id
);

/**
 * Delete a group (creator only)
 *
 * @param ctx: Messenger context
 * @param group_id: Group ID
 * @return: 0 on success, -1 on error
 */
int messenger_delete_group(
    messenger_context_t *ctx,
    int group_id
);

/**
 * Update group info (name, description)
 *
 * @param ctx: Messenger context
 * @param group_id: Group ID
 * @param name: New name (NULL to keep current)
 * @param description: New description (NULL to keep current)
 * @return: 0 on success, -1 on error
 */
int messenger_update_group_info(
    messenger_context_t *ctx,
    int group_id,
    const char *name,
    const char *description
);

/**
 * Send message to group
 *
 * Automatically sends to all group members.
 *
 * @param ctx: Messenger context
 * @param group_id: Group ID
 * @param message: Message text
 * @return: 0 on success, -1 on error
 */
int messenger_send_group_message(
    messenger_context_t *ctx,
    int group_id,
    const char *message
);

/**
 * Get conversation for a group
 *
 * @param ctx: Messenger context
 * @param group_id: Group ID
 * @param messages_out: Output array of message_info_t (caller must free)
 * @param count_out: Number of messages returned
 * @return: 0 on success, -1 on error
 */
int messenger_get_group_conversation(
    messenger_context_t *ctx,
    int group_id,
    message_info_t **messages_out,
    int *count_out
);

/**
 * Free group array
 *
 * @param groups: Array of group_info_t to free
 * @param count: Number of groups in array
 */
void messenger_free_groups(group_info_t *groups, int count);

#ifdef __cplusplus
}
#endif

#endif // MESSENGER_H
