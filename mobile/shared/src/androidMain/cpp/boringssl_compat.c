/**
 * BoringSSL Compatibility Shim for DNA Messenger
 *
 * Provides missing OpenSSL BIO/EVP API functions not in BoringSSL
 */

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/base64.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// EVP_aes_256_wrap Compatibility
// ============================================================================

// BoringSSL doesn't expose EVP_aes_256_wrap, but has the underlying AES wrap functions
// We return a non-null dummy pointer (EVP_CIPHER is opaque)
const EVP_CIPHER* EVP_aes_256_wrap(void) {
    // Return a magic value that indicates AES-256-KW mode
    // The DNA code just needs a non-null value to detect wrap mode
    return (const EVP_CIPHER*)0x01;  // Dummy non-null pointer
}

// ============================================================================
// BIO_f_base64 Compatibility
// ============================================================================

// BoringSSL has base64 encoding but doesn't expose BIO_f_base64
// We implement it using BoringSSL's EVP_EncodeBlock/EVP_DecodeBlock

// Custom BIO methods for base64 encoding
static int base64_bio_write(BIO *bio, const char *data, int len) {
    if (!data || len <= 0) return 0;

    // Base64 encoded size: ((len + 2) / 3) * 4
    size_t out_len = ((len + 2) / 3) * 4;
    char *encoded = malloc(out_len + 1);
    if (!encoded) return -1;

    // Encode using BoringSSL's EVP_EncodeBlock
    size_t actual = EVP_EncodeBlock((uint8_t*)encoded, (const uint8_t*)data, len);
    encoded[actual] = '\0';

    // Write to next BIO in chain
    BIO *next = BIO_next(bio);
    int written = next ? BIO_write(next, encoded, actual) : actual;

    free(encoded);
    return (written == actual) ? len : -1;
}

static int base64_bio_read(BIO *bio, char *data, int len) {
    if (!data || len <= 0) return 0;

    // Read base64 from next BIO
    BIO *next = BIO_next(bio);
    if (!next) return -1;

    char *encoded = malloc(len * 2);  // Base64 is larger than binary
    if (!encoded) return -1;

    int read_len = BIO_read(next, encoded, len * 2);
    if (read_len <= 0) {
        free(encoded);
        return read_len;
    }

    // Decode using BoringSSL's EVP_DecodeBlock
    size_t decoded_len;
    if (!EVP_DecodeBase64((uint8_t*)data, &decoded_len, len, (const uint8_t*)encoded, read_len)) {
        free(encoded);
        return -1;
    }

    free(encoded);
    return decoded_len;
}

static long base64_bio_ctrl(BIO *bio, int cmd, long num, void *ptr) {
    BIO *next = BIO_next(bio);
    if (!next) return 0;
    return BIO_ctrl(next, cmd, num, ptr);
}

static int base64_bio_create(BIO *bio) {
    return 1;
}

static int base64_bio_destroy(BIO *bio) {
    return 1;
}

// Global BIO_METHOD for base64
static BIO_METHOD *base64_method = NULL;

const BIO_METHOD* BIO_f_base64(void) {
    if (!base64_method) {
        // Create custom BIO_METHOD
        base64_method = BIO_meth_new(BIO_TYPE_BASE64, "base64");
        if (base64_method) {
            BIO_meth_set_write(base64_method, base64_bio_write);
            BIO_meth_set_read(base64_method, base64_bio_read);
            BIO_meth_set_ctrl(base64_method, base64_bio_ctrl);
            BIO_meth_set_create(base64_method, base64_bio_create);
            BIO_meth_set_destroy(base64_method, base64_bio_destroy);
        }
    }
    return base64_method;
}

// Android's bionic libc has getrandom in newer versions,
// but for compatibility with API 26, we use /dev/urandom
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

ssize_t getrandom(void *buf, size_t buflen, unsigned int flags) {
    // Fallback implementation for older Android
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    ssize_t result = read(fd, buf, buflen);
    close(fd);
    return result;
}

// ============================================================================
// Android API Compatibility Shims for API < 30
// ============================================================================

#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <android/log.h>

#define COMPAT_LOG_TAG "AndroidCompat"

/**
 * pthread_cond_clockwait() - Wait on condition variable with clock
 *
 * This function is available in API 30+, but we need to support API 26.
 * We provide a compatibility implementation using pthread_cond_timedwait.
 */
int pthread_cond_clockwait(pthread_cond_t *cond,
                           pthread_mutex_t *mutex,
                           clockid_t clock_id,
                           const struct timespec *abstime) {
    // For API < 30, fall back to pthread_cond_timedwait
    // This uses CLOCK_REALTIME, which may not be exactly what the caller wants,
    // but it's the best we can do on older Android
    __android_log_print(ANDROID_LOG_DEBUG, COMPAT_LOG_TAG,
                       "pthread_cond_clockwait compat shim called (clock_id=%d)", clock_id);

    return pthread_cond_timedwait(cond, mutex, abstime);
}

/**
 * aligned_alloc() - Allocate aligned memory
 *
 * This function is available in API 28+, but we need to support API 26.
 * We provide a compatibility implementation using posix_memalign.
 */
void* aligned_alloc(size_t alignment, size_t size) {
    void *ptr = NULL;

    // posix_memalign is available on older Android
    int result = posix_memalign(&ptr, alignment, size);

    if (result != 0) {
        errno = result;
        return NULL;
    }

    return ptr;
}
