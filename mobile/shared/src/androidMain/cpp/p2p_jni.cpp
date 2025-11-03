/**
 * P2P/DHT JNI Bridge for Android
 *
 * Exposes P2P transport and DHT functionality to Kotlin via JNI.
 * Uses messenger_p2p.h for hybrid P2P + PostgreSQL fallback messaging.
 */

#include <jni.h>
#include <android/log.h>
#include "jni_utils.h"
#include <cstdlib>
#include <cstring>

// C library headers - need extern "C" linkage
extern "C" {
#include "messenger.h"       // Now includes stub libpq-fe.h
#include "messenger_p2p.h"   // ACTUAL desktop code!
#include "p2p/p2p_transport.h"
#include "dht/dht_context.h"
}

#define LOG_TAG "P2P_JNI"

// ============================================================================
// Mobile Messenger Context (No PostgreSQL)
// ============================================================================

/**
 * Create mobile messenger context (without PostgreSQL)
 *
 * @param identity User identity string
 * @param dnaCtxPtr DNA context pointer (from DNAMessenger.nativeInit())
 * @return Messenger context pointer, or 0 on error
 */
extern "C" JNIEXPORT jlong JNICALL
Java_io_cpunk_dna_domain_MessengerContext_nativeInit(
    JNIEnv* env,
    jobject obj,
    jstring identity,
    jlong dnaCtxPtr
) {
    const char* identity_str = env->GetStringUTFChars(identity, nullptr);
    if (!identity_str) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Failed to get identity string");
        return 0;
    }

    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,
                       "Creating mobile messenger context for identity: %s", identity_str);

    // Allocate messenger context (mobile version: no PostgreSQL)
    messenger_context_t* ctx = (messenger_context_t*)calloc(1, sizeof(messenger_context_t));
    if (!ctx) {
        env->ReleaseStringUTFChars(identity, identity_str);
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Failed to allocate messenger context");
        return 0;
    }

    // Set identity
    ctx->identity = strdup(identity_str);
    env->ReleaseStringUTFChars(identity, identity_str);

    // Use existing DNA context from DNAMessenger
    ctx->dna_ctx = reinterpret_cast<dna_context_t*>(dnaCtxPtr);

    // No PostgreSQL on mobile
    ctx->pg_conn = nullptr;

    // P2P will be initialized separately via P2PTransport.init()
    ctx->p2p_transport = nullptr;
    ctx->p2p_enabled = true;  // Enable P2P by default on mobile

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG,
                       "Mobile messenger context created: %p", ctx);

    return reinterpret_cast<jlong>(ctx);
}

/**
 * Free mobile messenger context
 *
 * @param messengerCtxPtr Messenger context pointer
 */
extern "C" JNIEXPORT void JNICALL
Java_io_cpunk_dna_domain_MessengerContext_nativeFree(
    JNIEnv* env,
    jobject obj,
    jlong messengerCtxPtr
) {
    messenger_context_t* ctx = reinterpret_cast<messenger_context_t*>(messengerCtxPtr);
    if (!ctx) {
        return;
    }

    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "Freeing mobile messenger context");

    // Shutdown P2P if initialized
    if (ctx->p2p_transport) {
        messenger_p2p_shutdown(ctx);
    }

    // Free identity string
    if (ctx->identity) {
        free(ctx->identity);
    }

    // Note: We don't free dna_ctx here because it's owned by DNAMessenger
    // Also no PostgreSQL connection to close

    free(ctx);

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Mobile messenger context freed");
}

// ============================================================================
// P2P Initialization
// ============================================================================

/**
 * Initialize P2P transport for messenger
 *
 * @param messengerCtxPtr Messenger context pointer (long)
 * @return 0 on success, -1 on error
 */
extern "C" JNIEXPORT jint JNICALL
Java_io_cpunk_dna_domain_P2PTransport_nativeInit(
    JNIEnv* env,
    jobject obj,
    jlong messengerCtxPtr
) {
    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "Initializing P2P transport");

    messenger_context_t* ctx = reinterpret_cast<messenger_context_t*>(messengerCtxPtr);
    if (!ctx) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Null messenger context");
        return -1;
    }

    // Use ACTUAL messenger_p2p_init() from desktop code
    int result = messenger_p2p_init(ctx);

    if (result == 0) {
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "P2P transport initialized successfully");
    } else {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Failed to initialize P2P transport");
    }

    return result;
}

