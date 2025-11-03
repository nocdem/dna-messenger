package io.cpunk.dna.domain

import android.util.Log

/**
 * Android implementation of MessengerContext using JNI
 *
 * Mobile version: No PostgreSQL, pure P2P messaging only.
 */
actual class MessengerContext {
    private var contextPtr: Long = 0
    private var identity: String = ""

    actual fun init(identity: String, dnaMessenger: DNAMessenger): Result<Long> {
        return runCatching {
            if (this.contextPtr != 0L) {
                throw IllegalStateException("MessengerContext already initialized")
            }

            // Get DNA context pointer from DNAMessenger
            // We need to access the internal contextPtr field
            val dnaCtxPtr = getDnaContextPtr(dnaMessenger)

            if (dnaCtxPtr == 0L) {
                throw IllegalStateException("DNAMessenger context not initialized")
            }

            // Create mobile messenger context (no PostgreSQL)
            val messengerCtxPtr = nativeInit(identity, dnaCtxPtr)

            if (messengerCtxPtr == 0L) {
                throw RuntimeException("Failed to create messenger context")
            }

            this.contextPtr = messengerCtxPtr
            this.identity = identity

            Log.i(TAG, "MessengerContext initialized for identity: $identity (ptr: $contextPtr)")

            messengerCtxPtr
        }
    }

    actual fun getContextPtr(): Long {
        return contextPtr
    }

    actual fun getIdentity(): String {
        return identity
    }

    actual fun close() {
        if (contextPtr != 0L) {
            nativeFree(contextPtr)
            contextPtr = 0
            Log.i(TAG, "MessengerContext closed for identity: $identity")
        }
    }

    /**
     * Get DNA context pointer from DNAMessenger using reflection
     * (since contextPtr is private in DNAMessenger)
     */
    private fun getDnaContextPtr(dnaMessenger: DNAMessenger): Long {
        return try {
            val field = dnaMessenger::class.java.getDeclaredField("contextPtr")
            field.isAccessible = true
            field.getLong(dnaMessenger)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to get DNA context pointer via reflection", e)
            throw RuntimeException("Cannot access DNAMessenger context pointer", e)
        }
    }

    // Native method declarations (implemented in p2p_jni.cpp)
    private external fun nativeInit(identity: String, dnaCtxPtr: Long): Long
    private external fun nativeFree(messengerCtxPtr: Long)

    companion object {
        private const val TAG = "MessengerContext"

        init {
            // Load the same library as DNAMessenger and P2PTransport
            System.loadLibrary("dna_jni")
        }
    }
}
