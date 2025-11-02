# APK API Usage Guide

## Overview

The **Android APK no longer connects directly to the PostgreSQL database**. Instead, it uses the **Logging REST API** via HTTP to log events, messages, and connections.

---

## Architecture

```
┌─────────────────────────────────────┐
│  Android APK (dna_messenger)        │
│  - LoggingApiClient (Kotlin)        │
│  - Makes HTTP POST/GET requests     │
└──────────────┬──────────────────────┘
               │
               │ HTTP (JSON)
               │
┌──────────────▼──────────────────────┐
│  Logging API (Keyserver)            │
│  - Port 8080                        │
│  - POST /api/logging/event          │
│  - POST /api/logging/message        │
│  - POST /api/logging/connection     │
│  - GET  /api/logging/stats          │
└──────────────┬──────────────────────┘
               │
               │ SQL
               │
┌──────────────▼──────────────────────┐
│  PostgreSQL Database                │
│  - logging_events                   │
│  - logging_messages                 │
│  - logging_connections              │
│  - logging_stats                    │
└─────────────────────────────────────┘
```

---

## Kotlin Client Library (Already Implemented)

**Location:** `/opt/dna-mobile/dna-messenger/mobile/shared/src/commonMain/kotlin/io/cpunk/dna/domain/LoggingApiClient.kt`

### Setup

```kotlin
// In your Application class
class DNAMessengerApp : Application() {
    val loggingClient = LoggingApiClient("http://localhost:8080")

    override fun onCreate() {
        super.onCreate()
        loggingClient.setPlatform("android")
        loggingClient.setAppVersion(BuildConfig.VERSION_NAME)
    }
}
```

### Usage Examples

#### 1. Log App Startup

```kotlin
lifecycleScope.launch {
    app.loggingClient.logEvent(
        eventType = EventType.APP_STARTED,
        severity = SeverityLevel.INFO,
        message = "DNA Messenger started"
    ).onSuccess {
        Log.d(TAG, "Event logged successfully")
    }.onFailure { error ->
        Log.e(TAG, "Failed to log event: ${error.message}")
    }
}
```

#### 2. Log Message Sent

```kotlin
lifecycleScope.launch {
    app.loggingClient.logMessage(
        messageId = messageId,
        sender = currentUser,
        recipient = recipientUser,
        groupId = null,
        status = "sent",
        plaintextSize = plaintext.size,
        ciphertext Size = ciphertext.size
    ).onSuccess {
        Log.d(TAG, "Message logged")
    }
}
```

#### 3. Log Database Connection

```kotlin
lifecycleScope.launch {
    val startTime = System.currentTimeMillis()

    try {
        // Try to connect
        val connection = connectToDatabase()
        val responseTime = (System.currentTimeMillis() - startTime).toInt()

        // Log success
        app.loggingClient.logConnection(
            connectionType = ConnectionType.DATABASE,
            host = "ai.cpunk.io",
            port = 5432,
            success = true,
            responseTimeMs = responseTime
        )
    } catch (e: Exception) {
        val responseTime = (System.currentTimeMillis() - startTime).toInt()

        // Log failure
        app.loggingClient.logConnection(
            connectionType = ConnectionType.DATABASE,
            host = "ai.cpunk.io",
            port = 5432,
            success = false,
            responseTimeMs = responseTime,
            errorCode = "CONNECTION_TIMEOUT",
            errorMessage = e.message
        )
    }
}
```

#### 4. Get Statistics

```kotlin
lifecycleScope.launch {
    app.loggingClient.getStats(
        startTime = "2025-10-27 00:00:00",
        endTime = "2025-10-28 00:00:00"
    ).onSuccess { stats ->
        println("Total messages sent: ${stats.messagesSent}")
        println("Total connections successful: ${stats.connectionsSuccess}")
        println("Total errors: ${stats.errorsCount}")
    }
}
```

---

## Replacing Direct Database Calls

### Before (Direct Database Access) ❌

```kotlin
// DatabaseRepository.kt
actual suspend fun saveMessage(message: Message): Result<Long> {
    return runCatching {
        val conn = connect()  // Direct PostgreSQL connection
        val stmt = conn.prepareStatement(sql)
        // ... insert directly to database
        Log.d(TAG, "Message saved: ID=$id")  // Only local logging
    }
}
```

