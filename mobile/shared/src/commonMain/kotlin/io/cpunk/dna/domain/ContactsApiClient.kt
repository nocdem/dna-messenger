package io.cpunk.dna.domain

import io.cpunk.dna.domain.models.Contact
import kotlinx.serialization.json.Json
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonPrimitive
import kotlin.io.encoding.Base64
import kotlin.io.encoding.ExperimentalEncodingApi

/**
 * Contacts API Client
 *
 * Provides HTTP API access to contact operations instead of direct database access.
 */
class ContactsApiClient(
    private val apiBaseUrl: String,
    private val authToken: String = "",
    private val timeoutSeconds: Int = 10
) {
    private val json = Json { ignoreUnknownKeys = true }

    /**
     * Save or update a contact (registers to C keyserver)
     */
    @OptIn(ExperimentalEncodingApi::class)
    suspend fun saveContact(contact: Contact): Result<Unit> = runCatching {
        val payload = buildMap<String, Any> {
            put("v", 1)  // Schema version
            put("dna", contact.identity)
            put("dilithium_pub", Base64.encode(contact.signingPubkey))
            put("kyber_pub", Base64.encode(contact.encryptionPubkey))
            put("cf20pub", "")  // Cellframe address (empty for now)
            put("version", 1)  // Monotonic version
            put("updated_at", (System.currentTimeMillis() / 1000).toInt())
            put("sig", "")  // TODO: Add Dilithium signature
        }

        httpPost("$apiBaseUrl/api/keyserver/register", payload)
        Unit
    }

    /**
     * Load contact by identity (from C keyserver)
     */
    @OptIn(ExperimentalEncodingApi::class)
    suspend fun loadContact(identity: String): Result<Contact?> = runCatching {
        val url = "$apiBaseUrl/api/keyserver/lookup/$identity"
        val response = try {
            httpGet(url)
        } catch (e: Exception) {
            // 404 means contact not found
            return@runCatching null
        }

        val jsonResponse = json.parseToJsonElement(response).jsonObject
        if (jsonResponse["success"]?.jsonPrimitive?.content != "true") {
            return@runCatching null
        }

        val dataObj = jsonResponse["data"]?.jsonObject ?: return@runCatching null

        Contact(
            id = 0,  // C keyserver doesn't return numeric ID
            identity = dataObj["dna"]?.jsonPrimitive?.content ?: "",
            signingPubkey = Base64.decode(dataObj["dilithium_pub"]?.jsonPrimitive?.content ?: ""),
            signingPubkeyLen = Base64.decode(dataObj["dilithium_pub"]?.jsonPrimitive?.content ?: "").size,
            encryptionPubkey = Base64.decode(dataObj["kyber_pub"]?.jsonPrimitive?.content ?: ""),
            encryptionPubkeyLen = Base64.decode(dataObj["kyber_pub"]?.jsonPrimitive?.content ?: "").size,
            fingerprint = "",  // Not stored in C keyserver
            createdAt = dataObj["updated_at"]?.jsonPrimitive?.content?.toLongOrNull()
        )
    }

    /**
     * Load all contacts (from C keyserver)
     */
    @OptIn(ExperimentalEncodingApi::class)
    suspend fun loadAllContacts(): Result<List<Contact>> = runCatching {
        val url = "$apiBaseUrl/api/keyserver/list"
        val response = httpGet(url)

        // Log raw response for debugging
        println("ContactsApiClient: Raw response from $url: $response")

        val jsonResponse = try {
            json.parseToJsonElement(response).jsonObject
        } catch (e: Exception) {
            println("ContactsApiClient: Failed to parse JSON response: ${e.message}")
            println("ContactsApiClient: Response was: $response")
            throw Exception("Invalid JSON response from server: ${e.message}")
        }

        if (jsonResponse["success"]?.jsonPrimitive?.content != "true") {
            return@runCatching emptyList()
        }

        val identitiesArray = jsonResponse["identities"]?.jsonArray ?: return@runCatching emptyList()

        // For list endpoint, we only get basic info, need to fetch full details for each
        identitiesArray.mapNotNull { identityElement ->
            val identityObj = identityElement.jsonObject
            val dna = identityObj["dna"]?.jsonPrimitive?.content ?: return@mapNotNull null

            // Note: list endpoint doesn't return full keys, just metadata
            // For now, return minimal contact info
            Contact(
                id = 0,
                identity = dna,
                signingPubkey = ByteArray(0),  // Would need separate lookup
                signingPubkeyLen = 0,
                encryptionPubkey = ByteArray(0),  // Would need separate lookup
                encryptionPubkeyLen = 0,
                fingerprint = "",
                createdAt = null
            )
        }
    }

    /**
     * Delete a contact (not supported by C keyserver)
     */
    suspend fun deleteContact(identity: String): Result<Unit> = runCatching {
        // C keyserver doesn't support deletion
        // Keys are permanent once registered
        throw UnsupportedOperationException("C keyserver does not support key deletion")
    }

    /**
     * Platform-specific HTTP POST
     */
    private suspend fun httpPost(url: String, payload: Map<String, Any>): String {
        return httpPostImpl(url, json.encodeToString(payload), timeoutSeconds, authToken.ifEmpty { null })
    }

    /**
     * Platform-specific HTTP GET
     */
    private suspend fun httpGet(url: String): String {
        return httpGetImpl(url, timeoutSeconds, authToken.ifEmpty { null })
    }

    /**
     * Platform-specific HTTP DELETE
     */
    private suspend fun httpDelete(url: String): String {
        return httpDeleteImpl(url, timeoutSeconds, authToken.ifEmpty { null })
    }
}
