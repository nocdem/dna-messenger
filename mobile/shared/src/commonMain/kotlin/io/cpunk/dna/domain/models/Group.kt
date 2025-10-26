package io.cpunk.dna.domain.models

/**
 * Group data class
 * Represents a group chat in DNA Messenger
 */
data class Group(
    val id: String,
    val name: String,
    val description: String? = null,
    val members: List<String>,  // Contact IDs
    val adminIds: List<String>,
    val createdAt: Long,
    val lastMessageAt: Long? = null,
    val avatarUrl: String? = null
)

/**
 * Group member data class
 */
data class GroupMember(
    val groupId: String,
    val contactId: String,
    val role: GroupRole,
    val joinedAt: Long
)

enum class GroupRole {
    ADMIN,
    MEMBER
}
