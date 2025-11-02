package io.cpunk.dna.domain

import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive
import kotlinx.serialization.encodeToString

/**
 * Logging API Client for DNA Messenger
 *
 * Provides a client to call the logging API endpoints instead of
 * directly writing to the database.
 *
 * Usage:
 * ```
 * val client = LoggingApiClient("http://localhost:8080")
 * client.logEvent(
 *     eventType = "message_sent",
 *     severity = "info",
 *     message = "Message sent successfully",
 *     identity = "user123"
 * )
 * ```
 */
class LoggingApiClient(
    private val apiBaseUrl: String,
    private val timeoutSeconds: Int = 5,
    private var identity: String = "",
    private var platform: String = "android",
    private var appVersion: String = ""
) {
    private val json = Json { ignoreUnknownKeys = true }

    /**
     * Set the identity for logging
     */
    fun setIdentity(identity: String) {
        this.identity = identity
    }

    /**
     * Set the platform identifier
     */
    fun setPlatform(platform: String) {
        this.platform = platform
    }

    /**
     * Set the app version
     */
    fun setAppVersion(version: String) {
        this.appVersion = version
    }

    /**
     * Log a general event
     */
    suspend fun logEvent(
        eventType: String,
        severity: String = "info",
        message: String,
        detailsJson: String? = null,
        messageId: Long? = null,
        groupId: Int? = null
    ): Result<Unit> = runCatching {
        val payload = buildMap<String, Any> {
            put("event_type", eventType)
            put("severity", severity)
            put("message", message)
            if (identity.isNotEmpty()) put("identity", identity)
            if (platform.isNotEmpty()) put("platform", platform)
            if (appVersion.isNotEmpty()) put("app_version", appVersion)
            if (detailsJson != null) put("details", json.parseToJsonElement(detailsJson))
            put("client_timestamp", System.currentTimeMillis() / 1000)
            if (messageId != null && messageId > 0) put("message_id", messageId)
            if (groupId != null && groupId > 0) put("group_id", groupId)
        }

        httpPost("$apiBaseUrl/api/logging/event", payload)
    }

    /**
     * Log a message event
     */
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
    ): Result<Unit> = runCatching {
        val payload = buildMap<String, Any> {
            if (messageId != null && messageId > 0) put("message_id", messageId)
            put("sender", sender)
            put("recipient", recipient)
            if (groupId != null && groupId > 0) put("group_id", groupId)
            put("status", status)
            put("plaintext_size", plaintextSize)
            put("ciphertext_size", ciphertextSize)
            if (platform.isNotEmpty()) put("platform", platform)
            if (errorCode != null) put("error_code", errorCode)
            if (errorMessage != null) put("error_message", errorMessage)
        }

        httpPost("$apiBaseUrl/api/logging/message", payload)
    }

    /**
     * Log a connection event
     */
    suspend fun logConnection(
        connectionType: String,
        host: String,
        port: Int,
        success: Boolean,
        responseTimeMs: Int = 0,
        errorCode: String? = null,
        errorMessage: String? = null
    ): Result<Unit> = runCatching {
        val payload = buildMap<String, Any> {
            if (identity.isNotEmpty()) put("identity", identity)
            put("connection_type", connectionType)
            put("host", host)
            put("port", port)
            put("success", success)
            if (responseTimeMs > 0) put("response_time_ms", responseTimeMs)
            if (platform.isNotEmpty()) put("platform", platform)
            if (appVersion.isNotEmpty()) put("app_version", appVersion)
            if (errorCode != null) put("error_code", errorCode)
            if (errorMessage != null) put("error_message", errorMessage)
        }

        httpPost("$apiBaseUrl/api/logging/connection", payload)
    }

    /**
     * Get statistics for a time period
     */
    suspend fun getStats(
        startTime: String,
        endTime: String
    ): Result<LoggingStats> = runCatching {
        val url = "$apiBaseUrl/api/logging/stats?start_time=$startTime&end_time=$endTime"
        val response = httpGet(url)
        val jsonResponse = json.parseToJsonElement(response).jsonObject
        val statsObj = jsonResponse["statistics"]!!.jsonObject

        LoggingStats(
            totalEvents = statsObj["total_events"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0,
            totalMessages = statsObj["total_messages"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0,
            totalConnections = statsObj["total_connections"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0,
            messagesSent = statsObj["messages_sent"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0,
            messagesDelivered = statsObj["messages_delivered"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0,
            messagesFailed = statsObj["messages_failed"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0,
            connectionsSuccess = statsObj["connections_success"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0,
            connectionsFailed = statsObj["connections_failed"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0,
            errorsCount = statsObj["errors_count"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0,
            warningsCount = statsObj["warnings_count"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0
        )
    }

    /**
     * Perform HTTP POST request
     */
    private suspend fun httpPost(url: String, payload: Map<String, Any>) {
        // Platform-specific implementation (expect/actual)
        // Ignore return value - we don't need it for logging
        httpPostImpl(url, json.encodeToString(payload), timeoutSeconds)
    }

    /**
     * Perform HTTP GET request
     */
    private suspend fun httpGet(url: String): String {
        // Platform-specific implementation (expect/actual)
        return httpGetImpl(url, timeoutSeconds)
    }

}

/**
 * Statistics data class
 */
@Serializable
data class LoggingStats(
    val totalEvents: Long,
    val totalMessages: Long,
    val totalConnections: Long,
    val messagesSent: Long,
    val messagesDelivered: Long,
    val messagesFailed: Long,
    val connectionsSuccess: Long,
    val connectionsFailed: Long,
    val errorsCount: Long,
    val warningsCount: Long
)

/**
 * Event types
 */
object EventType {
    const val MESSAGE_SENT = "message_sent"
    const val MESSAGE_RECEIVED = "message_received"
    const val MESSAGE_FAILED = "message_failed"
    const val CONNECTION_SUCCESS = "connection_success"
    const val CONNECTION_FAILED = "connection_failed"
    const val AUTH_SUCCESS = "auth_success"
    const val AUTH_FAILED = "auth_failed"
    const val KEY_GENERATED = "key_generated"
    const val KEY_EXPORTED = "key_exported"
    const val GROUP_CREATED = "group_created"
    const val GROUP_JOINED = "group_joined"
    const val GROUP_LEFT = "group_left"
    const val CONTACT_ADDED = "contact_added"
    const val CONTACT_REMOVED = "contact_removed"
    const val APP_STARTED = "app_started"
    const val APP_STOPPED = "app_stopped"
    const val ERROR = "error"
    const val WARNING = "warning"
    const val INFO = "info"
    const val DEBUG = "debug"
}

/**
 * Severity levels
 */
object SeverityLevel {
    const val DEBUG = "debug"
    const val INFO = "info"
    const val WARNING = "warning"
    const val ERROR = "error"
    const val CRITICAL = "critical"
}

/**
 * Connection types
 */
object ConnectionType {
    const val DATABASE = "database"
    const val KEYSERVER = "keyserver"
    const val RPC = "rpc"
    const val PEER = "peer"
}
