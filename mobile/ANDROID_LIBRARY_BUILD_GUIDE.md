# Android Library Build Guide

**Complexity:** High (4-8 hours)  
**Prerequisites:** Android NDK, CMake, patience  
**Status:** Requires manual cross-compilation

---

## 📋 What Needs to Be Built

The C libraries must be cross-compiled for Android ARM64 (and other ABIs). Here's what's required:

### Dependencies:

1. **OpenSSL/BoringSSL** (for AES, SHA256, random)
   - Desktop uses system OpenSSL
   - Android requires BoringSSL or cross-compiled OpenSSL

2. **Kyber512** (self-contained ✅)
   - No external dependencies
   - Can build independently

3. **Dilithium3** (self-contained ✅)
   - No external dependencies
   - Can build independently

4. **Main DNA Library** (depends on OpenSSL)
   - Uses OpenSSL for AES, SHA, random
   - Needs porting to BoringSSL APIs

---

## 🎯 Approach 1: Use BoringSSL (Recommended)

Android includes BoringSSL. Update the C code to use it.

### Step 1: Install BoringSSL for NDK

```bash
# BoringSSL is included in Android but not exposed
# Option A: Build BoringSSL separately
cd /tmp
git clone https://boringssl.googlesource.com/boringssl
cd boringssl
mkdir build-android && cd build-android

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=/home/cc/android-sdk/ndk/25.2.9519653/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)

# Libraries will be in crypto/libcrypto.a and ssl/libssl.a
```

### Step 2: Build Crypto Libraries

```bash
cd /opt/dna-mobile/dna-messenger

# Create standalone CMakeLists for crypto only
cat > build-android-arm64-v8a/CMakeLists.txt << 'CMAKEOF'
cmake_minimum_required(VERSION 3.18.1)
project(dna_crypto C)

# Kyber512
file(GLOB KYBER_SRC crypto/kyber512/*.c)
add_library(kyber512 STATIC ${KYBER_SRC})
target_include_directories(kyber512 PUBLIC crypto/kyber512)

# Dilithium3
file(GLOB DILITHIUM_SRC crypto/dilithium/*.c)
add_library(dilithium STATIC ${DILITHIUM_SRC})
target_include_directories(dilithium PUBLIC crypto/dilithium)
target_compile_definitions(dilithium PRIVATE DILITHIUM_MODE=3)
CMAKEOF

cd build-android-arm64-v8a
cmake . \
  -DCMAKE_TOOLCHAIN_FILE=/home/cc/android-sdk/ndk/25.2.9519653/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26

make -j$(nproc)
```

### Step 3: Build Main Library (with BoringSSL)

```bash
# Update dna_api.c and other files to use BoringSSL APIs
# Key changes:
# - Replace <openssl/evp.h> includes if needed
# - BoringSSL is mostly compatible with OpenSSL 1.1.1

cmake /opt/dna-mobile/dna-messenger \
  -DCMAKE_TOOLCHAIN_FILE=/home/cc/android-sdk/ndk/25.2.9519653/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DOPENSSL_ROOT_DIR=/tmp/boringssl/build-android \
  -DOPENSSL_INCLUDE_DIR=/tmp/boringssl/include \
  -DOPENSSL_CRYPTO_LIBRARY=/tmp/boringssl/build-android/crypto/libcrypto.a

make dna_lib kyber512 dilithium cellframe_minimal
```

### Step 4: Copy to Mobile Project

```bash
mkdir -p mobile/native/libs/android/arm64-v8a
cp build-android-arm64-v8a/lib*.a mobile/native/libs/android/arm64-v8a/
```

---

## 🎯 Approach 2: Minimal Stub Implementation (Quick)

For testing the JNI/Kotlin integration without real crypto:

### Create Stub Libraries

```bash
cd /opt/dna-mobile/dna-messenger/mobile/native

# Create stub implementations
cat > stub_crypto.c << 'STUBEOF'
#include <string.h>
#include <stdint.h>

// Stub Kyber512
int crypto_kem_keypair(unsigned char *pk, unsigned char *sk) {
    memset(pk, 0xAA, 800);
    memset(sk, 0xBB, 1632);
    return 0;
}

int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk) {
    memset(ct, 0xCC, 768);
    memset(ss, 0xDD, 32);
    return 0;
}

int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk) {
    memset(ss, 0xDD, 32);
    return 0;
}

// Stub Dilithium3
int pqcrystals_dilithium3_ref_keypair(uint8_t *pk, uint8_t *sk) {
    memset(pk, 0xEE, 1952);
    memset(sk, 0xFF, 4032);
    return 0;
}

// Stub DNA context
typedef struct { int dummy; } dna_context_t;

dna_context_t* dna_context_new(void) {
    return (dna_context_t*)malloc(sizeof(dna_context_t));
}

void dna_context_free(dna_context_t *ctx) {
    free(ctx);
}

const char* dna_version(void) {
    return "0.1.0-stub";
}
STUBEOF

# Build stub library
/home/cc/android-sdk/ndk/25.2.9519653/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang \
  -c stub_crypto.c -o stub_crypto.o

ar rcs libdna_stub.a stub_crypto.o
```

