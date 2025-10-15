# DNA Messenger - Web Development Plan

**Branch:** `feature/web-messenger`
**Version:** 0.2.0-alpha
**Status:** Planning Complete - Ready for Implementation
**Timeline:** 6-8 weeks (full-time development)

---

## Executive Summary

Comprehensive development plan for building a web-based messenger client that maintains full post-quantum security while providing real-time browser-based messaging.

**Core Technologies:**
- **Backend:** Node.js 18 + Express + Socket.IO
- **Frontend:** React 18 + WebAssembly
- **Database:** PostgreSQL (existing schema)
- **Crypto:** Kyber512 + Dilithium3 (compiled to WASM)

**Security Guarantee:** End-to-end post-quantum encryption maintained at application layer.

---

## System Requirements Analysis

### Current Infrastructure ✅
- ✅ Node.js v18.19.0 installed
- ✅ npm v9.2.0 installed
- ✅ PostgreSQL running (keyserver + messages tables)
- ✅ User identity exists (~/.dna/nocdem)
- ✅ C library (dna_api.c) with PQ crypto

### Required Installations
- ❌ Emscripten SDK (for WebAssembly compilation)
- ❌ nginx (for production deployment)
- ❌ Redis (optional, for session caching)

---

## Phase 1: Environment Setup (Week 1)

### 1.1 Install Emscripten SDK

**Purpose:** Compile C library to WebAssembly

```bash
# Install Emscripten
cd /opt
sudo git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
sudo ./emsdk install latest
sudo ./emsdk activate latest

# Add to PATH (for current user)
echo 'source /opt/emsdk/emsdk_env.sh' >> ~/.bashrc
source ~/.bashrc

# Verify installation
emcc --version
```

**Expected Output:**
```
emcc (Emscripten gcc/clang-like replacement + linker emulating GNU ld) 3.1.50
```

**Time Estimate:** 30 minutes (includes download)

---

### 1.2 Install Additional Dependencies

```bash
# Install build tools
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build

# Install nginx (for production)
sudo apt-get install -y nginx

# Install Redis (optional, for caching)
sudo apt-get install -y redis-server

# Verify installations
nginx -v
redis-server --version
```

**Time Estimate:** 15 minutes

---

### 1.3 Initialize Node.js Projects

**Server Setup:**
```bash
cd /opt/dna-messenger/web/server
npm init -y

# Install core dependencies
npm install express socket.io pg jsonwebtoken bcrypt cors dotenv

# Install dev dependencies
npm install -D nodemon typescript @types/node @types/express @types/socket.io

# Install security middleware
npm install helmet express-rate-limit express-validator

# Install testing tools
npm install -D jest supertest

# Verify installation
npm list --depth=0
```

**Client Setup:**
```bash
cd /opt/dna-messenger/web/client
npx create-react-app . --template typescript

# Install additional dependencies
npm install socket.io-client
npm install dompurify @types/dompurify
npm install idb  # IndexedDB wrapper

# Install UI library
npm install @mui/material @mui/icons-material @emotion/react @emotion/styled

# Install dev tools
npm install -D @testing-library/react @testing-library/jest-dom
```

**Time Estimate:** 20 minutes

---

## Phase 2: WebAssembly Crypto Module (Week 1-2)

### 2.1 Create WASM Build Script

**File:** `web/wasm/build_wasm.sh`

```bash
#!/bin/bash
set -e

echo "Building DNA Messenger WebAssembly Module..."

# Source Emscripten environment
source /opt/emsdk/emsdk_env.sh

# Output directory
OUTPUT_DIR="."
SRC_DIR="../.."

# Compile flags
EMCC_FLAGS="-O3 \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_FUNCTIONS='[\"_malloc\",\"_free\"]' \
  -s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\",\"getValue\",\"setValue\",\"HEAPU8\"]' \
  -s MODULARIZE=1 \
  -s EXPORT_NAME='DNAModule' \
  -fno-unroll-loops \
  -fno-vectorize"

# Include paths
INCLUDES="-I${SRC_DIR} \
  -I${SRC_DIR}/crypto/kyber512 \
  -I${SRC_DIR}/crypto/dilithium"

# Source files
SOURCES="${SRC_DIR}/dna_api.c \
  ${SRC_DIR}/qgp_aes.c \
  ${SRC_DIR}/qgp_kyber.c \
  ${SRC_DIR}/qgp_dilithium.c \
  ${SRC_DIR}/qgp_random.c \
  ${SRC_DIR}/qgp_key.c \
  ${SRC_DIR}/aes_keywrap.c \
  ${SRC_DIR}/armor.c \
  ${SRC_DIR}/utils.c \
  ${SRC_DIR}/crypto/kyber512/*.c \
  ${SRC_DIR}/crypto/dilithium/*.c"

# Exclude files with main() functions
EXCLUDE_PATTERN="PQCgenKAT|test_|nistkat"

# Filter sources
FILTERED_SOURCES=$(echo $SOURCES | tr ' ' '\n' | grep -v -E "$EXCLUDE_PATTERN" | tr '\n' ' ')

# Compile
echo "Compiling C sources to WebAssembly..."
emcc $EMCC_FLAGS $INCLUDES $FILTERED_SOURCES -o ${OUTPUT_DIR}/dna_wasm.js

# Check output
if [ -f "${OUTPUT_DIR}/dna_wasm.js" ] && [ -f "${OUTPUT_DIR}/dna_wasm.wasm" ]; then
    echo "✅ Build successful!"
    echo "Output files:"
    ls -lh ${OUTPUT_DIR}/dna_wasm.*
    echo ""
    echo "WASM module size: $(stat -f%z ${OUTPUT_DIR}/dna_wasm.wasm 2>/dev/null || stat -c%s ${OUTPUT_DIR}/dna_wasm.wasm) bytes"
else
    echo "❌ Build failed!"
    exit 1
fi
```

