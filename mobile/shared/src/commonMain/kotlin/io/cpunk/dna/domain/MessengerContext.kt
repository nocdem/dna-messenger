package io.cpunk.dna.domain

/**
 * Messenger Context - Cross-platform interface
 *
 * Manages messenger context lifecycle (identity + P2P transport).
 * Mobile version: No PostgreSQL, pure P2P messaging only.
 */
expect class MessengerContext {
    /**
     * Initialize messenger context for mobile (no PostgreSQL)
     *
     * Creates a lightweight messenger context that wraps the DNA context
     * and manages P2P transport lifecycle.
     *
     * @param identity User identity (e.g., "alice")
     * @param dnaMessenger DNA messenger instance (provides DNA context)
     * @return Result with messenger context pointer (Long), or error
     */
    fun init(identity: String, dnaMessenger: DNAMessenger): Result<Long>

    /**
     * Get the messenger context pointer
     *
     * @return Messenger context pointer, or 0 if not initialized
     */
    fun getContextPtr(): Long

    /**
     * Get user identity
     *
     * @return User identity string
     */
    fun getIdentity(): String

    /**
     * Close and cleanup messenger context
     *
     * Shuts down P2P transport and frees native resources.
     */
    fun close()
}
