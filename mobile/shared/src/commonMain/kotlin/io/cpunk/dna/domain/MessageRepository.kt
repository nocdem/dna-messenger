package io.cpunk.dna.domain

import io.cpunk.dna.domain.models.Contact
import io.cpunk.dna.domain.models.Group
import io.cpunk.dna.domain.models.Message
import io.cpunk.dna.domain.models.MessageStatus

/**
 * MessageRepository - High-level messaging API with encryption
 *
 * Combines DatabaseRepository (storage) with DNAMessenger (crypto)
 * to provide secure end-to-end encrypted messaging.
 *
 * Features:
 * - Automatic encryption before storing messages
 * - Automatic decryption when loading messages
 * - Contact management (public keys)
 * - Group messaging support
 */
expect class MessageRepository {
    /**
     * Initialize with user's private keys
     *
     * @param identity User's identity name
     * @param encryptionPrivateKey User's Kyber512 private key (1632 bytes)
     * @param signingPrivateKey User's Dilithium3 private key (4032 bytes)
     * @param signingPublicKey User's Dilithium3 public key (1952 bytes)
     */
    fun initialize(
        identity: String,
        encryptionPrivateKey: ByteArray,
        signingPrivateKey: ByteArray,
        signingPublicKey: ByteArray
    ): Result<Unit>

    // ============================================================================
    // MESSAGE OPERATIONS
    // ============================================================================

    /**
     * Send encrypted message to recipient
     *
     * Automatically:
     * 1. Loads recipient's public key from keyserver
     * 2. Encrypts plaintext using Kyber512 + AES-256-GCM
     * 3. Signs with sender's Dilithium3 key
     * 4. Saves to database
     *
     * @param recipient Recipient's identity name
     * @param plaintext Message text
     * @return Message ID
     */
    suspend fun sendMessage(
        recipient: String,
        plaintext: String
    ): Result<Long>

    /**
     * Send encrypted message to group
     *
     * @param groupId Group ID
     * @param plaintext Message text
     * @return Message ID
     */
    suspend fun sendGroupMessage(
        groupId: Int,
        plaintext: String
    ): Result<Long>

    /**
     * Load conversation and decrypt messages
     *
     * @param otherUser Other user's identity
     * @param limit Maximum number of messages
     * @param offset Offset for pagination
     * @return List of decrypted messages
     */
    suspend fun loadConversation(
        otherUser: String,
        limit: Int = 50,
        offset: Int = 0
    ): Result<List<DecryptedMessage>>

    /**
     * Load group messages and decrypt
     *
     * @param groupId Group ID
     * @param limit Maximum number of messages
     * @param offset Offset for pagination
     * @return List of decrypted messages
     */
    suspend fun loadGroupMessages(
        groupId: Int,
        limit: Int = 50,
        offset: Int = 0
    ): Result<List<DecryptedMessage>>

    /**
     * Update message status
     *
     * @param messageId Message ID
     * @param status New status ('sent', 'delivered', 'read')
     */
    suspend fun updateMessageStatus(
        messageId: Long,
        status: String
    ): Result<Unit>

    // ============================================================================
    // CONTACT OPERATIONS
    // ============================================================================

    /**
     * Add contact (store their public keys)
     *
     * @param contact Contact to add
     */
    suspend fun addContact(contact: Contact): Result<Unit>

    /**
     * Get contact by identity
     *
     * @param identity Contact's identity name
     * @return Contact or null
     */
    suspend fun getContact(identity: String): Result<Contact?>

    /**
     * List all contacts
     *
     * @return List of contacts
     */
    suspend fun listContacts(): Result<List<Contact>>

    /**
     * Delete contact
     *
     * @param identity Contact's identity name
     */
    suspend fun deleteContact(identity: String): Result<Unit>

    // ============================================================================
    // GROUP OPERATIONS
    // ============================================================================

    /**
     * Create group
     *
     * @param group Group to create
     * @return Group ID
     */
    suspend fun createGroup(group: Group): Result<Int>

    /**
     * Get group by ID
     *
     * @param groupId Group ID
     * @return Group with members or null
     */
    suspend fun getGroup(groupId: Int): Result<Group?>

    /**
     * List user's groups
     *
     * @return List of groups
     */
    suspend fun listGroups(): Result<List<Group>>

    /**
     * Close and cleanup resources
     */
    fun close()
}

/**
 * Decrypted message with plaintext
 */
data class DecryptedMessage(
    val id: Long,
    val sender: String,
    val recipient: String,
    val plaintext: String,              // Decrypted plaintext
    val createdAt: Long,
    val status: MessageStatus,
    val deliveredAt: Long? = null,
    val readAt: Long? = null,
    val groupId: Int? = null,
    val decryptionFailed: Boolean = false
)
