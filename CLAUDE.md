# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

**Last Updated:** 2025-10-26
**Branch:** feature/mobile
**Current Phase:** Phase 6 (Mobile Applications) - Kotlin Multiplatform Mobile Setup

---

## Repository Overview

DNA Messenger is a post-quantum encrypted messaging platform with cpunk wallet integration. This branch (`feature/mobile`) contains both desktop (Qt5) and mobile (Kotlin Multiplatform) implementations sharing the same C cryptographic libraries.

**Key Features:**
- Post-quantum encryption (Kyber512 + Dilithium3)
- End-to-end encrypted messaging with groups
- cpunk wallet (CPUNK, CELL, KEL tokens via Cellframe blockchain)
- BIP39 recovery phrases
- Cross-platform: Desktop (Linux/Windows) + Mobile (Android/iOS)

---

## Architecture

### Three-Layer Stack

```
┌─────────────────────────────────────────┐
│  UI Layer                               │
│  - Desktop: Qt5 (gui/)                  │
│  - Mobile: Jetpack Compose + SwiftUI    │
│           (mobile/androidApp, iosApp)   │
└─────────────────┬───────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│  Business Logic                         │
│  - Desktop: C++ (gui/)                  │
│  - Mobile: Kotlin Multiplatform         │
│           (mobile/shared/commonMain)    │
└─────────────────┬───────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│  Native C Libraries (SHARED)            │
│  - libdna_lib.a (crypto + messaging)    │
│  - libkyber512.a, libdilithium.a        │
│  - libcellframe_minimal.a (wallet)      │
└─────────────────────────────────────────┘
```

### Key Architectural Points

**C Library Layer:**
- All cryptography in C for performance and security
- Memory-based API (no file I/O in crypto operations)
- Multi-recipient encryption via Kyber512
- Dilithium3 signatures for authentication
- Two separate Dilithium implementations:
  - `crypto/dilithium/` - For DNA message signing
  - `crypto/cellframe_dilithium/` - For Cellframe wallet signatures

