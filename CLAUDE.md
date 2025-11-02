# DNA Messenger - Development Guidelines for Claude AI

**Last Updated:** 2025-11-02
**Project:** DNA Messenger (Post-Quantum Encrypted Messenger)
**Current Phase:** Phase 5 (Web Messenger) - Phase 4, 8, 9.1, 9.2 Complete

---

## Project Overview

DNA Messenger is a post-quantum end-to-end encrypted messaging platform with cpunk wallet integration. It uses Kyber512 for key encapsulation, Dilithium3 for signatures, and AES-256-GCM for symmetric encryption.

**Key Features:**
- End-to-end encrypted messaging
- Group chats with member management
- cpunk wallet integration (CPUNK, CELL, KEL tokens)
- P2P messaging with DHT-based peer discovery (OpenDHT)
- Offline message queueing (7-day DHT storage)
- Cross-platform (Linux, Windows)
- Qt5 GUI with theme support
- BIP39 recovery phrases

---

## Current Architecture

### Components
1. **Core Library** (`libdna.a`)
   - Post-quantum cryptography (Kyber512, Dilithium3)
   - Memory-based encryption/decryption API
   - Multi-recipient support

2. **CLI Client** (`dna_messenger`)
   - Command-line interface
   - PostgreSQL message storage
   - Contact management

3. **GUI Client** (`dna_messenger_gui`)
   - Qt5 desktop application
   - Modern card-based UI
   - Theme system (cpunk.io cyan, cpunk.club orange)
   - Integrated wallet with transaction history

4. **Wallet Integration**
   - Cellframe wallet file support (.dwallet format)
   - RPC integration with Cellframe node
   - Transaction builder and signing
   - Balance and history queries

5. **P2P Transport Layer** (Phase 9.1 & 9.2)
   - OpenDHT for peer discovery and storage
   - Direct peer-to-peer TCP connections (port 4001)
   - DHT-based offline message queueing
   - 3 public bootstrap nodes (US/EU)
   - Automatic message delivery with 2-minute polling

### Directory Structure
```
/opt/dna-messenger/
├── crypto/                  # Cryptography libraries
│   ├── dilithium/          # Dilithium3 signatures
│   └── kyber512/           # Kyber512 key encapsulation
├── dht/                     # DHT layer (Phase 9.1 & 9.2)
│   ├── dht_context.*       # OpenDHT integration
│   └── dht_offline_queue.* # Offline message queueing
├── p2p/                     # P2P transport layer (Phase 9.1)
│   ├── p2p_transport.*     # TCP connections + DHT
│   └── test_p2p_basic.c    # P2P transport tests
├── gui/                     # Qt5 GUI application
│   ├── MainWindow.*        # Main chat window
│   ├── WalletDialog.*      # Wallet balance & transactions
│   ├── SendTokensDialog.*  # Send tokens form
│   ├── ReceiveDialog.*     # Receive with QR codes
│   ├── TransactionHistoryDialog.* # Full transaction list
│   └── ThemeManager.*      # Global theme management
├── wallet.c/h              # Cellframe wallet integration
├── cellframe_rpc.c/h       # Cellframe RPC client
├── cellframe_tx_builder_minimal.c/h # Transaction builder
├── messenger_p2p.*         # P2P messaging integration
├── dna_api.h               # Public library API
└── CMakeLists.txt          # Build configuration
```

---

## Development Guidelines

### 1. Code Style
- **C Code:** Follow existing style (K&R-ish with 4-space indentation)
- **C++ Code (Qt):** Qt coding conventions, camelCase for methods
- **Comments:** Use clear, concise comments for complex logic
- **Memory Management:** Always free allocated memory, check for NULL

### 2. Cryptography
- **DO NOT modify crypto primitives** without expert review
- Use existing API functions from `dna_api.h`
- All encryption/decryption must use memory-based operations
- Never log or print keys or decrypted messages

