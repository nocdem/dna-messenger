package io.cpunk.dna.domain

import android.util.Log
import io.cpunk.dna.domain.models.Message
import io.cpunk.dna.domain.models.Contact
import io.cpunk.dna.domain.models.Group
import java.sql.Connection
import java.sql.DriverManager
import java.sql.PreparedStatement
import java.sql.ResultSet

/**
 * Android implementation of DatabaseRepository using PostgreSQL JDBC
 *
 * Connects to ai.cpunk.io:5432
 */
actual class DatabaseRepository {
    private var connection: Connection? = null

    init {
        try {
            Class.forName("org.postgresql.Driver")
            Log.d(TAG, "PostgreSQL driver loaded")
        } catch (e: ClassNotFoundException) {
            Log.e(TAG, "PostgreSQL driver not found", e)
        }
    }

    /**
     * Establish database connection
     */
    private fun connect(): Connection {
        if (connection == null || connection?.isClosed == true) {
            connection = DriverManager.getConnection(
                "jdbc:postgresql://ai.cpunk.io:5432/dna_messenger",
                "dna_user",  // TODO: Move to secure config
                "dna_pass"   // TODO: Move to secure config
            )
            Log.d(TAG, "Database connection established")
        }
        return connection!!
    }

    /**
     * Save encrypted message to database
     */
    actual suspend fun saveMessage(message: Message): Result<Long> {
        return runCatching {
            val conn = connect()
            val sql = """
                INSERT INTO messages (sender_id, recipient_id, content, timestamp, is_read, message_type)
                VALUES (?, ?, ?, ?, ?, ?)
                RETURNING id
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, message.senderId)
            stmt.setString(2, message.recipientId)
            stmt.setBytes(3, message.content)
            stmt.setLong(4, message.timestamp)
            stmt.setBoolean(5, message.isRead)
            stmt.setString(6, message.messageType.name)

            val rs: ResultSet = stmt.executeQuery()
            if (rs.next()) {
                val id = rs.getLong(1)
                Log.d(TAG, "Message saved with ID: $id")
                id
            } else {
                throw RuntimeException("Failed to save message")
            }
        }
    }

    /**
     * Load messages for conversation
     */
    actual suspend fun loadMessages(
        contactId: String,
        limit: Int,
        offset: Int
    ): Result<List<Message>> {
        return runCatching {
            val conn = connect()
            val sql = """
                SELECT id, sender_id, recipient_id, content, timestamp, is_read, message_type
                FROM messages
                WHERE sender_id = ? OR recipient_id = ?
                ORDER BY timestamp DESC
                LIMIT ? OFFSET ?
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, contactId)
            stmt.setString(2, contactId)
            stmt.setInt(3, limit)
            stmt.setInt(4, offset)

            val rs: ResultSet = stmt.executeQuery()
            val messages = mutableListOf<Message>()

            while (rs.next()) {
                messages.add(
                    Message(
                        id = rs.getLong("id"),
                        senderId = rs.getString("sender_id"),
                        recipientId = rs.getString("recipient_id"),
                        content = rs.getBytes("content"),
                        timestamp = rs.getLong("timestamp"),
                        isRead = rs.getBoolean("is_read"),
                        messageType = io.cpunk.dna.domain.models.MessageType.valueOf(
                            rs.getString("message_type")
                        )
                    )
                )
            }

            Log.d(TAG, "Loaded ${messages.size} messages for contact: $contactId")
            messages
        }
    }

    /**
     * Save contact to database
     */
    actual suspend fun saveContact(contact: Contact): Result<Unit> {
        return runCatching {
            val conn = connect()
            val sql = """
                INSERT INTO contacts (id, name, encryption_pubkey, signing_pubkey, last_seen, is_online, avatar_url)
                VALUES (?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT (id) DO UPDATE SET
                    name = EXCLUDED.name,
                    encryption_pubkey = EXCLUDED.encryption_pubkey,
                    signing_pubkey = EXCLUDED.signing_pubkey,
                    last_seen = EXCLUDED.last_seen,
                    is_online = EXCLUDED.is_online,
                    avatar_url = EXCLUDED.avatar_url
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, contact.id)
            stmt.setString(2, contact.name)
            stmt.setBytes(3, contact.encryptionPublicKey)
            stmt.setBytes(4, contact.signingPublicKey)
            stmt.setObject(5, contact.lastSeen)
            stmt.setBoolean(6, contact.isOnline)
            stmt.setString(7, contact.avatarUrl)

            stmt.executeUpdate()
            Log.d(TAG, "Contact saved: ${contact.name}")
        }
    }

    /**
     * Load contact by ID
     */
    actual suspend fun loadContact(contactId: String): Result<Contact?> {
        return runCatching {
            val conn = connect()
            val sql = """
                SELECT id, name, encryption_pubkey, signing_pubkey, last_seen, is_online, avatar_url
                FROM contacts
                WHERE id = ?
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, contactId)

            val rs: ResultSet = stmt.executeQuery()
            if (rs.next()) {
                Contact(
                    id = rs.getString("id"),
                    name = rs.getString("name"),
                    encryptionPublicKey = rs.getBytes("encryption_pubkey"),
                    signingPublicKey = rs.getBytes("signing_pubkey"),
                    lastSeen = rs.getObject("last_seen") as? Long,
                    isOnline = rs.getBoolean("is_online"),
                    avatarUrl = rs.getString("avatar_url")
                )
            } else {
                null
            }
        }
    }

    /**
     * Load all contacts
     */
    actual suspend fun loadAllContacts(): Result<List<Contact>> {
        return runCatching {
            val conn = connect()
            val sql = """
                SELECT id, name, encryption_pubkey, signing_pubkey, last_seen, is_online, avatar_url
                FROM contacts
                ORDER BY name
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            val rs: ResultSet = stmt.executeQuery()
            val contacts = mutableListOf<Contact>()

            while (rs.next()) {
                contacts.add(
                    Contact(
                        id = rs.getString("id"),
                        name = rs.getString("name"),
                        encryptionPublicKey = rs.getBytes("encryption_pubkey"),
                        signingPublicKey = rs.getBytes("signing_pubkey"),
                        lastSeen = rs.getObject("last_seen") as? Long,
                        isOnline = rs.getBoolean("is_online"),
                        avatarUrl = rs.getString("avatar_url")
                    )
                )
            }

            Log.d(TAG, "Loaded ${contacts.size} contacts")
            contacts
        }
    }

    /**
     * Delete contact
     */
    actual suspend fun deleteContact(contactId: String): Result<Unit> {
        return runCatching {
            val conn = connect()
            val sql = "DELETE FROM contacts WHERE id = ?"
            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, contactId)
            stmt.executeUpdate()
            Log.d(TAG, "Contact deleted: $contactId")
        }
    }

    /**
     * Save group to database
     */
    actual suspend fun saveGroup(group: Group): Result<Unit> {
        return runCatching {
            val conn = connect()
            val sql = """
                INSERT INTO groups (id, name, description, admin_ids, created_at, last_message_at, avatar_url)
                VALUES (?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT (id) DO UPDATE SET
                    name = EXCLUDED.name,
                    description = EXCLUDED.description,
                    admin_ids = EXCLUDED.admin_ids,
                    last_message_at = EXCLUDED.last_message_at,
                    avatar_url = EXCLUDED.avatar_url
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, group.id)
            stmt.setString(2, group.name)
            stmt.setString(3, group.description)
            stmt.setString(4, group.adminIds.joinToString(","))
            stmt.setLong(5, group.createdAt)
            stmt.setObject(6, group.lastMessageAt)
            stmt.setString(7, group.avatarUrl)

            stmt.executeUpdate()
            Log.d(TAG, "Group saved: ${group.name}")
        }
    }

    /**
     * Load group by ID
     */
    actual suspend fun loadGroup(groupId: String): Result<Group?> {
        return runCatching {
            val conn = connect()
            val sql = """
                SELECT id, name, description, admin_ids, created_at, last_message_at, avatar_url
                FROM groups
                WHERE id = ?
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, groupId)

            val rs: ResultSet = stmt.executeQuery()
            if (rs.next()) {
                Group(
                    id = rs.getString("id"),
                    name = rs.getString("name"),
                    description = rs.getString("description"),
                    members = emptyList(),  // TODO: Load from group_members table
                    adminIds = rs.getString("admin_ids").split(","),
                    createdAt = rs.getLong("created_at"),
                    lastMessageAt = rs.getObject("last_message_at") as? Long,
                    avatarUrl = rs.getString("avatar_url")
                )
            } else {
                null
            }
        }
    }

    /**
     * Load all groups
     */
    actual suspend fun loadAllGroups(): Result<List<Group>> {
        return runCatching {
            val conn = connect()
            val sql = """
                SELECT id, name, description, admin_ids, created_at, last_message_at, avatar_url
                FROM groups
                ORDER BY name
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            val rs: ResultSet = stmt.executeQuery()
            val groups = mutableListOf<Group>()

            while (rs.next()) {
                groups.add(
                    Group(
                        id = rs.getString("id"),
                        name = rs.getString("name"),
                        description = rs.getString("description"),
                        members = emptyList(),  // TODO: Load from group_members table
                        adminIds = rs.getString("admin_ids").split(","),
                        createdAt = rs.getLong("created_at"),
                        lastMessageAt = rs.getObject("last_message_at") as? Long,
                        avatarUrl = rs.getString("avatar_url")
                    )
                )
            }

            Log.d(TAG, "Loaded ${groups.size} groups")
            groups
        }
    }

    /**
     * Delete group
     */
    actual suspend fun deleteGroup(groupId: String): Result<Unit> {
        return runCatching {
            val conn = connect()
            val sql = "DELETE FROM groups WHERE id = ?"
            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, groupId)
            stmt.executeUpdate()
            Log.d(TAG, "Group deleted: $groupId")
        }
    }

    /**
     * Close database connection
     */
    actual fun close() {
        connection?.close()
        connection = null
        Log.d(TAG, "Database connection closed")
    }

    companion object {
        private const val TAG = "DatabaseRepository"
    }
}
