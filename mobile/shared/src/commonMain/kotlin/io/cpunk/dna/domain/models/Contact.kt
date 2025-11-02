package io.cpunk.dna.domain.models

/**
 * Contact data class
 * Represents a contact in DNA Messenger
 *
 * Maps to database table: keyserver
 * Schema:
 * - id (integer) - auto-generated
 * - identity (text) - unique identity name
 * - signing_pubkey (bytea) - Dilithium3 public key
 * - signing_pubkey_len (integer)
 * - encryption_pubkey (bytea) - Kyber512 public key
 * - encryption_pubkey_len (integer)
 * - fingerprint (text) - key fingerprint
 * - created_at (timestamp)
 */
data class Contact(
    val id: Int = 0,                     // Database ID (auto-generated)
    val identity: String,                // Unique identity name
    val signingPubkey: ByteArray,        // Dilithium3 public key (1952 bytes)
    val signingPubkeyLen: Int = signingPubkey.size,
    val encryptionPubkey: ByteArray,     // Kyber512 public key (800 bytes)
    val encryptionPubkeyLen: Int = encryptionPubkey.size,
    val fingerprint: String? = null,     // Key fingerprint (optional)
    val createdAt: Long? = null          // Unix timestamp (milliseconds)
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other == null || this::class != other::class) return false

        other as Contact

        if (id != other.id) return false
        if (identity != other.identity) return false
        if (!signingPubkey.contentEquals(other.signingPubkey)) return false
        if (signingPubkeyLen != other.signingPubkeyLen) return false
        if (!encryptionPubkey.contentEquals(other.encryptionPubkey)) return false
        if (encryptionPubkeyLen != other.encryptionPubkeyLen) return false
        if (fingerprint != other.fingerprint) return false
        if (createdAt != other.createdAt) return false

        return true
    }

    override fun hashCode(): Int {
        var result = id
        result = 31 * result + identity.hashCode()
        result = 31 * result + signingPubkey.contentHashCode()
        result = 31 * result + signingPubkeyLen
        result = 31 * result + encryptionPubkey.contentHashCode()
        result = 31 * result + encryptionPubkeyLen
        result = 31 * result + (fingerprint?.hashCode() ?: 0)
        result = 31 * result + (createdAt?.hashCode() ?: 0)
        return result
    }
}