**Time Estimate:** 2 hours (includes debugging)

---

### 2.2 Create JavaScript Wrapper API

**File:** `web/wasm/dna_crypto.js`

```javascript
/**
 * DNA Messenger - WebAssembly Crypto API
 *
 * JavaScript wrapper for DNA C library compiled to WebAssembly
 */

class DNACrypto {
  constructor() {
    this.module = null;
    this.initialized = false;
  }

  /**
   * Initialize WASM module
   */
  async init() {
    if (this.initialized) return;

    const DNAModule = require('./dna_wasm.js');
    this.module = await DNAModule();
    this.initialized = true;
    console.log('✅ DNA Crypto WASM initialized');
  }

  /**
   * Encrypt message for recipient
   *
   * @param {Uint8Array} plaintext - Message to encrypt
   * @param {Uint8Array} recipientEncPubkey - Kyber512 public key (800 bytes)
   * @param {Uint8Array} senderSignPubkey - Dilithium3 public key (1952 bytes)
   * @param {Uint8Array} senderSignPrivkey - Dilithium3 private key (4032 bytes)
   * @returns {Uint8Array} Encrypted ciphertext
   */
  encryptMessage(plaintext, recipientEncPubkey, senderSignPubkey, senderSignPrivkey) {
    if (!this.initialized) throw new Error('WASM not initialized');

    // Allocate memory for inputs
    const plaintextPtr = this.module._malloc(plaintext.length);
    const recipientPubPtr = this.module._malloc(recipientEncPubkey.length);
    const senderPubPtr = this.module._malloc(senderSignPubkey.length);
    const senderPrivPtr = this.module._malloc(senderSignPrivkey.length);

    // Copy data to WASM memory
    this.module.HEAPU8.set(plaintext, plaintextPtr);
    this.module.HEAPU8.set(recipientEncPubkey, recipientPubPtr);
    this.module.HEAPU8.set(senderSignPubkey, senderPubPtr);
    this.module.HEAPU8.set(senderSignPrivkey, senderPrivPtr);

    // Allocate pointers for output
    const ciphertextPtrPtr = this.module._malloc(4);
    const ciphertextLenPtr = this.module._malloc(4);

    try {
      // Call C function: dna_encrypt_message_raw
      const result = this.module.ccall(
        'dna_encrypt_message_raw',
        'number',
        ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number'],
        [
          0, // ctx (NULL for now)
          plaintextPtr, plaintext.length,
          recipientPubPtr,
          senderPubPtr,
          senderPrivPtr,
          ciphertextPtrPtr,
          ciphertextLenPtr
        ]
      );

      if (result !== 0) {
        throw new Error(`Encryption failed with error code: ${result}`);
      }

      // Read output pointers
      const ciphertextPtr = this.module.getValue(ciphertextPtrPtr, 'i32');
      const ciphertextLen = this.module.getValue(ciphertextLenPtr, 'i32');

      // Copy ciphertext from WASM memory
      const ciphertext = new Uint8Array(ciphertextLen);
      ciphertext.set(this.module.HEAPU8.subarray(ciphertextPtr, ciphertextPtr + ciphertextLen));

      // Free C-allocated ciphertext
      this.module._free(ciphertextPtr);

      return ciphertext;
    } finally {
      // Free all allocated memory
      this.module._free(plaintextPtr);
      this.module._free(recipientPubPtr);
      this.module._free(senderPubPtr);
      this.module._free(senderPrivPtr);
      this.module._free(ciphertextPtrPtr);
      this.module._free(ciphertextLenPtr);
    }
  }

  /**
   * Decrypt message
   *
   * @param {Uint8Array} ciphertext - Encrypted message
   * @param {Uint8Array} recipientEncPrivkey - Kyber512 private key (1632 bytes)
   * @returns {Object} { plaintext: Uint8Array, senderPubkey: Uint8Array }
   */
  decryptMessage(ciphertext, recipientEncPrivkey) {
    if (!this.initialized) throw new Error('WASM not initialized');

    // Allocate memory for inputs
    const ciphertextPtr = this.module._malloc(ciphertext.length);
    const recipientPrivPtr = this.module._malloc(recipientEncPrivkey.length);

    // Copy data to WASM memory
    this.module.HEAPU8.set(ciphertext, ciphertextPtr);
    this.module.HEAPU8.set(recipientEncPrivkey, recipientPrivPtr);

    // Allocate pointers for output
    const plaintextPtrPtr = this.module._malloc(4);
    const plaintextLenPtr = this.module._malloc(4);
    const senderPubkeyPtrPtr = this.module._malloc(4);
    const senderPubkeyLenPtr = this.module._malloc(4);

    try {
      // Call C function: dna_decrypt_message_raw
      const result = this.module.ccall(
        'dna_decrypt_message_raw',
        'number',
        ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number'],
        [
          0, // ctx (NULL for now)
          ciphertextPtr, ciphertext.length,
          recipientPrivPtr,
          plaintextPtrPtr,
          plaintextLenPtr,
          senderPubkeyPtrPtr,
          senderPubkeyLenPtr
        ]
      );

      if (result !== 0) {
        throw new Error(`Decryption failed with error code: ${result}`);
      }

      // Read output pointers
      const plaintextPtr = this.module.getValue(plaintextPtrPtr, 'i32');
      const plaintextLen = this.module.getValue(plaintextLenPtr, 'i32');
      const senderPubkeyPtr = this.module.getValue(senderPubkeyPtrPtr, 'i32');
      const senderPubkeyLen = this.module.getValue(senderPubkeyLenPtr, 'i32');

      // Copy data from WASM memory
      const plaintext = new Uint8Array(plaintextLen);
      plaintext.set(this.module.HEAPU8.subarray(plaintextPtr, plaintextPtr + plaintextLen));

      const senderPubkey = new Uint8Array(senderPubkeyLen);
      senderPubkey.set(this.module.HEAPU8.subarray(senderPubkeyPtr, senderPubkeyPtr + senderPubkeyLen));

      // Free C-allocated memory
      this.module._free(plaintextPtr);
      this.module._free(senderPubkeyPtr);

      return { plaintext, senderPubkey };
    } finally {
      // Free all allocated memory
      this.module._free(ciphertextPtr);
      this.module._free(recipientPrivPtr);
      this.module._free(plaintextPtrPtr);
      this.module._free(plaintextLenPtr);
      this.module._free(senderPubkeyPtrPtr);
      this.module._free(senderPubkeyLenPtr);
    }
  }
}

// Export for both Node.js and browser
if (typeof module !== 'undefined' && module.exports) {
  module.exports = DNACrypto;
}
if (typeof window !== 'undefined') {
  window.DNACrypto = DNACrypto;
}
```

