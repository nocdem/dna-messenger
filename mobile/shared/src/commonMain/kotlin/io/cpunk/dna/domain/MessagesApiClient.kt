package io.cpunk.dna.domain

import io.cpunk.dna.domain.models.Message
import io.cpunk.dna.domain.models.MessageStatus
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonPrimitive
import kotlin.io.encoding.Base64
import kotlin.io.encoding.ExperimentalEncodingApi

/**
 * Messages API Client
 *
 * Provides HTTP API access to message operations instead of direct database access.
 */
class MessagesApiClient(
    private val apiBaseUrl: String,
    private val timeoutSeconds: Int = 10
) {
    private val json = Json { ignoreUnknownKeys = true }

    /**
     * Save a message
     */
    @OptIn(ExperimentalEncodingApi::class)
    suspend fun saveMessage(
        sender: String,
        recipient: String,
        ciphertext: ByteArray,
        status: String = "pending",
        groupId: Int? = null
    ): Result<Long> = runCatching {
        val ciphertextBase64 = Base64.encode(ciphertext)

        val payload = buildMap<String, Any?> {
            put("sender", sender)
            put("recipient", recipient)
            put("ciphertext", ciphertextBase64)
            put("status", status)
            if (groupId != null && groupId > 0) {
                put("group_id", groupId)
            }
        }

        val response = httpPost("$apiBaseUrl/api/messages", payload)
        val jsonResponse = json.parseToJsonElement(response).jsonObject

        jsonResponse["message_id"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0L
    }

    /**
     * Load conversation between two users
     */
    @OptIn(ExperimentalEncodingApi::class)
    suspend fun loadConversation(
        user1: String,
        user2: String,
        limit: Int = 50,
        offset: Int = 0
    ): Result<List<Message>> = runCatching {
        val url = "$apiBaseUrl/api/messages/conversation?user1=$user1&user2=$user2&limit=$limit&offset=$offset"
        val response = httpGet(url)
        val jsonResponse = json.parseToJsonElement(response).jsonObject

        val messagesArray = jsonResponse["messages"]?.jsonArray ?: return@runCatching emptyList()

        messagesArray.map { msgElement ->
            val msgObj = msgElement.jsonObject

            Message(
                id = msgObj["id"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0L,
                sender = msgObj["sender"]?.jsonPrimitive?.content ?: "",
                recipient = msgObj["recipient"]?.jsonPrimitive?.content ?: "",
                ciphertext = Base64.decode(msgObj["ciphertext"]?.jsonPrimitive?.content ?: ""),
                ciphertextLen = msgObj["ciphertext_len"]?.jsonPrimitive?.content?.toIntOrNull() ?: 0,
                createdAt = msgObj["created_at"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0L,
                status = MessageStatus.fromString(msgObj["status"]?.jsonPrimitive?.content ?: "pending"),
                deliveredAt = msgObj["delivered_at"]?.jsonPrimitive?.content?.toLongOrNull(),
                readAt = msgObj["read_at"]?.jsonPrimitive?.content?.toLongOrNull(),
                groupId = msgObj["group_id"]?.jsonPrimitive?.content?.toIntOrNull()
            )
        }
    }

    /**
     * Load group messages
     */
    @OptIn(ExperimentalEncodingApi::class)
    suspend fun loadGroupMessages(
        groupId: Int,
        limit: Int = 50,
        offset: Int = 0
    ): Result<List<Message>> = runCatching {
        val url = "$apiBaseUrl/api/messages/group/$groupId?limit=$limit&offset=$offset"
        val response = httpGet(url)
        val jsonResponse = json.parseToJsonElement(response).jsonObject

        val messagesArray = jsonResponse["messages"]?.jsonArray ?: return@runCatching emptyList()

        messagesArray.map { msgElement ->
            val msgObj = msgElement.jsonObject

            Message(
                id = msgObj["id"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0L,
                sender = msgObj["sender"]?.jsonPrimitive?.content ?: "",
                recipient = msgObj["recipient"]?.jsonPrimitive?.content ?: "",
                ciphertext = Base64.decode(msgObj["ciphertext"]?.jsonPrimitive?.content ?: ""),
                ciphertextLen = msgObj["ciphertext_len"]?.jsonPrimitive?.content?.toIntOrNull() ?: 0,
                createdAt = msgObj["created_at"]?.jsonPrimitive?.content?.toLongOrNull() ?: 0L,
                status = MessageStatus.fromString(msgObj["status"]?.jsonPrimitive?.content ?: "pending"),
                deliveredAt = msgObj["delivered_at"]?.jsonPrimitive?.content?.toLongOrNull(),
                readAt = msgObj["read_at"]?.jsonPrimitive?.content?.toLongOrNull(),
                groupId = msgObj["group_id"]?.jsonPrimitive?.content?.toIntOrNull()
            )
        }
    }

    /**
     * Update message status
     */
    suspend fun updateMessageStatus(
        messageId: Long,
        status: String
    ): Result<Unit> = runCatching {
        val payload = mapOf("status" to status)
        httpPatch("$apiBaseUrl/api/messages/$messageId/status", payload)
    }

    /**
     * Platform-specific HTTP POST
     */
    private suspend fun httpPost(url: String, payload: Map<String, Any?>): String {
        return httpPostImpl(url, json.encodeToString(payload), timeoutSeconds)
    }

    /**
     * Platform-specific HTTP GET
     */
    private suspend fun httpGet(url: String): String {
        return httpGetImpl(url, timeoutSeconds)
    }

    /**
     * Platform-specific HTTP PATCH
     */
    private suspend fun httpPatch(url: String, payload: Map<String, Any?>): String {
        return httpPatchImpl(url, json.encodeToString(payload), timeoutSeconds)
    }
}