### 3. Database
- PostgreSQL for message storage (ai.cpunk.io:5432)
- Schema: `messages`, `contacts`, `groups`, `group_members`
- Use prepared statements to prevent SQL injection
- Handle connection failures gracefully

### 4. GUI Development (Qt5)
- Use Qt signals/slots for event handling
- Apply themes via `ThemeManager::instance()`
- All dialogs should be theme-aware
- Use `setAttribute(Qt::WA_DeleteOnClose)` for modal dialogs
- Handle window focus and activation properly

### 5. Wallet Integration
- Read Cellframe wallet files from `~/.dna/` or system wallet dir
- Use `cellframe_rpc.h` API for RPC calls
- All token amounts are strings to preserve precision
- Use smart decimal formatting (8 decimals for tiny amounts)
- Transaction builder uses minimal serialization (no JSON-C in builder)

### 6. Cross-Platform Support
- **Linux:** Primary development platform
- **Windows:** Cross-compile using MXE (M cross environment)
- Use CMake for build configuration
- Avoid platform-specific code when possible
- Test on both platforms before committing

---

## Phase 8: cpunk Wallet Integration (COMPLETE)

**Status:** ✅ Complete (2025-10-23)

### What Was Implemented
1. **WalletDialog** - Main wallet view
   - Card-based token display (CPUNK, CELL, KEL)
   - Balance queries via Cellframe RPC
   - Recent 3 transactions display
   - 4-button layout: Send, Receive, DEX, History

2. **SendTokensDialog** - Send tokens
   - Transaction builder integration
   - UTXO query and selection
   - Network fee handling (0.002 CELL)
   - Dilithium signature generation
   - RPC transaction submission

3. **ReceiveDialog** - Receive tokens
   - Display wallet addresses per network
   - QR code generation
   - Easy copy-to-clipboard

4. **TransactionHistoryDialog** - Full transaction list
   - All transactions with pagination
   - Color-coded arrows (green incoming, red outgoing)
   - Status display (ACCEPTED in green, DECLINED in red)
   - Smart timestamp formatting

5. **ThemeManager** - Global theme system
   - Singleton pattern for theme management
   - Signal-based theme updates
   - Support for cpunk.io (cyan) and cpunk.club (orange)

### Key Technical Decisions
- **No wallet selection in send dialog:** Already in that wallet context
- **Smart decimal formatting:** 8 decimals for tiny amounts, 2 for normal
- **Direct RPC integration:** No external wallet utilities needed
- **Theme-aware:** All wallet dialogs respond to theme changes
- **Transaction builder:** Minimal serialization for cross-platform compatibility

---

## Phase 9.1 & 9.2: P2P Transport & Offline Message Queue (COMPLETE)

**Status:** ✅ Complete (2025-11-02)

### Phase 9.1: P2P Transport Layer (COMPLETE)
**What Was Implemented:**
1. **OpenDHT Integration** - DHT-based peer discovery
   - 3 public bootstrap nodes (US/EU)
   - Peer registration and lookup via DHT
   - SHA256-based peer keys

2. **P2P Transport** - Direct peer-to-peer messaging
   - TCP connections on port 4001
   - Post-quantum encryption (Kyber512 + AES-256-GCM)
   - Connection management and presence updates

3. **Hybrid Delivery** - Multi-tier message delivery
   - Primary: Direct P2P (if peer online)
   - Secondary: DHT queue (if peer offline)
   - Tertiary: PostgreSQL fallback

### Phase 9.2: Offline Message Queueing (COMPLETE)
**What Was Implemented:**
1. **DHT Offline Queue** (`dht/dht_offline_queue.c/h`)
   - Binary message serialization with magic bytes
   - 7-day TTL with configurable expiry
   - SHA256-based queue keys: `hash(recipient + ":offline_queue")`
   - Single-queue-per-recipient architecture

2. **Message Storage Protocol**
   - Encrypted message storage in DHT
   - Automatic append to existing queue
   - Platform-independent network byte order (htonl/ntohl)
   - Cross-platform support (Windows/Linux)

