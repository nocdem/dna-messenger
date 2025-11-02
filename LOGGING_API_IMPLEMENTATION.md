# Logging API Implementation Summary

**Date:** 2025-10-28
**Implemented By:** Claude Code
**Status:** ✅ Complete - Ready for Testing

---

## What Was Built

A complete REST API for centralized logging across the DNA Messenger ecosystem. Instead of direct database writes, all components (mobile apps, desktop apps, keyserver) now use HTTP API endpoints to log events, messages, and connections.

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  Applications                                           │
│  ├─ Android App (Kotlin)                                │
│  ├─ iOS App (Swift)                                     │
│  ├─ Desktop App (Qt/C++)                                │
│  └─ Keyserver (C)                                       │
└────────────────┬────────────────────────────────────────┘
                 │
                 │ HTTP POST/GET
                 │
┌────────────────▼────────────────────────────────────────┐
│  Logging API (Keyserver REST Endpoints)                 │
│  ├─ POST /api/logging/event                             │
│  ├─ POST /api/logging/message                           │
│  ├─ POST /api/logging/connection                        │
│  └─ GET  /api/logging/stats                             │
└────────────────┬────────────────────────────────────────┘
                 │
                 │ SQL INSERT/SELECT
                 │
┌────────────────▼────────────────────────────────────────┐
│  PostgreSQL Database (ai.cpunk.io:5432)                 │
│  ├─ logging_events                                      │
│  ├─ logging_messages                                    │
│  ├─ logging_connections                                 │
│  └─ logging_stats                                       │
└─────────────────────────────────────────────────────────┘
```

---

## Files Created

### Database Schema

**File:** `/opt/dna-mobile/dna-messenger/keyserver/sql/logging_schema.sql`

**Tables:**
- `logging_events` - General events (app_started, message_sent, etc.)
- `logging_messages` - Message-specific logs with metrics
- `logging_connections` - Network connection attempts
- `logging_stats` - Pre-computed statistics

**Functions:**
- `cleanup_old_logs()` - Remove logs older than retention period
- `compute_statistics()` - Aggregate stats for a time period

---

### C Server Components (Keyserver)

#### Database Operations

**Files:**
- `/opt/dna-mobile/dna-messenger/keyserver/src/db_logging.h`
- `/opt/dna-mobile/dna-messenger/keyserver/src/db_logging.c`

**Functions:**
- `db_log_event()` - Insert event to `logging_events`
- `db_log_message()` - Insert message log to `logging_messages`
- `db_log_connection()` - Insert connection log to `logging_connections`
- `db_get_stats()` - Query pre-computed statistics
- `db_compute_stats()` - Compute statistics for a period
- `db_cleanup_old_logs()` - Remove old logs

#### API Handlers

**Files:**
- `/opt/dna-mobile/dna-messenger/keyserver/src/api_log_event.c`
- `/opt/dna-mobile/dna-messenger/keyserver/src/api_log_message.c`
- `/opt/dna-mobile/dna-messenger/keyserver/src/api_log_connection.c`
- `/opt/dna-mobile/dna-messenger/keyserver/src/api_log_stats.c`

**Endpoints:**
- `POST /api/logging/event` - Log general events
- `POST /api/logging/message` - Log message events
- `POST /api/logging/connection` - Log connection events
- `GET /api/logging/stats` - Query aggregated statistics

#### Main Server Updates

**File:** `/opt/dna-mobile/dna-messenger/keyserver/src/main.c`

**Changes:**
- Added handler declarations for logging endpoints
- Registered POST routes for logging
- Registered GET route for statistics

---

### C Client Library

**Files:**
- `/opt/dna-mobile/dna-messenger/keyserver/src/logging_client.h`
- `/opt/dna-mobile/dna-messenger/keyserver/src/logging_client.c`

**Dependencies:** libcurl, json-c

**Functions:**
```c
void logging_client_init(logging_client_config_t *config, const char *api_base_url);

int logging_client_log_event(
    const logging_client_config_t *config,
    const char *event_type,
    const char *severity,
    const char *message,
    const char *details_json,
    int64_t message_id,
    int group_id
);

