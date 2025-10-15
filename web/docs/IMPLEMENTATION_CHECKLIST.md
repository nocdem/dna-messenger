# DNA Messenger Web - Implementation Checklist

**Branch:** `feature/web-messenger`
**Start Date:** 2025-10-15
**Target Completion:** 2025-12-01 (6-8 weeks)

---

## Week 1: Environment Setup & WASM (Oct 15-22)

### Day 1-2: Prerequisites
- [ ] Install Emscripten SDK
  ```bash
  cd /opt
  sudo git clone https://github.com/emscripten-core/emsdk.git
  cd emsdk && sudo ./emsdk install latest && sudo ./emsdk activate latest
  source emsdk_env.sh
  ```
- [ ] Verify: `emcc --version`
- [ ] Install nginx: `sudo apt-get install nginx`
- [ ] Install Redis: `sudo apt-get install redis-server`

### Day 3-4: WASM Module
- [ ] Create `web/wasm/build_wasm.sh` (from plan)
- [ ] Run build: `./build_wasm.sh`
- [ ] Verify outputs: `dna_wasm.js` and `dna_wasm.wasm` exist
- [ ] Check WASM size: Should be < 2MB
- [ ] Create `web/wasm/dna_crypto.js` wrapper
- [ ] Create `web/wasm/test_wasm.js` test suite

### Day 5: WASM Testing
- [ ] Run: `node test_wasm.js`
- [ ] Verify encryption works (< 50ms)
- [ ] Verify decryption works (< 30ms)
- [ ] Test 100 iterations (performance benchmark)
- [ ] Test with real keys from `~/.dna/nocdem`

### Day 6-7: Backend Setup
- [ ] Initialize server: `cd web/server && npm init -y`
- [ ] Install dependencies:
  ```bash
  npm install express socket.io pg jsonwebtoken bcrypt cors dotenv helmet express-rate-limit
  npm install -D nodemon jest supertest
  ```
- [ ] Create `.env` file (copy from plan)
- [ ] Create `src/db/postgres.js` (DB connection pool)
- [ ] Test DB connection: `node -e "require('./src/db/postgres')"`

---

## Week 2: Backend Server (Oct 22-29)

### Day 8-9: Authentication & Middleware
- [ ] Create `src/middleware/auth.js` (JWT)
- [ ] Create `src/routes/auth.js` (POST /api/auth/login)
- [ ] Test login endpoint:
  ```bash
  curl -X POST http://localhost:8080/api/auth/login \
    -H "Content-Type: application/json" \
    -d '{"identity":"nocdem"}'
  ```

### Day 10-11: Keyserver API
- [ ] Create `src/routes/keyserver.js`
- [ ] Implement GET `/api/keyserver/load/:identity`
- [ ] Implement POST `/api/keyserver/store`
- [ ] Test keyserver endpoints with curl

### Day 12-13: Contacts API
- [ ] Create `src/routes/contacts.js`
- [ ] Implement GET `/api/contacts/list`
- [ ] Add JWT verification middleware
- [ ] Test with authenticated request

### Day 14: WebSocket Foundation
- [ ] Create `src/websocket.js`
- [ ] Setup Socket.IO server
- [ ] Implement JWT authentication for WS
- [ ] Test connection: Use Socket.IO client test script

---

## Week 3: WebSocket & Messages (Oct 29 - Nov 5)

### Day 15-16: WebSocket Events
- [ ] Implement `connection` event (user online)
- [ ] Implement `disconnect` event (user offline)
- [ ] Implement `message:send` event
- [ ] Implement `message:new` event (push to recipient)
- [ ] Test with two browser tabs (different users)

### Day 17-18: Message Persistence
- [ ] Implement `messages:load` event
- [ ] Query PostgreSQL for conversation history
- [ ] Handle sender/recipient filtering
- [ ] Test loading conversation between two users

### Day 19-20: Main Server
- [ ] Create `src/server.js` (main entry point)
- [ ] Add security middleware (helmet, CORS, rate limiting)
- [ ] Register all routes
- [ ] Setup WebSocket server
- [ ] Add health check endpoint

### Day 21: Backend Testing
- [ ] Run server: `npm run dev`
- [ ] Test all REST endpoints
- [ ] Test WebSocket connection
- [ ] Test message send/receive
- [ ] Verify database storage

---

## Week 4: React Frontend Foundation (Nov 5-12)

### Day 22-23: React Setup
- [ ] Initialize React app:
  ```bash
  cd web/client
  npx create-react-app . --template typescript
  ```
- [ ] Install dependencies:
  ```bash
  npm install socket.io-client idb dompurify @types/dompurify
  npm install @mui/material @mui/icons-material @emotion/react @emotion/styled
  ```
- [ ] Copy WASM files to `public/` directory

### Day 24-25: Key Storage Utility
- [ ] Create `src/utils/keyStorage.js`
- [ ] Implement `deriveKey()` (PBKDF2, 600k iterations)
- [ ] Implement `storeKeys()` (encrypt with AES-GCM)
- [ ] Implement `loadKeys()` (decrypt keys)
- [ ] Implement `hasKeys()` (check if keys exist)
- [ ] Test in browser console

### Day 26-27: Crypto Hook
- [ ] Create `src/hooks/useCrypto.js`
- [ ] Load WASM module on init
- [ ] Wrap `encryptMessage()` function
- [ ] Wrap `decryptMessage()` function
- [ ] Add error handling and loading states

### Day 28: WebSocket Hook
- [ ] Create `src/hooks/useWebSocket.js`
- [ ] Implement connection management
- [ ] Implement JWT authentication
- [ ] Handle reconnection logic
- [ ] Add event listeners (message:new, user:online, etc.)

---

