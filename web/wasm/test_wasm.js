#!/usr/bin/env node
/**
 * DNA Messenger WASM Test Script
 *
 * Tests the WebAssembly crypto module with encrypt/decrypt round-trip
 *
 * Usage:
 *   node test_wasm.js
 */

// Load WASM module wrapper (needs to be in same directory)
global.DNAModule = require('./dna_wasm.js');
const DNACrypto = require('./dna_crypto.js');
const QGPKeyLoader = require('./load_keys.js');

// ANSI color codes for pretty output
const colors = {
    reset: '\x1b[0m',
    green: '\x1b[32m',
    red: '\x1b[31m',
    yellow: '\x1b[33m',
    blue: '\x1b[34m',
    cyan: '\x1b[36m',
};

function log(message, color = 'reset') {
    console.log(`${colors[color]}${message}${colors.reset}`);
}

function logSection(title) {
    console.log('');
    log('═'.repeat(70), 'cyan');
    log(`  ${title}`, 'cyan');
    log('═'.repeat(70), 'cyan');
}

function logTest(name, status, details = '') {
    const icon = status === 'pass' ? '✓' : status === 'fail' ? '✗' : '●';
    const color = status === 'pass' ? 'green' : status === 'fail' ? 'red' : 'yellow';
    log(`${icon} ${name}`, color);
    if (details) {
        console.log(`  ${details}`);
    }
}

/**
 * Load real cryptographic keys from ~/.qgp/
 */
function loadRealKeys() {
    const alice = QGPKeyLoader.loadAliceKeys();
    const bob = QGPKeyLoader.loadBobKeys();

    return {
        alice: {
            signPubKey: alice.signPubKey,
            signPrivKey: alice.signPrivKey,
        },
        bob: {
            encPubKey: bob.encPubKey,
            encPrivKey: bob.encPrivKey,
        }
    };
}

/**
 * Run all tests
 */