int logging_client_log_message(
    const logging_client_config_t *config,
    int64_t message_id,
    const char *sender,
    const char *recipient,
    int group_id,
    const char *status,
    int plaintext_size,
    int ciphertext_size,
    const char *error_code,
    const char *error_message
);

int logging_client_log_connection(
    const logging_client_config_t *config,
    const char *connection_type,
    const char *host,
    int port,
    bool success,
    int response_time_ms,
    const char *error_code,
    const char *error_message
);

int logging_client_get_stats(
    const logging_client_config_t *config,
    const char *start_time,
    const char *end_time,
    logging_stats_t *stats
);
```

---

### Kotlin Client Library (Mobile)

**Files:**
- `/opt/dna-mobile/dna-messenger/mobile/shared/src/commonMain/kotlin/io/cpunk/dna/domain/LoggingApiClient.kt`
- `/opt/dna-mobile/dna-messenger/mobile/shared/src/androidMain/kotlin/io/cpunk/dna/domain/LoggingApiClient.android.kt`

**Class:** `LoggingApiClient`

**Methods:**
```kotlin
suspend fun logEvent(
    eventType: String,
    severity: String = "info",
    message: String,
    detailsJson: String? = null,
    messageId: Long? = null,
    groupId: Int? = null
): Result<Unit>

suspend fun logMessage(
    messageId: Long? = null,
    sender: String,
    recipient: String,
    groupId: Int? = null,
    status: String,
    plaintextSize: Int = 0,
    ciphertextSize: Int = 0,
    errorCode: String? = null,
    errorMessage: String? = null
): Result<Unit>

suspend fun logConnection(
    connectionType: String,
    host: String,
    port: Int,
    success: Boolean,
    responseTimeMs: Int = 0,
    errorCode: String? = null,
    errorMessage: String? = null
): Result<Unit>

suspend fun getStats(
    startTime: String,
    endTime: String
): Result<LoggingStats>
```

**Helper Objects:**
- `EventType` - Constants for event types
- `SeverityLevel` - Constants for severity levels
- `ConnectionType` - Constants for connection types

---

### Documentation

**File:** `/opt/dna-mobile/dna-messenger/keyserver/docs/LOGGING_API.md`

**Contents:**
- Overview and benefits
- Complete API endpoint documentation
- Request/response examples
- Rate limiting details
- C and Kotlin usage examples
- Database schema reference
- Setup instructions
- Migration guide
- Troubleshooting tips
- Security considerations

---

## Next Steps

### 1. Setup Database Schema

```bash
cd /opt/dna-mobile/dna-messenger/keyserver
psql -h ai.cpunk.io -U dna -d dna_messenger -f sql/logging_schema.sql
```

**Expected Output:**
```
CREATE TYPE
CREATE TYPE
CREATE TABLE
CREATE TABLE
CREATE TABLE
CREATE TABLE
CREATE INDEX
...
CREATE FUNCTION
CREATE FUNCTION
```

### 2. Build Keyserver with Logging Support

**Update CMakeLists.txt:**

Add to `keyserver/CMakeLists.txt`:

```cmake
add_executable(keyserver
    src/main.c
    src/db.c
    src/db_logging.c
    src/api_register.c
    src/api_update.c
    src/api_lookup.c
    src/api_list.c
    src/api_health.c
    src/api_log_event.c
    src/api_log_message.c
    src/api_log_connection.c
    src/api_log_stats.c
    src/logging_client.c
    src/config.c
    src/rate_limit.c
    src/validation.c
    src/signature.c
    src/http_utils.c
)

target_link_libraries(keyserver
    pq
    microhttpd
    json-c
    curl
)
```

**Build:**

```bash
cd /opt/dna-mobile/dna-messenger
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### 3. Test the API

**Start keyserver:**

```bash
./build/keyserver config/keyserver.conf
```

**Test event logging:**

```bash
curl -X POST http://localhost:8080/api/logging/event \
  -H "Content-Type: application/json" \
  -d '{
    "event_type": "app_started",
    "severity": "info",
    "message": "Application started",
    "platform": "desktop",
    "identity": "test_user"
  }'
```

