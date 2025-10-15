/*
 * WASM-specific utility functions
 *
 * Minimal implementation for WebAssembly environment
 * Only includes functions needed for DNA context creation
 */

#include "../../qgp_platform.h"
#include <stdbool.h>

/**
 * Get home directory (stub for WASM)
 * Returns "/" since WASM doesn't have real filesystem access
 */
char* get_home_dir(void) {
    return (char*)qgp_platform_home_dir();
}

/**
 * Build file path (stub for WASM)
 * Returns allocated path string
 */
char* build_path(const char *dir, const char *filename) {
    return qgp_platform_join_path(dir, filename);
}

/**
 * Check if file exists (stub for WASM)
 * Always returns false in WASM environment
 */
bool file_exists(const char *path) {
    return qgp_platform_file_exists(path);
}
