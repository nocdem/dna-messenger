package io.cpunk.dna.domain

import io.cpunk.dna.domain.models.Message
import io.cpunk.dna.domain.models.Contact
import io.cpunk.dna.domain.models.Group

/**
 * DatabaseRepository - Cross-platform database operations
 *
 * Provides:
 * - Message storage and retrieval
 * - Contact management
 * - Group management
 *
 * Platform implementations:
 * - Android: PostgreSQL JDBC (ai.cpunk.io:5432)
 * - iOS: PostgreSQL via libpq or HTTP API
 */
expect class DatabaseRepository() {
    /**
     * Save encrypted message to database
     *
     * @param message Message to save
     * @return Message ID
     */
    suspend fun saveMessage(message: Message): Result<Long>

    /**
     * Load messages for conversation
     *
     * @param contactId Contact ID or group ID
     * @param limit Maximum number of messages
     * @param offset Offset for pagination
     * @return List of messages
     */
    suspend fun loadMessages(
        contactId: String,
        limit: Int = 50,
        offset: Int = 0
    ): Result<List<Message>>

    /**
     * Save contact to database
     *
     * @param contact Contact to save
     */
    suspend fun saveContact(contact: Contact): Result<Unit>

    /**
     * Load contact by ID
     *
     * @param contactId Contact ID
     * @return Contact or null
     */
    suspend fun loadContact(contactId: String): Result<Contact?>

    /**
     * Load all contacts
     *
     * @return List of contacts
     */
    suspend fun loadAllContacts(): Result<List<Contact>>

    /**
     * Delete contact
     *
     * @param contactId Contact ID
     */
    suspend fun deleteContact(contactId: String): Result<Unit>

    /**
     * Save group to database
     *
     * @param group Group to save
     */
    suspend fun saveGroup(group: Group): Result<Unit>

    /**
     * Load group by ID
     *
     * @param groupId Group ID
     * @return Group or null
     */
    suspend fun loadGroup(groupId: String): Result<Group?>

    /**
     * Load all groups
     *
     * @return List of groups
     */
    suspend fun loadAllGroups(): Result<List<Group>>

    /**
     * Delete group
     *
     * @param groupId Group ID
     */
    suspend fun deleteGroup(groupId: String): Result<Unit>

    /**
     * Close database connection
     */
    fun close()
}