**Time Estimate:** 4 hours (includes testing)

---

### 2.3 WASM Testing Suite

**File:** `web/wasm/test_wasm.js`

```javascript
const DNACrypto = require('./dna_crypto.js');
const fs = require('fs');

async function testWASMCrypto() {
  console.log('🧪 Testing DNA WASM Crypto Module...\n');

  // Initialize
  const crypto = new DNACrypto();
  await crypto.init();

  // Load test keys (from ~/.dna/nocdem)
  const kyberPrivkey = fs.readFileSync('/home/nocdem/.dna/nocdem-kyber512.pqkey');
  const dilithiumPrivkey = fs.readFileSync('/home/nocdem/.dna/nocdem-dilithium.pqkey');

  // Parse public key bundle to extract keys
  const pubkeyBundle = fs.readFileSync('/home/nocdem/.dna/nocdem.pub');
  const kyberPubkey = pubkeyBundle.slice(24, 824); // Skip header (24 bytes) + Dilithium pubkey (1952 bytes)
  const dilithiumPubkey = pubkeyBundle.slice(24, 24 + 1952);

  console.log('✅ Keys loaded:');
  console.log(`   Kyber512 private: ${kyberPrivkey.length} bytes`);
  console.log(`   Dilithium3 private: ${dilithiumPrivkey.length} bytes`);
  console.log(`   Kyber512 public: ${kyberPubkey.length} bytes`);
  console.log(`   Dilithium3 public: ${dilithiumPubkey.length} bytes\n`);

  // Test 1: Encrypt → Decrypt
  console.log('Test 1: Encrypt → Decrypt');
  const plaintext = Buffer.from('Hello from WebAssembly! 🔐');
  console.log(`   Plaintext: "${plaintext.toString()}"`);
  console.log(`   Size: ${plaintext.length} bytes`);

  const startEncrypt = Date.now();
  const ciphertext = crypto.encryptMessage(
    plaintext,
    kyberPubkey,
    dilithiumPubkey,
    dilithiumPrivkey
  );
  const encryptTime = Date.now() - startEncrypt;
  console.log(`   ✅ Encrypted in ${encryptTime}ms`);
  console.log(`   Ciphertext size: ${ciphertext.length} bytes\n`);

  const startDecrypt = Date.now();
  const { plaintext: decrypted, senderPubkey } = crypto.decryptMessage(
    ciphertext,
    kyberPrivkey
  );
  const decryptTime = Date.now() - startDecrypt;
  console.log(`   ✅ Decrypted in ${decryptTime}ms`);
  console.log(`   Decrypted: "${Buffer.from(decrypted).toString()}"`);
  console.log(`   Sender pubkey: ${senderPubkey.length} bytes\n`);

  // Verify
  if (Buffer.from(decrypted).toString() === plaintext.toString()) {
    console.log('✅ Test 1 PASSED: Plaintext matches!\n');
  } else {
    console.error('❌ Test 1 FAILED: Plaintext mismatch!\n');
    process.exit(1);
  }

  // Test 2: Performance (100 iterations)
  console.log('Test 2: Performance Benchmark (100 iterations)');
  const iterations = 100;
  let totalEncryptTime = 0;
  let totalDecryptTime = 0;

  for (let i = 0; i < iterations; i++) {
    const start1 = Date.now();
    const ct = crypto.encryptMessage(plaintext, kyberPubkey, dilithiumPubkey, dilithiumPrivkey);
    totalEncryptTime += Date.now() - start1;

    const start2 = Date.now();
    crypto.decryptMessage(ct, kyberPrivkey);
    totalDecryptTime += Date.now() - start2;
  }

  console.log(`   Average encrypt time: ${(totalEncryptTime / iterations).toFixed(2)}ms`);
  console.log(`   Average decrypt time: ${(totalDecryptTime / iterations).toFixed(2)}ms`);
  console.log('   ✅ Test 2 PASSED\n');

  console.log('🎉 All WASM crypto tests passed!');
}

testWASMCrypto().catch(err => {
  console.error('❌ Test failed:', err);
  process.exit(1);
});
```

