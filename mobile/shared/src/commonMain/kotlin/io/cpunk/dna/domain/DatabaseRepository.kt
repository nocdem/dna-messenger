package io.cpunk.dna.domain

import io.cpunk.dna.domain.models.Message
import io.cpunk.dna.domain.models.Contact
import io.cpunk.dna.domain.models.Group
import io.cpunk.dna.domain.models.GroupMember

/**
 * DatabaseRepository - Cross-platform database operations
 *
 * DEPRECATED: Direct database access is insecure for mobile apps.
 * This should be replaced with REST API calls to a backend server.
 *
 * Provides:
 * - Message storage and retrieval (messages table)
 * - Contact management (keyserver table)
 * - Group management (groups + group_members tables)
 *
 * Platform implementations:
 * - Android: Disabled (no database credentials)
 * - iOS: Disabled (no database credentials)
 *
 * TODO: Migrate to REST API architecture
 */
expect class DatabaseRepository(
    dbHost: String = "",  // REMOVED: No hardcoded credentials
    dbPort: Int = 0,
    dbName: String = "",
    dbUser: String = "",
    dbPassword: String = ""
) {
    // ============================================================================
    // MESSAGE OPERATIONS
    // ============================================================================

    /**
     * Save encrypted message to database
     *
     * @param message Message to save
     * @return Message ID (auto-generated)
     */
    suspend fun saveMessage(message: Message): Result<Long>

    /**
     * Load conversation between two users
     *
     * @param currentUser Current user's identity
     * @param otherUser Other user's identity
     * @param limit Maximum number of messages
     * @param offset Offset for pagination
     * @return List of messages (sorted by created_at DESC)
     */
    suspend fun loadConversation(
        currentUser: String,
        otherUser: String,
        limit: Int = 50,
        offset: Int = 0
    ): Result<List<Message>>

    /**
     * Load group messages
     *
     * @param groupId Group ID
     * @param limit Maximum number of messages
     * @param offset Offset for pagination
     * @return List of messages
     */
    suspend fun loadGroupMessages(
        groupId: Int,
        limit: Int = 50,
        offset: Int = 0
    ): Result<List<Message>>

    /**
     * Update message status
     *
     * @param messageId Message ID
     * @param status New status ('sent', 'delivered', 'read')
     * @return Unit
     */
    suspend fun updateMessageStatus(
        messageId: Long,
        status: String
    ): Result<Unit>

    // ============================================================================
    // CONTACT OPERATIONS (keyserver table)
    // ============================================================================

    /**
     * Save contact to keyserver
     *
     * @param contact Contact to save
     * @return Unit
     */
    suspend fun saveContact(contact: Contact): Result<Unit>

    /**
     * Load contact by identity name
     *
     * @param identity Identity name
     * @return Contact or null
     */
    suspend fun loadContact(identity: String): Result<Contact?>

    /**
     * Load all contacts from keyserver
     *
     * @return List of contacts
     */
    suspend fun loadAllContacts(): Result<List<Contact>>

    /**
     * Delete contact from keyserver
     *
     * @param identity Identity name
     */
    suspend fun deleteContact(identity: String): Result<Unit>

    // ============================================================================
    // GROUP OPERATIONS
    // ============================================================================

    /**
     * Create group
     *
     * @param group Group to create
     * @return Group ID (auto-generated)
     */
    suspend fun createGroup(group: Group): Result<Int>

    /**
     * Load group by ID (with members)
     *
     * @param groupId Group ID
     * @return Group with members or null
     */
    suspend fun loadGroup(groupId: Int): Result<Group?>

    /**
     * Load all groups for current user
     *
     * @param userIdentity User's identity
     * @return List of groups
     */
    suspend fun loadUserGroups(userIdentity: String): Result<List<Group>>

    /**
     * Add member to group
     *
     * @param groupId Group ID
     * @param member Member to add
     * @return Unit
     */
    suspend fun addGroupMember(groupId: Int, member: GroupMember): Result<Unit>

    /**
     * Remove member from group
     *
     * @param groupId Group ID
     * @param memberIdentity Member identity to remove
     * @return Unit
     */
    suspend fun removeGroupMember(groupId: Int, memberIdentity: String): Result<Unit>

    /**
     * Delete group (CASCADE deletes members)
     *
     * @param groupId Group ID
     */
    suspend fun deleteGroup(groupId: Int): Result<Unit>

    /**
     * Close database connection
     */
    fun close()
}