async function runTests() {
    logSection('DNA Messenger WebAssembly Crypto Tests');

    const crypto = new DNACrypto();
    let testsPassed = 0;
    let testsFailed = 0;

    try {
        // Test 1: Initialize WASM module
        logSection('Test 1: WASM Module Initialization');
        try {
            await crypto.init();
            logTest('WASM module loaded', 'pass', `Version: ${crypto.getVersion()}`);
            testsPassed++;
        } catch (error) {
            logTest('WASM module failed to load', 'fail', error.message);
            testsFailed++;
            return; // Can't continue without initialization
        }

        // Test 2: Load real keys from ~/.qgp/
        logSection('Test 2: Load Real Cryptographic Keys');
        let keys;
        try {
            keys = loadRealKeys();
            logTest('Loaded real keys from ~/.qgp/', 'pass',
                `Alice (sender) sign keys: pub=${keys.alice.signPubKey.length} bytes, priv=${keys.alice.signPrivKey.length} bytes\n` +
                `  Bob (recipient) enc keys: pub=${keys.bob.encPubKey.length} bytes, priv=${keys.bob.encPrivKey.length} bytes`
            );
            testsPassed++;
        } catch (error) {
            logTest('Key loading failed', 'fail', error.message);
            testsFailed++;
            return;
        }

        // Test 3: Encrypt message
        logSection('Test 3: Message Encryption');
        const testMessage = 'Hello, Bob! This is a test message from Alice.';
        let ciphertext;
        try {
            ciphertext = await crypto.encryptMessage(
                testMessage,
                keys.bob.encPubKey,
                {
                    pubKey: keys.alice.signPubKey,
                    privKey: keys.alice.signPrivKey
                }
            );
            logTest('Message encrypted successfully', 'pass',
                `Plaintext: ${testMessage.length} bytes\n` +
                `  Ciphertext: ${ciphertext.length} bytes\n` +
                `  Overhead: ${ciphertext.length - testMessage.length} bytes`
            );
            testsPassed++;
        } catch (error) {
            logTest('Encryption failed', 'fail', error.message);
            testsFailed++;
            return;
        }

        // Test 4: Decrypt message
        logSection('Test 4: Message Decryption');
        let decryptResult;
        try {
            decryptResult = await crypto.decryptMessage(
                ciphertext,
                keys.bob.encPrivKey
            );
            const decryptedText = new TextDecoder().decode(decryptResult.plaintext);
            logTest('Message decrypted successfully', 'pass',
                `Ciphertext: ${ciphertext.length} bytes\n` +
                `  Plaintext: ${decryptResult.plaintext.length} bytes\n` +
                `  Sender public key: ${decryptResult.senderPubKey.length} bytes`
            );
            testsPassed++;
        } catch (error) {
            logTest('Decryption failed', 'fail', error.message);
            testsFailed++;
            return;
        }

        // Test 5: Verify round-trip correctness
        logSection('Test 5: Round-Trip Verification');
        try {
            const decryptedText = new TextDecoder().decode(decryptResult.plaintext);

            if (decryptedText === testMessage) {
                logTest('Plaintext matches original', 'pass',
                    `Original:  "${testMessage}"\n` +
                    `  Decrypted: "${decryptedText}"`
                );
                testsPassed++;
            } else {
                logTest('Plaintext mismatch', 'fail',
                    `Original:  "${testMessage}"\n` +
                    `  Decrypted: "${decryptedText}"`
                );
                testsFailed++;
            }
        } catch (error) {
            logTest('Verification failed', 'fail', error.message);
            testsFailed++;
        }

        // Test 6: Sender public key verification
        logSection('Test 6: Sender Public Key Verification');
        try {
            const expectedSize = 1952; // Dilithium3 public key size
            if (decryptResult.senderPubKey.length === expectedSize) {
                logTest('Sender public key size correct', 'pass',
                    `Expected: ${expectedSize} bytes, Got: ${decryptResult.senderPubKey.length} bytes`
                );
                testsPassed++;
            } else {
                logTest('Sender public key size incorrect', 'fail',
                    `Expected: ${expectedSize} bytes, Got: ${decryptResult.senderPubKey.length} bytes`
                );
                testsFailed++;
            }
        } catch (error) {
            logTest('Public key verification failed', 'fail', error.message);
            testsFailed++;
        }

        // Test 7: Binary data encryption
        logSection('Test 7: Binary Data Encryption');
        try {
            const binaryData = new Uint8Array([0x00, 0x01, 0x02, 0xFF, 0xFE, 0xFD]);
            const binaryCiphertext = await crypto.encryptMessage(
                binaryData,
                keys.bob.encPubKey,
                {
                    pubKey: keys.alice.signPubKey,
                    privKey: keys.alice.signPrivKey
                }
            );
            const binaryDecryptResult = await crypto.decryptMessage(
                binaryCiphertext,
                keys.bob.encPrivKey
            );

            // Compare byte-by-byte
            let match = binaryData.length === binaryDecryptResult.plaintext.length;
            if (match) {
                for (let i = 0; i < binaryData.length; i++) {
                    if (binaryData[i] !== binaryDecryptResult.plaintext[i]) {
                        match = false;
                        break;
                    }
                }
            }

            if (match) {
                logTest('Binary data encrypted and decrypted correctly', 'pass',
                    `Original: [${Array.from(binaryData).join(', ')}]\n` +
                    `  Decrypted: [${Array.from(binaryDecryptResult.plaintext).join(', ')}]`
                );
                testsPassed++;
            } else {
                logTest('Binary data mismatch', 'fail',
                    `Original: [${Array.from(binaryData).join(', ')}]\n` +
                    `  Decrypted: [${Array.from(binaryDecryptResult.plaintext).join(', ')}]`
                );
                testsFailed++;
            }
        } catch (error) {
            logTest('Binary encryption failed', 'fail', error.message);
            testsFailed++;
        }

        // Test 8: Error handling - invalid key sizes
        logSection('Test 8: Error Handling (Invalid Key Sizes)');
        try {
            const invalidPubKey = new Uint8Array(100); // Wrong size
            await crypto.encryptMessage(
                'test',
                invalidPubKey,
                {
                    pubKey: keys.alice.signPubKey,
                    privKey: keys.alice.signPrivKey
                }
            );
            logTest('Should have thrown error for invalid key', 'fail', 'No error thrown');
            testsFailed++;
        } catch (error) {
            if (error.message.includes('Invalid recipient public key size')) {
                logTest('Correctly rejected invalid key size', 'pass', error.message);
                testsPassed++;
            } else {
                logTest('Unexpected error', 'fail', error.message);
                testsFailed++;
            }
        }

    } catch (error) {
        log(`\n\nFatal error: ${error.message}`, 'red');
        console.error(error.stack);
        testsFailed++;
    }

    // Summary
    logSection('Test Summary');
    const total = testsPassed + testsFailed;
    const percentage = total > 0 ? Math.round((testsPassed / total) * 100) : 0;

    log(`Total tests: ${total}`, 'cyan');
    log(`Passed: ${testsPassed}`, 'green');
    log(`Failed: ${testsFailed}`, testsFailed > 0 ? 'red' : 'green');
    log(`Success rate: ${percentage}%`, percentage === 100 ? 'green' : 'yellow');

    if (testsPassed === total) {
        log('\n🎉 All tests passed!', 'green');
        process.exit(0);
    } else {
        log('\n❌ Some tests failed', 'red');
        process.exit(1);
    }
}

// Run tests
if (require.main === module) {
    runTests().catch(error => {
        console.error('Test runner failed:', error);
        process.exit(1);
    });
}