**Run Tests:**
```bash
cd /opt/dna-messenger/web/wasm
./build_wasm.sh
node test_wasm.js
```

**Expected Output:**
```
✅ Encrypted in ~20ms
✅ Decrypted in ~15ms
✅ All WASM crypto tests passed!
```

**Time Estimate:** 3 hours (includes debugging)

---

## Phase 3: Backend Server (Week 2-3)

### 3.1 Database Connection Pool

**File:** `web/server/src/db/postgres.js`

```javascript
const { Pool } = require('pg');

const pool = new Pool({
  host: process.env.DB_HOST || 'localhost',
  port: process.env.DB_PORT || 5432,
  database: process.env.DB_NAME || 'dna_messenger',
  user: process.env.DB_USER || 'dna',
  password: process.env.DB_PASSWORD || 'dna_password',
  max: 20, // Maximum pool size
  idleTimeoutMillis: 30000,
  connectionTimeoutMillis: 2000,
});

// Test connection on startup
pool.on('connect', () => {
  console.log('✅ PostgreSQL connected');
});

pool.on('error', (err) => {
  console.error('❌ PostgreSQL error:', err);
  process.exit(1);
});

module.exports = pool;
```

**Time Estimate:** 30 minutes

---

### 3.2 JWT Authentication Middleware

**File:** `web/server/src/middleware/auth.js`

```javascript
const jwt = require('jsonwebtoken');

const JWT_SECRET = process.env.JWT_SECRET || 'change-this-in-production';
const JWT_EXPIRES_IN = '7d';

/**
 * Generate JWT token for authenticated user
 */
function generateToken(identity) {
  return jwt.sign({ identity }, JWT_SECRET, { expiresIn: JWT_EXPIRES_IN });
}

/**
 * Verify JWT token middleware (for HTTP routes)
 */
function verifyToken(req, res, next) {
  const token = req.headers.authorization?.replace('Bearer ', '');

  if (!token) {
    return res.status(401).json({ error: 'No token provided' });
  }

  try {
    const decoded = jwt.verify(token, JWT_SECRET);
    req.identity = decoded.identity;
    next();
  } catch (err) {
    return res.status(401).json({ error: 'Invalid token' });
  }
}

/**
 * Verify JWT token for WebSocket connections
 */
function verifyTokenWS(socket, next) {
  const token = socket.handshake.auth.token;

  if (!token) {
    return next(new Error('Authentication error: No token provided'));
  }

  try {
    const decoded = jwt.verify(token, JWT_SECRET);
    socket.identity = decoded.identity;
    next();
  } catch (err) {
    return next(new Error('Authentication error: Invalid token'));
  }
}

module.exports = { generateToken, verifyToken, verifyTokenWS };
```

**Time Estimate:** 1 hour

---

### 3.3 REST API Routes

**File:** `web/server/src/routes/auth.js`

