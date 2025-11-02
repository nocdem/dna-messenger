package io.cpunk.dna.domain.models

/**
 * Group data class
 * Represents a group chat in DNA Messenger
 *
 * Maps to database table: groups
 * Schema:
 * - id (integer) - auto-generated
 * - name (text) - NOT NULL
 * - description (text) - nullable
 * - creator (text) - creator identity, NOT NULL
 * - created_at (timestamp) - default CURRENT_TIMESTAMP
 * - updated_at (timestamp) - default CURRENT_TIMESTAMP
 */
data class Group(
    val id: Int = 0,                         // Database ID (auto-generated)
    val name: String,                        // Group name
    val description: String? = null,         // Group description (optional)
    val creator: String,                     // Creator identity name
    val members: List<GroupMember> = emptyList(),  // Members loaded from group_members table
    val createdAt: Long? = null,             // Unix timestamp (milliseconds)
    val updatedAt: Long? = null              // Unix timestamp (milliseconds)
)

/**
 * Group member data class
 *
 * Maps to database table: group_members
 * Schema:
 * - group_id (integer) - foreign key to groups.id
 * - member (text) - member identity name
 * - joined_at (timestamp) - default CURRENT_TIMESTAMP
 * - role (text) - 'creator', 'admin', or 'member'
 */
data class GroupMember(
    val groupId: Int,                        // Group ID
    val member: String,                      // Member identity name
    val role: GroupRole,                     // Member role
    val joinedAt: Long? = null               // Unix timestamp (milliseconds)
)

/**
 * Group role enum
 * Maps to group_members.role column
 */
enum class GroupRole {
    CREATOR,   // Group creator (original admin)
    ADMIN,     // Group administrator
    MEMBER;    // Regular member

    companion object {
        fun fromString(role: String): GroupRole {
            return when (role.lowercase()) {
                "creator" -> CREATOR
                "admin" -> ADMIN
                "member" -> MEMBER
                else -> MEMBER
            }
        }
    }

    override fun toString(): String {
        return name.lowercase()
    }
}
