// DNA Messenger WebAssembly Crypto Wrapper
// Provides high-level JavaScript API for post-quantum encryption
//
// Usage:
//   const crypto = new DNACrypto();
//   await crypto.init();
//   const ciphertext = await crypto.encryptMessage(plaintext, recipientPubKey, senderSignKeys);
//   const result = await crypto.decryptMessage(ciphertext, recipientPrivKey);

class DNACrypto {
    constructor() {
        this.module = null;
        this.ctx = 0;
        this.initialized = false;
    }

    /**
     * Initialize the WASM module
     * Must be called before any crypto operations
     */
    async init() {
        if (this.initialized) {
            return;
        }

        // Load WASM module (DNAModule is exported by dna_wasm.js)
        this.module = await DNAModule();

        // Initialize libsodium (MUST be called first!)
        const initResult = this.module._wasm_crypto_init();
        if (initResult !== 0) {
            throw new Error('Failed to initialize libsodium');
        }

        // Create DNA context (required for all operations)
        this.ctx = this.module._dna_context_new();
        if (this.ctx === 0) {
            throw new Error('Failed to create DNA context');
        }

        this.initialized = true;
    }

    /**
     * Cleanup resources
     * Call this when done with the crypto instance
     */
    cleanup() {
        if (this.ctx !== 0) {
            this.module._dna_context_free(this.ctx);
            this.ctx = 0;
        }
        this.initialized = false;
    }

    /**
     * Check if module is initialized
     */
    _checkInit() {
        if (!this.initialized) {
            throw new Error('DNACrypto not initialized. Call init() first.');
        }
    }

    // ========================================================================
    // MEMORY MANAGEMENT HELPERS
    // ========================================================================

    /**
     * Allocate memory in WASM heap and copy data
     * @param {Uint8Array} data - Data to copy
     * @return {Object} { ptr: number, len: number }
     */
    _allocateBytes(data) {
        const ptr = this.module._malloc(data.length);
        if (ptr === 0) {
            throw new Error('Failed to allocate WASM memory');
        }
        this.module.HEAPU8.set(data, ptr);
        return { ptr, len: data.length };
    }

    /**
     * Allocate string in WASM heap
     * @param {string} str - String to copy
     * @return {Object} { ptr: number, len: number }
     */
    _allocateString(str) {
        const bytes = new TextEncoder().encode(str);
        return this._allocateBytes(bytes);
    }

    /**
     * Read bytes from WASM heap
     * @param {number} ptr - Memory pointer
     * @param {number} len - Length in bytes
     * @return {Uint8Array} Copied data
     */
    _readBytes(ptr, len) {
        return new Uint8Array(this.module.HEAPU8.buffer, ptr, len).slice();
    }

    /**
     * Free WASM memory pointer
     * @param {number} ptr - Pointer to free
     */
    _free(ptr) {
        if (ptr !== 0) {
            this.module._free(ptr);
        }
    }

    /**
     * Allocate pointer-to-pointer (for output parameters)
     * @return {number} Pointer to pointer
     */
    _allocateOutPtr() {
        const ptrPtr = this.module._malloc(4); // 32-bit pointer
        if (ptrPtr === 0) {
            throw new Error('Failed to allocate output pointer');
        }
        this.module.setValue(ptrPtr, 0, 'i32');
        return ptrPtr;
    }

    /**
     * Allocate size_t pointer (for output length parameters)
     * @return {number} Pointer to size_t
     */
    _allocateSizePtr() {
        const sizePtr = this.module._malloc(4); // 32-bit size_t
        if (sizePtr === 0) {
            throw new Error('Failed to allocate size pointer');
        }
        this.module.setValue(sizePtr, 0, 'i32');
        return sizePtr;
    }

    /**
     * Read pointer value from pointer-to-pointer
     * @param {number} ptrPtr - Pointer to pointer
     * @return {number} Dereferenced pointer
     */
    _readPtr(ptrPtr) {
        return this.module.getValue(ptrPtr, 'i32');
    }

    /**
     * Read size_t value from pointer
     * @param {number} sizePtr - Pointer to size_t
     * @return {number} Size value
     */
    _readSize(sizePtr) {
        return this.module.getValue(sizePtr, 'i32');
    }

    // ========================================================================
    // ENCRYPTION API
    // ========================================================================

