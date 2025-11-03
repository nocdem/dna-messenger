#!/bin/bash
#
# Build OpenDHT and ALL dependencies for Android (NDK cross-compilation)
# This builds a complete P2P stack for Android ARM64
#
# Based on build-opendht-windows.sh but adapted for Android NDK
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
BUILD_DIR="/tmp/dna-android-deps"
INSTALL_PREFIX="/opt/dna-mobile/dna-messenger/mobile/native/libs/android/${ANDROID_ABI}"

# Toolchain setup
export PATH="${ANDROID_TOOLCHAIN}/bin:$PATH"
export AR="${ANDROID_TOOLCHAIN}/bin/llvm-ar"
export AS="${ANDROID_TOOLCHAIN}/bin/llvm-as"
export CC="${ANDROID_TOOLCHAIN}/bin/aarch64-linux-android${ANDROID_PLATFORM}-clang"
export CXX="${ANDROID_TOOLCHAIN}/bin/aarch64-linux-android${ANDROID_PLATFORM}-clang++"
export LD="${ANDROID_TOOLCHAIN}/bin/ld"
export RANLIB="${ANDROID_TOOLCHAIN}/bin/llvm-ranlib"
export STRIP="${ANDROID_TOOLCHAIN}/bin/llvm-strip"
export PKG_CONFIG_PATH="${INSTALL_PREFIX}/lib/pkgconfig"
export CMAKE_PREFIX_PATH="${INSTALL_PREFIX}"

echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE} Building Full P2P Stack for Android${NC}"
echo -e "${BLUE}=========================================${NC}"
echo -e "NDK Directory: ${NDK_DIR}"
echo -e "Android ABI: ${ANDROID_ABI}"
echo -e "Android Platform: ${ANDROID_PLATFORM}"
echo -e "Install Prefix: ${INSTALL_PREFIX}"
echo ""

# Check NDK exists
if [ ! -d "$NDK_DIR" ]; then
    echo -e "${RED}Error: Android NDK not found at ${NDK_DIR}${NC}"
    echo -e "Set ANDROID_NDK_ROOT environment variable or install NDK"
    exit 1
fi

echo -e "${GREEN}✓${NC} Android NDK found at ${NDK_DIR}"

# Check if already built
if [ -f "${INSTALL_PREFIX}/lib/libopendht.a" ] && [ -f "${INSTALL_PREFIX}/include/opendht/dhtrunner.h" ]; then
    echo -e "${GREEN}✓${NC} OpenDHT already built for Android"
    exit 0
fi

mkdir -p "$BUILD_DIR"
mkdir -p "$INSTALL_PREFIX"
cd "$BUILD_DIR"

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

# 1. Build GMP (arbitrary precision arithmetic - needed by Nettle)
echo -e "${BLUE}[1/9] Building GMP...${NC}"
if [ ! -f "${INSTALL_PREFIX}/lib/libgmp.a" ]; then
    if [ ! -d "gmp-6.3.0" ]; then
        wget -q https://gmplib.org/download/gmp/gmp-6.3.0.tar.xz
        tar -xf gmp-6.3.0.tar.xz
    fi
    cd gmp-6.3.0

    ./configure \
        --host=aarch64-linux-android \
        --prefix="${INSTALL_PREFIX}" \
        --enable-static \
        --disable-shared \
        --with-pic

    make -j$(nproc)
    make install
    cd "$BUILD_DIR"
    echo -e "${GREEN}✓${NC} GMP installed"
else
    echo -e "${GREEN}✓${NC} GMP already installed"
fi

# 2. Build Nettle (crypto library - needed by GnuTLS)
echo -e "${BLUE}[2/9] Building Nettle...${NC}"
if [ ! -f "${INSTALL_PREFIX}/lib/libnettle.a" ]; then
    if [ ! -d "nettle-3.9.1" ]; then
        wget -q https://ftp.gnu.org/gnu/nettle/nettle-3.9.1.tar.gz
        tar -xf nettle-3.9.1.tar.gz
    fi
    cd nettle-3.9.1

    ./configure \
        --host=aarch64-linux-android \
        --prefix="${INSTALL_PREFIX}" \
        --enable-static \
        --disable-shared \
        --with-pic \
        --disable-documentation \
        CFLAGS="-I${INSTALL_PREFIX}/include" \
        LDFLAGS="-L${INSTALL_PREFIX}/lib"

    make -j$(nproc)
    make install
    cd "$BUILD_DIR"
    echo -e "${GREEN}✓${NC} Nettle installed"
else
    echo -e "${GREEN}✓${NC} Nettle already installed"
fi