/**
 * Shutdown P2P transport for messenger
 *
 * @param messengerCtxPtr Messenger context pointer (long)
 */
extern "C" JNIEXPORT void JNICALL
Java_io_cpunk_dna_domain_P2PTransport_nativeShutdown(
    JNIEnv* env,
    jobject obj,
    jlong messengerCtxPtr
) {
    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "Shutting down P2P transport");

    messenger_context_t* ctx = reinterpret_cast<messenger_context_t*>(messengerCtxPtr);
    if (!ctx) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Null messenger context");
        return;
    }

    messenger_p2p_shutdown(ctx);
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "P2P transport shut down");
}

// ============================================================================
// Hybrid Messaging (P2P + PostgreSQL Fallback)
// ============================================================================

/**
 * Send message via P2P with PostgreSQL fallback
 *
 * Tries P2P first, falls back to PostgreSQL if peer is offline.
 *
 * @param messengerCtxPtr Messenger context pointer (long)
 * @param recipient Recipient identity
 * @param encryptedMessage Encrypted message data
 * @return 0 on success, -1 on error
 */
extern "C" JNIEXPORT jint JNICALL
Java_io_cpunk_dna_domain_P2PTransport_nativeSendMessage(
    JNIEnv* env,
    jobject obj,
    jlong messengerCtxPtr,
    jstring recipient,
    jbyteArray encryptedMessage
) {
    messenger_context_t* ctx = reinterpret_cast<messenger_context_t*>(messengerCtxPtr);
    if (!ctx) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Null messenger context");
        return -1;
    }

    // Convert Java string to C string
    const char* recipient_str = env->GetStringUTFChars(recipient, nullptr);
    if (!recipient_str) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Failed to get recipient string");
        return -1;
    }

    // Get encrypted message bytes
    jsize msg_len = env->GetArrayLength(encryptedMessage);
    jbyte* msg_bytes = env->GetByteArrayElements(encryptedMessage, nullptr);
    if (!msg_bytes) {
        env->ReleaseStringUTFChars(recipient, recipient_str);
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Failed to get message bytes");
        return -1;
    }

    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,
                       "Sending P2P message to %s (%d bytes)",
                       recipient_str, msg_len);

    // Call hybrid send (tries P2P first, PostgreSQL fallback)
    int result = messenger_send_p2p(
        ctx,
        recipient_str,
        reinterpret_cast<const uint8_t*>(msg_bytes),
        static_cast<size_t>(msg_len)
    );

    // Release JNI resources
    env->ReleaseByteArrayElements(encryptedMessage, msg_bytes, JNI_ABORT);
    env->ReleaseStringUTFChars(recipient, recipient_str);

    if (result == 0) {
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Message sent successfully");
    } else {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Failed to send message");
    }

    return result;
}

/**
 * Broadcast message to multiple recipients via P2P
 *
 * @param messengerCtxPtr Messenger context pointer (long)
 * @param recipients Array of recipient identities
 * @param encryptedMessage Encrypted message data
 * @return 0 on success, -1 on error
 */