```javascript
const express = require('express');
const router = express.Router();
const pool = require('../db/postgres');
const { generateToken } = require('../middleware/auth');

/**
 * POST /api/auth/login
 * Generate JWT token for existing identity
 */
router.post('/login', async (req, res) => {
  const { identity } = req.body;

  if (!identity) {
    return res.status(400).json({ error: 'Identity required' });
  }

  try {
    // Verify identity exists in keyserver
    const result = await pool.query(
      'SELECT identity FROM keyserver WHERE identity = $1',
      [identity]
    );

    if (result.rows.length === 0) {
      return res.status(404).json({ error: 'Identity not found in keyserver' });
    }

    // Generate JWT token
    const token = generateToken(identity);

    res.json({
      token,
      identity,
      expiresIn: '7d'
    });
  } catch (err) {
    console.error('Login error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

module.exports = router;
```

**File:** `web/server/src/routes/keyserver.js`

```javascript
const express = require('express');
const router = express.Router();
const pool = require('../db/postgres');

/**
 * GET /api/keyserver/load/:identity
 * Load public keys for identity
 */
router.get('/load/:identity', async (req, res) => {
  const { identity } = req.params;

  try {
    const result = await pool.query(
      'SELECT signing_pubkey, encryption_pubkey FROM keyserver WHERE identity = $1',
      [identity]
    );

    if (result.rows.length === 0) {
      return res.status(404).json({ error: 'Identity not found' });
    }

    res.json({
      identity,
      signing_pubkey: Array.from(result.rows[0].signing_pubkey),
      encryption_pubkey: Array.from(result.rows[0].encryption_pubkey)
    });
  } catch (err) {
    console.error('Keyserver load error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

/**
 * POST /api/keyserver/store
 * Store public keys (registration)
 */
router.post('/store', async (req, res) => {
  const { identity, signing_pubkey, encryption_pubkey } = req.body;

  if (!identity || !signing_pubkey || !encryption_pubkey) {
    return res.status(400).json({ error: 'Missing required fields' });
  }

  try {
    await pool.query(
      `INSERT INTO keyserver (identity, signing_pubkey, signing_pubkey_len, encryption_pubkey, encryption_pubkey_len)
       VALUES ($1, $2, $3, $4, $5)
       ON CONFLICT (identity) DO UPDATE SET
       signing_pubkey = EXCLUDED.signing_pubkey,
       encryption_pubkey = EXCLUDED.encryption_pubkey`,
      [
        identity,
        Buffer.from(signing_pubkey),
        signing_pubkey.length,
        Buffer.from(encryption_pubkey),
        encryption_pubkey.length
      ]
    );

    res.json({ success: true, identity });
  } catch (err) {
    console.error('Keyserver store error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

module.exports = router;
```

**File:** `web/server/src/routes/contacts.js`

```javascript
const express = require('express');
const router = express.Router();
const pool = require('../db/postgres');
const { verifyToken } = require('../middleware/auth');

/**
 * GET /api/contacts/list
 * List all identities in keyserver
 */
router.get('/list', verifyToken, async (req, res) => {
  try {
    const result = await pool.query(
      'SELECT identity, created_at FROM keyserver ORDER BY identity ASC'
    );

    res.json({
      contacts: result.rows.map(row => ({
        identity: row.identity,
        created_at: row.created_at.toISOString()
      }))
    });
  } catch (err) {
    console.error('Contacts list error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

module.exports = router;
```

**Time Estimate:** 3 hours

---

### 3.4 WebSocket Server

**File:** `web/server/src/websocket.js`

```javascript
const { Server } = require('socket.io');
const pool = require('./db/postgres');
const { verifyTokenWS } = require('./middleware/auth');

// Connected users map: identity -> socket.id
const connectedUsers = new Map();

function setupWebSocket(httpServer) {
  const io = new Server(httpServer, {
    cors: {
      origin: process.env.CLIENT_URL || 'http://localhost:3000',
      methods: ['GET', 'POST']
    }
  });

  // JWT authentication middleware
  io.use(verifyTokenWS);

  io.on('connection', (socket) => {
    const { identity } = socket;
    console.log(`✅ User connected: ${identity} (${socket.id})`);

    // Register user as online
    connectedUsers.set(identity, socket.id);
    socket.join(identity); // Join personal room

    // Broadcast online status
    io.emit('user:online', { identity });

    // Handle message send
    socket.on('message:send', async (data) => {
      const { recipient, ciphertext, timestamp } = data;

      try {
        // Store in PostgreSQL
        const result = await pool.query(
          `INSERT INTO messages (sender, recipient, ciphertext, ciphertext_len, created_at)
           VALUES ($1, $2, $3, $4, $5) RETURNING id`,
          [identity, recipient, Buffer.from(ciphertext), ciphertext.length, new Date(timestamp)]
        );

        const messageId = result.rows[0].id;

        // Real-time delivery if recipient is online
        if (connectedUsers.has(recipient)) {
          io.to(recipient).emit('message:new', {
            id: messageId,
            sender: identity,
            ciphertext: Array.from(Buffer.from(ciphertext)),
            timestamp
          });
        }

        // Confirm to sender
        socket.emit('message:sent', {
          messageId,
          status: connectedUsers.has(recipient) ? 'delivered' : 'sent'
        });
      } catch (err) {
        console.error('Message send error:', err);
        socket.emit('message:error', { error: err.message });
      }
    });

    // Handle messages load
    socket.on('messages:load', async (data) => {
      const { contactIdentity } = data;

      try {
        const result = await pool.query(
          `SELECT id, sender, recipient, ciphertext, created_at
           FROM messages
           WHERE (sender = $1 AND recipient = $2) OR (sender = $2 AND recipient = $1)
           ORDER BY created_at ASC`,
          [identity, contactIdentity]
        );

        socket.emit('messages:loaded', {
          messages: result.rows.map(row => ({
            id: row.id,
            sender: row.sender,
            recipient: row.recipient,
            ciphertext: Array.from(row.ciphertext),
            timestamp: row.created_at.toISOString()
          }))
        });
      } catch (err) {
        console.error('Messages load error:', err);
        socket.emit('messages:error', { error: err.message });
      }
    });

    // Handle disconnect
    socket.on('disconnect', () => {
      console.log(`❌ User disconnected: ${identity} (${socket.id})`);
      connectedUsers.delete(identity);
      io.emit('user:offline', { identity });
    });
  });

  return io;
}

module.exports = setupWebSocket;
```

