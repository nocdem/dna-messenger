#!/bin/bash
#
# Build DNA P2P/DHT libraries for Android (NDK cross-compilation)
# Builds the P2P stack to work with OpenDHT on Android ARM64
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Android configuration
# Try to detect NDK from mobile/local.properties first
if [ -f "mobile/local.properties" ]; then
    NDK_FROM_PROPS=$(grep "ndk.dir" mobile/local.properties | cut -d'=' -f2)
fi
NDK_DIR="${ANDROID_NDK_ROOT:-${NDK_FROM_PROPS:-$HOME/Android/Sdk/ndk/25.2.9519653}}"
ANDROID_ABI="arm64-v8a"
ANDROID_PLATFORM="30"
ANDROID_ARCH="aarch64"
ANDROID_TOOLCHAIN="${NDK_DIR}/toolchains/llvm/prebuilt/linux-x86_64"

# Build configuration
PROJECT_ROOT="/opt/dna-mobile/dna-messenger"
BUILD_DIR="/tmp/dna-android-p2p-build"
INSTALL_PREFIX="${PROJECT_ROOT}/mobile/native/libs/android/${ANDROID_ABI}"
OPENDHT_PREFIX="${INSTALL_PREFIX}"
BORINGSSL_DIR="/tmp/boringssl/build-arm64-v8a"

# Toolchain setup
export PATH="${ANDROID_TOOLCHAIN}/bin:$PATH"
export AR="${ANDROID_TOOLCHAIN}/bin/llvm-ar"
export AS="${ANDROID_TOOLCHAIN}/bin/llvm-as"
export CC="${ANDROID_TOOLCHAIN}/bin/aarch64-linux-android${ANDROID_PLATFORM}-clang"
export CXX="${ANDROID_TOOLCHAIN}/bin/aarch64-linux-android${ANDROID_PLATFORM}-clang++)"
export LD="${ANDROID_TOOLCHAIN}/bin/ld"
export RANLIB="${ANDROID_TOOLCHAIN}/bin/llvm-ranlib"
export STRIP="${ANDROID_TOOLCHAIN}/bin/llvm-strip"
export CMAKE_PREFIX_PATH="${INSTALL_PREFIX}"

echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE} Building DNA P2P/DHT for Android${NC}"
echo -e "${BLUE}=========================================${NC}"
echo -e "NDK Directory: ${NDK_DIR}"
echo -e "Android ABI: ${ANDROID_ABI}"
echo -e "Android Platform: ${ANDROID_PLATFORM}"
echo -e "Install Prefix: ${INSTALL_PREFIX}"
echo -e "OpenDHT Prefix: ${OPENDHT_PREFIX}"
echo -e "BoringSSL Dir: ${BORINGSSL_DIR}"
echo ""

# Check NDK exists
if [ ! -d "$NDK_DIR" ]; then
    echo -e "${RED}Error: Android NDK not found at ${NDK_DIR}${NC}"
    echo -e "Set ANDROID_NDK_ROOT environment variable or install NDK"
    exit 1
fi

# Check OpenDHT is built
if [ ! -f "${OPENDHT_PREFIX}/lib/libopendht.a" ]; then
    echo -e "${RED}Error: OpenDHT not built for Android${NC}"
    echo -e "Run ./build-opendht-android.sh first"
    exit 1
fi

# Check BoringSSL is built
if [ ! -f "${BORINGSSL_DIR}/libcrypto.a" ]; then
    echo -e "${RED}Error: BoringSSL not built for Android${NC}"
    echo -e "BoringSSL directory: ${BORINGSSL_DIR}"
    exit 1
fi

echo -e "${GREEN}✓${NC} Android NDK found"
echo -e "${GREEN}✓${NC} OpenDHT found"
echo -e "${GREEN}✓${NC} BoringSSL found"
echo ""

mkdir -p "$BUILD_DIR"
mkdir -p "${INSTALL_PREFIX}/lib"
mkdir -p "${INSTALL_PREFIX}/include"

# Helper function for Android CMake
android_cmake() {
    cmake \
        -DCMAKE_TOOLCHAIN_FILE="${NDK_DIR}/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="${ANDROID_ABI}" \
        -DANDROID_PLATFORM="${ANDROID_PLATFORM}" \
        -DANDROID_STL=c++_static \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
        -DBUILD_SHARED_LIBS=OFF \
        "$@"
}