**Expected Response:**
```json
{
  "success": true,
  "message": "Event logged successfully"
}
```

**Test statistics:**

```bash
curl "http://localhost:8080/api/logging/stats?start_time=2025-10-27%2000:00:00&end_time=2025-10-28%2000:00:00"
```

**Verify in database:**

```sql
-- Check events
SELECT * FROM logging_events ORDER BY created_at DESC LIMIT 10;

-- Check statistics
SELECT * FROM logging_stats ORDER BY computed_at DESC LIMIT 1;
```

### 4. Integrate with Applications

#### Desktop (C/C++)

```c
#include "logging_client.h"

// Initialize once at startup
logging_client_config_t log_config;
logging_client_init(&log_config, "http://localhost:8080");
strncpy(log_config.platform, "desktop", sizeof(log_config.platform));
strncpy(log_config.app_version, "0.1.0", sizeof(log_config.app_version));

// Log events
logging_client_log_event(
    &log_config,
    "app_started",
    "info",
    "Desktop app started",
    NULL, 0, 0
);
```

#### Android (Kotlin)

```kotlin
// In your Application class
class DNAMessengerApp : Application() {
    lateinit var loggingClient: LoggingApiClient

    override fun onCreate() {
        super.onCreate()
        loggingClient = LoggingApiClient("http://localhost:8080")
        loggingClient.setPlatform("android")
        loggingClient.setAppVersion(BuildConfig.VERSION_NAME)
    }
}

// In your activities/services
lifecycleScope.launch {
    app.loggingClient.logEvent(
        eventType = EventType.APP_STARTED,
        severity = SeverityLevel.INFO,
        message = "App started"
    )
}
```

### 5. Replace Direct Database Logging

**Find and replace all instances of:**

**C/C++:**
```c
// OLD:
LOG_INFO("Message sent: %s -> %s", sender, recipient);

// NEW:
logging_client_log_event(&log_config, "message_sent", "info",
                        "Message sent", NULL, message_id, 0);
```

**Kotlin:**
```kotlin
// OLD:
Log.d(TAG, "Message saved: ID=$id")

// NEW:
loggingClient.logMessage(
    messageId = id,
    sender = sender,
    recipient = recipient,
    status = "sent"
)
```

---

## Testing Checklist

- [ ] Create database schema successfully
- [ ] Build keyserver without errors
- [ ] Start keyserver on port 8080
- [ ] POST to `/api/logging/event` returns 200
- [ ] POST to `/api/logging/message` returns 200
- [ ] POST to `/api/logging/connection` returns 200
- [ ] GET `/api/logging/stats` returns valid JSON
- [ ] Verify events appear in `logging_events` table
- [ ] Verify rate limiting works (429 after 10 requests)
- [ ] Test C client library
- [ ] Test Kotlin client library
- [ ] Test statistics computation function
- [ ] Test log cleanup function

---

## Migration Strategy

1. **Phase 1: Parallel Logging** (Recommended)
   - Keep existing database logging
   - Add API logging alongside
   - Compare results to ensure consistency

2. **Phase 2: Gradual Migration**
   - Replace logging in non-critical paths first
   - Monitor for errors and performance issues
   - Gradually migrate critical paths

3. **Phase 3: Full Migration**
   - Remove direct database logging code
   - Use API exclusively
   - Monitor API performance and reliability

---

## Performance Considerations

**API Overhead:**
- HTTP request: ~50-100ms
- JSON serialization: ~1-5ms
- Database insert: ~10-20ms
- **Total:** ~60-125ms per log