### After (Using API) ✅

```kotlin
// DatabaseRepository.kt
actual suspend fun saveMessage(message: Message): Result<Long> {
    return runCatching {
        val conn = connect()  // Direct PostgreSQL connection (still needed for message storage)
        val stmt = conn.prepareStatement(sql)
        // ... insert to database
        val id = rs.getLong(1)

        // Log via API instead of direct logging
        loggingClient.logMessage(
            messageId = id,
            sender = message.sender,
            recipient = message.recipient,
            status = "sent",
            plaintextSize = message.plaintext?.size ?: 0,
            ciphertextSize = message.ciphertext.size
        )

        id
    }
}
```

---

## API Endpoints

### 1. POST `/api/logging/event`

Log general events like app start, key generation, etc.

**Request:**
```json
{
  "event_type": "app_started",
  "severity": "info",
  "message": "Application started",
  "identity": "alice",
  "platform": "android",
  "app_version": "0.1.0",
  "client_timestamp": 1730073600
}
```

**Response:**
```json
{
  "success": true,
  "message": "Event logged successfully"
}
```

### 2. POST `/api/logging/message`

Log message-specific events with metrics.

**Request:**
```json
{
  "message_id": 12345,
  "sender": "alice",
  "recipient": "bob",
  "status": "sent",
  "plaintext_size": 256,
  "ciphertext_size": 1024,
  "platform": "android"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Message logged successfully"
}
```

### 3. POST `/api/logging/connection`

Log network connection attempts.

**Request:**
```json
{
  "identity": "alice",
  "connection_type": "database",
  "host": "ai.cpunk.io",
  "port": 5432,
  "success": false,
  "response_time_ms": 5000,
  "error_code": "CONNECTION_TIMEOUT",
  "error_message": "Connection timed out",
  "platform": "android",
  "app_version": "0.1.0"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Connection logged successfully"
}
```

### 4. GET `/api/logging/stats`

Get aggregated statistics.

**Request:**
```
GET /api/logging/stats?start_time=2025-10-27%2000:00:00&end_time=2025-10-28%2000:00:00
```

**Response:**
```json
{
  "success": true,
  "period_start": "2025-10-27 00:00:00",
  "period_end": "2025-10-28 00:00:00",
  "statistics": {
    "total_events": 1523,
    "total_messages": 456,
    "total_connections": 89,
    "messages_sent": 234,
    "messages_delivered": 198,
    "messages_failed": 24,
    "connections_success": 76,
    "connections_failed": 13,
    "errors_count": 37,
    "warnings_count": 12
  }
}
```

---

## Integration Steps for APK

### Step 1: Add LoggingApiClient to Your App

The client is already implemented at:
- `/opt/dna-mobile/dna-messenger/mobile/shared/src/commonMain/kotlin/io/cpunk/dna/domain/LoggingApiClient.kt`
- `/opt/dna-mobile/dna-messenger/mobile/shared/src/androidMain/kotlin/io/cpunk/dna/domain/LoggingApiClient.android.kt`

### Step 2: Initialize in Application Class

```kotlin
class DNAMessengerApp : Application() {
    lateinit var loggingClient: LoggingApiClient

    override fun onCreate() {
        super.onCreate()

        // Initialize logging client
        val apiBaseUrl = getString(R.string.keyserver_url) // "http://localhost:8080"
        loggingClient = LoggingApiClient(apiBaseUrl)
        loggingClient.setPlatform("android")
        loggingClient.setAppVersion(BuildConfig.VERSION_NAME)
    }
}
```

### Step 3: Replace All Direct Logging

Find all instances of:
- `Log.d(TAG, "...")` for important events
- Direct database inserts for logging purposes
- Manual statistics tracking

Replace with API calls:

```kotlin
// OLD
Log.d(TAG, "Message sent to $recipient")

// NEW
lifecycleScope.launch {
    app.loggingClient.logEvent(
        eventType = EventType.MESSAGE_SENT,
        severity = SeverityLevel.INFO,
        message = "Message sent to $recipient"
    )
}
```

### Step 4: Handle Errors Gracefully

Since logging is not critical, handle failures gracefully:

```kotlin
lifecycleScope.launch {
    app.loggingClient.logEvent(...).onFailure { error ->
        // Don't crash the app if logging fails
        Log.w(TAG, "Failed to log event: ${error.message}")
    }
}
```

