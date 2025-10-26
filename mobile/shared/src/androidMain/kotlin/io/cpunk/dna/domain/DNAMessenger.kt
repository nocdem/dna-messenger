package io.cpunk.dna.domain

import android.util.Log

/**
 * Android implementation of DNAMessenger using JNI
 */
actual class DNAMessenger {
    private var contextPtr: Long = 0

    init {
        // Load native libraries
        try {
            System.loadLibrary("dna_jni")
            contextPtr = nativeInit()
            Log.d(TAG, "DNAMessenger initialized with context: $contextPtr")
        } catch (e: UnsatisfiedLinkError) {
            Log.e(TAG, "Failed to load native library", e)
            throw RuntimeException("Failed to load DNA native library", e)
        }
    }

    /**
     * Generate Kyber512 encryption keypair
     */
    actual fun generateEncryptionKeyPair(): Result<Pair<ByteArray, ByteArray>> {
        return runCatching {
            if (contextPtr == 0L) {
                throw IllegalStateException("DNAMessenger context not initialized")
            }
            nativeGenerateEncryptionKeyPair(contextPtr)
        }
    }

    /**
     * Generate Dilithium3 signing keypair
     */
    actual fun generateSigningKeyPair(): Result<Pair<ByteArray, ByteArray>> {
        return runCatching {
            if (contextPtr == 0L) {
                throw IllegalStateException("DNAMessenger context not initialized")
            }
            nativeGenerateSigningKeyPair(contextPtr)
        }
    }

    /**
     * Encrypt message for recipient
     */
    actual fun encryptMessage(
        plaintext: ByteArray,
        recipientEncPubKey: ByteArray,
        senderSignPubKey: ByteArray,
        senderSignPrivKey: ByteArray
    ): Result<ByteArray> {
        return runCatching {
            if (contextPtr == 0L) {
                throw IllegalStateException("DNAMessenger context not initialized")
            }

            // Validate key sizes
            require(recipientEncPubKey.size == 800) { "Recipient encryption public key must be 800 bytes (Kyber512)" }
            require(senderSignPubKey.size == 1952) { "Sender signing public key must be 1952 bytes (Dilithium3)" }
            require(senderSignPrivKey.size == 4032) { "Sender signing private key must be 4032 bytes (Dilithium3)" }

            nativeEncrypt(
                contextPtr,
                plaintext,
                recipientEncPubKey,
                senderSignPubKey,
                senderSignPrivKey
            )
        }
    }

    /**
     * Decrypt message
     */
    actual fun decryptMessage(
        ciphertext: ByteArray,
        recipientEncPrivKey: ByteArray
    ): Result<Pair<ByteArray, ByteArray>> {
        return runCatching {
            if (contextPtr == 0L) {
                throw IllegalStateException("DNAMessenger context not initialized")
            }

            // Validate key size
            require(recipientEncPrivKey.size == 1632) { "Recipient encryption private key must be 1632 bytes (Kyber512)" }

            nativeDecrypt(contextPtr, ciphertext, recipientEncPrivKey)
        }
    }

    /**
     * Get library version
     */
    actual fun getVersion(): String {
        return nativeGetVersion()
    }

    /**
     * Close and cleanup resources
     */
    actual fun close() {
        if (contextPtr != 0L) {
            nativeFree(contextPtr)
            contextPtr = 0
            Log.d(TAG, "DNAMessenger context freed")
        }
    }

    // Native method declarations
    private external fun nativeInit(): Long
    private external fun nativeFree(contextPtr: Long)
    private external fun nativeGenerateEncryptionKeyPair(contextPtr: Long): Pair<ByteArray, ByteArray>
    private external fun nativeGenerateSigningKeyPair(contextPtr: Long): Pair<ByteArray, ByteArray>
    private external fun nativeEncrypt(
        contextPtr: Long,
        plaintext: ByteArray,
        recipientEncPubKey: ByteArray,
        senderSignPubKey: ByteArray,
        senderSignPrivKey: ByteArray
    ): ByteArray
    private external fun nativeDecrypt(
        contextPtr: Long,
        ciphertext: ByteArray,
        recipientEncPrivKey: ByteArray
    ): Pair<ByteArray, ByteArray>
    private external fun nativeGetVersion(): String

    companion object {
        private const val TAG = "DNAMessenger"
    }
}
