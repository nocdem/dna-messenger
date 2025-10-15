# DNA Messenger - Web Client

**Branch:** `feature/web-messenger`
**Status:** 🚧 Development (Phase 4)
**Version:** 0.2.0-alpha

---

## Overview

Web-based messenger client for DNA Messenger, providing browser access to the post-quantum encrypted messaging platform.

**Key Features:**
- ✅ Post-quantum encryption (Kyber512 + Dilithium3 via WebAssembly)
- ✅ Real-time messaging (WebSocket)
- ✅ End-to-end encrypted
- ✅ Zero-knowledge server
- ✅ Cross-platform (any modern browser)

---

## Architecture

```
web/
├── server/          # Node.js backend (Express + Socket.IO)
├── client/          # React frontend
├── wasm/            # WebAssembly build of DNA C library
└── docs/            # Web-specific documentation
```

---

## Quick Start

### Prerequisites

- Node.js 18+
- PostgreSQL (existing DNA Messenger database)
- Emscripten (for WASM builds)

### 1. Build WebAssembly Module

```bash
cd web/wasm
./build_wasm.sh
# Outputs: dna_wasm.js, dna_wasm.wasm
```

### 2. Start Backend Server

```bash
cd web/server
npm install
npm run dev
# Runs on http://localhost:8080
```

### 3. Start Frontend Development Server

```bash
cd web/client
npm install
npm start
# Runs on http://localhost:3000
```

---

## Security Model

### ✅ Post-Quantum Secure
- **Message encryption**: Kyber512 (NIST PQC)
- **Digital signatures**: Dilithium3 (FIPS 204)
- **Symmetric encryption**: AES-256-GCM

### ⚠️ Transport Layer
- **Current**: TLS 1.3 (ECDHE - quantum-vulnerable metadata)
- **Future**: Hybrid PQ-TLS (X25519 + Kyber768) via Cloudflare

### 🔒 Key Storage
- **Browser**: Encrypted IndexedDB (PBKDF2, 600k iterations)
- **Server**: Never sees private keys (zero-knowledge)

**Security Guarantee:** Even if TLS breaks in the future, message contents remain quantum-resistant due to Kyber512 encryption at the application layer.

---

## Development Workflow

### Branch Strategy
```
main (v0.1.52-alpha)          # Stable CLI/Qt GUI
  └── feature/web-messenger   # Web development (you are here)
```

**Merge Policy:**
- Test thoroughly before merging to main
- Ensure backward compatibility with existing database
- Update CLAUDE.md version and status

### Testing
```bash
# Run all tests
npm test

# Security tests
npm run test:security

# WASM crypto tests
npm run test:wasm
```

---

## Components

### Server (`web/server/`)
- WebSocket server (Socket.IO)
- REST API (Express)
- PostgreSQL integration (reuses existing tables)
- JWT authentication
- Zero-knowledge message routing

### Client (`web/client/`)
- React 18 application
- WebAssembly crypto module
- Real-time messaging UI
- Encrypted key storage (IndexedDB)

### WASM (`web/wasm/`)
- Compiled DNA C library (dna_api.c)
- Kyber512 + Dilithium3 (pq-crystals)
- JavaScript wrapper API

---

## Database Compatibility

**Reuses existing tables:**
- `keyserver` - Public keys (Dilithium3 + Kyber512)
- `messages` - Encrypted message blobs

**No schema changes required** - Web client is fully compatible with CLI/Qt GUI.

---

## API Endpoints

### REST API (HTTPS)
- `POST /api/auth/login` - JWT token generation
- `GET /api/keyserver/load/:identity` - Load public keys
- `POST /api/keyserver/store` - Store public keys
- `GET /api/contacts/list` - List all contacts

### WebSocket Events (WSS)
- `message:send` - Send encrypted message
- `message:new` - Receive encrypted message (push)
- `user:online` - User online status
- `user:offline` - User offline status

---

## Deployment

### Development
```bash
npm run dev  # Both server and client with hot reload
```

### Production
```bash
# Build client
cd web/client
npm run build

# Build server
cd web/server
npm run build

# Deploy to ai.cpunk.io
./deploy.sh
```

### Docker (Future)
```bash
docker-compose up -d
```

---

## Security Testing

### Required Tests
- [ ] XSS vulnerability scanning (OWASP ZAP)
- [ ] Timing side-channel analysis (WASM)
- [ ] Key extraction attempts (IndexedDB)
- [ ] TLS configuration audit (SSL Labs)
- [ ] Penetration testing (authorized)

### Test Commands
```bash
npm run test:security        # Run security test suite
npm run test:timing          # WASM timing analysis
npm run test:xss             # XSS vulnerability scan
```

---

## Performance Targets

| Operation | Target | Notes |
|-----------|--------|-------|
| WASM encryption | <50ms | Kyber512 + Dilithium3 |
| WASM decryption | <30ms | AES-256-GCM + verify |
| WebSocket latency | <100ms | Server round-trip |
| Key derivation | <500ms | PBKDF2 (600k iterations) |
| Page load time | <2s | Including WASM module |

---

## Roadmap

### Phase 4A: Core Implementation (Current)
- [ ] WebAssembly crypto module
- [ ] Node.js WebSocket server
- [ ] React frontend UI
- [ ] Encrypted IndexedDB storage

### Phase 4B: Security Hardening
- [ ] CSP headers
- [ ] Input sanitization
- [ ] Timing noise injection
- [ ] Security audit

### Phase 4C: Production Deployment
- [ ] SSL/TLS setup
- [ ] Load balancing
- [ ] Monitoring/logging
- [ ] Performance optimization

### Phase 5: Advanced Features
- [ ] File attachments
- [ ] Voice messages
- [ ] Push notifications (Service Workers)
- [ ] Offline support (PWA)

### Phase 6: Hybrid PQ-TLS
- [ ] Cloudflare PQ-TLS proxy
- [ ] Or nginx with liboqs integration

---

## Contributing

See [CLAUDE.md](/CLAUDE.md) for development guidelines.

**Important:**
- All web development happens on `feature/web-messenger` branch
- Maintain backward compatibility with CLI/Qt GUI
- Follow Protocol Mode for crypto operations
- Never log or expose private keys

---

## References

- **DNA Messenger (main)**: CLI and Qt GUI implementation
- **CLAUDE.md**: Project development guidelines
- **NETWORK_RELAY_OPTIONS.md**: Network architecture design
- **pq-crystals**: https://github.com/pq-crystals/
- **NIST PQC**: https://csrc.nist.gov/projects/post-quantum-cryptography

---

**Last Updated:** 2025-10-15
**Branch:** feature/web-messenger
**Status:** Initial setup complete
**Next Step:** Implement WebAssembly build
