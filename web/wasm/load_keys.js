/**
 * QGP/DNA Key File Loader
 *
 * Parses .pqkey files to extract raw cryptographic key bytes
 *
 * File Format: [header (276 bytes) | public_key | private_key]
 *
 * Header Structure (276 bytes):
 *   - magic[8]: "PQSIGNUM"
 *   - version (uint8): 1
 *   - key_type (uint8): 1=Dilithium3, 2=Kyber512
 *   - purpose (uint8): 1=signing, 2=encryption
 *   - reserved (uint8): 0
 *   - public_key_size (uint32): bytes
 *   - private_key_size (uint32): bytes
 *   - name[256]: key name
 */

const fs = require('fs');

class QGPKeyLoader {
    /**
     * Load a .pqkey file and extract key material
     * @param {string} path - Path to .pqkey file
     * @return {Object} { publicKey: Uint8Array, privateKey: Uint8Array, keyType: string, name: string }
     */
    static loadPrivateKey(path) {
        // Read entire file
        const fileData = fs.readFileSync(path);

        // Parse header (276 bytes)
        const magic = fileData.slice(0, 8).toString('ascii');
        const version = fileData.readUInt8(8);
        const keyType = fileData.readUInt8(9);
        const purpose = fileData.readUInt8(10);
        const reserved = fileData.readUInt8(11);
        const publicKeySize = fileData.readUInt32LE(12);
        const privateKeySize = fileData.readUInt32LE(16);

        // Extract name (null-terminated string from bytes 20-275)
        let nameEnd = 20;
        while (nameEnd < 276 && fileData[nameEnd] !== 0) {
            nameEnd++;
        }
        const name = fileData.slice(20, nameEnd).toString('ascii');

        // Validate magic
        if (magic !== 'PQSIGNUM') {
            throw new Error(`Invalid magic: ${magic} (expected PQSIGNUM)`);
        }

        // Validate version
        if (version !== 1) {
            throw new Error(`Unsupported version: ${version}`);
        }

        // Parse key type
        const keyTypeNames = {
            1: 'Dilithium3',
            2: 'Kyber512'
        };
        const keyTypeName = keyTypeNames[keyType] || 'Unknown';

        const purposeNames = {
            1: 'signing',
            2: 'encryption'
        };
        const purposeName = purposeNames[purpose] || 'unknown';

        // Extract public key (starts at byte 276)
        const publicKeyStart = 276;
        const publicKeyEnd = publicKeyStart + publicKeySize;
        const publicKey = new Uint8Array(fileData.slice(publicKeyStart, publicKeyEnd));

        // Extract private key (follows public key)
        const privateKeyStart = publicKeyEnd;
        const privateKeyEnd = privateKeyStart + privateKeySize;
        const privateKey = new Uint8Array(fileData.slice(privateKeyStart, privateKeyEnd));

        // Validate sizes
        if (publicKey.length !== publicKeySize) {
            throw new Error(`Public key size mismatch: got ${publicKey.length}, expected ${publicKeySize}`);
        }
        if (privateKey.length !== privateKeySize) {
            throw new Error(`Private key size mismatch: got ${privateKey.length}, expected ${privateKeySize}`);
        }

        return {
            publicKey,
            privateKey,
            keyType: keyTypeName,
            purpose: purposeName,
            name,
            fileSize: fileData.length
        };
    }

    /**
     * Load Alice's keys (sender)
     * @param {string} keyringPath - Path to keyring directory (default: ~/.qgp)
     * @return {Object} { signPubKey, signPrivKey }
     */
    static loadAliceKeys(keyringPath = null) {
        if (!keyringPath) {
            keyringPath = `${process.env.HOME}/.qgp`;
        }

        const signingKeyPath = `${keyringPath}/alice-dilithium3.pqkey`;
        const signingKey = this.loadPrivateKey(signingKeyPath);

        if (signingKey.keyType !== 'Dilithium3') {
            throw new Error(`Expected Dilithium3 key, got ${signingKey.keyType}`);
        }

        return {
            signPubKey: signingKey.publicKey,
            signPrivKey: signingKey.privateKey
        };
    }

    /**
     * Load Bob's keys (recipient)
     * @param {string} keyringPath - Path to keyring directory (default: ~/.qgp)
     * @return {Object} { encPubKey, encPrivKey }
     */
    static loadBobKeys(keyringPath = null) {
        if (!keyringPath) {
            keyringPath = `${process.env.HOME}/.qgp`;
        }

        const encryptionKeyPath = `${keyringPath}/bob-kyber512.pqkey`;
        const encryptionKey = this.loadPrivateKey(encryptionKeyPath);

        if (encryptionKey.keyType !== 'Kyber512') {
            throw new Error(`Expected Kyber512 key, got ${encryptionKey.keyType}`);
        }

        return {
            encPubKey: encryptionKey.publicKey,
            encPrivKey: encryptionKey.privateKey
        };
    }
}

module.exports = QGPKeyLoader;