extern "C" JNIEXPORT jint JNICALL
Java_io_cpunk_dna_domain_P2PTransport_nativeBroadcastMessage(
    JNIEnv* env,
    jobject obj,
    jlong messengerCtxPtr,
    jobjectArray recipients,
    jbyteArray encryptedMessage
) {
    messenger_context_t* ctx = reinterpret_cast<messenger_context_t*>(messengerCtxPtr);
    if (!ctx) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Null messenger context");
        return -1;
    }

    // Get recipient count
    jsize recipient_count = env->GetArrayLength(recipients);
    if (recipient_count == 0) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Empty recipients array");
        return -1;
    }

    // Allocate array for C strings
    const char** recipient_strs = new const char*[recipient_count];

    // Convert Java strings to C strings
    for (jsize i = 0; i < recipient_count; i++) {
        jstring jstr = (jstring)env->GetObjectArrayElement(recipients, i);
        recipient_strs[i] = env->GetStringUTFChars(jstr, nullptr);
        env->DeleteLocalRef(jstr);
    }

    // Get encrypted message bytes
    jsize msg_len = env->GetArrayLength(encryptedMessage);
    jbyte* msg_bytes = env->GetByteArrayElements(encryptedMessage, nullptr);

    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,
                       "Broadcasting P2P message to %d recipients (%d bytes)",
                       recipient_count, msg_len);

    // Call broadcast
    int result = messenger_broadcast_p2p(
        ctx,
        recipient_strs,
        static_cast<size_t>(recipient_count),
        reinterpret_cast<const uint8_t*>(msg_bytes),
        static_cast<size_t>(msg_len)
    );

    // Release JNI resources
    env->ReleaseByteArrayElements(encryptedMessage, msg_bytes, JNI_ABORT);

    // Release recipient strings
    for (jsize i = 0; i < recipient_count; i++) {
        jstring jstr = (jstring)env->GetObjectArrayElement(recipients, i);
        env->ReleaseStringUTFChars(jstr, recipient_strs[i]);
        env->DeleteLocalRef(jstr);
    }
    delete[] recipient_strs;

    if (result == 0) {
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Broadcast successful");
    } else {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Broadcast failed");
    }

    return result;
}

// ============================================================================
// Presence & Peer Discovery
// ============================================================================

/**
 * Check if peer is online via DHT
 *
 * @param messengerCtxPtr Messenger context pointer (long)
 * @param identity Peer identity to check
 * @return true if online, false if offline
 */
extern "C" JNIEXPORT jboolean JNICALL
Java_io_cpunk_dna_domain_P2PTransport_nativeIsPeerOnline(
    JNIEnv* env,
    jobject obj,
    jlong messengerCtxPtr,
    jstring identity
) {
    messenger_context_t* ctx = reinterpret_cast<messenger_context_t*>(messengerCtxPtr);
    if (!ctx) {
        return JNI_FALSE;
    }

    const char* identity_str = env->GetStringUTFChars(identity, nullptr);
    if (!identity_str) {
        return JNI_FALSE;
    }

    bool is_online = messenger_p2p_peer_online(ctx, identity_str);

    env->ReleaseStringUTFChars(identity, identity_str);

    return is_online ? JNI_TRUE : JNI_FALSE;
}

/**
 * Refresh presence announcement in DHT
 *
 * Should be called periodically (every 5 minutes) to maintain presence.
 *
 * @param messengerCtxPtr Messenger context pointer (long)
 * @return 0 on success, -1 on error
 */
extern "C" JNIEXPORT jint JNICALL
Java_io_cpunk_dna_domain_P2PTransport_nativeRefreshPresence(
    JNIEnv* env,
    jobject obj,
    jlong messengerCtxPtr
) {
    messenger_context_t* ctx = reinterpret_cast<messenger_context_t*>(messengerCtxPtr);
    if (!ctx) {
        return -1;
    }

    int result = messenger_p2p_refresh_presence(ctx);

    if (result == 0) {
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "Presence refreshed");
    }

    return result;
}

// ============================================================================
// Offline Message Queue
// ============================================================================

/**
 * Check for offline messages in DHT
 *
 * Retrieves messages that were queued while user was offline.
 * Should be called periodically (every 2 minutes).
 *
 * @param messengerCtxPtr Messenger context pointer (long)
 * @return Number of messages retrieved, or -1 on error
 */
extern "C" JNIEXPORT jint JNICALL
Java_io_cpunk_dna_domain_P2PTransport_nativeCheckOfflineMessages(
    JNIEnv* env,
    jobject obj,
    jlong messengerCtxPtr
) {
    messenger_context_t* ctx = reinterpret_cast<messenger_context_t*>(messengerCtxPtr);
    if (!ctx) {
        return -1;
    }

    size_t messages_received = 0;
    int result = messenger_p2p_check_offline_messages(ctx, &messages_received);

    if (result == 0 && messages_received > 0) {
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG,
                           "Retrieved %zu offline messages", messages_received);
    }

    return (result == 0) ? static_cast<jint>(messages_received) : -1;
}
