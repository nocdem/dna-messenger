plugins {
    kotlin("multiplatform")
    kotlin("plugin.serialization")
    id("com.android.library")
}

kotlin {
    androidTarget {
        compilations.all {
            kotlinOptions {
                jvmTarget = "17"
            }
        }
    }

    listOf(
        iosX64(),
        iosArm64(),
        iosSimulatorArm64()
    ).forEach {
        it.binaries.framework {
            baseName = "shared"
            isStatic = true
        }

        // C Interop configuration for libdna
        it.compilations.getByName("main") {
            cinterops {
                val dna by creating {
                    // Path to dna_api.h and libraries
                    defFile = project.file("src/nativeInterop/cinterop/dna.def")
                    packageName = "io.cpunk.dna.native"
                    includeDirs(
                        project.file("../../dna_api.h").parentFile,
                        project.file("../native/include")
                    )
                }

                val wallet by creating {
                    // Path to wallet.h
                    defFile = project.file("src/nativeInterop/cinterop/wallet.def")
                    packageName = "io.cpunk.wallet.native"
                    includeDirs(
                        project.file("../../wallet.h").parentFile,
                        project.file("../native/include")
                    )
                }
            }
        }
    }

    sourceSets {
        val commonMain by getting {
            dependencies {
                // Coroutines for async operations
                implementation("org.jetbrains.kotlinx:kotlinx-coroutines-core:1.7.3")

                // Serialization for JSON
                implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.6.0")

                // DateTime
                implementation("org.jetbrains.kotlinx:kotlinx-datetime:0.4.1")
            }
        }

        val commonTest by getting {
            dependencies {
                implementation(kotlin("test"))
            }
        }

        val androidMain by getting {
            dependencies {
                // PostgreSQL driver for Android
                implementation("org.postgresql:postgresql:42.6.0")

                // HTTP client (for logging API)
                implementation("io.ktor:ktor-client-android:2.3.5")

                // DataStore for preferences
                implementation("androidx.datastore:datastore-preferences:1.0.0")
            }
        }

        val iosMain by creating {
            dependsOn(commonMain)
        }

        val iosX64Main by getting {
            dependsOn(iosMain)
        }

        val iosArm64Main by getting {
            dependsOn(iosMain)
        }

        val iosSimulatorArm64Main by getting {
            dependsOn(iosMain)
        }
    }
}

android {
    namespace = "io.cpunk.dna.shared"
    compileSdk = 34

    defaultConfig {
        minSdk = 26

        ndk {
            // ABIs to build for (only arm64-v8a for stub testing)
            abiFilters += setOf("arm64-v8a")
        }

        externalNativeBuild {
            cmake {
                // Arguments for CMake to build C libraries
                arguments += listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DANDROID_PLATFORM=android-26"
                )
                cppFlags += "-std=c++17"
            }
        }
    }

    externalNativeBuild {
        cmake {
            // Point to JNI wrapper CMakeLists.txt
            path = file("src/androidMain/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    // Configure JNI libraries location
    sourceSets {
        getByName("main") {
            jniLibs.srcDirs("../native/libs/android")
        }
    }
}
