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

    jbyteArray jarray = env->NewByteArray(len);
    if (jarray == nullptr) {
        LOGE("bytesToJbyteArray: Failed to allocate byte array of size %zu", len);
        return nullptr;
    }

    env->SetByteArrayRegion(jarray, 0, len, reinterpret_cast<const jbyte*>(bytes));

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

    LOGD("jstringToString: Converted string: %s", str);
    return str;
}

/**
 * Release string obtained from jstringToString
 */
void releaseString(JNIEnv* env, jstring jstr, const char* str) {
    if (jstr != nullptr && str != nullptr) {
        env->ReleaseStringUTFChars(jstr, str);
        LOGD("releaseString: Released string");
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

    LOGD("stringToJstring: Converted string: %s", str);
    return jstr;
}

/**
 * Throw Java exception
 */
void throwException(JNIEnv* env, const char* exception_class, const char* message) {
    jclass exClass = env->FindClass(exception_class);
    if (exClass == nullptr) {
        LOGE("throwException: Failed to find exception class: %s", exception_class);
        return;
    }

    env->ThrowNew(exClass, message);
    LOGE("throwException: %s: %s", exception_class, message);
}

/**
 * Throw DNA-specific exception
 */
void throwDNAException(JNIEnv* env, int error_code, const char* context) {
    const char* error_msg = dna_error_string(static_cast<dna_error_t>(error_code));

    char full_message[512];
    snprintf(full_message, sizeof(full_message), "%s: %s (code: %d)",
             context, error_msg, error_code);

    throwException(env, "java/lang/RuntimeException", full_message);
}