3. **Automatic Retrieval**
   - 2-minute polling timer in GUI (`MainWindow.cpp`)
   - Automatic delivery to PostgreSQL when retrieved
   - Queue clearing after successful delivery
   - Message expiry handling

4. **Integration**
   - `p2p_transport.c` - Queue/retrieve API
   - `messenger_p2p.c` - Hybrid send flow
   - `gui/MainWindow.cpp` - Automatic polling

### Key Technical Decisions
- **Single queue per recipient:** Append all messages to one DHT entry (workaround for OpenDHT C wrapper limitations)
- **Binary serialization:** Custom format with magic bytes (0x444E4120 = "DNA ")
- **7-day TTL:** Configurable via `offline_ttl_seconds` parameter
- **2-minute polling:** Balance between responsiveness and DHT load
- **Hybrid delivery:** Ensures no message loss across all failure scenarios

### Files Modified/Created
- `dht/dht_offline_queue.h` (NEW - 170 lines)
- `dht/dht_offline_queue.c` (NEW - 650 lines)
- `dht/CMakeLists.txt` (MODIFIED)
- `p2p/p2p_transport.h` (MODIFIED - added queue API)
- `p2p/p2p_transport.c` (MODIFIED - implemented queue/retrieve)
- `messenger_p2p.h` (MODIFIED - added check API)
- `messenger_p2p.c` (MODIFIED - hybrid send flow)
- `gui/MainWindow.h` (MODIFIED - added timer)
- `gui/MainWindow.cpp` (MODIFIED - polling callback)

---

## Phase 5: Web Messenger (IN PROGRESS)

**Status:** 🚧 Active development on `feature/web-messenger` branch

### What's Done
- [x] DNA API compiled to WebAssembly (dna_wasm.js/wasm)
- [x] JavaScript wrapper functions
- [x] Emscripten toolchain setup

### What's Next
- [ ] HTML5/CSS3 responsive UI
- [ ] Browser-based messaging interface
- [ ] IndexedDB for key storage
- [ ] Real-time updates (WebSocket)

---

## Phase 10: DNA Board - Censorship-Resistant Social Media (PLANNED)

**Status:** 📋 Planning (Post-Phases 7-9)
**Timeline:** 12 weeks
**Prerequisites:** Distributed validator storage, DNA-Keyserver merge, Offline messaging

### Overview

DNA Board is a **censorship-resistant social media platform** built on cpunk validator network:

**Core Principles:**
1. **NO CENSORSHIP** - Content cannot be removed (no deletion endpoint)
2. **PoH Required to Post** - Only verified humans (PoH ≥70) can create posts
3. **Open Responses** - Anyone can reply (no PoH for replies)
4. **Community Voting** - Thumbs up/down (FREE) to surface quality
5. **Burn Economics** - All fees burned (deflationary)
6. **Validator Rewards** - DAO pool distribution

### Architecture

```
Self-Healing Validator Network (3-Replica)
┌─────────────────────────────────────┐
│  Post → Primary Validator           │
│  ↓ Replicate to 2 more (lowest GB)  │
│  ↓ 3 Validators store content       │
│  ↓ Heartbeat monitoring (30s)       │
│  ↓ Auto-heal if validator offline   │
│  ↓ Back to 3 replicas               │
└─────────────────────────────────────┘
```

### Content & Economics

| Type | Burn Fee | PoH Required | Max Size |
|------|----------|--------------|----------|
| Post | 1 CPUNK | Yes (≥70) | 5,000 chars |
| Image | 2 CPUNK | Yes | 5 MB |
| Video | 5 CPUNK | Yes | 50 MB |
| Reply | 0.5 CPUNK | **No** | 2,000 chars |
| Vote | **FREE** | No | N/A |

**Estimated burn:** 16M CPUNK/year (deflationary pressure)

### Proof of Humanity (PoH)

**Who Can Post:** humanity_score ≥ 70

