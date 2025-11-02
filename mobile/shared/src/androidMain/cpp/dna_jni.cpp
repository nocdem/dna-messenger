/**
 * dna_jni.cpp - JNI Implementation for DNA Messenger
 *
 * Bridges Kotlin code to C cryptographic library.
 */

#include "dna_jni.h"
#include "jni_utils.h"
#include <cstdlib>
#include <cstring>

// C library headers - need extern "C" linkage
extern "C" {
#include "dna_api.h"
#include "kem.h"                    // Kyber512: crypto_kem_keypair()
#include "api.h"                    // Dilithium3: pqcrystals_dilithium3_ref_keypair()
}

/**
 * Initialize DNA context
 */
JNIEXPORT jlong JNICALL
Java_io_cpunk_dna_domain_DNAMessenger_nativeInit(JNIEnv* env, jobject obj) {
    LOGI("nativeInit: Creating DNA context");

    dna_context_t* ctx = dna_context_new();
    if (ctx == nullptr) {
        LOGE("nativeInit: Failed to create DNA context");
        throwException(env, "java/lang/RuntimeException", "Failed to create DNA context");
        return 0;
    }

    LOGI("nativeInit: DNA context created successfully: %p", ctx);
    return reinterpret_cast<jlong>(ctx);
}

/**
 * Free DNA context
 */
JNIEXPORT void JNICALL
Java_io_cpunk_dna_domain_DNAMessenger_nativeFree(JNIEnv* env, jobject obj, jlong contextPtr) {
    LOGI("nativeFree: Freeing DNA context: %p", reinterpret_cast<void*>(contextPtr));

    if (contextPtr == 0) {
        LOGW("nativeFree: Context pointer is null, nothing to free");
        return;
    }

    dna_context_t* ctx = reinterpret_cast<dna_context_t*>(contextPtr);
    dna_context_free(ctx);

    LOGI("nativeFree: DNA context freed");
}

/**
 * Generate Kyber512 encryption keypair
 */
JNIEXPORT jobject JNICALL
Java_io_cpunk_dna_domain_DNAMessenger_nativeGenerateEncryptionKeyPair(JNIEnv* env, jobject obj, jlong contextPtr) {
    LOGI("nativeGenerateEncryptionKeyPair: Generating Kyber512 keypair");

    if (contextPtr == 0) {
        throwException(env, "java/lang/IllegalArgumentException", "Context pointer is null");
        return nullptr;
    }

    dna_context_t* ctx = reinterpret_cast<dna_context_t*>(contextPtr);

    // Kyber512 key sizes
    const size_t PK_SIZE = 800;
    const size_t SK_SIZE = 1632;

    uint8_t* pk = (uint8_t*)malloc(PK_SIZE);
    uint8_t* sk = (uint8_t*)malloc(SK_SIZE);

    if (pk == nullptr || sk == nullptr) {
        LOGE("nativeGenerateEncryptionKeyPair: Memory allocation failed");
        free(pk);
        free(sk);
        throwException(env, "java/lang/OutOfMemoryError", "Failed to allocate memory for keypair");
        return nullptr;
    }

    // Generate keypair (using kyber512 library directly)
    // Note: dna_api.h doesn't expose keygen yet, so we'll use kyber512 directly
    int result = crypto_kem_keypair(pk, sk);

    if (result != 0) {
        LOGE("nativeGenerateEncryptionKeyPair: Kyber512 keygen failed: %d", result);
        free(pk);
        free(sk);
        throwException(env, "java/lang/RuntimeException", "Failed to generate Kyber512 keypair");
        return nullptr;
    }

    // Convert to Java byte arrays
    jbyteArray jpk = bytesToJbyteArray(env, pk, PK_SIZE);
    jbyteArray jsk = bytesToJbyteArray(env, sk, SK_SIZE);

    // Clean up C arrays
    memset(sk, 0, SK_SIZE);  // Securely wipe private key
    free(pk);
    free(sk);

    if (jpk == nullptr || jsk == nullptr) {
        LOGE("nativeGenerateEncryptionKeyPair: Failed to convert keys to Java arrays");
        return nullptr;
    }

    // Create Kotlin Pair object: Pair(publicKey, privateKey)
    jclass pairClass = env->FindClass("kotlin/Pair");
    if (pairClass == nullptr) {
        LOGE("nativeGenerateEncryptionKeyPair: Failed to find kotlin.Pair class");
        return nullptr;
    }

    jmethodID pairConstructor = env->GetMethodID(pairClass, "<init>", "(Ljava/lang/Object;Ljava/lang/Object;)V");
    if (pairConstructor == nullptr) {
        LOGE("nativeGenerateEncryptionKeyPair: Failed to find Pair constructor");
        return nullptr;
    }

    jobject pairObj = env->NewObject(pairClass, pairConstructor, jpk, jsk);
    if (pairObj == nullptr) {
        LOGE("nativeGenerateEncryptionKeyPair: Failed to create Pair object");
        return nullptr;
    }

    LOGI("nativeGenerateEncryptionKeyPair: Kyber512 keypair generated successfully (pk=%zu bytes, sk=%zu bytes)", PK_SIZE, SK_SIZE);
    return pairObj;
}