# 3. Build libtasn1 (ASN.1 library - needed by GnuTLS)
echo -e "${BLUE}[3/9] Building libtasn1...${NC}"
if [ ! -f "${INSTALL_PREFIX}/lib/libtasn1.a" ]; then
    if [ ! -d "libtasn1-4.19.0" ]; then
        wget -q https://ftp.gnu.org/gnu/libtasn1/libtasn1-4.19.0.tar.gz
        tar -xf libtasn1-4.19.0.tar.gz
    fi
    cd libtasn1-4.19.0

    ./configure \
        --host=aarch64-linux-android \
        --prefix="${INSTALL_PREFIX}" \
        --enable-static \
        --disable-shared \
        --with-pic

    make -j$(nproc)
    make install
    cd "$BUILD_DIR"
    echo -e "${GREEN}✓${NC} libtasn1 installed"
else
    echo -e "${GREEN}✓${NC} libtasn1 already installed"
fi

# 4. Build GnuTLS (TLS library - needed by OpenDHT)
echo -e "${BLUE}[4/9] Building GnuTLS...${NC}"
if [ ! -f "${INSTALL_PREFIX}/lib/libgnutls.a" ]; then
    if [ ! -d "gnutls-3.8.0" ]; then
        wget -q https://www.gnupg.org/ftp/gcrypt/gnutls/v3.8/gnutls-3.8.0.tar.xz
        tar -xf gnutls-3.8.0.tar.xz
    fi
    cd gnutls-3.8.0

    ./configure \
        --host=aarch64-linux-android \
        --prefix="${INSTALL_PREFIX}" \
        --enable-static \
        --disable-shared \
        --with-pic \
        --without-p11-kit \
        --without-brotli \
        --without-zlib \
        --without-zstd \
        --without-idn \
        --disable-doc \
        --disable-tests \
        --disable-tools \
        --with-included-libtasn1 \
        --with-included-unistring \
        CFLAGS="-I${INSTALL_PREFIX}/include" \
        LDFLAGS="-L${INSTALL_PREFIX}/lib" \
        NETTLE_CFLAGS="-I${INSTALL_PREFIX}/include" \
        NETTLE_LIBS="-L${INSTALL_PREFIX}/lib -lnettle -lhogweed" \
        HOGWEED_CFLAGS="-I${INSTALL_PREFIX}/include" \
        HOGWEED_LIBS="-L${INSTALL_PREFIX}/lib -lhogweed -lnettle" \
        GMP_CFLAGS="-I${INSTALL_PREFIX}/include" \
        GMP_LIBS="-L${INSTALL_PREFIX}/lib -lgmp"

    make -j$(nproc)
    make install
    cd "$BUILD_DIR"
    echo -e "${GREEN}✓${NC} GnuTLS installed"
else
    echo -e "${GREEN}✓${NC} GnuTLS already installed"
fi

# 5. Build fmt (formatting library)
echo -e "${BLUE}[5/9] Building fmt...${NC}"
if [ ! -f "${INSTALL_PREFIX}/lib/libfmt.a" ]; then
    if [ ! -d "fmt" ]; then
        git clone --depth 1 --branch 10.2.1 https://github.com/fmtlib/fmt.git
    fi
    cd fmt
    mkdir -p build-android && cd build-android

    android_cmake .. \
        -DFMT_TEST=OFF \
        -DFMT_DOC=OFF

    make -j$(nproc)
    make install
    cd "$BUILD_DIR"
    echo -e "${GREEN}✓${NC} fmt installed"
else
    echo -e "${GREEN}✓${NC} fmt already installed"
fi

# 6. Build jsoncpp (JSON library)
echo -e "${BLUE}[6/9] Building jsoncpp...${NC}"
if [ ! -f "${INSTALL_PREFIX}/lib/libjsoncpp.a" ]; then
    if [ ! -d "jsoncpp" ]; then
        git clone --depth 1 --branch 1.9.5 https://github.com/open-source-parsers/jsoncpp.git
    fi
    cd jsoncpp
    mkdir -p build-android && cd build-android

    android_cmake .. \
        -DJSONCPP_WITH_TESTS=OFF \
        -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF

    make -j$(nproc)
    make install
    cd "$BUILD_DIR"
    echo -e "${GREEN}✓${NC} jsoncpp installed"
else
    echo -e "${GREEN}✓${NC} jsoncpp already installed"
fi

# 7. Build libargon2 (password hashing)
echo -e "${BLUE}[7/9] Building libargon2...${NC}"
if [ ! -f "${INSTALL_PREFIX}/lib/libargon2.a" ]; then
    if [ ! -d "phc-winner-argon2" ]; then
        git clone --depth 1 --branch 20190702 https://github.com/P-H-C/phc-winner-argon2.git
    fi
    cd phc-winner-argon2

    # Cross-compile argon2 (it uses Makefile, not CMake)
    # IMPORTANT: Add -fPIC flag for shared library linking
    make clean || true
    make CC="${CC}" \
         AR="${AR}" \
         RANLIB="${RANLIB}" \
         CFLAGS="-fPIC -O2 -Iinclude" \
         LIBRARY_REL=lib \
         -j$(nproc)

    # Manual install
    cp libargon2.a "${INSTALL_PREFIX}/lib/"
    cp -r include/argon2.h "${INSTALL_PREFIX}/include/"

    cd "$BUILD_DIR"
    echo -e "${GREEN}✓${NC} argon2 installed"
