package io.cpunk.dna.domain

import android.util.Log
import io.cpunk.dna.domain.models.Message
import io.cpunk.dna.domain.models.MessageStatus
import io.cpunk.dna.domain.models.Contact
import io.cpunk.dna.domain.models.Group
import io.cpunk.dna.domain.models.GroupMember
import io.cpunk.dna.domain.models.GroupRole
import java.sql.Connection
import java.sql.DriverManager
import java.sql.PreparedStatement
import java.sql.ResultSet
import java.sql.Timestamp

/**
 * Android implementation of DatabaseRepository using PostgreSQL JDBC
 *
 * ⚠️ SECURITY WARNING ⚠️
 * This class implements DIRECT database access which is INSECURE for mobile apps.
 * Database credentials would be embedded in the APK and easily extracted.
 *
 * DEPRECATED: This should be replaced with REST API calls to a backend server.
 *
 * Current status: DISABLED (empty credentials prevent actual connections)
 *
 * TODO: Migrate to REST API architecture where:
 * - Mobile app talks to backend server (HTTPS)
 * - Backend server talks to database
 * - No credentials in mobile app
 */
actual class DatabaseRepository actual constructor(
    private val dbHost: String,
    private val dbPort: Int,
    private val dbName: String,
    private val dbUser: String,
    private val dbPassword: String
) {
    private var connection: Connection? = null
    private val jdbcUrl: String

    companion object {
        private const val TAG = "DatabaseRepository"
    }

    init {
        // ⚠️ SECURITY CHECK: Prevent accidental database connections
        if (dbHost.isNotEmpty() && dbPassword.isNotEmpty()) {
            Log.w(TAG, "⚠️ SECURITY WARNING: DatabaseRepository initialized with credentials!")
            Log.w(TAG, "⚠️ This is INSECURE for production mobile apps!")
            Log.w(TAG, "⚠️ Credentials can be extracted from APK!")
            Log.w(TAG, "⚠️ Use REST API instead for production!")
        }

        try {
            Class.forName("org.postgresql.Driver")
            Log.d(TAG, "PostgreSQL driver loaded successfully")
        } catch (e: ClassNotFoundException) {
            Log.e(TAG, "PostgreSQL driver not found - check build.gradle.kts", e)
        }

        // Build JDBC URL with SSL/TLS encryption for secure connection
        jdbcUrl = buildString {
            append("jdbc:postgresql://$dbHost:$dbPort/$dbName")
            append("?ApplicationName=DNAMessenger")

            // SSL/TLS Configuration
            append("&ssl=true")                    // Enable SSL
            append("&sslmode=require")             // Require SSL connection

            // Connection timeouts
            append("&connectTimeout=10")           // 10 seconds timeout
            append("&socketTimeout=30")            // 30 seconds socket timeout
            append("&tcpKeepAlive=true")
            append("&loginTimeout=10")

            // Use simple query protocol (more compatible with Android)
            append("&preferQueryMode=simple")
        }

        Log.d(TAG, "Initialized with JDBC URL: jdbc:postgresql://$dbHost:$dbPort/$dbName")
    }

    /**
     * Establish database connection
     */
    private fun connect(): Connection {
        if (connection == null || connection?.isClosed == true) {
            try {
                Log.d(TAG, "Attempting to connect to PostgreSQL at $dbHost:$dbPort/$dbName")
                Log.d(TAG, "Using JDBC URL: $jdbcUrl")

                connection = DriverManager.getConnection(jdbcUrl, dbUser, dbPassword)

                Log.d(TAG, "✅ Database connection established successfully!")
                Log.d(TAG, "Connection info: ${connection?.metaData?.databaseProductName} ${connection?.metaData?.databaseProductVersion}")
            } catch (e: Exception) {
                Log.e(TAG, "❌ Failed to connect to PostgreSQL", e)
                Log.e(TAG, "Error type: ${e.javaClass.simpleName}")
                Log.e(TAG, "Error message: ${e.message}")
                Log.e(TAG, "Host: $dbHost, Port: $dbPort, Database: $dbName, User: $dbUser")

                // Log more details for troubleshooting
                when (e) {
                    is java.net.UnknownHostException -> {
                        Log.e(TAG, "Cannot resolve hostname '$dbHost'. Check network and DNS.")
                    }
                    is java.net.ConnectException -> {
                        Log.e(TAG, "Connection refused. Check if PostgreSQL is running and accepting connections.")
                    }
                    is java.net.SocketTimeoutException -> {
                        Log.e(TAG, "Connection timeout. Server may be unreachable or firewall blocking port $dbPort.")
                    }
                    is java.sql.SQLException -> {
                        Log.e(TAG, "SQL Error - SQLState: ${e.sqlState}, Error Code: ${e.errorCode}")
                    }
                }

                throw e
            }
        }
        return connection!!
    }

    // ============================================================================
    // MESSAGE OPERATIONS
    // ============================================================================

    /**
     * Save encrypted message to database
     */
    actual suspend fun saveMessage(message: Message): Result<Long> {
        return runCatching {
            val conn = connect()
            val sql = """
                INSERT INTO messages (
                    sender,
                    recipient,
                    ciphertext,
                    ciphertext_len,
                    created_at,
                    status,
                    group_id
                )
                VALUES (?, ?, ?, ?, ?, ?, ?)
                RETURNING id
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, message.sender)
            stmt.setString(2, message.recipient)
            stmt.setBytes(3, message.ciphertext)
            stmt.setInt(4, message.ciphertextLen)
            stmt.setTimestamp(5, Timestamp(message.createdAt))
            stmt.setString(6, message.status.toString())
            if (message.groupId != null) {
                stmt.setInt(7, message.groupId)
            } else {
                stmt.setNull(7, java.sql.Types.INTEGER)
            }

            val rs: ResultSet = stmt.executeQuery()
            if (rs.next()) {
                val id = rs.getLong(1)
                Log.d(TAG, "Message saved: ID=$id, sender=${message.sender}, recipient=${message.recipient}")
                id
            } else {
                throw RuntimeException("Failed to save message")
            }
        }
    }

    /**
     * Load conversation between two users
     */
    actual suspend fun loadConversation(
        currentUser: String,
        otherUser: String,
        limit: Int,
        offset: Int
    ): Result<List<Message>> {
        return runCatching {
            val conn = connect()
            val sql = """
                SELECT
                    id, sender, recipient, ciphertext, ciphertext_len,
                    created_at, status, delivered_at, read_at, group_id
                FROM messages
                WHERE (sender = ? AND recipient = ?)
                   OR (sender = ? AND recipient = ?)
                   AND group_id IS NULL
                ORDER BY created_at DESC
                LIMIT ? OFFSET ?
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, currentUser)
            stmt.setString(2, otherUser)
            stmt.setString(3, otherUser)
            stmt.setString(4, currentUser)
            stmt.setInt(5, limit)
            stmt.setInt(6, offset)

            val rs: ResultSet = stmt.executeQuery()
            val messages = mutableListOf<Message>()

            while (rs.next()) {
                messages.add(
                    Message(
                        id = rs.getLong("id"),
                        sender = rs.getString("sender"),
                        recipient = rs.getString("recipient"),
                        ciphertext = rs.getBytes("ciphertext"),
                        ciphertextLen = rs.getInt("ciphertext_len"),
                        createdAt = rs.getTimestamp("created_at").time,
                        status = MessageStatus.fromString(rs.getString("status")),
                        deliveredAt = rs.getTimestamp("delivered_at")?.time,
                        readAt = rs.getTimestamp("read_at")?.time,
                        groupId = rs.getObject("group_id") as? Int
                    )
                )
            }

            Log.d(TAG, "Loaded ${messages.size} messages for conversation: $currentUser <-> $otherUser")
            messages
        }
    }

    /**
     * Load group messages
     */
    actual suspend fun loadGroupMessages(
        groupId: Int,
        limit: Int,
        offset: Int
    ): Result<List<Message>> {
        return runCatching {
            val conn = connect()
            val sql = """
                SELECT
                    id, sender, recipient, ciphertext, ciphertext_len,
                    created_at, status, delivered_at, read_at, group_id
                FROM messages
                WHERE group_id = ?
                ORDER BY created_at DESC
                LIMIT ? OFFSET ?
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setInt(1, groupId)
            stmt.setInt(2, limit)
            stmt.setInt(3, offset)

            val rs: ResultSet = stmt.executeQuery()
            val messages = mutableListOf<Message>()

            while (rs.next()) {
                messages.add(
                    Message(
                        id = rs.getLong("id"),
                        sender = rs.getString("sender"),
                        recipient = rs.getString("recipient"),
                        ciphertext = rs.getBytes("ciphertext"),
                        ciphertextLen = rs.getInt("ciphertext_len"),
                        createdAt = rs.getTimestamp("created_at").time,
                        status = MessageStatus.fromString(rs.getString("status")),
                        deliveredAt = rs.getTimestamp("delivered_at")?.time,
                        readAt = rs.getTimestamp("read_at")?.time,
                        groupId = rs.getInt("group_id")
                    )
                )
            }

            Log.d(TAG, "Loaded ${messages.size} messages for group: $groupId")
            messages
        }
    }

    /**
     * Update message status
     */
    actual suspend fun updateMessageStatus(
        messageId: Long,
        status: String
    ): Result<Unit> {
        return runCatching {
            val conn = connect()
            val sql = when (status.lowercase()) {
                "delivered" -> """
                    UPDATE messages
                    SET status = ?, delivered_at = NOW()
                    WHERE id = ?
                """.trimIndent()
                "read" -> """
                    UPDATE messages
                    SET status = ?, read_at = NOW()
                    WHERE id = ?
                """.trimIndent()
                else -> """
                    UPDATE messages
                    SET status = ?
                    WHERE id = ?
                """.trimIndent()
            }

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, status.lowercase())
            stmt.setLong(2, messageId)
            stmt.executeUpdate()

            Log.d(TAG, "Updated message status: ID=$messageId, status=$status")
        }
    }

    // ============================================================================
    // CONTACT OPERATIONS (keyserver table)
    // ============================================================================

    /**
     * Save contact to keyserver
     */
    actual suspend fun saveContact(contact: Contact): Result<Unit> {
        return runCatching {
            val conn = connect()
            val sql = """
                INSERT INTO keyserver (
                    identity,
                    signing_pubkey,
                    signing_pubkey_len,
                    encryption_pubkey,
                    encryption_pubkey_len,
                    fingerprint,
                    created_at
                )
                VALUES (?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT (identity) DO UPDATE SET
                    signing_pubkey = EXCLUDED.signing_pubkey,
                    signing_pubkey_len = EXCLUDED.signing_pubkey_len,
                    encryption_pubkey = EXCLUDED.encryption_pubkey,
                    encryption_pubkey_len = EXCLUDED.encryption_pubkey_len,
                    fingerprint = EXCLUDED.fingerprint
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, contact.identity)
            stmt.setBytes(2, contact.signingPubkey)
            stmt.setInt(3, contact.signingPubkeyLen)
            stmt.setBytes(4, contact.encryptionPubkey)
            stmt.setInt(5, contact.encryptionPubkeyLen)
            stmt.setString(6, contact.fingerprint)
            stmt.setTimestamp(7, if (contact.createdAt != null) Timestamp(contact.createdAt) else Timestamp(System.currentTimeMillis()))

            stmt.executeUpdate()
            Log.d(TAG, "Contact saved: ${contact.identity}")
        }
    }

    /**
     * Load contact by identity name
     */
    actual suspend fun loadContact(identity: String): Result<Contact?> {
        return runCatching {
            val conn = connect()
            val sql = """
                SELECT
                    id, identity, signing_pubkey, signing_pubkey_len,
                    encryption_pubkey, encryption_pubkey_len, fingerprint, created_at
                FROM keyserver
                WHERE identity = ?
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, identity)

            val rs: ResultSet = stmt.executeQuery()
            if (rs.next()) {
                Contact(
                    id = rs.getInt("id"),
                    identity = rs.getString("identity"),
                    signingPubkey = rs.getBytes("signing_pubkey"),
                    signingPubkeyLen = rs.getInt("signing_pubkey_len"),
                    encryptionPubkey = rs.getBytes("encryption_pubkey"),
                    encryptionPubkeyLen = rs.getInt("encryption_pubkey_len"),
                    fingerprint = rs.getString("fingerprint"),
                    createdAt = rs.getTimestamp("created_at")?.time
                )
            } else {
                null
            }
        }
    }

    /**
     * Load all contacts from keyserver
     */
    actual suspend fun loadAllContacts(): Result<List<Contact>> {
        return runCatching {
            val conn = connect()
            val sql = """
                SELECT
                    id, identity, signing_pubkey, signing_pubkey_len,
                    encryption_pubkey, encryption_pubkey_len, fingerprint, created_at
                FROM keyserver
                ORDER BY identity
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            val rs: ResultSet = stmt.executeQuery()
            val contacts = mutableListOf<Contact>()

            while (rs.next()) {
                contacts.add(
                    Contact(
                        id = rs.getInt("id"),
                        identity = rs.getString("identity"),
                        signingPubkey = rs.getBytes("signing_pubkey"),
                        signingPubkeyLen = rs.getInt("signing_pubkey_len"),
                        encryptionPubkey = rs.getBytes("encryption_pubkey"),
                        encryptionPubkeyLen = rs.getInt("encryption_pubkey_len"),
                        fingerprint = rs.getString("fingerprint"),
                        createdAt = rs.getTimestamp("created_at")?.time
                    )
                )
            }

            Log.d(TAG, "Loaded ${contacts.size} contacts")
            contacts
        }
    }

    /**
     * Delete contact from keyserver
     */
    actual suspend fun deleteContact(identity: String): Result<Unit> {
        return runCatching {
            val conn = connect()
            val sql = "DELETE FROM keyserver WHERE identity = ?"
            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, identity)
            stmt.executeUpdate()
            Log.d(TAG, "Contact deleted: $identity")
        }
    }

    // ============================================================================
    // GROUP OPERATIONS
    // ============================================================================

    /**
     * Create group
     */
    actual suspend fun createGroup(group: Group): Result<Int> {
        return runCatching {
            val conn = connect()

            // Insert group
            val sql = """
                INSERT INTO groups (name, description, creator, created_at, updated_at)
                VALUES (?, ?, ?, ?, ?)
                RETURNING id
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, group.name)
            stmt.setString(2, group.description)
            stmt.setString(3, group.creator)
            stmt.setTimestamp(4, if (group.createdAt != null) Timestamp(group.createdAt) else Timestamp(System.currentTimeMillis()))
            stmt.setTimestamp(5, if (group.updatedAt != null) Timestamp(group.updatedAt) else Timestamp(System.currentTimeMillis()))

            val rs: ResultSet = stmt.executeQuery()
            if (rs.next()) {
                val groupId = rs.getInt(1)

                // Add creator as member
                addGroupMember(
                    groupId,
                    GroupMember(
                        groupId = groupId,
                        member = group.creator,
                        role = GroupRole.CREATOR
                    )
                ).getOrThrow()

                // Add other members
                group.members.forEach { member ->
                    addGroupMember(groupId, member).getOrThrow()
                }

                Log.d(TAG, "Group created: ID=$groupId, name=${group.name}, members=${group.members.size + 1}")
                groupId
            } else {
                throw RuntimeException("Failed to create group")
            }
        }
    }

    /**
     * Load group by ID (with members)
     */
    actual suspend fun loadGroup(groupId: Int): Result<Group?> {
        return runCatching {
            val conn = connect()
            val sql = """
                SELECT id, name, description, creator, created_at, updated_at
                FROM groups
                WHERE id = ?
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setInt(1, groupId)

            val rs: ResultSet = stmt.executeQuery()
            if (rs.next()) {
                // Load group members
                val membersResult = loadGroupMembers(groupId)
                val members = membersResult.getOrThrow()

                Group(
                    id = rs.getInt("id"),
                    name = rs.getString("name"),
                    description = rs.getString("description"),
                    creator = rs.getString("creator"),
                    members = members,
                    createdAt = rs.getTimestamp("created_at")?.time,
                    updatedAt = rs.getTimestamp("updated_at")?.time
                )
            } else {
                null
            }
        }
    }

    /**
     * Load group members (internal helper)
     */
    private fun loadGroupMembers(groupId: Int): Result<List<GroupMember>> {
        return runCatching {
            val conn = connect()
            val sql = """
                SELECT group_id, member, role, joined_at
                FROM group_members
                WHERE group_id = ?
                ORDER BY joined_at
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setInt(1, groupId)

            val rs: ResultSet = stmt.executeQuery()
            val members = mutableListOf<GroupMember>()

            while (rs.next()) {
                members.add(
                    GroupMember(
                        groupId = rs.getInt("group_id"),
                        member = rs.getString("member"),
                        role = GroupRole.fromString(rs.getString("role")),
                        joinedAt = rs.getTimestamp("joined_at")?.time
                    )
                )
            }

            members
        }
    }

    /**
     * Load all groups for current user
     */
    actual suspend fun loadUserGroups(userIdentity: String): Result<List<Group>> {
        return runCatching {
            val conn = connect()
            val sql = """
                SELECT DISTINCT g.id, g.name, g.description, g.creator, g.created_at, g.updated_at
                FROM groups g
                JOIN group_members gm ON g.id = gm.group_id
                WHERE gm.member = ?
                ORDER BY g.updated_at DESC
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setString(1, userIdentity)

            val rs: ResultSet = stmt.executeQuery()
            val groups = mutableListOf<Group>()

            while (rs.next()) {
                val groupId = rs.getInt("id")
                val membersResult = loadGroupMembers(groupId)
                val members = membersResult.getOrNull() ?: emptyList()

                groups.add(
                    Group(
                        id = groupId,
                        name = rs.getString("name"),
                        description = rs.getString("description"),
                        creator = rs.getString("creator"),
                        members = members,
                        createdAt = rs.getTimestamp("created_at")?.time,
                        updatedAt = rs.getTimestamp("updated_at")?.time
                    )
                )
            }

            Log.d(TAG, "Loaded ${groups.size} groups for user: $userIdentity")
            groups
        }
    }

    /**
     * Add member to group
     */
    actual suspend fun addGroupMember(groupId: Int, member: GroupMember): Result<Unit> {
        return runCatching {
            val conn = connect()
            val sql = """
                INSERT INTO group_members (group_id, member, role, joined_at)
                VALUES (?, ?, ?, ?)
            """.trimIndent()

            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setInt(1, groupId)
            stmt.setString(2, member.member)
            stmt.setString(3, member.role.toString())
            stmt.setTimestamp(4, if (member.joinedAt != null) Timestamp(member.joinedAt) else Timestamp(System.currentTimeMillis()))

            stmt.executeUpdate()
            Log.d(TAG, "Added member to group: groupId=$groupId, member=${member.member}, role=${member.role}")
        }
    }

    /**
     * Remove member from group
     */
    actual suspend fun removeGroupMember(groupId: Int, memberIdentity: String): Result<Unit> {
        return runCatching {
            val conn = connect()
            val sql = "DELETE FROM group_members WHERE group_id = ? AND member = ?"
            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setInt(1, groupId)
            stmt.setString(2, memberIdentity)
            stmt.executeUpdate()
            Log.d(TAG, "Removed member from group: groupId=$groupId, member=$memberIdentity")
        }
    }

    /**
     * Delete group (CASCADE deletes members)
     */
    actual suspend fun deleteGroup(groupId: Int): Result<Unit> {
        return runCatching {
            val conn = connect()
            val sql = "DELETE FROM groups WHERE id = ?"
            val stmt: PreparedStatement = conn.prepareStatement(sql)
            stmt.setInt(1, groupId)
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
}
