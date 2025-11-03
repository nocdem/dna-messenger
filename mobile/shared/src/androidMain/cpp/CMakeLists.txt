# CMakeLists.txt for DNA Messenger JNI wrapper
# Builds native library for Android (libdna_jni.so)

cmake_minimum_required(VERSION 3.18.1)
project(dna_jni)

# C++17 standard required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Android NDK's log library
find_library(log-lib log)

# DNA C libraries directory (built for Android)
set(DNA_LIBS_DIR "${CMAKE_SOURCE_DIR}/../../../../native/libs/android/${ANDROID_ABI}")
set(DNA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../../../../../")
set(BORINGSSL_DIR "/tmp/boringssl/build-arm64-v8a")

# Include directories
include_directories(
    ${DNA_INCLUDE_DIR}
    ${DNA_INCLUDE_DIR}/crypto/kyber512
    ${DNA_INCLUDE_DIR}/crypto/dilithium
    ${DNA_INCLUDE_DIR}/dht
    ${DNA_INCLUDE_DIR}/p2p
    ${DNA_LIBS_DIR}/include
    ${DNA_LIBS_DIR}/include/dht
    ${DNA_LIBS_DIR}/include/p2p
    /tmp/boringssl/include
    ${CMAKE_SOURCE_DIR}
)

# JNI source files
add_library(
    dna_jni
    SHARED
    dna_jni.cpp
    # wallet_jni.cpp  # TODO: Add back when json-c is available for Android
    p2p_jni.cpp                 # P2P/DHT JNI wrapper
    jni_utils.cpp
    boringssl_compat.c          # Compatibility shim for missing BoringSSL functions
    messenger_mobile_stubs.c    # Mobile stubs for PostgreSQL functions
)

# Link libraries (order matters for static libs!)
# For static linking, libraries must appear AFTER their dependencies
target_link_libraries(
    dna_jni
    ${log-lib}

    # Application libraries:
    ${DNA_LIBS_DIR}/libdna_lib.a
    ${DNA_LIBS_DIR}/libkyber512.a
    ${DNA_LIBS_DIR}/libdilithium.a
    ${DNA_LIBS_DIR}/libcellframe_dilithium.a

    # P2P/DHT libraries (must come before OpenDHT):
    ${DNA_LIBS_DIR}/lib/libmessenger_p2p.a
    ${DNA_LIBS_DIR}/lib/libp2p_transport.a
    ${DNA_LIBS_DIR}/lib/libdht_lib.a

    # OpenDHT and dependencies (in dependency order):
    ${DNA_LIBS_DIR}/lib/libopendht-c.a
    ${DNA_LIBS_DIR}/lib/libopendht.a
    ${DNA_LIBS_DIR}/lib/libjsoncpp.a
    ${DNA_LIBS_DIR}/lib/libfmt.a
    ${DNA_LIBS_DIR}/lib/libargon2.a
    ${DNA_LIBS_DIR}/lib/libgnutls.a
    ${DNA_LIBS_DIR}/lib/libhogweed.a
    ${DNA_LIBS_DIR}/lib/libnettle.a
    ${DNA_LIBS_DIR}/lib/libtasn1.a
    ${DNA_LIBS_DIR}/lib/libgmp.a

    # BoringSSL (after all libs, many depend on it):
    ${BORINGSSL_DIR}/libssl.a
    ${BORINGSSL_DIR}/libcrypto.a
    # Link crypto again for circular dependencies:
    ${BORINGSSL_DIR}/libssl.a
    ${BORINGSSL_DIR}/libcrypto.a

    # Android system libraries:
    m
    atomic
    c++_static  # C++ standard library (needed by BoringSSL + OpenDHT)
)

# Compiler flags
target_compile_options(
    dna_jni
    PRIVATE
    -Wall
    -Wextra
    -fPIC
    -DANDROID
)