**Time Estimate:** 4 hours

---

### 3.5 Main Server Entry Point

**File:** `web/server/src/server.js`

```javascript
require('dotenv').config();
const express = require('express');
const { createServer } = require('http');
const helmet = require('helmet');
const rateLimit = require('express-rate-limit');
const cors = require('cors');

const setupWebSocket = require('./websocket');
const authRoutes = require('./routes/auth');
const keyserverRoutes = require('./routes/keyserver');
const contactsRoutes = require('./routes/contacts');

const app = express();
const httpServer = createServer(app);

// Security middleware
app.use(helmet());
app.use(cors({
  origin: process.env.CLIENT_URL || 'http://localhost:3000'
}));
app.use(express.json({ limit: '10mb' }));

// Rate limiting
const limiter = rateLimit({
  windowMs: 60 * 1000, // 1 minute
  max: 100 // 100 requests per minute
});
app.use('/api/', limiter);

// Routes
app.use('/api/auth', authRoutes);
app.use('/api/keyserver', keyserverRoutes);
app.use('/api/contacts', contactsRoutes);

// Health check
app.get('/health', (req, res) => {
  res.json({ status: 'ok', timestamp: new Date().toISOString() });
});

// Setup WebSocket
setupWebSocket(httpServer);

// Start server
const PORT = process.env.PORT || 8080;
httpServer.listen(PORT, () => {
  console.log(`🚀 DNA Messenger Server running on http://localhost:${PORT}`);
});
```

**File:** `web/server/.env.example`

```env
PORT=8080
DB_HOST=localhost
DB_PORT=5432
DB_NAME=dna_messenger
DB_USER=dna
DB_PASSWORD=dna_password
JWT_SECRET=your-very-secure-random-secret-here
CLIENT_URL=http://localhost:3000
NODE_ENV=development
```

**File:** `web/server/package.json` (add scripts)

```json
{
  "scripts": {
    "start": "node src/server.js",
    "dev": "nodemon src/server.js",
    "test": "jest"
  }
}
```

**Time Estimate:** 2 hours

---

## Phase 4: Frontend React App (Week 3-4)

### 4.1 IndexedDB Key Storage

**File:** `web/client/src/utils/keyStorage.js`

```javascript
import { openDB } from 'idb';

const DB_NAME = 'dna-messenger';
const STORE_NAME = 'keys';

/**
 * Initialize IndexedDB
 */
async function initDB() {
  return openDB(DB_NAME, 1, {
    upgrade(db) {
      if (!db.objectStoreNames.contains(STORE_NAME)) {
        db.createObjectStore(STORE_NAME);
      }
    }
  });
}

/**
 * Derive encryption key from passphrase using PBKDF2
 */
async function deriveKey(passphrase, salt) {
  const encoder = new TextEncoder();
  const keyMaterial = await crypto.subtle.importKey(
    'raw',
    encoder.encode(passphrase),
    { name: 'PBKDF2' },
    false,
    ['deriveKey']
  );

  return await crypto.subtle.deriveKey(
    {
      name: 'PBKDF2',
      salt: salt,
      iterations: 600000, // OWASP 2023 recommendation
      hash: 'SHA-256'
    },
    keyMaterial,
    { name: 'AES-GCM', length: 256 },
    false,
    ['encrypt', 'decrypt']
  );
}

/**
 * Store encrypted keys in IndexedDB
 */
