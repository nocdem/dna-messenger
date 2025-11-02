# DNA Messenger - Android Publishing Guide

Complete guide for publishing DNA Messenger to Google Play Store.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Build Release APK/Bundle](#build-release-apkbundle)
3. [Google Play Store Setup](#google-play-store-setup)
4. [Upload to Play Store](#upload-to-play-store)
5. [App Metadata](#app-metadata)
6. [Screenshots & Graphics](#screenshots--graphics)
7. [Testing](#testing)
8. [Publishing Checklist](#publishing-checklist)

---

## Prerequisites

### 1. Keystore Setup

If you haven't created a keystore yet:

```bash
cd mobile
./generate-keystore.sh
```

This will create:
- `androidApp/keystore/dna-messenger-release.keystore` (secure signing key)
- `keystore.properties` (credentials file - gitignored)

**⚠️ CRITICAL:** Back up your keystore securely! If lost, you cannot update the app.

### 2. Google Play Developer Account

- Cost: $25 one-time registration fee
- Sign up: https://play.google.com/console/signup
- Verify your identity (required by Google)

---

## Build Release APK/Bundle

### Option A: Android App Bundle (AAB) - Recommended

Google Play requires AAB format for new apps:

```bash
cd mobile
./gradlew :androidApp:bundleRelease
```

**Output:** `androidApp/build/outputs/bundle/release/androidApp-release.aab`

### Option B: APK - For Direct Distribution

For distribution outside Play Store:

```bash
./gradlew :androidApp:assembleRelease
```

**Output:** `androidApp/build/outputs/apk/release/androidApp-release.apk`

### Verify the Build

```bash
# Check signature
jarsigner -verify -verbose -certs androidApp/build/outputs/apk/release/androidApp-release.apk

# View certificate info
keytool -printcert -jarfile androidApp/build/outputs/apk/release/androidApp-release.apk
```

---

## Google Play Store Setup

### 1. Create App in Play Console

1. Go to: https://play.google.com/console/
2. Click **Create app**
3. Fill in:
   - **App name:** DNA Messenger
   - **Default language:** English (United States)
   - **App or game:** App
   - **Free or paid:** Free
   - **Declarations:** Check all required boxes

### 2. Set Up App Access

**Dashboard → App access**

DNA Messenger requires account creation:

- Select: **Not restricted**
- Provide test account credentials (for Google reviewers)

Example:
```
Username: test@cpunk.io
Password: TestPassword123!
```

### 3. Configure Data Safety

**Dashboard → Data safety**

DNA Messenger is a secure messaging app. Declare:

**Data Collection:**
- ✅ Collects: Phone number (for registration), messages, contacts
- ✅ Encrypted in transit (HTTPS)
- ✅ Encrypted at rest (AES-256)
- ✅ Data can be deleted (user can delete account)

**Data Sharing:**
- ❌ Does NOT share data with third parties

**Data Usage:**
- App functionality (messaging)
- Account management

### 4. Content Rating

**Dashboard → Content rating**

1. Click **Start questionnaire**
2. Select **IARC** questionnaire
3. Email: support@cpunk.io
4. Category: **Social / Communication**
5. Answer questions:
   - Violence: No
   - Sexual content: No
   - Profanity: No
   - Controlled substances: No
   - User interaction: Yes (users can communicate)
   - Shares location: No (optional, if you add location features)
   - Shares personal info: No (end-to-end encrypted)

Rating result: **Everyone** or **Teen** (depending on features)

### 5. Target Audience & Content

**Dashboard → Target audience**

- **Age group:** 13+ (Teen and older)
- **Appeal to children:** No

### 6. App Content

**Dashboard → App content**

Fill in all sections:
- Privacy policy (URL or upload)
- App access instructions
- Ads (declare if using ads - currently: No)

---

## App Metadata

### App Details

**Dashboard → Store presence → Main store listing**

#### Title
```
DNA Messenger - Quantum-Safe Chat
```
(Max 30 characters)

#### Short Description
```
Secure, post-quantum encrypted messaging with cpunk wallet integration.
```
(Max 80 characters)

#### Full Description
```
DNA Messenger: The Future of Secure Communication

Experience true privacy with post-quantum cryptography, ensuring your messages remain secure even against future quantum computers.

🔒 KEY FEATURES:

• Post-Quantum Encryption
  - Kyber512 key encapsulation
  - Dilithium3 digital signatures
  - AES-256-GCM authenticated encryption
  - Future-proof against quantum attacks

• End-to-End Encrypted Messaging
  - One-on-one conversations
  - Group chats
  - Multi-device support
  - No server-side message storage

• Integrated cpunk Wallet
  - Send/receive CPUNK, CELL, KEL tokens
  - BIP39 recovery phrases
  - Cellframe blockchain integration
  - Secure token management

• Privacy-First Design
  - No phone number required
  - No email required
  - No tracking or analytics
  - Open-source (auditable code)

• User-Friendly
  - Modern Material Design 3 UI
  - Dark/Light themes (cpunk.io cyan & cpunk.club orange)
  - Biometric authentication
  - QR code contact sharing

🌐 OPEN SOURCE

DNA Messenger is open-source software. Review our code:
https://gitlab.cpunk.io/cpunk/dna-messenger

📖 ABOUT POST-QUANTUM CRYPTOGRAPHY

Traditional encryption (RSA, ECC) will be vulnerable when quantum computers become powerful enough. DNA Messenger uses NIST-approved post-quantum algorithms to protect your messages today and tomorrow.

🔐 SECURITY AUDIT

Our cryptographic implementation follows NIST standards for post-quantum cryptography. Regular security audits ensure your communications remain secure.

💬 SUPPORT

Need help? Contact us:
- Email: support@cpunk.io
- Website: https://cpunk.io
- Documentation: https://docs.cpunk.io/dna-messenger

🌟 JOIN THE FUTURE OF SECURE MESSAGING

Download DNA Messenger and communicate with confidence.
```
(Max 4000 characters)

### App Category

- **Category:** Communication
- **Tags:** messaging, secure chat, encryption, privacy, quantum-safe

### Contact Details

- **Email:** support@cpunk.io
- **Website:** https://cpunk.io
- **Phone:** (Optional)
- **Privacy policy URL:** https://cpunk.io/privacy (you need to create this)

---

## Screenshots & Graphics

### Required Assets

#### App Icon
- Size: 512x512 px
- Format: PNG (32-bit with alpha)
- No rounded corners (Google adds them)

#### Feature Graphic
- Size: 1024x500 px
- Format: JPG or PNG
- Use case: Top banner on Play Store

#### Screenshots (Required)
- **Phone:** At least 2 screenshots (up to 8)
  - Dimensions: 1080x2400 px or similar 16:9/9:16
  - Format: PNG or JPG
  - Recommended: 3-5 screenshots showing key features

- **Tablet (Optional):** 7-inch and 10-inch
  - Dimensions: 1200x1920 px or similar

#### Screenshots to Include

1. **Main Chat List** - Shows conversation threads
2. **Conversation View** - Encrypted messaging interface
3. **Wallet Screen** - Token balances and transactions
4. **Settings/Security** - Highlighting encryption features
5. **Group Chat** (if implemented)

### Creating Screenshots

Use Android Emulator or real device:

```bash
# Start emulator
emulator -avd Pixel_6_API_34

# Install debug APK
adb install androidApp/build/outputs/apk/debug/androidApp-debug.apk

# Take screenshots
adb shell screencap /sdcard/screenshot.png
adb pull /sdcard/screenshot.png
```

Or use Android Studio's screenshot tool: **Tools → App Inspection → Screenshot**

### Graphic Design Tools

Free tools for creating graphics:
- [Figma](https://figma.com) - UI design
- [Canva](https://canva.com) - Feature graphics
- [GIMP](https://gimp.org) - Image editing

---

## Testing

### Internal Testing Track

Before releasing to production, use internal testing:

**Dashboard → Testing → Internal testing**

1. Create internal testing release
2. Upload `androidApp-release.aab`
3. Add internal testers (email addresses)
4. Share test link with testers

### Closed Testing (Beta)

For wider testing:

**Dashboard → Testing → Closed testing**

1. Create closed testing track (Beta)
2. Upload AAB
3. Add testers or create email list
4. Run beta for 1-2 weeks
5. Collect feedback

### Pre-Launch Report

Google provides automated testing on ~20 devices:
- Crashes
- ANRs (App Not Responding)
- Screenshot comparisons

Review report: **Dashboard → Release → Pre-launch report**

---

## Upload to Play Store

### Production Release

**Dashboard → Production → Create new release**

1. **Upload AAB:**
   ```
   androidApp/build/outputs/bundle/release/androidApp-release.aab
   ```

2. **Release name:** `0.1.0 (1)` - matches versionName and versionCode

3. **Release notes:**
   ```
   🎉 Initial Release - DNA Messenger v0.1.0

   Features:
   • Post-quantum encrypted messaging (Kyber512 + Dilithium3)
   • End-to-end encrypted one-on-one and group chats
   • Integrated cpunk wallet (CPUNK, CELL, KEL tokens)
   • BIP39 recovery phrases for key backup
   • Modern Material Design 3 UI with dark/light themes
   • Biometric authentication for enhanced security

   This is an alpha release. We welcome your feedback!
   ```

4. **Review and rollout:**
   - Start rollout: 5% of users (staged rollout recommended)
   - Monitor for 24-48 hours
   - Increase to 20%, 50%, 100% gradually

### Rollback Plan

If critical issues are found:

**Dashboard → Production → Halt rollout**

Then:
1. Fix issues
2. Increment versionCode (e.g., 1 → 2)
3. Build new AAB
4. Upload hotfix release

---

## App Versioning

### Version Naming Scheme

DNA Messenger uses semantic versioning:

```
versionCode: 1, 2, 3, ... (increments with each release)
versionName: "0.1.0", "0.1.1", "0.2.0", "1.0.0"
```

Update in `androidApp/build.gradle.kts`:

```kotlin
defaultConfig {
    versionCode = 2  // Increment for each upload
    versionName = "0.1.1"  // Semantic version
}
```

### Alpha/Beta Labels

- `0.x.x` = Alpha (breaking changes expected)
- `1.x.x` = Stable (production-ready)
- Append `-alpha`, `-beta` to versionName if needed

---

## Publishing Checklist

Use this checklist before submitting:

### Code & Build
- [ ] All features tested on real device
- [ ] No debug code or logging in release build
- [ ] ProGuard/R8 enabled and tested
- [ ] Keystore backed up securely
- [ ] Version code incremented
- [ ] Version name updated

### Play Console
- [ ] App access configured
- [ ] Data safety form completed
- [ ] Content rating received
- [ ] Privacy policy URL added
- [ ] Store listing filled out (title, description, etc.)
- [ ] Screenshots uploaded (at least 2 for phone)
- [ ] Feature graphic uploaded
- [ ] App icon uploaded
- [ ] Contact details added

### Testing
- [ ] Internal testing completed
- [ ] Beta testing completed (optional but recommended)
- [ ] Pre-launch report reviewed
- [ ] No critical crashes or ANRs

### Legal
- [ ] Privacy policy published and linked
- [ ] Terms of service published (optional)
- [ ] App complies with Google Play policies
- [ ] Export compliance declared (for cryptography)

### Launch
- [ ] AAB uploaded to production
- [ ] Release notes written
- [ ] Staged rollout configured (recommended)
- [ ] Monitoring plan in place

---

## Post-Launch

### Monitor Metrics

**Dashboard → Statistics**
- Installs
- Uninstalls
- Crashes (ANR rate should be <0.47%)
- User ratings

**Dashboard → Vitals**
- Crash-free users (target: >99%)
- ANR rate (target: <0.47%)
- App size
- Battery usage

### Respond to Reviews

- Reply to user reviews within 24-48 hours
- Thank positive reviews
- Address negative feedback constructively
- Offer support email for issues

### Update Cadence

- **Bug fixes:** As needed (hotfix releases)
- **Minor updates:** Every 2-4 weeks
- **Major features:** Every 2-3 months

### Analytics (Optional)

Consider adding analytics (privacy-preserving):
- Firebase Analytics (anonymized)
- Custom backend analytics
- Crashlytics for crash reporting

**Note:** If adding analytics, update Data Safety form!

---

## Additional Resources

- [Google Play Console Help](https://support.google.com/googleplay/android-developer/)
- [Android App Bundle](https://developer.android.com/guide/app-bundle)
- [App Signing by Google Play](https://support.google.com/googleplay/android-developer/answer/9842756)
- [Play Store Listing Guidelines](https://support.google.com/googleplay/android-developer/answer/9859455)

---

## Support

Questions about publishing? Contact the dev team:
- Email: dev@cpunk.io
- Documentation: https://docs.cpunk.io/dna-messenger/publishing

---

**Last Updated:** 2025-10-27
**Version:** 1.0
