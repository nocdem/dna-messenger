package io.cpunk.dna.domain.models

/**
 * Wallet data class
 * Represents a Cellframe wallet
 */
data class Wallet(
    val name: String,
    val address: String,
    val network: String,
    val sigType: WalletSignatureType,
    val isProtected: Boolean,
    val balances: Map<String, String> = emptyMap()  // Token -> Balance
)

enum class WalletSignatureType {
    DILITHIUM,
    PICNIC,
    BLISS,
    TESLA,
    UNKNOWN
}

/**
 * Transaction data class
 */
data class Transaction(
    val hash: String,
    val from: String,
    val to: String,
    val token: String,
    val amount: String,
    val fee: String,
    val timestamp: Long,
    val status: TransactionStatus
)

enum class TransactionStatus {
    PENDING,
    CONFIRMED,
    FAILED
}

/**
 * Token balance data class
 */
data class TokenBalance(
    val token: String,
    val balance: String,
    val network: String
)
