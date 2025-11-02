# DNA Messenger - APK Connection Failure Diagnosis & Fix

**Date**: 2025-10-27
**Issue**: APK cannot connect to PostgreSQL database at `ai.cpunk.io:5432`

---

## Root Cause Analysis

### Primary Issue: **Network/Firewall Restriction**

The connection test revealed:
```
✓ DNS Resolution: ai.cpunk.io → 195.174.168.27
✗ TCP Connection: FAILED to connect to port 5432
```

**Finding**: The PostgreSQL server at `ai.cpunk.io:5432` is NOT accessible from all networks.

- ✅ Works from: Your PC "coker" (as shown in your netstat output)
- ❌ Blocked from: Build server and likely your Android device

### Secondary Issue Fixed: **Android Cleartext Traffic Policy**

AndroidManifest.xml had `android:usesCleartextTraffic="false"` which would block PostgreSQL JDBC connections even if the network was reachable, since PostgreSQL uses cleartext TCP by default (not TLS).

---

## Changes Applied

### 1. Network Security Configuration (✅ Applied)

**Created**: `androidApp/src/main/res/xml/network_security_config.xml`
```xml
<?xml version="1.0" encoding="utf-8"?>
<network-security-config>
    <!-- Allow cleartext traffic for PostgreSQL database servers -->
    <domain-config cleartextTrafficPermitted="true">
        <domain includeSubdomains="true">ai.cpunk.io</domain>
        <domain includeSubdomains="false">195.174.168.27</domain>
    </domain-config>

    <!-- Default: Block all other cleartext traffic (secure by default) -->
    <base-config cleartextTrafficPermitted="false">
        <trust-anchors>
            <certificates src="system" />
        </trust-anchors>
    </base-config>
</network-security-config>
```

**Modified**: `AndroidManifest.xml`
```xml
<application
    ...
    android:networkSecurityConfig="@xml/network_security_config">
```

**Files Changed**:
- `/opt/dna-mobile/dna-messenger/mobile/androidApp/src/main/res/xml/network_security_config.xml` (created)
- `/opt/dna-mobile/dna-messenger/mobile/androidApp/src/main/AndroidManifest.xml` (modified)

### 2. APK Rebuilt
```bash
./gradlew clean :androidApp:assembleDebug
# Output: androidApp/build/outputs/apk/debug/androidApp-debug.apk
```

---

## Solutions to Test

Since the primary issue is **network accessibility**, here are solutions in order of preference:

### Solution 1: Whitelist Android Device IP (Recommended)

1. **Get your Android device's IP address**:
   ```bash
   adb shell ip addr show wlan0 | grep "inet "
   # Example output: inet 192.168.1.50/24
   ```

2. **Whitelist this IP on your PostgreSQL server**:
   - Edit `/etc/postgresql/*/main/pg_hba.conf` on the server
   - Add line:
     ```
     host    dna_messenger    dna    192.168.1.50/32    md5
     ```
   - Restart PostgreSQL:
     ```bash
     sudo systemctl restart postgresql
     ```

3. **Verify firewall allows the connection**:
   ```bash
   # On the PostgreSQL server
   sudo ufw allow from 192.168.1.50 to any port 5432
   ```

### Solution 2: SSH Tunnel (For Testing)

If you can't modify the server firewall:

```bash
# From your PC "coker" (which CAN connect), create tunnel:
ssh -L 5432:ai.cpunk.io:5432 user@android-device-ip

# OR use adb reverse port forwarding:
adb reverse tcp:5432 tcp:5432

# Then modify PreferencesManager.kt defaults:
dbHost = "localhost"  # or "127.0.0.1"
dbPort = 5432
```

### Solution 3: Use VPN

If your PC "coker" is on a VPN:
1. Install same VPN on Android device
2. Connect to VPN
3. APK should then be able to reach the database

### Solution 4: Test on Same Network as "coker"

Connect your Android device to the same network as your PC "coker" and test again.

---

## Testing Instructions

### 1. Install Updated APK