/**
 * Generate Dilithium3 signing keypair
 */
JNIEXPORT jobject JNICALL
Java_io_cpunk_dna_domain_DNAMessenger_nativeGenerateSigningKeyPair(JNIEnv* env, jobject obj, jlong contextPtr) {
    LOGI("nativeGenerateSigningKeyPair: Generating Dilithium3 keypair");

    if (contextPtr == 0) {
        throwException(env, "java/lang/IllegalArgumentException", "Context pointer is null");
        return nullptr;
    }

    dna_context_t* ctx = reinterpret_cast<dna_context_t*>(contextPtr);

    // Dilithium3 key sizes
    const size_t PK_SIZE = 1952;
    const size_t SK_SIZE = 4032;

    uint8_t* pk = (uint8_t*)malloc(PK_SIZE);
    uint8_t* sk = (uint8_t*)malloc(SK_SIZE);

    if (pk == nullptr || sk == nullptr) {
        LOGE("nativeGenerateSigningKeyPair: Memory allocation failed");
        free(pk);
        free(sk);
        throwException(env, "java/lang/OutOfMemoryError", "Failed to allocate memory for keypair");
        return nullptr;
    }

    // Generate keypair (using dilithium library directly)
    int result = pqcrystals_dilithium3_ref_keypair(pk, sk);

    if (result != 0) {
        LOGE("nativeGenerateSigningKeyPair: Dilithium3 keygen failed: %d", result);
        free(pk);
        free(sk);
        throwException(env, "java/lang/RuntimeException", "Failed to generate Dilithium3 keypair");
        return nullptr;
    }

    // Convert to Java byte arrays
    jbyteArray jpk = bytesToJbyteArray(env, pk, PK_SIZE);
    jbyteArray jsk = bytesToJbyteArray(env, sk, SK_SIZE);

    // Clean up C arrays
    memset(sk, 0, SK_SIZE);  // Securely wipe private key
    free(pk);
    free(sk);

    if (jpk == nullptr || jsk == nullptr) {
        LOGE("nativeGenerateSigningKeyPair: Failed to convert keys to Java arrays");
        return nullptr;
    }

    // Create Kotlin Pair object
    jclass pairClass = env->FindClass("kotlin/Pair");
    if (pairClass == nullptr) {
        LOGE("nativeGenerateSigningKeyPair: Failed to find kotlin.Pair class");
        return nullptr;
    }

    jmethodID pairConstructor = env->GetMethodID(pairClass, "<init>", "(Ljava/lang/Object;Ljava/lang/Object;)V");
    if (pairConstructor == nullptr) {
        LOGE("nativeGenerateSigningKeyPair: Failed to find Pair constructor");
        return nullptr;
    }

    jobject pairObj = env->NewObject(pairClass, pairConstructor, jpk, jsk);
    if (pairObj == nullptr) {
        LOGE("nativeGenerateSigningKeyPair: Failed to create Pair object");
        return nullptr;
    }

    LOGI("nativeGenerateSigningKeyPair: Dilithium3 keypair generated successfully (pk=%zu bytes, sk=%zu bytes)", PK_SIZE, SK_SIZE);
    return pairObj;
}