    /**
     * Encrypt message for recipient
     *
     * @param {string|Uint8Array} plaintext - Message to encrypt
     * @param {Uint8Array} recipientEncPubKey - Recipient's Kyber512 public key (800 bytes)
     * @param {Object} senderSignKeys - Sender's signing keys
     * @param {Uint8Array} senderSignKeys.pubKey - Sender's Dilithium3 public key (1952 bytes)
     * @param {Uint8Array} senderSignKeys.privKey - Sender's Dilithium3 private key (4032 bytes)
     * @return {Uint8Array} Encrypted ciphertext
     *
     * @example
     * const ciphertext = await crypto.encryptMessage(
     *   "Hello, Bob!",
     *   bobPublicKey,
     *   { pubKey: aliceSignPubKey, privKey: aliceSignPrivKey }
     * );
     */
    async encryptMessage(plaintext, recipientEncPubKey, senderSignKeys) {
        this._checkInit();

        // Convert plaintext to bytes if needed
        const plaintextBytes = typeof plaintext === 'string'
            ? new TextEncoder().encode(plaintext)
            : plaintext;

        // Validate input sizes
        if (recipientEncPubKey.length !== 800) {
            throw new Error(`Invalid recipient public key size: ${recipientEncPubKey.length} (expected 800)`);
        }
        if (senderSignKeys.pubKey.length !== 1952) {
            throw new Error(`Invalid sender public key size: ${senderSignKeys.pubKey.length} (expected 1952)`);
        }
        if (senderSignKeys.privKey.length !== 4032) {
            throw new Error(`Invalid sender private key size: ${senderSignKeys.privKey.length} (expected 4032)`);
        }

        // Allocate input buffers
        const plaintextAlloc = this._allocateBytes(plaintextBytes);
        const recipientKeyAlloc = this._allocateBytes(recipientEncPubKey);
        const senderPubKeyAlloc = this._allocateBytes(senderSignKeys.pubKey);
        const senderPrivKeyAlloc = this._allocateBytes(senderSignKeys.privKey);

        // Allocate output buffers
        const ciphertextPtrPtr = this._allocateOutPtr();
        const ciphertextLenPtr = this._allocateSizePtr();

        try {
            // Call WASM function: dna_encrypt_message_raw
            // int dna_encrypt_message_raw(
            //     dna_context_t *ctx,                      // NULL for now
            //     const uint8_t *plaintext,
            //     size_t plaintext_len,
            //     const uint8_t *recipient_enc_pubkey,
            //     const uint8_t *sender_sign_pubkey,
            //     const uint8_t *sender_sign_privkey,
            //     uint8_t **ciphertext_out,
            //     size_t *ciphertext_len_out
            // );
            const result = this.module._dna_encrypt_message_raw(
                this.ctx,                    // DNA context
                plaintextAlloc.ptr,
                plaintextAlloc.len,
                recipientKeyAlloc.ptr,
                senderPubKeyAlloc.ptr,
                senderPrivKeyAlloc.ptr,
                ciphertextPtrPtr,
                ciphertextLenPtr
            );

            if (result !== 0) {
                throw new Error(`Encryption failed with error code: ${result}`);
            }

            // Read output
            const ciphertextPtr = this._readPtr(ciphertextPtrPtr);
            const ciphertextLen = this._readSize(ciphertextLenPtr);

            if (ciphertextPtr === 0 || ciphertextLen === 0) {
                throw new Error('Encryption produced no output');
            }

            // Copy ciphertext from WASM memory
            const ciphertext = this._readBytes(ciphertextPtr, ciphertextLen);

            // Free output buffer (allocated by C code)
            this._free(ciphertextPtr);

            return ciphertext;

        } finally {
            // Free input buffers
            this._free(plaintextAlloc.ptr);
            this._free(recipientKeyAlloc.ptr);
            this._free(senderPubKeyAlloc.ptr);
            this._free(senderPrivKeyAlloc.ptr);
            this._free(ciphertextPtrPtr);
            this._free(ciphertextLenPtr);
        }
    }

    // ========================================================================
    // DECRYPTION API
    // ========================================================================

