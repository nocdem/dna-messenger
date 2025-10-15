/*
 * QGP Platform Abstraction - WebAssembly Implementation
 *
 * Minimal implementation for WASM environment using libsodium.
 */

#include "../../qgp_platform.h"
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Initialization (WASM Implementation)
 * ============================================================================ */

/**
 * Initialize libsodium
 * MUST be called before any crypto operations
 * @return 0 on success, -1 on error
 */
int wasm_crypto_init(void) {
    if (sodium_init() < 0) {
        fprintf(stderr, "libsodium initialization failed\n");
        return -1;
    }
    return 0;
}

/* ============================================================================
 * Random Number Generation (WASM Implementation)
 * ============================================================================ */

int qgp_platform_random(uint8_t *buf, size_t len) {
    if (!buf || len == 0) {
        return -1;
    }

    // Use libsodium's randombytes which works in WASM via Emscripten
    randombytes_buf(buf, len);
    return 0;
}

/* ============================================================================
 * Directory Operations (WASM stubs - not applicable in browser)
 * ============================================================================ */

int qgp_platform_mkdir(const char *path) {
    // WASM in browser doesn't have filesystem access
    // Keys are stored in IndexedDB via JavaScript
    (void)path; // Suppress unused parameter warning
    return -1; // Not implemented
}

int qgp_platform_file_exists(const char *path) {
    (void)path;
    return 0; // Always return false in WASM
}

int qgp_platform_is_directory(const char *path) {
    (void)path;
    return 0; // Always return false in WASM
}

/* ============================================================================
 * Path Operations (WASM stubs)
 * ============================================================================ */

const char* qgp_platform_home_dir(void) {
    // No home directory concept in WASM/browser
    return "/"; // Return root as placeholder
}

char* qgp_platform_join_path(const char *dir, const char *file) {
    if (!dir || !file) {
        return NULL;
    }

    size_t dir_len = strlen(dir);
    size_t file_len = strlen(file);

    /* Check if dir already ends with '/' */
    int need_separator = (dir_len > 0 && dir[dir_len - 1] != '/') ? 1 : 0;

    /* Allocate: dir + '/' + file + '\0' */
    size_t total_len = dir_len + need_separator + file_len + 1;
    char *result = malloc(total_len);
    if (!result) {
        return NULL;
    }

    /* Copy dir */
    memcpy(result, dir, dir_len);
    size_t pos = dir_len;

    /* Add separator if needed */
    if (need_separator) {
        result[pos++] = '/';
    }

    /* Copy file */
    memcpy(result + pos, file, file_len);
    result[pos + file_len] = '\0';

    return result;
}
