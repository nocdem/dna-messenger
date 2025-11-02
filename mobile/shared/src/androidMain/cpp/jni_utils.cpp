/**
 * jni_utils.cpp - JNI Helper Functions Implementation
 */

#include "jni_utils.h"
#include "dna_api.h"
#include <cstdlib>
#include <cstring>

/**
 * Convert jbyteArray to C byte array
 */
uint8_t* jbyteArrayToBytes(JNIEnv* env, jbyteArray jarray, size_t* len_out) {
    if (jarray == nullptr) {
        LOGE("jbyteArrayToBytes: Input jarray is null");
        return nullptr;
    }

    jsize len = env->GetArrayLength(jarray);
    if (len <= 0) {
        LOGE("jbyteArrayToBytes: Invalid array length: %d", len);
        return nullptr;
    }

    uint8_t* bytes = (uint8_t*)malloc(len);
    if (bytes == nullptr) {
        LOGE("jbyteArrayToBytes: Memory allocation failed");
        return nullptr;
    }

    env->GetByteArrayRegion(jarray, 0, len, reinterpret_cast<jbyte*>(bytes));

    if (env->ExceptionCheck()) {
        LOGE("jbyteArrayToBytes: Exception during GetByteArrayRegion");
        free(bytes);
        return nullptr;
    }

    *len_out = (size_t)len;
    LOGD("jbyteArrayToBytes: Converted %zu bytes", *len_out);
    return bytes;
}

/**
 * Convert C byte array to jbyteArray
 */
jbyteArray bytesToJbyteArray(JNIEnv* env, const uint8_t* bytes, size_t len) {
    if (bytes == nullptr || len == 0) {
        LOGE("bytesToJbyteArray: Invalid input (bytes=%p, len=%zu)", bytes, len);
        return nullptr;
    }

    jbyteArray jarray = env->NewByteArray((jsize)len);
    if (jarray == nullptr) {
        LOGE("bytesToJbyteArray: Failed to allocate jbyteArray of size %zu", len);
        return nullptr;
    }

    env->SetByteArrayRegion(jarray, 0, (jsize)len, reinterpret_cast<const jbyte*>(bytes));

    if (env->ExceptionCheck()) {
        LOGE("bytesToJbyteArray: Exception during SetByteArrayRegion");
        return nullptr;
    }

    LOGD("bytesToJbyteArray: Converted %zu bytes", len);
    return jarray;
}

/**
 * Convert jstring to C string
 */
const char* jstringToString(JNIEnv* env, jstring jstr) {
    if (jstr == nullptr) {
        LOGE("jstringToString: Input jstring is null");
        return nullptr;
    }

    const char* str = env->GetStringUTFChars(jstr, nullptr);
    if (str == nullptr) {
        LOGE("jstringToString: GetStringUTFChars failed");
        return nullptr;
    }

    return str;
}

/**
 * Release string obtained from jstringToString
 */
void releaseString(JNIEnv* env, jstring jstr, const char* str) {
    if (jstr != nullptr && str != nullptr) {
        env->ReleaseStringUTFChars(jstr, str);
    }
}

/**
 * Convert C string to jstring
 */
jstring stringToJstring(JNIEnv* env, const char* str) {
    if (str == nullptr) {
        LOGE("stringToJstring: Input string is null");
        return nullptr;
    }

    jstring jstr = env->NewStringUTF(str);
    if (jstr == nullptr) {
        LOGE("stringToJstring: NewStringUTF failed");
        return nullptr;
    }

    return jstr;
}

/**
 * Throw Java exception
 */
void throwException(JNIEnv* env, const char* exception_class, const char* message) {
    jclass exClass = env->FindClass(exception_class);
    if (exClass == nullptr) {
        LOGE("throwException: Cannot find exception class: %s", exception_class);
        return;
    }

    env->ThrowNew(exClass, message);
    LOGE("throwException: %s: %s", exception_class, message);
}

/**
 * Throw DNA-specific exception
 */
void throwDNAException(JNIEnv* env, int error_code, const char* context) {
    char message[256];

    const char* error_str = "Unknown error";
    switch (error_code) {
        case DNA_OK:
            return; // No error
        case DNA_ERROR_INVALID_ARG:
            error_str = "Invalid argument";
            break;
        case DNA_ERROR_MEMORY:
            error_str = "Memory allocation failed";
            break;
        case DNA_ERROR_KEY_LOAD:
            error_str = "Failed to load key";
            break;
        case DNA_ERROR_KEY_INVALID:
            error_str = "Invalid key";
            break;
        case DNA_ERROR_CRYPTO:
            error_str = "Cryptographic operation failed";
            break;
        case DNA_ERROR_VERIFY:
            error_str = "Signature verification failed";
            break;
        case DNA_ERROR_DECRYPT:
            error_str = "Decryption failed";
            break;
        case DNA_ERROR_NOT_FOUND:
            error_str = "Resource not found";
            break;
        case DNA_ERROR_INTERNAL:
            error_str = "Internal error";
            break;
        default:
            error_str = "Unknown error";
            break;
    }

    snprintf(message, sizeof(message), "DNA Error %d (%s): %s",
             error_code, error_str, context ? context : "");

    throwException(env, "java/lang/RuntimeException", message);
}

/**
 * Securely wipe memory
 */
void secure_wipe(void* ptr, size_t len) {
    if (ptr == nullptr || len == 0) {
        return;
    }

    // Use volatile to prevent compiler optimization
    volatile uint8_t* p = (volatile uint8_t*)ptr;
    for (size_t i = 0; i < len; i++) {
        p[i] = 0;
    }
}
