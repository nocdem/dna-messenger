# WebAssembly Build

Compiles DNA Messenger C library to WebAssembly for browser use.

## Prerequisites

```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

## Build

```bash
./build_wasm.sh
```

**Outputs:**
- `dna_wasm.js` - JavaScript wrapper
- `dna_wasm.wasm` - WebAssembly binary

## Usage

```javascript
import Module from './dna_wasm.js';

const dna = await Module();

// Encrypt message
const plaintext = new TextEncoder().encode("Hello");
const ciphertext = dna.encryptMessage(plaintext, recipientPubkey, senderPrivkey);

// Decrypt message
const decrypted = dna.decryptMessage(ciphertext, recipientPrivkey);
```

## Security

**Compilation flags:**
- `-O3` - Maximum optimization
- `-fno-unroll-loops` - Constant-time execution
- `-s ALLOW_MEMORY_GROWTH=1` - Dynamic memory
- `-s WASM=1` - WebAssembly output

**Testing:**
```bash
npm run test:wasm
```
