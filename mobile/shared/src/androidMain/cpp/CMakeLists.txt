# CMakeLists.txt for DNA Messenger JNI wrapper
# Builds native library for Android (libdna_jni.so)

cmake_minimum_required(VERSION 3.18.1)
project(dna_jni)

# C++17 standard required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Android NDK's log library
find_library(log-lib log)

# DNA C libraries directory (built separately)
set(DNA_LIBS_DIR "${CMAKE_SOURCE_DIR}/../../../../../build")
set(DNA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../../../../../")

# Include directories
include_directories(
    ${DNA_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# JNI source files
add_library(
    dna_jni
    SHARED
    dna_jni.cpp
    wallet_jni.cpp
    jni_utils.cpp
)

# Link libraries
target_link_libraries(
    dna_jni
    ${log-lib}
    ${DNA_LIBS_DIR}/libdna_lib.a
    ${DNA_LIBS_DIR}/libkyber512.a
    ${DNA_LIBS_DIR}/libdilithium.a
    ${DNA_LIBS_DIR}/crypto/cellframe_dilithium/libcellframe_dilithium.a
    ${DNA_LIBS_DIR}/libcellframe_minimal.a
    ssl
    crypto
    pq
    pthread
    m
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
