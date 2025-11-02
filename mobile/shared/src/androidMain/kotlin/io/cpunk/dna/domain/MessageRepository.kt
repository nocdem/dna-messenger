package io.cpunk.dna.domain

import android.util.Log
import io.cpunk.dna.domain.models.Contact
import io.cpunk.dna.domain.models.Group
import io.cpunk.dna.domain.models.Message
import io.cpunk.dna.domain.models.MessageStatus

/**
 * Android implementation of MessageRepository
 *
 * Combines DatabaseRepository (PostgreSQL) with DNAMessenger (post-quantum crypto)
 * to provide secure end-to-end encrypted messaging.
 */
actual class MessageRepository {
    private val database = DatabaseRepository()
    private val crypto = DNAMessenger()

    // User's identity and keys
    private var currentIdentity: String? = null
    private var encryptionPrivateKey: ByteArray? = null
    private var signingPrivateKey: ByteArray? = null
    private var signingPublicKey: ByteArray? = null

    companion object {
        private const val TAG = "MessageRepository"
    }

    /**
     * Initialize with user's private keys
     */
    actual fun initialize(
        identity: String,
        encryptionPrivateKey: ByteArray,
        signingPrivateKey: ByteArray,
        signingPublicKey: ByteArray
    ): Result<Unit> {
        return runCatching {
            this.currentIdentity = identity
            this.encryptionPrivateKey = encryptionPrivateKey
            this.signingPrivateKey = signingPrivateKey
            this.signingPublicKey = signingPublicKey

            Log.d(TAG, "Initialized for identity: $identity")
        }
    }

    // ============================================================================
    // MESSAGE OPERATIONS
    // ============================================================================

    /**
     * Send encrypted message to recipient
     */
    actual suspend fun sendMessage(
        recipient: String,
        plaintext: String
    ): Result<Long> {
        return runCatching {
            // Check initialization
            val identity = currentIdentity ?: throw IllegalStateException("MessageRepository not initialized")
            val signPrivKey = signingPrivateKey ?: throw IllegalStateException("Signing private key not set")
            val signPubKey = signingPublicKey ?: throw IllegalStateException("Signing public key not set")

            // Load recipient's public key
            val contact = database.loadContact(recipient)
                .getOrElse { error ->
                    throw Exception("Failed to load recipient public key: ${error.message}", error)
                }
                ?: throw Exception("Recipient not found: $recipient")

            Log.d(TAG, "Encrypting message for: $recipient")

            // Encrypt message
            val plaintextBytes = plaintext.toByteArray(Charsets.UTF_8)
            val ciphertext = crypto.encryptMessage(
                plaintext = plaintextBytes,
                recipientEncPubKey = contact.encryptionPubkey,
                senderSignPubKey = signPubKey,
                senderSignPrivKey = signPrivKey
            ).getOrElse { error ->
                throw Exception("Encryption failed: ${error.message}", error)
            }

            Log.d(TAG, "Message encrypted: ${ciphertext.size} bytes")

            // Save to database
            val message = Message(
                sender = identity,
                recipient = recipient,
                ciphertext = ciphertext,
                createdAt = System.currentTimeMillis(),
                status = MessageStatus.SENT
            )

            val messageId = database.saveMessage(message).getOrThrow()
            Log.d(TAG, "Message saved: ID=$messageId")

            messageId
        }
    }

    /**
     * Send encrypted message to group
     */
    actual suspend fun sendGroupMessage(
        groupId: Int,
        plaintext: String
    ): Result<Long> {
        return runCatching {
            // Check initialization
            val identity = currentIdentity ?: throw IllegalStateException("MessageRepository not initialized")
            val signPrivKey = signingPrivateKey ?: throw IllegalStateException("Signing private key not set")
            val signPubKey = signingPublicKey ?: throw IllegalStateException("Signing public key not set")

            // Load group and members
            val group = database.loadGroup(groupId)
                .getOrElse { error ->
                    throw Exception("Failed to load group: ${error.message}", error)
                }
                ?: throw Exception("Group not found: $groupId")

            if (group.members.isEmpty()) {
                throw Exception("Group has no members")
            }

            Log.d(TAG, "Sending message to group: ${group.name} (${group.members.size} members)")

            // For group messages, we'll encrypt separately for each member
            // This allows each member to decrypt with their own key
            val plaintextBytes = plaintext.toByteArray(Charsets.UTF_8)

            // For now, use a simplified approach: encrypt for the first member
            // TODO: Implement proper multi-recipient encryption
            val firstMember = group.members.firstOrNull()
                ?: throw Exception("Group has no members")

            val memberContact = database.loadContact(firstMember.member)
                .getOrElse { error ->
                    throw Exception("Failed to load member public key: ${error.message}", error)
                }
                ?: throw Exception("Member not found: ${firstMember.member}")

            val ciphertext = crypto.encryptMessage(
                plaintext = plaintextBytes,
                recipientEncPubKey = memberContact.encryptionPubkey,
                senderSignPubKey = signPubKey,
                senderSignPrivKey = signPrivKey
            ).getOrElse { error ->
                throw Exception("Encryption failed: ${error.message}", error)
            }

            // Save to database with group ID
            val message = Message(
                sender = identity,
                recipient = group.name,  // Use group name as recipient
                ciphertext = ciphertext,
                createdAt = System.currentTimeMillis(),
                status = MessageStatus.SENT,
                groupId = groupId
            )

            val messageId = database.saveMessage(message).getOrThrow()
            Log.d(TAG, "Group message saved: ID=$messageId")

            messageId
        }
    }

    /**
     * Load conversation and decrypt messages
     */
    actual suspend fun loadConversation(
        otherUser: String,
        limit: Int,
        offset: Int
    ): Result<List<DecryptedMessage>> {
        return runCatching {
            // Check initialization
            val identity = currentIdentity ?: throw IllegalStateException("MessageRepository not initialized")
            val encPrivKey = encryptionPrivateKey ?: throw IllegalStateException("Encryption private key not set")

            // Load encrypted messages
            val messages = database.loadConversation(
                currentUser = identity,
                otherUser = otherUser,
                limit = limit,
                offset = offset
            ).getOrThrow()

            Log.d(TAG, "Loaded ${messages.size} messages, decrypting...")

            // Decrypt each message
            messages.map { message ->
                decryptMessage(message, encPrivKey)
            }
        }
    }

    /**
     * Load group messages and decrypt
     */
    actual suspend fun loadGroupMessages(
        groupId: Int,
        limit: Int,
        offset: Int
    ): Result<List<DecryptedMessage>> {
        return runCatching {
            // Check initialization
            val encPrivKey = encryptionPrivateKey ?: throw IllegalStateException("Encryption private key not set")

            // Load encrypted messages
            val messages = database.loadGroupMessages(
                groupId = groupId,
                limit = limit,
                offset = offset
            ).getOrThrow()

            Log.d(TAG, "Loaded ${messages.size} group messages, decrypting...")

            // Decrypt each message
            messages.map { message ->
                decryptMessage(message, encPrivKey)
            }
        }
    }

    /**
     * Decrypt a single message
     */
    private fun decryptMessage(message: Message, encPrivKey: ByteArray): DecryptedMessage {
        val decryptResult = crypto.decryptMessage(
            ciphertext = message.ciphertext,
            recipientEncPrivKey = encPrivKey
        )

        return decryptResult.fold(
            onSuccess = { (plaintextBytes, _) ->
                val plaintext = String(plaintextBytes, Charsets.UTF_8)
                DecryptedMessage(
                    id = message.id,
                    sender = message.sender,
                    recipient = message.recipient,
                    plaintext = plaintext,
                    createdAt = message.createdAt,
                    status = message.status,
                    deliveredAt = message.deliveredAt,
                    readAt = message.readAt,
                    groupId = message.groupId,
                    decryptionFailed = false
                )
            },
            onFailure = { error ->
                Log.e(TAG, "Failed to decrypt message ${message.id}", error)
                DecryptedMessage(
                    id = message.id,
                    sender = message.sender,
                    recipient = message.recipient,
                    plaintext = "[Decryption failed: ${error.message}]",
                    createdAt = message.createdAt,
                    status = message.status,
                    deliveredAt = message.deliveredAt,
                    readAt = message.readAt,
                    groupId = message.groupId,
                    decryptionFailed = true
                )
            }
        )
    }

    /**
     * Update message status
     */
    actual suspend fun updateMessageStatus(
        messageId: Long,
        status: String
    ): Result<Unit> {
        return database.updateMessageStatus(messageId, status)
    }

    // ============================================================================
    // CONTACT OPERATIONS
    // ============================================================================

    actual suspend fun addContact(contact: Contact): Result<Unit> {
        return database.saveContact(contact)
    }

    actual suspend fun getContact(identity: String): Result<Contact?> {
        return database.loadContact(identity)
    }

    actual suspend fun listContacts(): Result<List<Contact>> {
        return database.loadAllContacts()
    }

    actual suspend fun deleteContact(identity: String): Result<Unit> {
        return database.deleteContact(identity)
    }

    // ============================================================================
    // GROUP OPERATIONS
    // ============================================================================

    actual suspend fun createGroup(group: Group): Result<Int> {
        return database.createGroup(group)
    }

    actual suspend fun getGroup(groupId: Int): Result<Group?> {
        return database.loadGroup(groupId)
    }

    actual suspend fun listGroups(): Result<List<Group>> {
        // Check initialization
        val identity = currentIdentity ?: throw IllegalStateException("MessageRepository not initialized")
        return database.loadUserGroups(identity)
    }

    /**
     * Close and cleanup resources
     */
    actual fun close() {
        database.close()
        crypto.close()
        Log.d(TAG, "MessageRepository closed")
    }
}