export async function storeKeys(identity, kyberPrivkey, dilithiumPrivkey, passphrase) {
  const salt = crypto.getRandomValues(new Uint8Array(32));
  const encryptionKey = await deriveKey(passphrase, salt);

  // Encrypt Kyber private key
  const kyberIV = crypto.getRandomValues(new Uint8Array(12));
  const encryptedKyber = await crypto.subtle.encrypt(
    { name: 'AES-GCM', iv: kyberIV },
    encryptionKey,
    kyberPrivkey
  );

  // Encrypt Dilithium private key
  const dilithiumIV = crypto.getRandomValues(new Uint8Array(12));
  const encryptedDilithium = await crypto.subtle.encrypt(
    { name: 'AES-GCM', iv: dilithiumIV },
    encryptionKey,
    dilithiumPrivkey
  );

  // Store in IndexedDB
  const db = await initDB();
  await db.put(STORE_NAME, {
    identity,
    salt: Array.from(salt),
    kyber: {
      iv: Array.from(kyberIV),
      ciphertext: Array.from(new Uint8Array(encryptedKyber))
    },
    dilithium: {
      iv: Array.from(dilithiumIV),
      ciphertext: Array.from(new Uint8Array(encryptedDilithium))
    }
  }, identity);

  console.log('✅ Keys stored in IndexedDB (encrypted)');
}

/**
 * Load and decrypt keys from IndexedDB
 */
export async function loadKeys(identity, passphrase) {
  const db = await initDB();
  const record = await db.get(STORE_NAME, identity);

  if (!record) {
    throw new Error('Keys not found');
  }

  const salt = new Uint8Array(record.salt);
  const encryptionKey = await deriveKey(passphrase, salt);

  try {
    // Decrypt Kyber private key
    const kyberPrivkey = await crypto.subtle.decrypt(
      { name: 'AES-GCM', iv: new Uint8Array(record.kyber.iv) },
      encryptionKey,
      new Uint8Array(record.kyber.ciphertext)
    );

    // Decrypt Dilithium private key
    const dilithiumPrivkey = await crypto.subtle.decrypt(
      { name: 'AES-GCM', iv: new Uint8Array(record.dilithium.iv) },
      encryptionKey,
      new Uint8Array(record.dilithium.ciphertext)
    );

    console.log('✅ Keys decrypted from IndexedDB');

    return {
      kyber: new Uint8Array(kyberPrivkey),
      dilithium: new Uint8Array(dilithiumPrivkey)
    };
  } catch (err) {
    throw new Error('Incorrect passphrase');
  }
}

/**
 * Check if keys exist for identity
 */
export async function hasKeys(identity) {
  const db = await initDB();
  const record = await db.get(STORE_NAME, identity);
  return !!record;
}
```

**Time Estimate:** 3 hours

---

### 4.2 React Components

Due to length constraints, I'll provide component specifications:

**Components to Create:**
1. `LoginView.jsx` - Login/passphrase entry (2 hours)
2. `ContactList.jsx` - Sidebar with contacts and online status (3 hours)
3. `ChatWindow.jsx` - Message display area (4 hours)
4. `MessageInput.jsx` - Compose message input (2 hours)
5. `KeyManager.jsx` - Generate/import keys UI (3 hours)

**Hooks to Create:**
1. `useWebSocket.js` - WebSocket connection management (3 hours)
2. `useCrypto.js` - WASM crypto wrapper (2 hours)
3. `useMessages.js` - Message state management (2 hours)

**Time Estimate:** 21 hours (3 days)

---

## Phase 5: Integration & Testing (Week 5)

### 5.1 End-to-End Testing

**Test Scenarios:**
1. User registration (key generation)
2. Login with passphrase
3. Load contacts
4. Send encrypted message
5. Receive real-time message
6. Decrypt and display message
7. Online/offline status
8. Multiple concurrent users

**Testing Tools:**
- Jest (unit tests)
- React Testing Library (component tests)
- Puppeteer (E2E tests)

**Time Estimate:** 20 hours (3 days)

---

### 5.2 Security Testing

**Tests Required:**
1. XSS vulnerability scan (OWASP ZAP)
2. WASM timing analysis
3. Key extraction attempts
4. Authentication bypass tests
5. Rate limiting verification

**Time Estimate:** 16 hours (2 days)

---

## Phase 6: Deployment (Week 6)

### 6.1 Production Build

```bash
# Build client
cd web/client
npm run build

# Build server (if using TypeScript)
cd web/server
npm run build

# Copy WASM files to client build
cp web/wasm/dna_wasm.* web/client/build/static/js/
```

**Time Estimate:** 2 hours

---

### 6.2 Nginx Configuration

**File:** `/etc/nginx/sites-available/dna-messenger`

```nginx
upstream dna_backend {
    server 127.0.0.1:8080;
}

server {
    listen 80;
    server_name messenger.ai.cpunk.io;
    return 301 https://$host$request_uri;
}

