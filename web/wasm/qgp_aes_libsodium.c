/*
 * QGP AES-256-GCM Encryption (AEAD) - libsodium Implementation
 *
 * WebAssembly-compatible implementation using libsodium instead of OpenSSL.
 * Maintains API compatibility with qgp_aes.c for encrypted data format.
 */

#include "../../qgp_aes.h"
#include "../../qgp_random.h"
#include <stdio.h>
#include <string.h>
#include <sodium.h>

/**
 * Calculate required buffer size for AES-256-GCM encryption
 *
 * GCM is a stream cipher - no padding required.
 * Ciphertext is exact same size as plaintext.
 */
size_t qgp_aes256_encrypt_size(size_t plaintext_len) {
    // GCM: exact plaintext length (no padding)
    // Nonce (12 bytes) and tag (16 bytes) stored separately
    return plaintext_len;
}

/**
 * Encrypt data with AES-256-GCM using libsodium
 *
 * Uses libsodium crypto_aead_aes256gcm_* API for AES-256-GCM with AAD support.
 * Generates random 12-byte nonce and 16-byte authentication tag.
 *
 * Note: libsodium concatenates tag to end of ciphertext, so we split them
 * to maintain API compatibility with OpenSSL version.
 */
int qgp_aes256_encrypt(const uint8_t *key,
                       const uint8_t *plaintext, size_t plaintext_len,
                       const uint8_t *aad, size_t aad_len,
                       uint8_t *ciphertext, size_t *ciphertext_len,
                       uint8_t *nonce,
                       uint8_t *tag) {
    if (!key || !plaintext || !ciphertext || !ciphertext_len || !nonce || !tag) {
        fprintf(stderr, "qgp_aes256_encrypt: NULL parameter\n");
        return -1;
    }

    if (plaintext_len == 0) {
        fprintf(stderr, "qgp_aes256_encrypt: Empty plaintext\n");
        return -1;
    }

    // Check if AES-256-GCM is available (requires CPU support)
    if (crypto_aead_aes256gcm_is_available() == 0) {
        fprintf(stderr, "qgp_aes256_encrypt: AES-256-GCM not available (no AES-NI support)\n");
        return -1;
    }

    // Generate random 12-byte nonce (GCM standard)
    if (qgp_randombytes(nonce, crypto_aead_aes256gcm_NPUBBYTES) != 0) {
        fprintf(stderr, "qgp_aes256_encrypt: Failed to generate nonce\n");
        return -1;
    }

    // Allocate buffer for ciphertext + tag (libsodium concatenates them)
    size_t combined_len = plaintext_len + crypto_aead_aes256gcm_ABYTES;
    unsigned char *combined = malloc(combined_len);
    if (!combined) {
        fprintf(stderr, "qgp_aes256_encrypt: Memory allocation failed\n");
        return -1;
    }

    unsigned long long actual_len;
    int ret = crypto_aead_aes256gcm_encrypt(
        combined,       // output: ciphertext + tag (concatenated)
        &actual_len,    // output length
        plaintext,      // input plaintext
        plaintext_len,  // plaintext length
        aad,            // additional authenticated data
        aad_len,        // AAD length
        NULL,           // nsec (not used, always NULL)
        nonce,          // 12-byte nonce
        key             // 32-byte key
    );

    if (ret != 0) {
        fprintf(stderr, "qgp_aes256_encrypt: libsodium encryption failed\n");
        free(combined);
        return -1;
    }

    // Split combined output into ciphertext and tag
    // libsodium format: [ciphertext][tag]
    memcpy(ciphertext, combined, plaintext_len);
    memcpy(tag, combined + plaintext_len, crypto_aead_aes256gcm_ABYTES);
    *ciphertext_len = plaintext_len;

    // Securely wipe temporary buffer
    sodium_memzero(combined, combined_len);
    free(combined);

    return 0;
}

/**
 * Decrypt data with AES-256-GCM using libsodium
 *
 * Uses libsodium crypto_aead_aes256gcm_* API for AES-256-GCM with AAD verification.
 * Verifies authentication tag before returning plaintext.
 * Returns error if tag verification fails (tampered data).
 *
 * Note: libsodium expects tag concatenated to ciphertext, so we combine them
 * to maintain API compatibility with OpenSSL version.
 */
int qgp_aes256_decrypt(const uint8_t *key,
                       const uint8_t *ciphertext, size_t ciphertext_len,
                       const uint8_t *aad, size_t aad_len,
                       const uint8_t *nonce,
                       const uint8_t *tag,
                       uint8_t *plaintext, size_t *plaintext_len) {
    if (!key || !ciphertext || !nonce || !tag || !plaintext || !plaintext_len) {
        fprintf(stderr, "qgp_aes256_decrypt: NULL parameter\n");
        return -1;
    }

    if (ciphertext_len == 0) {
        fprintf(stderr, "qgp_aes256_decrypt: Empty ciphertext\n");
        return -1;
    }

    // Check if AES-256-GCM is available (requires CPU support)
    if (crypto_aead_aes256gcm_is_available() == 0) {
        fprintf(stderr, "qgp_aes256_decrypt: AES-256-GCM not available (no AES-NI support)\n");
        return -1;
    }

    // Combine ciphertext and tag for libsodium
    // libsodium format: [ciphertext][tag]
    size_t combined_len = ciphertext_len + crypto_aead_aes256gcm_ABYTES;
    unsigned char *combined = malloc(combined_len);
    if (!combined) {
        fprintf(stderr, "qgp_aes256_decrypt: Memory allocation failed\n");
        return -1;
    }

    memcpy(combined, ciphertext, ciphertext_len);
    memcpy(combined + ciphertext_len, tag, crypto_aead_aes256gcm_ABYTES);

    unsigned long long actual_len;
    int ret = crypto_aead_aes256gcm_decrypt(
        plaintext,      // output plaintext
        &actual_len,    // output length
        NULL,           // nsec (not used, always NULL)
        combined,       // input: ciphertext + tag
        combined_len,   // combined length
        aad,            // additional authenticated data
        aad_len,        // AAD length
        nonce,          // 12-byte nonce
        key             // 32-byte key
    );

    // Wipe temporary buffer
    sodium_memzero(combined, combined_len);
    free(combined);

    if (ret != 0) {
        // Authentication failed - data was tampered with
        fprintf(stderr, "qgp_aes256_decrypt: Authentication failed (tag verification failed)\n");
        fprintf(stderr, "Ciphertext or AAD has been tampered with\n");

        // Wipe any partial plaintext (it's invalid)
        sodium_memzero(plaintext, *plaintext_len);
        return -1;
    }

    *plaintext_len = actual_len;
    return 0;
}