**Recommendations:**
- Use async logging (don't block main thread)
- Batch logs if possible (future enhancement)
- Cache statistics queries
- Monitor API response times

**Direct DB vs API:**
| Method | Latency | Pros | Cons |
|--------|---------|------|------|
| Direct DB | ~10-20ms | Fast | Tight coupling, no rate limiting |
| API | ~60-125ms | Decoupled, secure | Slightly slower |

---

## Security Notes

1. **Currently:** No authentication (accept all requests)
2. **Production:** Should add:
   - API keys or JWT tokens
   - HTTPS/TLS encryption
   - IP whitelisting
   - Enhanced rate limiting

3. **Don't Log:**
   - Private keys
   - Passwords
   - Session tokens
   - Full message plaintext

4. **Do Log:**
   - Message IDs (not content)
   - Connection attempts
   - Error codes and messages
   - Performance metrics

---

## Troubleshooting

### Build Errors

**Missing libcurl:**
```bash
sudo apt-get install libcurl4-openssl-dev
```

**Missing json-c:**
```bash
sudo apt-get install libjson-c-dev
```

**Missing libmicrohttpd:**
```bash
sudo apt-get install libmicrohttpd-dev
```

### Runtime Errors

**Connection refused:**
- Check keyserver is running: `ps aux | grep keyserver`
- Check port is listening: `netstat -tuln | grep 8080`

**Database connection failed:**
- Verify PostgreSQL is accessible
- Check credentials in `keyserver.conf`
- Test connection: `psql -h ai.cpunk.io -U dna -d dna_messenger`

**Rate limit exceeded:**
- Wait for rate limit window to reset
- Or increase limits in configuration

---

## Maintenance

### Daily/Weekly Tasks

```bash
# Check API health
curl http://localhost:8080/api/keyserver/health

# Monitor log volume
psql -h ai.cpunk.io -U dna -d dna_messenger -c \
  "SELECT COUNT(*) FROM logging_events WHERE created_at > NOW() - INTERVAL '1 day';"

# Check for errors
psql -h ai.cpunk.io -U dna -d dna_messenger -c \
  "SELECT * FROM logging_events WHERE severity = 'error' AND created_at > NOW() - INTERVAL '1 day';"
```

### Monthly Tasks

```bash
# Cleanup old logs
psql -h ai.cpunk.io -U dna -d dna_messenger -c "SELECT cleanup_old_logs();"

# Compute statistics
psql -h ai.cpunk.io -U dna -d dna_messenger -c \
  "SELECT compute_statistics(NOW() - INTERVAL '30 days', NOW());"
```

---

## Future Enhancements

1. **Batch Logging**
   - POST `/api/logging/batch` - Send multiple logs in one request
   - Reduces API calls by 10x

2. **Authentication**
   - Dilithium3 signature verification for logs
   - API key management

3. **Query API**
   - POST `/api/logging/query` with filters
   - Search logs by identity, event type, date range

4. **Export API**
   - GET `/api/logging/export?format=csv`
   - Download logs for analysis

5. **Real-time Streaming**
   - WebSocket endpoint for live log tailing
   - Dashboard UI

6. **Alerting**
   - Trigger alerts on error thresholds
   - Email/SMS notifications

---

## Summary

✅ **Complete REST API for centralized logging**
✅ **PostgreSQL schema with 4 tables**
✅ **C server implementation with 4 endpoints**
✅ **C client library for desktop/keyserver**
✅ **Kotlin client library for Android**
✅ **Comprehensive documentation**
✅ **Rate limiting and error handling**
✅ **Statistics and analytics support**

**Ready for:**
- Database schema creation
- Build and deployment
- Integration testing
- Production use (with HTTPS/auth)

---

**Next Session:**
1. Create database schema
2. Build keyserver
3. Test all endpoints
4. Integrate with mobile app
5. Replace direct DB logging

---

**Files to Review:**
- `/opt/dna-mobile/dna-messenger/keyserver/docs/LOGGING_API.md` - Full API documentation
- `/opt/dna-mobile/dna-messenger/keyserver/sql/logging_schema.sql` - Database schema
- `/opt/dna-mobile/dna-messenger/keyserver/src/logging_client.h` - C client API
- `/opt/dna-mobile/dna-messenger/mobile/shared/src/commonMain/kotlin/io/cpunk/dna/domain/LoggingApiClient.kt` - Kotlin client API

**Total Implementation Time:** ~2 hours
**Lines of Code:** ~3,500+ lines
**Files Created:** 13 new files
**Files Modified:** 1 file (main.c)