**Verification Tiers:**
- Auto-Verified (75-89): Behavioral algorithm
- Staked-Verified (90-94): Auto + 100 CPUNK stake
- DAO-Vouched (95-99): 3 human vouches + DAO
- Celebrity (100): Public figure + DAO

**Bot Detection:** Single-destination sends, mechanical timing
**Human Patterns:** Multiple services, irregular timing, staking

### Implementation (12 Weeks)

- **Weeks 1-2:** Validator backend + PoH scoring
- **Weeks 3-4:** Gossip protocol + 3-replica replication
- **Weeks 5-6:** Voting system + feed ranking
- **Weeks 7-8:** Qt GUI (DNABoardTab, ComposeDialog, PostWidget)
- **Weeks 9-10:** Media upload + wallet integration
- **Weeks 11-12:** Testing + mainnet launch

### Key Features

**Censorship Resistance:**
- No deletion endpoint (content permanent)
- 3-validator replication across jurisdictions
- No central authority
- Gossip protocol ensures propagation

**Quality Control:**
- PoH prevents bot spam
- Community voting surfaces quality
- Ranking algorithm (votes × humanity × time)

**Economic Model:**
- All fees burned (deflationary)
- Validators funded by DAO pool (100M Year 1, halvening)

### Design Document

Full specification: `/DNA_BOARD_PHASE10_PLAN.md`

---

## Phase 11: Post-Quantum Voice/Video Calls (PLANNED)

**Status:** 📋 Research & Planning
**Timeline:** ~20 weeks (5 months)
**Prerequisites:** Phase 9.1 (P2P Transport Layer)
**Design Doc:** `/futuredesign/VOICE-VIDEO-DESIGN.md`

### Overview

