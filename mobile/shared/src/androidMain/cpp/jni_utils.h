/**
 * jni_utils.h - JNI Helper Functions for DNA Messenger
 *
 * Provides utility functions for converting between JNI types and C types.
 * Handles memory management and error checking.
 */

#ifndef DNA_JNI_UTILS_H
#define DNA_JNI_UTILS_H

#include <jni.h>
#include <stdint.h>
#include <stddef.h>
#include <android/log.h>

// Logging macros
#define LOG_TAG "DNAMessenger"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert jbyteArray to C byte array
 *
 * @param env JNI environment
 * @param jarray Java byte array
 * @param len_out Output length
 * @return Allocated C byte array (caller must free), or NULL on error
 */
uint8_t* jbyteArrayToBytes(JNIEnv* env, jbyteArray jarray, size_t* len_out);

/**
 * Convert C byte array to jbyteArray
 *
 * @param env JNI environment
 * @param bytes C byte array
 * @param len Length of array
 * @return Java byte array, or NULL on error
 */
jbyteArray bytesToJbyteArray(JNIEnv* env, const uint8_t* bytes, size_t len);

/**
 * Convert jstring to C string
 *
 * @param env JNI environment
 * @param jstr Java string
 * @return Allocated C string (caller must free with releaseString), or NULL on error
 */
const char* jstringToString(JNIEnv* env, jstring jstr);

/**
 * Release string obtained from jstringToString
 *
 * @param env JNI environment
 * @param jstr Original Java string
 * @param str C string to release
 */
void releaseString(JNIEnv* env, jstring jstr, const char* str);

/**
 * Convert C string to jstring
 *
 * @param env JNI environment
 * @param str C string
 * @return Java string, or NULL on error
 */
jstring stringToJstring(JNIEnv* env, const char* str);

/**
 * Throw Java exception
 *
 * @param env JNI environment
 * @param exception_class Exception class name (e.g., "java/lang/RuntimeException")
 * @param message Error message
 */
void throwException(JNIEnv* env, const char* exception_class, const char* message);

/**
 * Throw DNA-specific exception
 *
 * @param env JNI environment
 * @param error_code DNA error code
 * @param context Additional context message
 */
void throwDNAException(JNIEnv* env, int error_code, const char* context);

#ifdef __cplusplus
}
#endif

#endif // DNA_JNI_UTILS_H