    /**
     * Decrypt message
     *
     * @param {Uint8Array} ciphertext - Encrypted message
     * @param {Uint8Array} recipientEncPrivKey - Recipient's Kyber512 private key (1632 bytes)
     * @return {Object} { plaintext: Uint8Array, senderPubKey: Uint8Array }
     *
     * @example
     * const result = await crypto.decryptMessage(ciphertext, bobPrivateKey);
     * console.log(new TextDecoder().decode(result.plaintext));
     * console.log('Sender:', result.senderPubKey);
     */
    async decryptMessage(ciphertext, recipientEncPrivKey) {
        this._checkInit();

        // Validate input sizes
        if (recipientEncPrivKey.length !== 1632) {
            throw new Error(`Invalid recipient private key size: ${recipientEncPrivKey.length} (expected 1632)`);
        }
        if (ciphertext.length === 0) {
            throw new Error('Ciphertext is empty');
        }

        // Allocate input buffers
        const ciphertextAlloc = this._allocateBytes(ciphertext);
        const recipientKeyAlloc = this._allocateBytes(recipientEncPrivKey);

        // Allocate output buffers
        const plaintextPtrPtr = this._allocateOutPtr();
        const plaintextLenPtr = this._allocateSizePtr();
        const senderPubKeyPtrPtr = this._allocateOutPtr();
        const senderPubKeyLenPtr = this._allocateSizePtr();

        try {
            // Call WASM function: dna_decrypt_message_raw
            // int dna_decrypt_message_raw(
            //     dna_context_t *ctx,                      // NULL for now
            //     const uint8_t *ciphertext,
            //     size_t ciphertext_len,
            //     const uint8_t *recipient_enc_privkey,
            //     uint8_t **plaintext_out,
            //     size_t *plaintext_len_out,
            //     uint8_t **sender_sign_pubkey_out,
            //     size_t *sender_sign_pubkey_len_out
            // );
            const result = this.module._dna_decrypt_message_raw(
                this.ctx,                    // DNA context
                ciphertextAlloc.ptr,
                ciphertextAlloc.len,
                recipientKeyAlloc.ptr,
                plaintextPtrPtr,
                plaintextLenPtr,
                senderPubKeyPtrPtr,
                senderPubKeyLenPtr
            );

            if (result !== 0) {
                throw new Error(`Decryption failed with error code: ${result}`);
            }

            // Read plaintext
            const plaintextPtr = this._readPtr(plaintextPtrPtr);
            const plaintextLen = this._readSize(plaintextLenPtr);

            if (plaintextPtr === 0 || plaintextLen === 0) {
                throw new Error('Decryption produced no plaintext output');
            }

            const plaintext = this._readBytes(plaintextPtr, plaintextLen);
            this._free(plaintextPtr);

            // Read sender public key
            const senderPubKeyPtr = this._readPtr(senderPubKeyPtrPtr);
            const senderPubKeyLen = this._readSize(senderPubKeyLenPtr);

            if (senderPubKeyPtr === 0 || senderPubKeyLen === 0) {
                throw new Error('Decryption produced no sender public key');
            }

            const senderPubKey = this._readBytes(senderPubKeyPtr, senderPubKeyLen);
            this._free(senderPubKeyPtr);

            return {
                plaintext,
                senderPubKey
            };

        } finally {
            // Free input buffers
            this._free(ciphertextAlloc.ptr);
            this._free(recipientKeyAlloc.ptr);
            this._free(plaintextPtrPtr);
            this._free(plaintextLenPtr);
            this._free(senderPubKeyPtrPtr);
            this._free(senderPubKeyLenPtr);
        }
    }

    // ========================================================================
    // UTILITY FUNCTIONS
    // ========================================================================

    /**
     * Convert bytes to hex string
     * @param {Uint8Array} bytes - Bytes to convert
     * @return {string} Hex string
     */
    bytesToHex(bytes) {
        return Array.from(bytes)
            .map(b => b.toString(16).padStart(2, '0'))
            .join('');
    }

    /**
     * Convert hex string to bytes
     * @param {string} hex - Hex string
     * @return {Uint8Array} Bytes
     */
    hexToBytes(hex) {
        const bytes = new Uint8Array(hex.length / 2);
        for (let i = 0; i < hex.length; i += 2) {
            bytes[i / 2] = parseInt(hex.substr(i, 2), 16);
        }
        return bytes;
    }

    /**
     * Get library version
     * @return {string} Version string
     */
    getVersion() {
        return '0.2.0-alpha (WASM)';
    }
}

// Export for Node.js and browser
if (typeof module !== 'undefined' && module.exports) {
    module.exports = DNACrypto;
}
if (typeof window !== 'undefined') {
    window.DNACrypto = DNACrypto;
}
