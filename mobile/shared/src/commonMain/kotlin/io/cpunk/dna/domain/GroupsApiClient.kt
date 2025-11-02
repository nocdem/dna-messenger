package io.cpunk.dna.domain

import io.cpunk.dna.domain.models.Group
import io.cpunk.dna.domain.models.GroupMember
import io.cpunk.dna.domain.models.GroupRole
import kotlinx.serialization.json.Json
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonPrimitive

/**
 * Groups API Client
 *
 * Provides HTTP API access to group operations instead of direct database access.
 */
class GroupsApiClient(
    private val apiBaseUrl: String,
    private val authToken: String = "",
    private val timeoutSeconds: Int = 10
) {
    private val json = Json { ignoreUnknownKeys = true }

    /**
     * Create a group
     */
    suspend fun createGroup(group: Group): Result<Int> = runCatching {
        val membersArray = group.members.map { member ->
            mapOf(
                "member" to member.member,
                "role" to member.role.toString().lowercase()
            )
        }

        val payload = buildMap<String, Any> {
            put("name", group.name)
            put("description", group.description ?: "")
            put("creator", group.creator)
            put("members", membersArray)
        }

        val response = httpPost("$apiBaseUrl/api/groups", payload)
        val jsonResponse = json.parseToJsonElement(response).jsonObject

        jsonResponse["group_id"]?.jsonPrimitive?.content?.toIntOrNull() ?: 0
    }

    /**
     * Load group by ID
     */
    suspend fun loadGroup(groupId: Int): Result<Group?> = runCatching {
        val url = "$apiBaseUrl/api/groups/$groupId"
        val response = try {
            httpGet(url)
        } catch (e: Exception) {
            // 404 means group not found
            return@runCatching null
        }

        val jsonResponse = json.parseToJsonElement(response).jsonObject
        val groupObj = jsonResponse["group"]?.jsonObject ?: return@runCatching null

        val membersArray = groupObj["members"]?.jsonArray ?: emptyList()
        val members = membersArray.map { memberElement ->
            val memberObj = memberElement.jsonObject
            GroupMember(
                groupId = groupId,
                member = memberObj["member"]?.jsonPrimitive?.content ?: "",
                role = GroupRole.fromString(memberObj["role"]?.jsonPrimitive?.content ?: "member"),
                joinedAt = memberObj["joined_at"]?.jsonPrimitive?.content?.toLongOrNull()
            )
        }

        Group(
            id = groupObj["id"]?.jsonPrimitive?.content?.toIntOrNull() ?: 0,
            name = groupObj["name"]?.jsonPrimitive?.content ?: "",
            description = groupObj["description"]?.jsonPrimitive?.content ?: "",
            creator = groupObj["creator"]?.jsonPrimitive?.content ?: "",
            members = members,
            createdAt = groupObj["created_at"]?.jsonPrimitive?.content?.toLongOrNull(),
            updatedAt = groupObj["updated_at"]?.jsonPrimitive?.content?.toLongOrNull()
        )
    }

    /**
     * Load user's groups
     */
    suspend fun loadUserGroups(userIdentity: String): Result<List<Group>> = runCatching {
        val url = "$apiBaseUrl/api/groups?member=$userIdentity"
        val response = httpGet(url)
        val jsonResponse = json.parseToJsonElement(response).jsonObject

        val groupsArray = jsonResponse["groups"]?.jsonArray ?: return@runCatching emptyList()

        groupsArray.map { groupElement ->
            val groupObj = groupElement.jsonObject

            val membersArray = groupObj["members"]?.jsonArray ?: emptyList()
            val members = membersArray.map { memberElement ->
                val memberObj = memberElement.jsonObject
                GroupMember(
                    groupId = groupObj["id"]?.jsonPrimitive?.content?.toIntOrNull() ?: 0,
                    member = memberObj["member"]?.jsonPrimitive?.content ?: "",
                    role = GroupRole.fromString(memberObj["role"]?.jsonPrimitive?.content ?: "member"),
                    joinedAt = memberObj["joined_at"]?.jsonPrimitive?.content?.toLongOrNull()
                )
            }

            Group(
                id = groupObj["id"]?.jsonPrimitive?.content?.toIntOrNull() ?: 0,
                name = groupObj["name"]?.jsonPrimitive?.content ?: "",
                description = groupObj["description"]?.jsonPrimitive?.content ?: "",
                creator = groupObj["creator"]?.jsonPrimitive?.content ?: "",
                members = members,
                createdAt = groupObj["created_at"]?.jsonPrimitive?.content?.toLongOrNull(),
                updatedAt = groupObj["updated_at"]?.jsonPrimitive?.content?.toLongOrNull()
            )
        }
    }

    /**
     * Add member to group
     */
    suspend fun addGroupMember(groupId: Int, member: GroupMember): Result<Unit> = runCatching {
        val payload = mapOf(
            "member" to member.member,
            "role" to member.role.toString().lowercase()
        )

        httpPost("$apiBaseUrl/api/groups/$groupId/members", payload)
        Unit
    }

    /**
     * Remove member from group
     */
    suspend fun removeGroupMember(groupId: Int, memberIdentity: String): Result<Unit> = runCatching {
        httpDelete("$apiBaseUrl/api/groups/$groupId/members/$memberIdentity")
        Unit
    }

    /**
     * Delete group
     */
    suspend fun deleteGroup(groupId: Int): Result<Unit> = runCatching {
        httpDelete("$apiBaseUrl/api/groups/$groupId")
        Unit
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
