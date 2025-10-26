package io.cpunk.dna.domain

/**
 * DNAMessenger - Cross-platform cryptographic operations interface
 *
 * Provides post-quantum encryption/decryption using:
 * - Kyber512 for key encapsulation
 * - Dilithium3 for digital signatures
 * - AES-256-GCM for symmetric encryption
 *
 * Platform implementations:
 * - Android: JNI wrapper to C library
 * - iOS: Direct C interop
 */
expect class DNAMessenger() {
    /**
     * Generate Kyber512 encryption keypair
     *
     * @return Pair of (publicKey, privateKey)
     *         - publicKey: 800 bytes
     *         - privateKey: 1632 bytes
     */
    fun generateEncryptionKeyPair(): Result<Pair<ByteArray, ByteArray>>

    /**
     * Generate Dilithium3 signing keypair
     *
     * @return Pair of (publicKey, privateKey)
     *         - publicKey: 1952 bytes
     *         - privateKey: 4032 bytes
     */
    fun generateSigningKeyPair(): Result<Pair<ByteArray, ByteArray>>

    /**
     * Encrypt message for recipient
     *
     * @param plaintext Message to encrypt
     * @param recipientEncPubKey Recipient's Kyber512 public key (800 bytes)
     * @param senderSignPubKey Sender's Dilithium3 public key (1952 bytes)
     * @param senderSignPrivKey Sender's Dilithium3 private key (4032 bytes)
     * @return Encrypted ciphertext
     */
    fun encryptMessage(
        plaintext: ByteArray,
        recipientEncPubKey: ByteArray,
        senderSignPubKey: ByteArray,
        senderSignPrivKey: ByteArray
    ): Result<ByteArray>

    /**
     * Decrypt message
     *
     * @param ciphertext Encrypted message
     * @param recipientEncPrivKey Recipient's Kyber512 private key (1632 bytes)
     * @return Pair of (plaintext, senderSignPubKey)
     */
    fun decryptMessage(
        ciphertext: ByteArray,
        recipientEncPrivKey: ByteArray
    ): Result<Pair<ByteArray, ByteArray>>

    /**
     * Get library version
     *
     * @return Version string (e.g., "0.1.0-alpha")
     */
    fun getVersion(): String

    /**
     * Close and cleanup resources
     */
    fun close()
}