### Update CMakeLists.txt to Use Stubs

```cmake
# In mobile/shared/src/androidMain/cpp/CMakeLists.txt
target_link_libraries(
    dna_jni
    ${log-lib}
    ${CMAKE_SOURCE_DIR}/../../native/libdna_stub.a  # Use stub
    m
    atomic
)
```

**This allows testing the app structure without real crypto.**

---

## 🎯 Approach 3: Use Prebuilt Libraries

If someone else has built the libraries, just copy them:

```bash
# Expected structure:
mobile/native/libs/android/
├── arm64-v8a/
│   ├── libdna_lib.a
│   ├── libkyber512.a
│   ├── libdilithium.a
│   └── libcellframe_minimal.a
├── armeabi-v7a/
│   └── ...
├── x86/
│   └── ...
└── x86_64/
    └── ...
```

---

## 📊 Estimated Time Per Approach

| Approach | Time | Complexity | Production Ready |
|----------|------|------------|------------------|
| BoringSSL | 6-8 hours | High | Yes ✅ |
| Stub | 30-60 min | Low | No (testing only) |
| Prebuilt | 5 min | None | Yes (if available) |

---

## 🔍 Current Blockers

1. **OpenSSL Dependency**
   - Main DNA library uses OpenSSL
   - Need to port to BoringSSL or build OpenSSL for Android

2. **json-c Dependency**
   - Wallet code needs json-c
   - Not included in Android NDK
   - Need to build separately or exclude wallet features

3. **Architecture Mismatch**
   - Desktop libraries are x86_64
   - Android needs ARM64 (and other ABIs)

---

## 💡 Recommended Path

**For Quick Testing (1 hour):**
1. Use Approach 2 (Stub Implementation)
2. Test JNI integration
3. Verify app structure works
4. Come back for real crypto later

**For Production (1-2 days):**
1. Build BoringSSL for Android
2. Port OpenSSL calls to BoringSSL (minimal changes)
3. Build all crypto libraries
4. Build for all 4 ABIs
5. Integrate and test

---

## 📝 Build Script Template

```bash
#!/bin/bash
# build-android-libs.sh

set -e

NDK=/home/cc/android-sdk/ndk/25.2.9519653
TOOLCHAIN=$NDK/build/cmake/android.toolchain.cmake
ABI=arm64-v8a
API=26

echo "Building for $ABI..."

# 1. Build BoringSSL
cd /tmp
if [ ! -d boringssl ]; then
    git clone https://boringssl.googlesource.com/boringssl
fi
cd boringssl
mkdir -p build-$ABI && cd build-$ABI
cmake .. -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN -DANDROID_ABI=$ABI -DANDROID_PLATFORM=android-$API
make -j$(nproc)

# 2. Build crypto libraries
cd /opt/dna-mobile/dna-messenger
mkdir -p build-android-$ABI && cd build-android-$ABI
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN \
    -DANDROID_ABI=$ABI \
    -DANDROID_PLATFORM=android-$API \
    -DOPENSSL_ROOT_DIR=/tmp/boringssl/build-$ABI
make -j$(nproc)

# 3. Copy to mobile project
cp crypto/kyber512/libkyber512.a ../mobile/native/libs/android/$ABI/
cp crypto/dilithium/libdilithium.a ../mobile/native/libs/android/$ABI/
cp libdna_lib.a ../mobile/native/libs/android/$ABI/

echo "Done! Libraries in mobile/native/libs/android/$ABI/"
```

---

## 🎯 Next Actions

**Choose one:**

1. **I want to test quickly** → Use Approach 2 (stubs)
2. **I want production build** → Use Approach 1 (BoringSSL)  
3. **I'll do this later** → Document and defer

**Current Recommendation:** Use stubs for now, build real libraries when time permits.

---

**Created:** 2025-10-26  
**Estimated Effort:** 4-8 hours (full) or 30-60 min (stubs)  
**Status:** Awaiting decision on approach