server {
    listen 443 ssl http2;
    server_name messenger.ai.cpunk.io;

    ssl_certificate /etc/letsencrypt/live/messenger.ai.cpunk.io/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/messenger.ai.cpunk.io/privkey.pem;

    # Security headers
    add_header Strict-Transport-Security "max-age=31536000; includeSubDomains" always;
    add_header X-Frame-Options "DENY" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header X-XSS-Protection "1; mode=block" always;

    # CSP
    add_header Content-Security-Policy "default-src 'self'; script-src 'self' 'wasm-unsafe-eval'; connect-src 'self' wss://messenger.ai.cpunk.io; style-src 'self' 'unsafe-inline';" always;

    # Serve React app
    location / {
        root /opt/dna-messenger/web/client/build;
        try_files $uri /index.html;
    }

    # WebSocket proxy
    location /socket.io/ {
        proxy_pass http://dna_backend;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }

    # REST API proxy
    location /api/ {
        proxy_pass http://dna_backend;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

**SSL Setup:**
```bash
sudo apt-get install certbot python3-certbot-nginx
sudo certbot --nginx -d messenger.ai.cpunk.io
```

**Time Estimate:** 3 hours

---

### 6.3 systemd Service

**File:** `/etc/systemd/system/dna-messenger-web.service`

```ini
[Unit]
Description=DNA Messenger Web Server
After=network.target postgresql.service

[Service]
Type=simple
User=nocdem
WorkingDirectory=/opt/dna-messenger/web/server
ExecStart=/usr/bin/node src/server.js
Restart=always
RestartSec=10
Environment=NODE_ENV=production

[Install]
WantedBy=multi-user.target
```

**Enable and start:**
```bash
sudo systemctl enable dna-messenger-web
sudo systemctl start dna-messenger-web
sudo systemctl status dna-messenger-web
```

**Time Estimate:** 1 hour

---

## Timeline Summary

| Phase | Duration | Tasks |
|-------|----------|-------|
| **Phase 1: Setup** | 1 week | Install Emscripten, Node packages |
| **Phase 2: WASM** | 1 week | Compile C library, test crypto |
| **Phase 3: Backend** | 1.5 weeks | Node.js server, WebSocket, API |
| **Phase 4: Frontend** | 2 weeks | React app, components, hooks |
| **Phase 5: Testing** | 1 week | E2E tests, security audit |
| **Phase 6: Deployment** | 0.5 weeks | Production build, nginx, SSL |
| **Total** | **6-8 weeks** | Full web messenger |

---

## Risk Mitigation

### Technical Risks

1. **WASM Compilation Issues**
   - **Risk:** C library may not compile to WASM
   - **Mitigation:** Start with minimal build, add features incrementally
   - **Contingency:** Use Node.js crypto on server-side (reduced security)

2. **Performance Issues**
   - **Risk:** WASM crypto may be slow on mobile
   - **Mitigation:** Optimize with -O3, test on various devices
   - **Contingency:** Add loading indicators, async operations

3. **Browser Compatibility**
   - **Risk:** Older browsers may not support WASM
   - **Mitigation:** Detect and show upgrade message
   - **Contingency:** Build legacy bundle (slower)

### Security Risks

1. **Key Storage Compromise**
   - **Risk:** Browser storage less secure than filesystem
   - **Mitigation:** Strong passphrase, PBKDF2 600k iterations
   - **Contingency:** Add 2FA, hardware key support (Phase 7)

2. **XSS Attacks**
   - **Risk:** Malicious scripts steal keys
   - **Mitigation:** Strict CSP, input sanitization
   - **Contingency:** Regular security audits

---

## Success Criteria

### Functional Requirements ✅
- [ ] User can login with identity + passphrase
- [ ] User can send encrypted messages
- [ ] User can receive real-time messages
- [ ] Messages decrypt successfully
- [ ] Online/offline status works
- [ ] Works on desktop and mobile browsers

### Performance Requirements ✅
- [ ] Message encryption < 50ms
- [ ] Message decryption < 30ms
- [ ] WebSocket latency < 100ms
- [ ] Page load time < 3s
- [ ] WASM module < 2MB

### Security Requirements ✅
- [ ] End-to-end PQ encryption maintained
- [ ] Zero-knowledge server (never sees plaintext)
- [ ] Keys encrypted in IndexedDB
- [ ] CSP headers configured
- [ ] No XSS vulnerabilities
- [ ] TLS 1.3 enabled

---

## Next Steps

**Immediate Actions (Week 1):**

1. Install Emscripten SDK
   ```bash
   cd /opt
   sudo git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk && sudo ./emsdk install latest && sudo ./emsdk activate latest
   ```

2. Create WASM build script
   ```bash
   cd /opt/dna-messenger/web/wasm
   nano build_wasm.sh  # Create as shown in Phase 2.1
   chmod +x build_wasm.sh
   ```

3. Build and test WASM module
   ```bash
   ./build_wasm.sh
   node test_wasm.js
   ```

4. Initialize server project
   ```bash
   cd /opt/dna-messenger/web/server
   npm init -y
   npm install express socket.io pg jsonwebtoken
   ```

**Ready to begin implementation!**

---

**Last Updated:** 2025-10-15
**Branch:** feature/web-messenger
**Status:** Ready for Development
