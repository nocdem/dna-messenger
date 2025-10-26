package io.cpunk.dna.domain.models

/**
 * Message data class
 * Represents an encrypted message in DNA Messenger
 */
data class Message(
    val id: Long,
    val senderId: String,
    val recipientId: String,
    val content: ByteArray,  // Encrypted ciphertext
    val timestamp: Long,
    val isRead: Boolean = false,
    val messageType: MessageType = MessageType.TEXT
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other == null || this::class != other::class) return false

        other as Message

        if (id != other.id) return false
        if (senderId != other.senderId) return false
        if (recipientId != other.recipientId) return false
        if (!content.contentEquals(other.content)) return false
        if (timestamp != other.timestamp) return false
        if (isRead != other.isRead) return false
        if (messageType != other.messageType) return false

        return true
    }

    override fun hashCode(): Int {
        var result = id.hashCode()
        result = 31 * result + senderId.hashCode()
        result = 31 * result + recipientId.hashCode()
        result = 31 * result + content.contentHashCode()
        result = 31 * result + timestamp.hashCode()
        result = 31 * result + isRead.hashCode()
        result = 31 * result + messageType.hashCode()
        return result
    }
}

enum class MessageType {
    TEXT,
    IMAGE,
    FILE,
    VOICE
}