/**
 * Encrypt message
 */
JNIEXPORT jbyteArray JNICALL
Java_io_cpunk_dna_domain_DNAMessenger_nativeEncrypt(
    JNIEnv* env, jobject obj, jlong contextPtr,
    jbyteArray plaintext,
    jbyteArray recipientEncPubKey,
    jbyteArray senderSignPubKey,
    jbyteArray senderSignPrivKey
) {
    LOGI("nativeEncrypt: Encrypting message");

    if (contextPtr == 0) {
        throwException(env, "java/lang/IllegalArgumentException", "Context pointer is null");
        return nullptr;
    }

    dna_context_t* ctx = reinterpret_cast<dna_context_t*>(contextPtr);

    // Convert Java arrays to C arrays
    size_t plaintext_len = 0;
    size_t recipient_pk_len = 0;
    size_t sender_sign_pk_len = 0;
    size_t sender_sign_sk_len = 0;

    uint8_t* plaintext_c = jbyteArrayToBytes(env, plaintext, &plaintext_len);
    uint8_t* recipient_pk = jbyteArrayToBytes(env, recipientEncPubKey, &recipient_pk_len);
    uint8_t* sender_sign_pk = jbyteArrayToBytes(env, senderSignPubKey, &sender_sign_pk_len);
    uint8_t* sender_sign_sk = jbyteArrayToBytes(env, senderSignPrivKey, &sender_sign_sk_len);

    if (plaintext_c == nullptr || recipient_pk == nullptr ||
        sender_sign_pk == nullptr || sender_sign_sk == nullptr) {
        LOGE("nativeEncrypt: Failed to convert input arrays");
        free(plaintext_c);
        free(recipient_pk);
        free(sender_sign_pk);
        memset(sender_sign_sk, 0, sender_sign_sk_len);
        free(sender_sign_sk);
        throwException(env, "java/lang/IllegalArgumentException", "Invalid input arrays");
        return nullptr;
    }

    LOGI("nativeEncrypt: plaintext=%zu bytes, recipient_pk=%zu bytes, sender_sign_pk=%zu bytes, sender_sign_sk=%zu bytes",
         plaintext_len, recipient_pk_len, sender_sign_pk_len, sender_sign_sk_len);

    // Call DNA encryption function
    uint8_t* ciphertext = nullptr;
    size_t ciphertext_len = 0;

    dna_error_t result = dna_encrypt_message_raw(
        ctx,
        plaintext_c, plaintext_len,
        recipient_pk,
        sender_sign_pk,
        sender_sign_sk,
        &ciphertext, &ciphertext_len
    );

    // Clean up input arrays
    free(plaintext_c);
    free(recipient_pk);
    free(sender_sign_pk);
    memset(sender_sign_sk, 0, sender_sign_sk_len);  // Securely wipe private key
    free(sender_sign_sk);

    if (result != DNA_OK) {
        LOGE("nativeEncrypt: Encryption failed: %d", result);
        throwDNAException(env, result, "Encryption failed");
        return nullptr;
    }

    // Convert ciphertext to Java array
    jbyteArray jciphertext = bytesToJbyteArray(env, ciphertext, ciphertext_len);
    free(ciphertext);

    if (jciphertext == nullptr) {
        LOGE("nativeEncrypt: Failed to convert ciphertext to Java array");
        return nullptr;
    }

    LOGI("nativeEncrypt: Message encrypted successfully (%zu bytes)", ciphertext_len);
    return jciphertext;
}

/**
 * Decrypt message
 */