**Desktop (Qt5):**
- Directly links C libraries
- PostgreSQL for message storage (ai.cpunk.io:5432)
- Cellframe RPC for wallet operations
- Themes: cpunk.io (cyan #00d4ff) and cpunk.club (orange #ff6b35)

**Mobile (Kotlin Multiplatform):**
- Shared business logic in `mobile/shared/commonMain/`
- Android: JNI wrapper calls C libraries
- iOS: Direct C interop (no JNI needed)
- Same PostgreSQL database as desktop
- Same theme colors as desktop

---

## Directory Structure

```
/opt/dna-mobile/dna-messenger/
├── crypto/
│   ├── kyber512/           # Post-quantum KEM
│   ├── dilithium/          # PQ signatures (DNA)
│   └── cellframe_dilithium/# PQ signatures (wallet)
├── gui/                    # Qt5 desktop app
│   ├── MainWindow.cpp      # Main chat interface
│   ├── WalletDialog.cpp    # Wallet UI
│   └── SendTokensDialog.cpp# Transaction builder UI
├── mobile/                 # Mobile apps
│   ├── docs/              # Beginner-friendly guides
│   │   ├── ANDROID_DEVELOPMENT_GUIDE.md
│   │   ├── DEVELOPMENT_TODO.md (12-week plan)
│   │   └── JNI_INTEGRATION_TUTORIAL.md
│   ├── shared/            # Kotlin Multiplatform
│   │   ├── src/commonMain/    # Shared logic
│   │   ├── src/androidMain/   # Android (JNI)
│   │   └── src/iosMain/       # iOS (C interop)
│   ├── androidApp/        # Android UI
│   └── iosApp/            # iOS UI
├── dna_api.h              # Public C API
├── wallet.h               # Cellframe wallet API
├── cellframe_rpc.h        # Blockchain RPC client
└── CMakeLists.txt         # Desktop + C library build
```

---

## Build Commands

### Desktop (Qt5 GUI + CLI)

**Linux:**
```bash
# Build C libraries + desktop apps
mkdir build && cd build
cmake ..
make -j$(nproc)

# Outputs:
# - build/dna_messenger (CLI)
# - build/gui/dna_messenger_gui (Qt GUI)
# - build/libdna_lib.a (C library)
```

**Windows (Cross-compile from Linux):**
```bash
# Requires MXE (M cross environment)
./build-cross-compile.sh windows-x64
# Output: dist/windows-x64/
```

### Mobile (Android + iOS)

**Prerequisites:**
```bash
# Build C libraries first (required for mobile)
cd build && make
```

**Android:**
```bash
cd mobile

# Debug APK
./gradlew :androidApp:assembleDebug
# Output: androidApp/build/outputs/apk/debug/androidApp-debug.apk

# Release bundle
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

### Testing

**Desktop:**
```bash
# No formal test suite yet for desktop
# Manual testing via CLI or GUI
```

**Mobile:**
```bash
cd mobile

# Shared logic tests
./gradlew :shared:test

# Android instrumented tests (requires emulator)
./gradlew :androidApp:connectedAndroidTest

# iOS tests (requires macOS)
./gradlew :shared:iosTest
```

---

## Critical APIs

### DNA Messenger API (dna_api.h)

**Core Functions:**
- `dna_encrypt_message_raw()` - Encrypt with Kyber512 + AES-256-GCM
- `dna_decrypt_message_raw()` - Decrypt and verify signature
- `dna_keygen_encryption()` - Generate Kyber512 keypair
- `dna_keygen_signing()` - Generate Dilithium3 keypair
- Error codes: `DNA_OK`, `DNA_ERROR_CRYPTO`, etc.

**Multi-Recipient Encryption:**
Messages can be encrypted for multiple recipients in a single ciphertext. Each recipient gets their own Kyber512-wrapped DEK (data encryption key). The actual message is encrypted once with AES-256-GCM.

### Wallet API (wallet.h, cellframe_rpc.h)

**Wallet Operations:**
- `wallet_read_cellframe()` - Read .dwallet files
- `wallet_get_address()` - Get address for network
- `cellframe_rpc_call()` - Make RPC request to Cellframe node
- `cellframe_tx_builder_*` - Build and sign transactions

**Supported Tokens:** CPUNK, CELL, KEL (all CF20 on Cellframe blockchain)

### Database Schema

**PostgreSQL (ai.cpunk.io:5432):**
- `messages` - Encrypted message blobs
- `contacts` - Public keys and metadata
- `groups` - Group info
- `group_members` - Membership table

---

## Development Workflow

### For Desktop Development

1. Modify C library or Qt GUI files
2. Rebuild: `cd build && make`
3. Test: `./gui/dna_messenger_gui`
4. Commit changes

### For Mobile Development

**Important:** Follow `mobile/docs/DEVELOPMENT_TODO.md` for structured 12-week plan.

**Quick workflow:**
1. Build C libraries for Android: See TODO Week 1, Day 5
2. Create JNI wrapper: `mobile/shared/src/androidMain/cpp/dna_jni.cpp`
3. Implement Kotlin actual classes: `mobile/shared/src/androidMain/kotlin/`
4. Build Android app: `./gradlew :androidApp:assembleDebug`
5. Test on emulator

**JNI Bridge Pattern:**
```
Kotlin → JNI (C++) → C Library → JNI → Kotlin
```

See `mobile/docs/JNI_INTEGRATION_TUTORIAL.md` for complete guide.

### Branch Strategy

- `main` - Stable desktop releases
- `feature/mobile` - Mobile development (this branch)
- Mobile changes must maintain desktop compatibility

**Rule:** Don't break desktop build when working on mobile code.

---

## Coding Conventions

### C Code (Crypto Libraries)

- K&R-ish style, 4-space indentation
- Free all allocated memory
- Never log private keys
- Use `dna_error_t` for error handling
- Comment complex crypto operations

**Example:**
```c
dna_error_t dna_encrypt_message_raw(
    dna_context_t *ctx,
    const uint8_t *plaintext,
    size_t plaintext_len,
    // ... more params
) {
    if (!plaintext) return DNA_ERROR_INVALID_ARG;

    // Generate DEK
    // Encrypt with AES-256-GCM
    // Wrap DEK with Kyber512
    // Sign with Dilithium3

    return DNA_OK;
}
```

### Kotlin (Mobile Shared Code)

Use `expect`/`actual` pattern for platform-specific code:

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
        init { System.loadLibrary("dna_lib") }
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

### Qt/C++ (Desktop GUI)

- Qt naming conventions (camelCase)
- Use signals/slots for events
- Theme via `ThemeManager::instance()`
- All dialogs should be theme-aware

### Theme Colors

**cpunk.io (Cyan):**
- Background: `#0a1e1e` (dark teal)
- Primary: `#00d4ff` (cyan)
- Secondary: `#14a098` (teal)

**cpunk.club (Orange):**
- Background: `#1a0f0a` (dark brown)
- Primary: `#ff6b35` (orange)
- Secondary: `#f7931e` (amber)

---

## Security Notes

**Private Key Storage:**
- Desktop: `~/.dna/` directory
- Android: Android Keystore (hardware-backed)
- iOS: iOS Keychain

**BIP39 Recovery:**
- 24-word mnemonic
- PBKDF2 derivation
- Deterministic key generation from seed

**Encryption Flow:**
1. Generate random DEK (32 bytes)
2. Encrypt message with AES-256-GCM (DEK as key)
3. For each recipient: Kyber512 encapsulation → wrap DEK
4. Sign entire blob with Dilithium3
5. Store: [header | wrapped_DEKs | nonce | ciphertext | tag | signature]

---

## Common Development Scenarios

### Adding a New C Function to Mobile

1. Declare in `dna_api.h`
2. Implement in C library
3. Create JNI wrapper in `mobile/shared/src/androidMain/cpp/dna_jni.cpp`
4. Declare `external fun` in Kotlin actual class
5. Call from Kotlin code

### Adding a New Screen to Android App

1. Create `@Composable` in `mobile/androidApp/src/main/java/io/cpunk/dna/android/ui/screen/`
2. Create ViewModel if needed
3. Add route to `Navigation.kt`
4. Use `DNAMessengerTheme` for styling

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

See `mobile/docs/JNI_INTEGRATION_TUTORIAL.md` for debugging guide.

---

## External Dependencies

**Desktop:**
- OpenSSL (AES, SHA256, random)
- PostgreSQL libpq
- Qt5 (GUI)
- libcurl (Cellframe RPC)
- json-c (JSON parsing)

**Mobile:**
- Android SDK/NDK
- Kotlin 1.9.20+
- Jetpack Compose
- PostgreSQL JDBC (Android)
- Ktor (HTTP client)

**All Platforms:**
- Kyber512 (vendored in `crypto/kyber512/`)
- Dilithium3 (vendored in `crypto/dilithium/`)

---

## Important Files

**Must Read (Mobile Development):**
- `mobile/docs/ANDROID_DEVELOPMENT_GUIDE.md` - Comprehensive 120-page guide
- `mobile/docs/DEVELOPMENT_TODO.md` - 12-week roadmap with 84 tasks
- `mobile/docs/JNI_INTEGRATION_TUTORIAL.md` - JNI patterns and examples

**API Headers:**
- `dna_api.h` - Core messaging API
- `wallet.h` - Wallet file operations
- `cellframe_rpc.h` - Blockchain RPC
- `messenger.h` - High-level messenger functions

**Configuration:**
- `CMakeLists.txt` - Desktop build
- `mobile/shared/build.gradle.kts` - Shared module build
- `mobile/androidApp/build.gradle.kts` - Android app build

---

## Version Information

**Versioning:** `0.1.x` (auto-incremented from git commit count)
- Major 0 = Alpha (breaking changes expected)
- Minor 1 = Current feature set
- Patch x = Git commit count

**Check version:**
```bash
# Desktop
./build/dna_messenger --version

# In code
dna_version() // Returns "0.1.0-alpha"
```

---

## Links

- **GitLab (Primary):** https://gitlab.cpunk.io/cpunk/dna-messenger
- **GitHub (Mirror):** https://github.com/nocdem/dna-messenger
- **Cellframe:** https://cellframe.net
- **cpunk:** https://cpunk.io

---

**Note:** This is the `feature/mobile` branch. Desktop development happens on `main`. Keep C libraries compatible with both.
