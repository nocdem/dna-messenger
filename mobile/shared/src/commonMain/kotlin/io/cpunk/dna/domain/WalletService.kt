package io.cpunk.dna.domain

import io.cpunk.dna.domain.models.Wallet
import io.cpunk.dna.domain.models.Transaction

/**
 * WalletService - Cross-platform Cellframe wallet operations
 *
 * Provides:
 * - Reading .dwallet files
 * - Querying balances (CPUNK, CELL, KEL tokens)
 * - Sending transactions via Cellframe RPC
 *
 * Platform implementations:
 * - Android: JNI wrapper to C wallet library
 * - iOS: Direct C interop
 */
expect class WalletService() {
    /**
     * Read Cellframe wallet from file
     *
     * @param walletPath Full path to .dwallet file
     * @return Wallet information
     */
    fun readWallet(walletPath: String): Result<Wallet>

    /**
     * List all Cellframe wallets
     *
     * @return List of wallet names
     */
    fun listWallets(): Result<List<String>>

    /**
     * Get wallet address for network
     *
     * @param walletName Wallet name
     * @param network Network name (e.g., "backbone", "subzero")
     * @return Wallet address
     */
    fun getAddress(walletName: String, network: String): Result<String>

    /**
     * Get token balance
     *
     * @param rpcUrl RPC endpoint URL
     * @param network Network name
     * @param address Wallet address
     * @param token Token name (CPUNK, CELL, KEL)
     * @return Balance as string
     */
    fun getBalance(
        rpcUrl: String,
        network: String,
        address: String,
        token: String
    ): Result<String>

    /**
     * Send transaction
     *
     * @param rpcUrl RPC endpoint URL
     * @param network Network name
     * @param walletPath Path to wallet file
     * @param toAddress Recipient address
     * @param token Token name (CPUNK, CELL, KEL)
     * @param amount Amount to send
     * @param fee Transaction fee
     * @return Transaction hash
     */
    fun sendTransaction(
        rpcUrl: String,
        network: String,
        walletPath: String,
        toAddress: String,
        token: String,
        amount: String,
        fee: String
    ): Result<String>
}
