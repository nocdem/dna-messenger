package io.cpunk.dna.domain

import kotlinx.serialization.json.Json
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive
import kotlinx.serialization.encodeToString

/**
 * Authentication API Client
 *
 * Provides login and registration functionality
 */
class AuthApiClient(
    private val apiBaseUrl: String,
    private val timeoutSeconds: Int = 10
) {
    private val json = Json { ignoreUnknownKeys = true }

    /**
     * Login with identity
     * @return JWT token
     */
    suspend fun login(identity: String): Result<String> = runCatching {
        val payload = """{"identity":"$identity"}"""
        val response = httpPostImpl("$apiBaseUrl/api/auth/login", payload, timeoutSeconds, null)
        val jsonResponse = json.parseToJsonElement(response).jsonObject

        jsonResponse["token"]?.jsonPrimitive?.content ?: throw RuntimeException("No token in response")
    }
}