else
    echo -e "${GREEN}✓${NC} argon2 already installed"
fi

# 8. Build ASIO (header-only networking library)
echo -e "${BLUE}[8/9] Building ASIO...${NC}"
if [ ! -f "${INSTALL_PREFIX}/include/asio.hpp" ]; then
    if [ ! -d "asio" ]; then
        git clone --depth 1 --branch asio-1-28-0 https://github.com/chriskohlhoff/asio.git
    fi
    cd asio/asio

    # ASIO is header-only, just copy the headers
    cp -r include/* "${INSTALL_PREFIX}/include/"

    cd "$BUILD_DIR"
    echo -e "${GREEN}✓${NC} ASIO installed"
else
    echo -e "${GREEN}✓${NC} ASIO already installed"
fi

# 9. Build msgpack-c (serialization library)
echo -e "${BLUE}[9/9] Building msgpack-c...${NC}"
if [ ! -f "${INSTALL_PREFIX}/lib/libmsgpackc.a" ]; then
    if [ ! -d "msgpack-c" ]; then
        git clone --depth 1 --branch cpp-6.1.0 https://github.com/msgpack/msgpack-c.git
    fi
    cd msgpack-c
    mkdir -p build-android && cd build-android

    android_cmake .. \
        -DMSGPACK_BUILD_EXAMPLES=OFF \
        -DMSGPACK_BUILD_TESTS=OFF \
        -DMSGPACK_USE_BOOST=OFF \
        -DMSGPACK_CXX11=ON

    make -j$(nproc)
    make install
    cd "$BUILD_DIR"
    echo -e "${GREEN}✓${NC} msgpack-c installed"
else
    echo -e "${GREEN}✓${NC} msgpack-c already installed"
fi

# 9. Build OpenDHT
echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE} Building OpenDHT for Android${NC}"
echo -e "${BLUE}=========================================${NC}"

if [ ! -d "opendht" ]; then
    git clone --depth 1 --branch v2.6.0 https://github.com/savoirfairelinux/opendht.git
fi
cd opendht

# Patch CMakeLists.txt to look for msgpack-cxx instead of msgpack
sed -i 's/find_package(msgpack /find_package(msgpack-cxx /g' CMakeLists.txt

mkdir -p build-android && cd build-android

# Create a symlink for msgpack to msgpack-cxx
    ln -sf "${INSTALL_PREFIX}/lib/cmake/msgpack-cxx" "${INSTALL_PREFIX}/lib/cmake/msgpack" 2>/dev/null || true

    android_cmake .. \
        -DOPENDHT_STATIC=ON \
        -DOPENDHT_SHARED=OFF \
        -DOPENDHT_PYTHON=OFF \
        -DOPENDHT_TOOLS=OFF \
        -DOPENDHT_SYSTEMD=OFF \
        -DOPENDHT_TESTS=OFF \
        -DOPENDHT_PROXY_SERVER=OFF \
        -DOPENDHT_PROXY_CLIENT=OFF \
        -DOPENDHT_PUSH_NOTIFICATIONS=OFF \
        -DOPENDHT_C=ON \
        -DBUILD_TESTING=OFF \
        -DGNUTLS_LIBRARIES="${INSTALL_PREFIX}/lib/libgnutls.a;${INSTALL_PREFIX}/lib/libhogweed.a;${INSTALL_PREFIX}/lib/libnettle.a;${INSTALL_PREFIX}/lib/libgmp.a" \
        -DGNUTLS_INCLUDE_DIR="${INSTALL_PREFIX}/include" \
        -DASIO_INCLUDE_DIR="${INSTALL_PREFIX}/include" \
        -Dfmt_DIR="${INSTALL_PREFIX}/lib/cmake/fmt" \
        -Djsoncpp_DIR="${INSTALL_PREFIX}/lib/cmake/jsoncpp" \
        -Dmsgpack-cxx_DIR="${INSTALL_PREFIX}/lib/cmake/msgpack-cxx"

make -j$(nproc)
make install

cd "${PROJECT_ROOT:-/opt/dna-mobile/dna-messenger}"

echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN} OpenDHT Build Complete!${NC}"
echo -e "${GREEN}=========================================${NC}"
echo -e "Install location: ${INSTALL_PREFIX}"
echo ""
echo -e "Libraries installed:"
ls -lh "${INSTALL_PREFIX}/lib/"*.a | awk '{print "  "$9" ("$5")"}'
echo ""
echo -e "${GREEN}✓${NC} Ready to build P2P/DHT Android libraries"
