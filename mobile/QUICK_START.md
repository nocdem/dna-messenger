# DNA Messenger - Quick Start Guide

Fast reference for building and publishing DNA Messenger Android app.

## Prerequisites

- JDK 17+
- Android SDK (API 34)
- NDK 25.2.9519653
- Gradle 8.5+

## Development Commands

### Build Debug APK

```bash
cd mobile
./gradlew :androidApp:assembleDebug
```

**Output:** `androidApp/build/outputs/apk/debug/androidApp-debug.apk` (22MB)

### Install on Device

```bash
# Install debug
adb install -r androidApp/build/outputs/apk/debug/androidApp-debug.apk

# Or use gradle
./gradlew :androidApp:installDebug
```

### View Logs

```bash
# Filter DNA Messenger logs
adb logcat -s "DNAMessenger"

# All app logs
adb logcat | grep "io.cpunk.dna"

# Clear logs
adb logcat -c
```

### Run Tests

```bash
# Unit tests
./gradlew :shared:test

# Android instrumented tests (requires emulator/device)
./gradlew :androidApp:connectedAndroidTest
```

## Production Build

### 1. Generate Keystore (First Time Only)

```bash
cd mobile
./generate-keystore.sh
```

This creates:
- `androidApp/keystore/dna-messenger-release.keystore`
- `keystore.properties`

**⚠️ Back up both files securely!**

### 2. Build Release APK

```bash
./gradlew :androidApp:assembleRelease
```

**Output:** `androidApp/build/outputs/apk/release/androidApp-release.apk`

### 3. Build Release Bundle (for Play Store)

```bash
./gradlew :androidApp:bundleRelease
```

**Output:** `androidApp/build/outputs/bundle/release/androidApp-release.aab`

### 4. Verify Signature

```bash
jarsigner -verify -verbose -certs androidApp/build/outputs/apk/release/androidApp-release.apk
```

## Common Tasks

### Clean Build

```bash
./gradlew clean
```

### Update Dependencies

```bash
./gradlew :androidApp:dependencies
./gradlew :shared:dependencies
```

### Check for Outdated Dependencies

```bash
./gradlew dependencyUpdates
```

### Lint Check

```bash
./gradlew :androidApp:lint
```

**Report:** `androidApp/build/reports/lint-results.html`

### Generate Build Report

```bash
./gradlew :androidApp:assembleDebug --scan
```

## Troubleshooting

### Build Fails with NDK Error

```bash
# Check NDK version
cat local.properties | grep ndk.dir

# Should use NDK 25.2.9519653
```

### Native Library Not Found

```bash
# Rebuild native libraries
./gradlew :shared:externalNativeBuildDebug
```

### Gradle Daemon Issues

```bash
./gradlew --stop
./gradlew clean
./gradlew :androidApp:assembleDebug
```

### Out of Memory

Edit `gradle.properties`:
```properties
org.gradle.jvmargs=-Xmx6144m
```

### ADB Device Not Found

```bash
adb kill-server
adb start-server
adb devices
```

## Project Structure

```
mobile/
├── androidApp/              # Android-specific UI
│   ├── src/main/
│   │   ├── java/            # Kotlin code
│   │   └── res/             # Resources (layouts, strings, etc.)
│   ├── build.gradle.kts     # Android app build config
│   └── keystore/            # Release keystores
├── shared/                  # Shared Kotlin Multiplatform code
│   ├── src/
│   │   ├── commonMain/      # Platform-agnostic code
│   │   ├── androidMain/     # Android-specific (JNI)
│   │   └── iosMain/         # iOS-specific
│   └── src/androidMain/cpp/ # JNI C++ bridge
├── iosApp/                  # iOS app (not yet implemented)
├── build.gradle.kts         # Root build config
├── settings.gradle.kts      # Gradle settings
├── gradle.properties        # Gradle properties
├── keystore.properties      # Signing credentials (gitignored)
├── generate-keystore.sh     # Keystore generator script
├── QUICK_START.md           # This file
└── PUBLISHING_GUIDE.md      # Full publishing guide
```

## Key Files

- **App build config:** `androidApp/build.gradle.kts`
- **Version info:** `androidApp/build.gradle.kts` (versionCode, versionName)
- **JNI code:** `shared/src/androidMain/cpp/dna_jni.cpp`
- **Native libs:** `native/libs/android/arm64-v8a/`
- **Signing config:** `keystore.properties` (gitignored)

## Version Management

Update versions in `androidApp/build.gradle.kts`:

```kotlin
defaultConfig {
    versionCode = 1        // Increment for each Play Store release
    versionName = "0.1.0"  // User-facing version
}
```

## Environment Variables (Optional)

For CI/CD, set these environment variables:

```bash
export KEYSTORE_FILE=/path/to/keystore.jks
export KEYSTORE_PASSWORD=your_store_password
export KEY_ALIAS=dna-messenger
export KEY_PASSWORD=your_key_password
```

Then use in `build.gradle.kts`:
```kotlin
storePassword = System.getenv("KEYSTORE_PASSWORD") ?: ""
```

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Build APK
on: [push]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-java@v3
        with:
          java-version: '17'
      - name: Build Debug APK
        run: |
          cd mobile
          ./gradlew :androidApp:assembleDebug
      - uses: actions/upload-artifact@v3
        with:
          name: debug-apk
          path: mobile/androidApp/build/outputs/apk/debug/*.apk
```

## Performance Optimization

### Enable Build Cache

Already enabled in `gradle.properties`:
```properties
org.gradle.caching=true
```

### Parallel Builds

```properties
org.gradle.parallel=true
```

### Use Gradle Daemon

```bash
# Daemon runs automatically, but you can configure:
org.gradle.daemon=true
```

## Resources

- **Full publishing guide:** `PUBLISHING_GUIDE.md`
- **Keystore setup:** `androidApp/keystore/README.md`
- **JNI integration:** `docs/JNI_INTEGRATION_TUTORIAL.md`
- **Development roadmap:** `docs/DEVELOPMENT_TODO.md`

## Support

- **Email:** dev@cpunk.io
- **GitLab:** https://gitlab.cpunk.io/cpunk/dna-messenger
- **Documentation:** https://docs.cpunk.io/dna-messenger

---

**Quick Commands Summary:**

```bash
# Debug build + install
./gradlew :androidApp:installDebug

# Release build (requires keystore)
./gradlew :androidApp:assembleRelease

# Play Store bundle
./gradlew :androidApp:bundleRelease

# View logs
adb logcat -s "DNAMessenger"

# Clean
./gradlew clean
```