```bash
# Connect Android device
adb devices

# Install APK
adb install -r androidApp/build/outputs/apk/debug/androidApp-debug.apk
```

### 2. Monitor Logs

```bash
# Clear previous logs
adb logcat -c

# Watch database connection logs
adb logcat -s DatabaseRepository:* DNAMessenger:* AndroidRuntime:E | grep -E "(Database|Connection|PostgreSQL|Error)"
```

### 3. Expected Output

**If Connection Succeeds**:
```
D/DatabaseRepository: PostgreSQL driver loaded
D/DatabaseRepository: Database connection established to ai.cpunk.io:5432/dna_messenger
D/HomeViewModel: DatabaseRepository initialized with host: ai.cpunk.io:5432
```

**If Connection Fails (Network Issue)**:
```
E/DatabaseRepository: Connection failed: Connection timed out
E/DatabaseRepository: Connection failed: Connection refused
```

**If Connection Fails (Cleartext Policy - shouldn't happen now)**:
```
E/DatabaseRepository: java.net.SocketException: Cleartext traffic not permitted
```

### 4. Test Database Connection from Device

Before launching the app, test connectivity:

```bash
# Install psql on Android (via Termux or similar)
# Or test using telnet:
adb shell

# Inside device shell:
telnet ai.cpunk.io 5432

# Should see:
# Connected to ai.cpunk.io
# (then PostgreSQL will send binary data)

# If you see "Connection refused" or "Network unreachable":
# → Network/firewall issue confirmed
```

---

## Quick Diagnosis Script

Run this from your PC to test if the Android device can reach the database:

```bash
#!/bin/bash
# Save as: test_android_db_access.sh

echo "Testing Android device database connectivity..."

# Get device IP
DEVICE_IP=$(adb shell ip addr show wlan0 | grep "inet " | awk '{print $2}' | cut -d/ -f1)
echo "Android device IP: $DEVICE_IP"

# Test from device
echo ""
echo "Testing connection from Android device..."
adb shell "timeout 5 nc -zv ai.cpunk.io 5432 2>&1"

if [ $? -eq 0 ]; then
    echo "✓ Android device CAN reach database"
else
    echo "✗ Android device CANNOT reach database"
    echo "  → Whitelist $DEVICE_IP on PostgreSQL server"
fi
```

---

## Additional Notes

### Security Considerations

The network security config ONLY allows cleartext traffic to `ai.cpunk.io` and `195.174.168.27`. All other connections remain secure (HTTPS only).

For production, consider:
1. **Enable PostgreSQL SSL/TLS**:
   ```kotlin
   // In DatabaseRepository.kt
   private val jdbcUrl = "jdbc:postgresql://$dbHost:$dbPort/$dbName?ssl=true&sslmode=require"
   ```
2. **Use a VPN** for all database connections
3. **Implement a REST API** layer instead of direct database access

### Database Configuration

Current settings (from PreferencesManager.kt:44-48):
```kotlin
dbHost = "ai.cpunk.io"
dbPort = 5432
dbName = "dna_messenger"
dbUser = "dna"
dbPassword = "dna_password"
```

These can be changed in the app's Settings screen.

---

## Next Steps

1. ✅ APK rebuilt with network security config
2. ⏳ **You**: Check firewall/network accessibility
3. ⏳ **You**: Whitelist Android device IP or setup tunnel
4. ⏳ **You**: Install APK and test
5. ⏳ **You**: Share logcat output if still failing

---

## Files Modified Summary

```
mobile/
├── androidApp/src/main/AndroidManifest.xml              [MODIFIED]
├── androidApp/src/main/res/xml/
│   └── network_security_config.xml                       [CREATED]
└── androidApp/build/outputs/apk/debug/
    └── androidApp-debug.apk                              [REBUILT]
```

## Test Script Created

```
mobile/test_db_connection.sh                              [CREATED]
```

Usage:
```bash
cd /opt/dna-mobile/dna-messenger/mobile
./test_db_connection.sh [host] [port] [dbname] [user] [password]
```

---

**Status**: ✅ Network security config applied, APK rebuilt
**Pending**: Network/firewall configuration on server side
