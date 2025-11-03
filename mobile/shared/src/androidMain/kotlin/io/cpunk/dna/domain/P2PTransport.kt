package io.cpunk.dna.domain

import android.util.Log

/**
 * Android implementation of P2PTransport using JNI to call native C functions
 */
actual class P2PTransport {
    actual fun init(messengerCtxPtr: Long): Result<Unit> {
        return runCatching {
            val result = nativeInit(messengerCtxPtr)
            if (result != 0) {
                throw RuntimeException("P2P init failed with code: $result")
            }
            Log.i(TAG, "P2P transport initialized successfully")
        }
    }

    actual fun shutdown(messengerCtxPtr: Long) {
        try {
            nativeShutdown(messengerCtxPtr)
            Log.i(TAG, "P2P transport shutdown complete")
        } catch (e: Exception) {
            Log.e(TAG, "Error during P2P shutdown", e)
        }
    }

    actual fun sendMessage(
        messengerCtxPtr: Long,
        recipient: String,
        encryptedMessage: ByteArray
    ): Result<Unit> {
        return runCatching {
            val result = nativeSendMessage(messengerCtxPtr, recipient, encryptedMessage)
            if (result != 0) {
                throw RuntimeException("P2P send failed with code: $result")
            }
            Log.d(TAG, "Message sent successfully to $recipient (${encryptedMessage.size} bytes)")
        }
    }

    actual fun broadcastMessage(
        messengerCtxPtr: Long,
        recipients: List<String>,
        encryptedMessage: ByteArray
    ): Result<Unit> {
        return runCatching {
            val recipientsArray = recipients.toTypedArray()
            val result = nativeBroadcastMessage(
                messengerCtxPtr,
                recipientsArray,
                encryptedMessage
            )
            if (result != 0) {
                throw RuntimeException("P2P broadcast failed with code: $result")
            }
            Log.d(TAG, "Message broadcast to ${recipients.size} recipients (${encryptedMessage.size} bytes)")
        }
    }

    actual fun isPeerOnline(messengerCtxPtr: Long, identity: String): Boolean {
        return try {
            val online = nativeIsPeerOnline(messengerCtxPtr, identity)
            Log.d(TAG, "Peer $identity is ${if (online) "online" else "offline"}")
            online
        } catch (e: Exception) {
            Log.e(TAG, "Error checking peer status for $identity", e)
            false
        }
    }

    actual fun refreshPresence(messengerCtxPtr: Long): Result<Unit> {
        return runCatching {
            val result = nativeRefreshPresence(messengerCtxPtr)
            if (result != 0) {
                throw RuntimeException("Presence refresh failed with code: $result")
            }
            Log.d(TAG, "Presence refreshed successfully")
        }
    }

    actual fun checkOfflineMessages(messengerCtxPtr: Long): Result<Int> {
        return runCatching {
            val count = nativeCheckOfflineMessages(messengerCtxPtr)
            if (count < 0) {
                throw RuntimeException("Offline message check failed")
            }
            if (count > 0) {
                Log.i(TAG, "Retrieved $count offline message(s)")
            } else {
                Log.d(TAG, "No offline messages")
            }
            count
        }
    }

    // Native method declarations (implemented in p2p_jni.cpp)
    private external fun nativeInit(messengerCtxPtr: Long): Int
    private external fun nativeShutdown(messengerCtxPtr: Long)
    private external fun nativeSendMessage(
        messengerCtxPtr: Long,
        recipient: String,
        encryptedMessage: ByteArray
    ): Int
    private external fun nativeBroadcastMessage(
        messengerCtxPtr: Long,
        recipients: Array<String>,
        encryptedMessage: ByteArray
    ): Int
    private external fun nativeIsPeerOnline(
        messengerCtxPtr: Long,
        identity: String
    ): Boolean
    private external fun nativeRefreshPresence(messengerCtxPtr: Long): Int
    private external fun nativeCheckOfflineMessages(messengerCtxPtr: Long): Int

    companion object {
        private const val TAG = "P2PTransport"

        init {
            // Load the same library as DNAMessenger (libdna_jni.so)
            System.loadLibrary("dna_jni")
        }
    }
}
