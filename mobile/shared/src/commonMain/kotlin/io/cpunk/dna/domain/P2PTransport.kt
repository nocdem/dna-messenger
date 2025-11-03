package io.cpunk.dna.domain

/**
 * P2P Transport Layer - Cross-platform interface
 *
 * Provides DHT-based peer discovery and P2P messaging with offline queue support.
 * Uses OpenDHT for peer discovery and direct TCP connections for messaging.
 */
expect class P2PTransport {
    /**
     * Initialize P2P transport for messenger context
     *
     * Sets up DHT node, registers presence, and prepares for P2P communication.
     *
     * @param messengerCtxPtr Messenger context pointer (from C layer)
     * @return Result with success/failure
     */
    fun init(messengerCtxPtr: Long): Result<Unit>

    /**
     * Shutdown P2P transport
     *
     * Stops DHT node and closes all P2P connections.
     *
     * @param messengerCtxPtr Messenger context pointer
     */
    fun shutdown(messengerCtxPtr: Long)

    /**
     * Send message via P2P with PostgreSQL fallback
     *
     * Attempts direct P2P delivery if peer is online, queues to DHT if offline,
     * falls back to PostgreSQL if both fail.
     *
     * @param messengerCtxPtr Messenger context pointer
     * @param recipient Recipient identity (public key hash)
     * @param encryptedMessage Already-encrypted message data
     * @return Result with success/failure
     */
    fun sendMessage(
        messengerCtxPtr: Long,
        recipient: String,
        encryptedMessage: ByteArray
    ): Result<Unit>

    /**
     * Broadcast message to multiple recipients
     *
     * Sends the same encrypted message to multiple peers via P2P.
     *
     * @param messengerCtxPtr Messenger context pointer
     * @param recipients List of recipient identities
     * @param encryptedMessage Already-encrypted message data
     * @return Result with success/failure
     */
    fun broadcastMessage(
        messengerCtxPtr: Long,
        recipients: List<String>,
        encryptedMessage: ByteArray
    ): Result<Unit>

    /**
     * Check if peer is online via DHT
     *
     * Queries DHT for peer's presence announcement.
     *
     * @param messengerCtxPtr Messenger context pointer
     * @param identity Peer identity to check
     * @return True if peer is online, false otherwise
     */
    fun isPeerOnline(messengerCtxPtr: Long, identity: String): Boolean

    /**
     * Refresh presence announcement in DHT
     *
     * Re-announces this peer's presence in the DHT.
     * Should be called every 5 minutes via timer.
     *
     * @param messengerCtxPtr Messenger context pointer
     * @return Result with success/failure
     */
    fun refreshPresence(messengerCtxPtr: Long): Result<Unit>

    /**
     * Check for offline messages in DHT
     *
     * Retrieves messages queued while this peer was offline (up to 7 days).
     * Should be called every 2 minutes via timer.
     *
     * @param messengerCtxPtr Messenger context pointer
     * @return Result with number of messages retrieved, or error
     */
    fun checkOfflineMessages(messengerCtxPtr: Long): Result<Int>
}