Fully quantum-safe voice/video calls using Kyber512 via DNA messaging + SRTP (bypasses WebRTC's quantum-vulnerable DTLS).

**Technology:** libnice, libsrtp2, libopus, libvpx/libx264, PortAudio
**Security:** Kyber512 key exchange, Dilithium3 signatures, forward secrecy, SAS verification

See design doc for complete technical specification

---

## Common Tasks

### Adding a New Feature
1. Check current phase in ROADMAP.md
2. Follow existing code patterns
3. Update CMakeLists.txt if adding new files
4. Test on Linux first, then Windows
5. Update documentation (README.md, ROADMAP.md)

### Building the Project
**Linux:**
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

**Windows (cross-compile from Linux):**
```bash
./build-cross-compile.sh windows-x64
```

### Running Tests
```bash
# CLI messenger
./build/dna_messenger --help

# GUI messenger
./build/gui/dna_messenger_gui
```

---

## API Documentation

### DNA Core API (`dna_api.h`)
```c
// Context management
dna_context_t* dna_context_new(void);
void dna_context_free(dna_context_t *ctx);

// Encryption
int dna_encrypt_message(
    const uint8_t *plaintext, size_t len,
    const public_key_t *recipient_key,
    dna_buffer_t *output);

// Decryption
int dna_decrypt_message(
    const uint8_t *ciphertext, size_t len,
    const private_key_t *my_key,
    dna_buffer_t *output);
```

### Cellframe RPC API (`cellframe_rpc.h`)
```c
// RPC request structure
typedef struct {
    const char *method;
    const char *subcommand;
    json_object *arguments;
    int id;
} cellframe_rpc_request_t;

// Make RPC call
int cellframe_rpc_call(
    cellframe_rpc_request_t *req,
    cellframe_rpc_response_t **response);

// Free response
void cellframe_rpc_response_free(cellframe_rpc_response_t *resp);
```

### Wallet API (`wallet.h`)
```c
// Load Cellframe wallets
int wallet_list_cellframe(wallet_list_t **list);

// Get wallet address
int wallet_get_address(
    const cellframe_wallet_t *wallet,
    const char *network,
    char *address_out);

// Free wallet list
void wallet_list_free(wallet_list_t *list);
```

---

## Testing Guidelines

### Manual Testing
1. **Messaging:**
   - Send message between two users
   - Verify encryption/decryption
   - Check delivery receipts

2. **Groups:**
   - Create group with multiple members
   - Send messages to group
   - Add/remove members

3. **Wallet:**
   - View balances (should match Cellframe CLI)
   - Send transaction (verify on blockchain explorer)
   - Check transaction history

### Debug Output
- Use `printf("[DEBUG] ...")` for development
- Remove debug output before committing
- Use descriptive prefixes: `[DEBUG TX]`, `[DEBUG RPC]`

---

## Common Issues and Solutions

### Wallet Balance Shows 0.00
- Check Cellframe node is running (port 8079)
- Verify RPC is enabled in Cellframe config
- Check wallet file exists in `~/.dna/` or `/opt/cellframe-node/var/lib/wallet/`

### Transaction Fails to Send
- Ensure sufficient balance + fee
- Check UTXO query returns results
- Verify network fee address is correct
- Check signature is generated correctly

### Theme Not Applied
- Ensure ThemeManager is initialized before dialog
- Connect to `themeChanged` signal
- Call `applyTheme()` in constructor

### Windows Cross-Compile Fails
- Check MXE is installed (`~/.cache/mxe/`)
- Verify MXE dependencies are built
- Check CMake toolchain file path

---

## Security Considerations

### Current Security Model
- Post-quantum encryption (Kyber512 + AES-256-GCM)
- Message signatures (Dilithium3)
- End-to-end encryption (server cannot read)
- Private keys stored locally (`~/.dna/`)

### Known Limitations
- No forward secrecy (planned Phase 7)
- Metadata not protected (server sees sender/receiver)
- No multi-device sync yet
- No disappearing messages

### Best Practices
- Never log private keys or decrypted messages
- Validate all inputs (addresses, amounts, signatures)
- Check return codes from crypto functions
- Use secure memory management (mlock in future)

---

## Git Workflow

### Branching
- `main` - Stable releases
- `feature/*` - New features (e.g., `feature/web-messenger`)
- `fix/*` - Bug fixes

### Commit Messages
```bash
# Good commit message format:
Short summary (50 chars or less)

Detailed explanation if needed:
- What changed
- Why it changed
- Any breaking changes

🤖 Generated with Claude Code
Co-Authored-By: Claude <noreply@anthropic.com>
```

### Before Committing
1. Test on Linux
2. Cross-compile for Windows
3. Remove debug output
4. Update documentation if needed
5. Check for memory leaks (valgrind)

---

## Resources

### Documentation
- **Main README:** `/opt/dna-messenger/README.md`
- **Roadmap:** `/opt/dna-messenger/ROADMAP.md`
- **API Docs:** `/opt/dna-messenger/dna_api.h` (inline comments)

### External Links
- **Cellframe Docs:** https://wiki.cellframe.net
- **Cellframe Dev Wiki:** https://dev-wiki.cellframe.net
- **Qt5 Docs:** https://doc.qt.io/qt-5/
- **Kyber:** https://pq-crystals.org/kyber/
- **Dilithium:** https://pq-crystals.org/dilithium/

### Repositories
- **GitLab (Primary):** https://gitlab.cpunk.io/cpunk/dna-messenger
- **GitHub (Mirror):** https://github.com/nocdem/dna-messenger

---

## Contact and Support

- **cpunk.io:** https://cpunk.io
- **cpunk.club:** https://cpunk.club
- **Telegram:** https://web.telegram.org/k/#@chippunk_official

---

## Phase 6: Mobile Applications (IN PROGRESS - feature/mobile branch)

**Status:** 🚧 Active development on `feature/mobile` branch

### Overview

Mobile apps using Kotlin Multiplatform to share business logic across Android and iOS, while using the same C cryptographic libraries as desktop.

### Three-Layer Mobile Stack

```
┌─────────────────────────────────────────┐
│  UI Layer                               │
│  - Android: Jetpack Compose             │
│  - iOS: SwiftUI                         │
│    (mobile/androidApp, mobile/iosApp)   │
└─────────────────┬───────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│  Business Logic (Shared)                │
│  - Kotlin Multiplatform                 │
│    (mobile/shared/commonMain)           │
│  - API Clients (Auth, Messages, etc.)   │
│  - Repository pattern                   │
└─────────────────┬───────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│  Native C Libraries (SHARED)            │
│  - libdna_lib.a (crypto + messaging)    │
│  - libkyber512.a, libdilithium.a        │
│  - libcellframe_minimal.a (wallet)      │
│  - Via JNI (Android) / C interop (iOS)  │
└─────────────────────────────────────────┘
```

### Mobile Directory Structure

```
mobile/
├── docs/                      # Beginner-friendly guides
│   ├── ANDROID_DEVELOPMENT_GUIDE.md  # 120-page comprehensive guide
│   ├── DEVELOPMENT_TODO.md           # 12-week roadmap (84 tasks)
│   └── JNI_INTEGRATION_TUTORIAL.md   # JNI patterns & examples
├── shared/                    # Kotlin Multiplatform module
│   ├── src/commonMain/       # Shared Kotlin code
│   │   └── kotlin/io/cpunk/dna/domain/
│   │       ├── ApiConfig.kt
│   │       ├── AuthApiClient.kt
│   │       ├── MessagesApiClient.kt
│   │       ├── ContactsApiClient.kt
│   │       ├── GroupsApiClient.kt
│   │       ├── LoggingApiClient.kt
│   │       ├── DatabaseRepository.kt
│   │       ├── MessageRepository.kt
│   │       └── models/
│   ├── src/androidMain/      # Android-specific code
│   │   ├── cpp/             # JNI bridge to C libraries
│   │   │   ├── dna_jni.cpp
│   │   │   ├── jni_utils.cpp
│   │   │   ├── boringssl_compat.c
│   │   │   └── CMakeLists.txt
│   │   └── kotlin/          # Android actual implementations
│   └── src/iosMain/         # iOS-specific code (direct C interop)
├── androidApp/              # Android application
│   └── src/main/java/io/cpunk/dna/android/
│       ├── MainActivity.kt
│       ├── Navigation.kt
│       └── ui/screen/
│           ├── login/
│           ├── home/
│           ├── chat/
│           ├── settings/
│           └── wallet/
├── iosApp/                  # iOS application (SwiftUI)
├── build.gradle.kts         # Root build configuration
├── gradle.properties        # Gradle settings
└── *.sh                     # Build scripts

```

### Mobile Build Commands

**Prerequisites:**
```bash
# Build C libraries for Android first
cd /opt/dna-mobile/dna-messenger
mkdir build && cd build
cmake ..
make -j$(nproc)

# Install Android SDK/NDK
# See mobile/docs/ANDROID_DEVELOPMENT_GUIDE.md
```

**Android:**
```bash
cd mobile

# Debug APK
./gradlew :androidApp:assembleDebug
# Output: androidApp/build/outputs/apk/debug/androidApp-debug.apk

# Release bundle (for Play Store)
./gradlew :androidApp:bundleRelease
# Output: androidApp/build/outputs/bundle/release/

# Install on device/emulator
adb install -r androidApp/build/outputs/apk/debug/androidApp-debug.apk

# View logs
adb logcat -s "DNAMessenger"
```

**iOS:**
```bash
cd mobile

# Build shared framework
./gradlew :shared:linkDebugFrameworkIos
# Output: shared/build/bin/ios/debugFramework/shared.framework

# Then open iosApp/iosApp.xcodeproj in Xcode
```

### Mobile Development Workflow

**Important:** Follow `mobile/docs/DEVELOPMENT_TODO.md` for structured 12-week plan.

**Quick workflow:**
1. Build C libraries for Android (see TODO Week 1, Day 5)
2. Create JNI wrapper in `mobile/shared/src/androidMain/cpp/dna_jni.cpp`
3. Implement Kotlin actual classes in `mobile/shared/src/androidMain/kotlin/`
4. Build Android app: `./gradlew :androidApp:assembleDebug`
5. Test on emulator

**JNI Bridge Pattern:**
```
Kotlin → JNI (C++) → C Library → JNI → Kotlin
```

### API Clients (Implemented)

All API clients are implemented in `mobile/shared/src/commonMain/kotlin/io/cpunk/dna/domain/`:

1. **AuthApiClient** - Authentication (login, register, restore from seed)
2. **MessagesApiClient** - Send/receive messages, conversations
3. **ContactsApiClient** - Save, load, delete contacts
4. **GroupsApiClient** - Create groups, manage members
5. **LoggingApiClient** - Event logging, stats

**HTTP Implementation:**
- Android: Ktor client
- iOS: Ktor client with Darwin engine
- Base URL configurable via `ApiConfig`

### Kotlin Multiplatform `expect`/`actual` Pattern

```kotlin
// commonMain - Interface
expect class DNAMessenger {
    fun encryptMessage(plaintext: ByteArray, recipientPubKey: ByteArray): Result<ByteArray>
}

// androidMain - JNI implementation
actual class DNAMessenger {
    private external fun nativeEncrypt(plaintext: ByteArray, recipientPubKey: ByteArray): ByteArray

    actual fun encryptMessage(...): Result<ByteArray> {
        return runCatching { nativeEncrypt(...) }
    }

    companion object {
        init { System.loadLibrary("dna_jni") }
    }
}

// iosMain - C interop
actual class DNAMessenger {
    actual fun encryptMessage(...): Result<ByteArray> {
        return runCatching {
            dna_encrypt_message_raw(...) // Direct C call
        }
    }
}
```

### Mobile-Specific Files

**Must Read:**
- `mobile/docs/ANDROID_DEVELOPMENT_GUIDE.md` - Comprehensive 120-page guide
- `mobile/docs/DEVELOPMENT_TODO.md` - 12-week roadmap with 84 tasks
- `mobile/docs/JNI_INTEGRATION_TUTORIAL.md` - JNI patterns and examples

**Build Configuration:**
- `mobile/shared/build.gradle.kts` - Shared module build
- `mobile/androidApp/build.gradle.kts` - Android app build
- `mobile/shared/src/androidMain/cpp/CMakeLists.txt` - JNI native build

### Debugging JNI Crashes

```bash
# View native stack trace
adb logcat | grep -A 50 "FATAL EXCEPTION"

# Or use ndk-stack (if symbols available)
adb logcat | ndk-stack -sym obj/local/arm64-v8a/
```

**Common JNI issues:**
- Memory leaks (forgot to free)
- Didn't release JNI array
- Null pointer dereference
- Buffer overflow

### Mobile Theme Colors (Same as Desktop)

**cpunk.io (Cyan):**
- Background: `#0a1e1e` (dark teal)
- Primary: `#00d4ff` (cyan)
- Secondary: `#14a098` (teal)

**cpunk.club (Orange):**
- Background: `#1a0f0a` (dark brown)
- Primary: `#ff6b35` (orange)
- Secondary: `#f7931e` (amber)

---

## Version History

| Version | Date | Description |
|---------|------|-------------|
| 0.1.0 | 2025-10-14 | Initial fork from QGP |
| 0.2.0 | 2025-10-15 | Library API complete |
| 0.3.0 | 2025-10-16 | CLI messenger complete |
| 0.4.0 | 2025-10-17 | Desktop GUI with groups |
| 0.8.0 | 2025-10-23 | cpunk Wallet integration |
| 0.5.0 | TBD | Web messenger (in progress) |

**Current Version:** 0.1.120+ (auto-incremented)

---

**Happy Coding!**

When in doubt, check existing code patterns and follow the established conventions. The project prioritizes simplicity, security, and cross-platform compatibility.
