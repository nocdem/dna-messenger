package io.cpunk.dna.domain.models

/**
 * Contact data class
 * Represents a contact in DNA Messenger
 */
data class Contact(
    val id: String,
    val name: String,
    val encryptionPublicKey: ByteArray,  // Kyber512 public key (800 bytes)
    val signingPublicKey: ByteArray,     // Dilithium3 public key (1952 bytes)
    val lastSeen: Long? = null,
    val isOnline: Boolean = false,
    val avatarUrl: String? = null
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other == null || this::class != other::class) return false

        other as Contact

        if (id != other.id) return false
        if (name != other.name) return false
        if (!encryptionPublicKey.contentEquals(other.encryptionPublicKey)) return false
        if (!signingPublicKey.contentEquals(other.signingPublicKey)) return false
        if (lastSeen != other.lastSeen) return false
        if (isOnline != other.isOnline) return false
        if (avatarUrl != other.avatarUrl) return false

        return true
    }

    override fun hashCode(): Int {
        var result = id.hashCode()
        result = 31 * result + name.hashCode()
        result = 31 * result + encryptionPublicKey.contentHashCode()
        result = 31 * result + signingPublicKey.contentHashCode()
        result = 31 * result + (lastSeen?.hashCode() ?: 0)
        result = 31 * result + isOnline.hashCode()
        result = 31 * result + (avatarUrl?.hashCode() ?: 0)
        return result
    }
}