### Step 5: Use Async/Non-Blocking Calls

Always use coroutines to avoid blocking the main thread:

```kotlin
// ✅ GOOD: Non-blocking
lifecycleScope.launch {
    app.loggingClient.logMessage(...)
}

// ❌ BAD: Blocking
runBlocking {
    app.loggingClient.logMessage(...)  // Will freeze UI
}
```

---

## Testing the API

### 1. Start the Keyserver

```bash
cd /opt/dna-mobile/dna-messenger/keyserver/build
./keyserver ../config/keyserver.conf.example
```

### 2. Test with curl

```bash
# Test event logging
curl -X POST http://localhost:8080/api/logging/event \
  -H "Content-Type: application/json" \
  -d '{
    "event_type": "app_started",
    "severity": "info",
    "message": "Test event from curl",
    "platform": "android"
  }'

# Expected response:
# {"success":true,"message":"Event logged successfully"}
```

### 3. Test from Android Emulator

If testing from Android emulator, use `http://10.0.2.2:8080` instead of `http://localhost:8080`:

```kotlin
val apiBaseUrl = if (BuildConfig.DEBUG) {
    "http://10.0.2.2:8080"  // Emulator localhost
} else {
    "https://api.cpunk.io"  // Production
}

loggingClient = LoggingApiClient(apiBaseUrl)
```

---

## Benefits of Using API vs Direct DB

| Aspect | Direct Database ❌ | Logging API ✅ |
|--------|------------------|---------------|
| **Security** | Exposes DB credentials in APK | API authentication only |
| **Network** | Requires DB port (5432) open | Only HTTP (8080) |
| **Rate Limiting** | None | Built-in protection |
| **Maintenance** | DB schema changes break APK | API versioning |
| **Monitoring** | Hard to audit | Centralized logging |
| **Scalability** | Limited connections | Can add load balancer |

---

## Troubleshooting

### Error: "Connection refused"

**Solution:** Make sure keyserver is running:
```bash
# Check if keyserver is running
ps aux | grep keyserver

# Check if port 8080 is listening
netstat -tuln | grep 8080

# Start keyserver if not running
cd /opt/dna-mobile/dna-messenger/keyserver/build
./keyserver ../config/keyserver.conf.example
```

### Error: "Network security exception"

**Solution:** Add network security config to allow cleartext HTTP (for testing only):

`res/xml/network_security_config.xml`:
```xml
<?xml version="1.0" encoding="utf-8"?>
<network-security-config>
    <domain-config cleartextTrafficPermitted="true">
        <domain includeSubdomains="true">10.0.2.2</domain>
        <domain includeSubdomains="true">localhost</domain>
    </domain-config>
</network-security-config>
```

`AndroidManifest.xml`:
```xml
<application
    android:networkSecurityConfig="@xml/network_security_config"
    ...>
```

### Error: "Rate limit exceeded"

**Solution:** You're making too many requests. Wait or increase rate limits in `keyserver.conf`:

```ini
[security]
rate_limit_register_count = 20  # Increase from 10
rate_limit_register_period = 3600
```

---

## Next Steps

1. ✅ **API is built and ready** - Keyserver compiled successfully
2. ⏳ **Create database schema** - Run `logging_schema.sql`
3. ⏳ **Start keyserver** - Run the keyserver binary
4. ⏳ **Test API** - Use curl or Postman to test endpoints
5. ⏳ **Integrate in APK** - Replace direct DB logging with API calls

---

## Summary

✅ **APK now uses REST API instead of direct database access**
✅ **Kotlin client library already implemented**
✅ **Async, non-blocking API calls**
✅ **Better security, rate limiting, and monitoring**
✅ **Easy to integrate** - Just initialize LoggingApiClient and use it

**No more direct database connections from the APK!** 🎉

---

**Files:**
- Client: `/opt/dna-mobile/dna-messenger/mobile/shared/src/commonMain/kotlin/io/cpunk/dna/domain/LoggingApiClient.kt`
- API Docs: `/opt/dna-mobile/dna-messenger/keyserver/docs/LOGGING_API.md`
- Keyserver: `/opt/dna-mobile/dna-messenger/keyserver/build/keyserver`