JNIEXPORT jobject JNICALL
Java_io_cpunk_dna_domain_DNAMessenger_nativeDecrypt(
    JNIEnv* env, jobject obj, jlong contextPtr,
    jbyteArray ciphertext,
    jbyteArray recipientEncPrivKey
) {
    LOGI("nativeDecrypt: Decrypting message");

    if (contextPtr == 0) {
        throwException(env, "java/lang/IllegalArgumentException", "Context pointer is null");
        return nullptr;
    }

    dna_context_t* ctx = reinterpret_cast<dna_context_t*>(contextPtr);

    // Convert Java arrays to C arrays
    size_t ciphertext_len = 0;
    size_t recipient_sk_len = 0;

    uint8_t* ciphertext_c = jbyteArrayToBytes(env, ciphertext, &ciphertext_len);
    uint8_t* recipient_sk = jbyteArrayToBytes(env, recipientEncPrivKey, &recipient_sk_len);

    if (ciphertext_c == nullptr || recipient_sk == nullptr) {
        LOGE("nativeDecrypt: Failed to convert input arrays");
        free(ciphertext_c);
        memset(recipient_sk, 0, recipient_sk_len);
        free(recipient_sk);
        throwException(env, "java/lang/IllegalArgumentException", "Invalid input arrays");
        return nullptr;
    }

    LOGI("nativeDecrypt: ciphertext=%zu bytes, recipient_sk=%zu bytes", ciphertext_len, recipient_sk_len);

    // Call DNA decryption function
    uint8_t* plaintext = nullptr;
    size_t plaintext_len = 0;
    uint8_t* sender_sign_pk = nullptr;
    size_t sender_sign_pk_len = 0;

    dna_error_t result = dna_decrypt_message_raw(
        ctx,
        ciphertext_c, ciphertext_len,
        recipient_sk,
        &plaintext, &plaintext_len,
        &sender_sign_pk, &sender_sign_pk_len
    );

    // Clean up input arrays
    free(ciphertext_c);
    memset(recipient_sk, 0, recipient_sk_len);  // Securely wipe private key
    free(recipient_sk);

    if (result != DNA_OK) {
        LOGE("nativeDecrypt: Decryption failed: %d", result);
        throwDNAException(env, result, "Decryption failed");
        return nullptr;
    }

    // Convert to Java arrays
    jbyteArray jplaintext = bytesToJbyteArray(env, plaintext, plaintext_len);
    jbyteArray jsender_pk = bytesToJbyteArray(env, sender_sign_pk, sender_sign_pk_len);

    free(plaintext);
    free(sender_sign_pk);

    if (jplaintext == nullptr || jsender_pk == nullptr) {
        LOGE("nativeDecrypt: Failed to convert output arrays to Java");
        return nullptr;
    }

    // Create Kotlin Pair object: Pair(plaintext, senderSignPubKey)
    jclass pairClass = env->FindClass("kotlin/Pair");
    if (pairClass == nullptr) {
        LOGE("nativeDecrypt: Failed to find kotlin.Pair class");
        return nullptr;
    }

    jmethodID pairConstructor = env->GetMethodID(pairClass, "<init>", "(Ljava/lang/Object;Ljava/lang/Object;)V");
    if (pairConstructor == nullptr) {
        LOGE("nativeDecrypt: Failed to find Pair constructor");
        return nullptr;
    }

    jobject pairObj = env->NewObject(pairClass, pairConstructor, jplaintext, jsender_pk);
    if (pairObj == nullptr) {
        LOGE("nativeDecrypt: Failed to create Pair object");
        return nullptr;
    }

    LOGI("nativeDecrypt: Message decrypted successfully (plaintext=%zu bytes, sender_pk=%zu bytes)",
         plaintext_len, sender_sign_pk_len);
    return pairObj;
}

/**
 * Get library version
 */
JNIEXPORT jstring JNICALL
Java_io_cpunk_dna_domain_DNAMessenger_nativeGetVersion(JNIEnv* env, jobject obj) {
    const char* version = dna_version();
    LOGI("nativeGetVersion: %s", version);
    return stringToJstring(env, version);
}
