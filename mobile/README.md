# DNA Messenger Mobile

Kotlin Multiplatform Mobile app for DNA Messenger with cpunk wallet integration.

## Architecture

```
mobile/
├── shared/              # Kotlin Multiplatform shared code
│   ├── src/
│   │   ├── commonMain/  # Shared business logic
│   │   ├── androidMain/ # Android-specific implementations
│   │   └── iosMain/     # iOS-specific implementations
│   └── build.gradle.kts
├── androidApp/          # Android UI (Jetpack Compose)
├── iosApp/              # iOS UI (SwiftUI)
└── native/              # C libraries and headers
    ├── libs/            # libdna_lib.a, libcellframe_minimal.a, etc.
    └── include/         # dna_api.h, wallet.h, etc.
```

## Technology Stack

### Shared Layer (Kotlin Multiplatform)
- **Crypto:** FFI/JNI to libdna_lib.a (Kyber512, Dilithium3, AES-256-GCM)
- **Database:** PostgreSQL client (ai.cpunk.io:5432)
- **Wallet:** FFI/JNI to libcellframe_minimal.a (transaction builder + RPC)
- **Networking:** Ktor for HTTP/WebSocket

### Android (Kotlin + Jetpack Compose)
- **UI:** Material 3 Design with cpunk themes
- **JNI:** Native library integration
- **Features:** Messaging, groups, wallet, notifications

### iOS (Swift + SwiftUI)
- **UI:** Native iOS components with cpunk themes
- **C Interop:** Bridging header for libdna.a
- **Features:** Messaging, groups, wallet, notifications

## Prerequisites

### Development Environment

**Linux (Development Machine):**
```bash
# Install JDK 17+
sudo apt install openjdk-17-jdk

# Install Gradle
sudo apt install gradle

# Install Kotlin
curl -s https://get.sdkman.io | bash
sdk install kotlin

# Install Android Studio
# Download from https://developer.android.com/studio
```

**Android SDK:**
```bash
# Via Android Studio or command line
sdkmanager "platform-tools" "platforms;android-34" "build-tools;34.0.0"
```

**iOS Development (macOS required for building):**
- Xcode 15+
- CocoaPods
- Kotlin Multiplatform Mobile plugin

## C Libraries Integration

### Required Libraries
Located in `/opt/dna-mobile/dna-messenger/build/`:
- `libdna_lib.a` - Core messaging + post-quantum crypto
- `libkyber512.a` - Kyber512 KEM
- `libdilithium.a` - Dilithium3 signatures (DNA)
- `libcellframe_dilithium.a` - Dilithium for Cellframe wallet
- `libcellframe_minimal.a` - Transaction builder
- OpenSSL libcrypto (system library)
- PostgreSQL libpq (system library)

### Headers
- `dna_api.h` - Main DNA Messenger API
- `wallet.h` - Cellframe wallet integration
- `cellframe_rpc.h` - RPC client for blockchain
- `cellframe_tx_builder_minimal.h` - Transaction builder
- `messenger.h` - Messenger functions

## Building

### Android
```bash
cd mobile
./gradlew :androidApp:assembleDebug
# APK: androidApp/build/outputs/apk/debug/androidApp-debug.apk
```

### iOS
```bash
cd mobile
./gradlew :shared:linkDebugFrameworkIos
# Then open iosApp/iosApp.xcodeproj in Xcode
```

## Project Setup (Current Status)

### ✅ Completed
- [x] Merged main branch into feature/mobile
- [x] Built all C libraries with wallet integration
- [x] Created mobile directory structure

### 🚧 In Progress
- [ ] KMM shared module setup
- [ ] JNI/FFI bindings for libdna and wallet
- [ ] Android app skeleton
- [ ] iOS app skeleton

### 📋 TODO
- [ ] Implement crypto wrapper (JNI/C interop)
- [ ] PostgreSQL client integration
- [ ] Wallet RPC integration
- [ ] Android UI (Jetpack Compose)
  - [ ] Login/Identity screen
  - [ ] Contact list
  - [ ] Chat interface
  - [ ] Group management
  - [ ] Wallet screen (balances, send, receive, history)
- [ ] iOS UI (SwiftUI)
  - [ ] Login/Identity screen
  - [ ] Contact list
  - [ ] Chat interface
  - [ ] Group management
  - [ ] Wallet screen
- [ ] Push notifications (FCM + APNS)
- [ ] Background sync
- [ ] Biometric authentication

## API Integration

### DNA Messenger API (dna_api.h)

```kotlin
// Kotlin wrapper for C API
class DNACrypto {
    external fun encryptMessage(
        plaintext: ByteArray,
        recipientPubKey: ByteArray,
        senderPrivKey: ByteArray
    ): ByteArray

    external fun decryptMessage(
        ciphertext: ByteArray,
        recipientPrivKey: ByteArray
    ): DecryptResult

    companion object {
        init {
            System.loadLibrary("dna_lib")
        }
    }
}
```

### Wallet API (wallet.h)

```kotlin
// Kotlin wrapper for wallet
class CellframeWallet {
    external fun listWallets(): List<WalletInfo>
    external fun getBalance(network: String, address: String): String
    external fun sendTransaction(
        from: String,
        to: String,
        amount: String,
        token: String,
        network: String
    ): TransactionResult

    companion object {
        init {
            System.loadLibrary("cellframe_minimal")
        }
    }
}
```

## PostgreSQL Integration

**Database:** ai.cpunk.io:5432

**Tables:**
- `messages` - Encrypted messages
- `contacts` - Public keys and user info
- `groups` - Group metadata
- `group_members` - Group membership

**Kotlin Library:** [postgresql-async](https://github.com/jasync-sql/jasync-sql)

```kotlin
// Shared code (commonMain)
expect class DatabaseClient {
    suspend fun getMessages(contactId: String): List<Message>
    suspend fun saveMessage(message: Message)
}

// Android implementation (androidMain)
actual class DatabaseClient {
    private val connection = PostgreSQLConnectionBuilder.createConnectionPool(
        "jdbc:postgresql://ai.cpunk.io:5432/dna_messenger"
    )
    // ...
}
```

## Theme System

Match Qt desktop themes:
- **cpunk.io:** Dark teal background (#0a1e1e), cyan accents (#00d4ff)
- **cpunk.club:** Dark brown background (#1a0f0a), orange accents (#ff6b35)

## Security Considerations

- Private keys stored in Android Keystore / iOS Keychain
- BIP39 mnemonic backup
- Biometric authentication for wallet operations
- Encrypted local database (SQLCipher)
- Certificate pinning for PostgreSQL/RPC connections

## Testing

```bash
# Run shared tests
./gradlew :shared:test

# Run Android tests
./gradlew :androidApp:connectedAndroidTest

# Run iOS tests
./gradlew :shared:iosTest
```

## Release Builds

### Android
```bash
./gradlew :androidApp:bundleRelease
# AAB: androidApp/build/outputs/bundle/release/androidApp-release.aab
```

### iOS
- Build in Xcode with Release configuration
- Archive and submit to App Store Connect

## Resources

- **KMM Docs:** https://kotlinlang.org/docs/multiplatform-mobile-getting-started.html
- **JNI Guide:** https://developer.android.com/training/articles/perf-jni
- **C Interop (iOS):** https://kotlinlang.org/docs/native-c-interop.html
- **Jetpack Compose:** https://developer.android.com/jetpack/compose
- **SwiftUI:** https://developer.apple.com/documentation/swiftui

---

**Current Phase:** Setup and scaffolding
**Next Steps:** Configure build.gradle.kts for shared module with JNI/C interop