# 1. Build libdht_lib.a (DHT wrapper for OpenDHT)
echo -e "${BLUE}[1/3] Building libdht_lib.a...${NC}"
if [ ! -f "${INSTALL_PREFIX}/lib/libdht_lib.a" ]; then
    cd "$BUILD_DIR"
    mkdir -p dht && cd dht

    # Create temporary CMakeLists.txt for Android (simpler than patching)
    cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(dna_dht C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android-specific OpenDHT paths
set(OPENDHT_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
set(OPENDHT_LIBRARIES "${CMAKE_INSTALL_PREFIX}/lib/libopendht.a")

# DHT library sources (use absolute paths)
set(DHT_SOURCE_DIR "/opt/dna-mobile/dna-messenger/dht")
set(BORINGSSL_INCLUDE_DIR "/tmp/boringssl/include")

add_library(dht_lib STATIC
    ${DHT_SOURCE_DIR}/dht_context.cpp
    ${DHT_SOURCE_DIR}/dht_offline_queue.c
)

target_include_directories(dht_lib PUBLIC
    ${DHT_SOURCE_DIR}
    ${OPENDHT_INCLUDE_DIRS}
    ${BORINGSSL_INCLUDE_DIR}
)

# Define MSGPACK_NO_BOOST to avoid Boost dependency
target_compile_definitions(dht_lib PRIVATE MSGPACK_NO_BOOST)

target_link_libraries(dht_lib
    ${OPENDHT_LIBRARIES}
)

install(TARGETS dht_lib DESTINATION lib)
install(FILES
    ${DHT_SOURCE_DIR}/dht_context.h
    ${DHT_SOURCE_DIR}/dht_offline_queue.h
    DESTINATION include/dht
)
EOF

    android_cmake .
    make -j$(nproc)
    make install

    cd "$BUILD_DIR"
    echo -e "${GREEN}✓${NC} libdht_lib.a installed"
else
    echo -e "${GREEN}✓${NC} libdht_lib.a already installed"
fi

# 2. Build libp2p_transport.a (P2P transport layer)
echo -e "${BLUE}[2/3] Building libp2p_transport.a...${NC}"
if [ ! -f "${INSTALL_PREFIX}/lib/libp2p_transport.a" ]; then
    cd "$BUILD_DIR"
    mkdir -p p2p && cd p2p

    # Create temporary CMakeLists.txt for Android
    cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(dna_p2p C)

set(CMAKE_C_STANDARD 11)

# Android-specific paths (use absolute paths)
set(OPENDHT_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
set(OPENDHT_LIBRARIES "${CMAKE_INSTALL_PREFIX}/lib/libopendht.a")
set(DHT_DIR "/opt/dna-mobile/dna-messenger/dht")
set(P2P_SOURCE_DIR "/opt/dna-mobile/dna-messenger/p2p")

# BoringSSL paths (Android uses BoringSSL instead of OpenSSL)
set(BORINGSSL_ROOT_DIR "/tmp/boringssl/build-arm64-v8a")
set(OPENSSL_INCLUDE_DIR "${BORINGSSL_ROOT_DIR}/../include")
set(OPENSSL_CRYPTO_LIBRARY "${BORINGSSL_ROOT_DIR}/libcrypto.a")
set(OPENSSL_SSL_LIBRARY "${BORINGSSL_ROOT_DIR}/libssl.a")

# P2P Transport library
add_library(p2p_transport STATIC
    ${P2P_SOURCE_DIR}/p2p_transport.c
)

target_include_directories(p2p_transport PUBLIC
    ${P2P_SOURCE_DIR}
    ${DHT_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${OPENDHT_INCLUDE_DIRS}
    ${CMAKE_INSTALL_PREFIX}/include/dht
)

target_link_libraries(p2p_transport
    ${CMAKE_INSTALL_PREFIX}/lib/libdht_lib.a
    ${OPENDHT_LIBRARIES}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
)

install(TARGETS p2p_transport DESTINATION lib)
install(FILES
    ${P2P_SOURCE_DIR}/p2p_transport.h
    DESTINATION include/p2p
)
EOF

    android_cmake .
    make -j$(nproc)
    make install

    cd "$BUILD_DIR"
    echo -e "${GREEN}✓${NC} libp2p_transport.a installed"
else
    echo -e "${GREEN}✓${NC} libp2p_transport.a already installed"
fi

# 3. Build messenger_p2p library
echo -e "${BLUE}[3/3] Building libmessenger_p2p.a...${NC}"
if [ ! -f "${INSTALL_PREFIX}/lib/libmessenger_p2p.a" ]; then
    cd "$BUILD_DIR"
    mkdir -p messenger_p2p && cd messenger_p2p

    # Create temporary CMakeLists.txt for Android
    cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(messenger_p2p C)

set(CMAKE_C_STANDARD 11)

# Paths
set(MESSENGER_SOURCE_DIR "/opt/dna-mobile/dna-messenger")
set(BORINGSSL_INCLUDE_DIR "/tmp/boringssl/include")

add_library(messenger_p2p STATIC
    ${MESSENGER_SOURCE_DIR}/messenger_p2p.c
)

target_include_directories(messenger_p2p PUBLIC
    ${MESSENGER_SOURCE_DIR}
    ${CMAKE_INSTALL_PREFIX}/include/dht
    ${CMAKE_INSTALL_PREFIX}/include/p2p
    ${CMAKE_INSTALL_PREFIX}/include
    ${BORINGSSL_INCLUDE_DIR}
    /opt/dna-mobile/dna-messenger/mobile/shared/src/androidMain/cpp  # For libpq-fe.h stub
)

target_compile_definitions(messenger_p2p PRIVATE MOBILE_BUILD)

target_link_libraries(messenger_p2p
    ${CMAKE_INSTALL_PREFIX}/lib/libp2p_transport.a
    ${CMAKE_INSTALL_PREFIX}/lib/libdht_lib.a
)

install(TARGETS messenger_p2p DESTINATION lib)
install(FILES
    ${MESSENGER_SOURCE_DIR}/messenger_p2p.h
    DESTINATION include
)
EOF

    android_cmake .
    make -j$(nproc)
    make install

    cd "$BUILD_DIR"
    echo -e "${GREEN}✓${NC} libmessenger_p2p.a installed"
else
    echo -e "${GREEN}✓${NC} libmessenger_p2p.a already installed"
fi

cd "${PROJECT_ROOT}"

echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN} DNA P2P/DHT Build Complete!${NC}"
echo -e "${GREEN}=========================================${NC}"
echo -e "Install location: ${INSTALL_PREFIX}"
echo ""
echo -e "P2P/DHT libraries installed:"
ls -lh "${INSTALL_PREFIX}/lib/"libdht_lib.a "${INSTALL_PREFIX}/lib/"libmessenger_p2p.a "${INSTALL_PREFIX}/lib/"libp2p_transport.a 2>/dev/null | awk '{print "  "$9" ("$5")"}'
echo ""
echo -e "${GREEN}✓${NC} Ready for JNI integration"