## Week 5: React Components (Nov 12-19)

### Day 29-30: Login Component
- [ ] Create `src/components/LoginView.jsx`
- [ ] Add identity input field
- [ ] Add passphrase input field
- [ ] Implement login logic (load keys from IndexedDB)
- [ ] Show error messages for wrong passphrase
- [ ] Add "Register" link (for new users)

### Day 31-32: Contact List Component
- [ ] Create `src/components/ContactList.jsx`
- [ ] Fetch contacts from `/api/contacts/list`
- [ ] Display list with online/offline indicators
- [ ] Handle contact selection (emit to parent)
- [ ] Add search/filter functionality

### Day 33-34: Chat Window Component
- [ ] Create `src/components/ChatWindow.jsx`
- [ ] Display messages with timestamps
- [ ] Distinguish sent vs received messages
- [ ] Auto-scroll to bottom on new message
- [ ] Show "typing..." indicator (future)

### Day 35: Message Input Component
- [ ] Create `src/components/MessageInput.jsx`
- [ ] Text input with send button
- [ ] Encrypt message on send
- [ ] Emit via WebSocket
- [ ] Clear input after send
- [ ] Handle Enter key press

---

## Week 6: Integration & Polish (Nov 19-26)

### Day 36-37: Main App Integration
- [ ] Create main `App.jsx` layout
- [ ] Integrate all components
- [ ] Add routing (if needed)
- [ ] Implement state management (Context API or Redux)
- [ ] Handle authentication state

### Day 38-39: Real-Time Features
- [ ] Test message send/receive between two browsers
- [ ] Verify encryption/decryption works
- [ ] Test online/offline status updates
- [ ] Test conversation loading
- [ ] Fix any WebSocket issues

### Day 40-41: UI/UX Polish
- [ ] Add loading spinners
- [ ] Add error notifications (toast messages)
- [ ] Improve mobile responsiveness
- [ ] Add dark mode (optional)
- [ ] Add message timestamps formatting

### Day 42: E2E Testing
- [ ] Create Puppeteer test scripts
- [ ] Test complete user flow:
  1. Login
  2. Load contacts
  3. Select contact
  4. Send message
  5. Receive message (second browser)
  6. Verify decryption

---

## Week 7: Security & Testing (Nov 26 - Dec 3)

### Day 43-44: Security Audit
- [ ] Run OWASP ZAP scan
- [ ] Check for XSS vulnerabilities
- [ ] Verify CSP headers
- [ ] Test WASM timing analysis
- [ ] Attempt key extraction from IndexedDB

### Day 45-46: Performance Testing
- [ ] Test with 100 contacts
- [ ] Test with 1000 messages in conversation
- [ ] Measure WASM crypto performance
- [ ] Check memory leaks (Chrome DevTools)
- [ ] Optimize slow components

### Day 47: Bug Fixes
- [ ] Fix any discovered security issues
- [ ] Fix any performance bottlenecks
- [ ] Fix any UI bugs
- [ ] Test on multiple browsers (Chrome, Firefox, Safari)
- [ ] Test on mobile devices

### Day 48-49: Documentation
- [ ] Update README.md with setup instructions
- [ ] Document API endpoints
- [ ] Create user guide
- [ ] Record demo video
- [ ] Update CLAUDE.md status

---

## Week 8: Deployment (Dec 3-10)

### Day 50: Production Build
- [ ] Build client: `cd web/client && npm run build`
- [ ] Copy WASM files to build directory
- [ ] Test production build locally
- [ ] Optimize bundle size (code splitting)

### Day 51-52: Server Deployment
- [ ] Create `.env.production` file
- [ ] Set strong JWT_SECRET
- [ ] Create systemd service file
- [ ] Enable and start service
- [ ] Verify server runs on boot

### Day 53: Nginx Configuration
- [ ] Create nginx config file
- [ ] Setup SSL with Let's Encrypt
- [ ] Configure WebSocket proxy
- [ ] Configure CSP headers
- [ ] Test with SSL Labs

### Day 54: Go Live
- [ ] Deploy to ai.cpunk.io
- [ ] Update DNS (if needed)
- [ ] Test from external network
- [ ] Monitor logs for errors
- [ ] Share with team for testing

### Day 55-56: Post-Launch
- [ ] Monitor server performance
- [ ] Fix any production issues
- [ ] Gather user feedback
- [ ] Create issue list for Phase 2
- [ ] Celebrate! 🎉

---

## Merge to Main

### Pre-Merge Checklist
- [ ] All tests passing
- [ ] Security audit complete
- [ ] Performance benchmarks met
- [ ] Documentation updated
- [ ] Demo video recorded
- [ ] Team review completed

### Merge Command
```bash
git checkout main
git merge feature/web-messenger
git push origin main
git tag v0.2.0-alpha
git push origin v0.2.0-alpha
```

---

## Post-Launch Roadmap

### Phase 4B: Advanced Features (Future)
- [ ] File attachments
- [ ] Voice messages (WebRTC)
- [ ] Read receipts
- [ ] Message editing/deletion
- [ ] Search messages (client-side)
- [ ] Group messaging

### Phase 4C: Mobile PWA (Future)
- [ ] Service Workers (offline support)
- [ ] Push notifications
- [ ] Install prompt
- [ ] Camera integration (QR codes)
- [ ] Biometric authentication

### Phase 4D: Hybrid PQ-TLS (Future)
- [ ] Cloudflare PQ-TLS proxy
- [ ] Or compile nginx with liboqs
- [ ] Test X25519 + Kyber768 handshake

---

**Current Progress:** 0% (Planning Complete)
**Next Action:** Install Emscripten SDK (Day 1)

**Last Updated:** 2025-10-15
