package io.cpunk.dna.domain.models

/**
 * Message data class
 * Represents an encrypted message in DNA Messenger
 *
 * Maps to database table: messages
 * Schema:
 * - id (integer)
 * - sender (text) - identity name
 * - recipient (text) - identity name
 * - ciphertext (bytea) - encrypted message blob
 * - ciphertext_len (integer)
 * - created_at (timestamp)
 * - status (text) - 'sent', 'delivered', 'read'
 * - delivered_at (timestamp)
 * - read_at (timestamp)
 * - group_id (integer) - null for 1:1 messages
 */
data class Message(
    val id: Long = 0,
    val sender: String,                      // Identity name (not ID)
    val recipient: String,                   // Identity name (not ID)
    val ciphertext: ByteArray,               // Encrypted message blob
    val ciphertextLen: Int = ciphertext.size,
    val createdAt: Long,                     // Unix timestamp (milliseconds)
    val status: MessageStatus = MessageStatus.SENT,
    val deliveredAt: Long? = null,
    val readAt: Long? = null,
    val groupId: Int? = null                 // Null for 1:1 messages
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other == null || this::class != other::class) return false

        other as Message

        if (id != other.id) return false
        if (sender != other.sender) return false
        if (recipient != other.recipient) return false
        if (!ciphertext.contentEquals(other.ciphertext)) return false
        if (ciphertextLen != other.ciphertextLen) return false
        if (createdAt != other.createdAt) return false
        if (status != other.status) return false
        if (deliveredAt != other.deliveredAt) return false
        if (readAt != other.readAt) return false
        if (groupId != other.groupId) return false

        return true
    }

    override fun hashCode(): Int {
        var result = id.hashCode()
        result = 31 * result + sender.hashCode()
        result = 31 * result + recipient.hashCode()
        result = 31 * result + ciphertext.contentHashCode()
        result = 31 * result + ciphertextLen
        result = 31 * result + createdAt.hashCode()
        result = 31 * result + status.hashCode()
        result = 31 * result + (deliveredAt?.hashCode() ?: 0)
        result = 31 * result + (readAt?.hashCode() ?: 0)
        result = 31 * result + (groupId?.hashCode() ?: 0)
        return result
    }
}

/**
 * Message status enum
 * Maps to messages.status column
 */
enum class MessageStatus {
    SENT,      // Message sent but not delivered
    DELIVERED, // Message delivered to recipient
    READ;      // Message read by recipient

    companion object {
        fun fromString(status: String): MessageStatus {
            return when (status.lowercase()) {
                "sent" -> SENT
                "delivered" -> DELIVERED
                "read" -> READ
                else -> SENT
            }
        }
    }

    override fun toString(): String {
        return name.lowercase()
    }
}
