package io.cpunk.dna.android.ui.screen.wallet

import android.app.Application
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import io.cpunk.dna.domain.PreferencesManager
import io.cpunk.dna.domain.WalletService
import io.cpunk.dna.domain.models.TokenBalance
import io.cpunk.dna.domain.models.Transaction
import io.cpunk.dna.domain.models.TransactionStatus
import kotlinx.coroutines.flow.*
import kotlinx.coroutines.launch
import java.text.DecimalFormat

/**
 * WalletViewModel - Business logic for wallet screen
 *
 * Manages:
 * - Wallet loading and configuration
 * - Token balance queries
 * - Transaction sending
 * - Transaction history
 */
class WalletViewModel(application: Application) : AndroidViewModel(application) {

    private val preferencesManager = PreferencesManager(application)
    private val walletService = WalletService()

    private val _uiState = MutableStateFlow(WalletUiState())
    val uiState: StateFlow<WalletUiState> = _uiState.asStateFlow()

    // Configuration
    private val rpcUrl = "https://backbone.cellframe.net:8080"
    private val network = "backbone"
    private val walletPath = "${application.filesDir}/wallets/default.dwallet"

    init {
        loadWallet()
    }

    /**
     * Load wallet and initial balances
     */
    fun loadWallet() {
        viewModelScope.launch {
            try {
                _uiState.update { it.copy(isLoading = true, error = null) }

                // Get wallet info
                val walletResult = walletService.readWallet(walletPath)
                if (walletResult.isFailure) {
                    _uiState.update {
                        it.copy(
                            isLoading = false,
                            error = "Failed to load wallet: ${walletResult.exceptionOrNull()?.message}",
                            // Use demo data for now
                            walletName = "My cpunk Wallet",
                            walletAddress = "0x1234...5678"
                        )
                    }
                    // Load demo data even on error
                    loadDemoData()
                    return@launch
                }

                val wallet = walletResult.getOrNull()

                // Get wallet address
                val addressResult = walletService.getAddress(
                    wallet?.name ?: "default",
                    network
                )
                val address = addressResult.getOrNull() ?: "Unknown"

                _uiState.update {
                    it.copy(
                        walletName = wallet?.name ?: "My Wallet",
                        walletAddress = address,
                        isLoading = false
                    )
                }

                // Load balances
                refreshBalances()

            } catch (e: Exception) {
                Log.e(TAG, "Error loading wallet", e)
                _uiState.update {
                    it.copy(
                        isLoading = false,
                        error = "Error: ${e.message}",
                        // Use demo data on error
                        walletName = "My cpunk Wallet",
                        walletAddress = "0x1234...5678"
                    )
                }
                // Load demo data even on error
                loadDemoData()
            }
        }
    }

    /**
     * Refresh token balances from Cellframe network
     */
    fun refreshBalances() {
        viewModelScope.launch {
            try {
                _uiState.update { it.copy(isLoading = true) }

                val address = _uiState.value.walletAddress
                if (address.isEmpty() || address == "Unknown") {
                    loadDemoData()
                    _uiState.update { it.copy(isLoading = false) }
                    return@launch
                }

                val tokens = listOf("CPUNK", "CELL", "KEL")
                val balances = mutableListOf<TokenBalance>()
                var totalValue = 0.0

                for (token in tokens) {
                    val balanceResult = walletService.getBalance(
                        rpcUrl = rpcUrl,
                        network = network,
                        address = address,
                        token = token
                    )

                    if (balanceResult.isSuccess) {
                        val balance = balanceResult.getOrNull() ?: "0.00"
                        balances.add(
                            TokenBalance(
                                token = token,
                                balance = balance,
                                network = network
                            )
                        )

                        // Calculate total (simplified - in reality would need token prices)
                        try {
                            totalValue += balance.toDoubleOrNull() ?: 0.0
                        } catch (e: Exception) {
                            Log.e(TAG, "Error parsing balance for $token", e)
                        }
                    } else {
                        Log.w(TAG, "Failed to get balance for $token: ${balanceResult.exceptionOrNull()?.message}")
                        // Add zero balance
                        balances.add(
                            TokenBalance(
                                token = token,
                                balance = "0.00",
                                network = network
                            )
                        )
                    }
                }

                val df = DecimalFormat("#,##0.00")
                val totalBalanceFormatted = df.format(totalValue)
                val totalBalanceUsdFormatted = df.format(totalValue * 0.1) // Placeholder conversion

                _uiState.update {
                    it.copy(
                        tokenBalances = balances,
                        totalBalance = totalBalanceFormatted,
                        totalBalanceUsd = totalBalanceUsdFormatted,
                        isLoading = false
                    )
                }

            } catch (e: Exception) {
                Log.e(TAG, "Error refreshing balances", e)
                _uiState.update {
                    it.copy(
                        isLoading = false,
                        error = "Failed to refresh balances: ${e.message}"
                    )
                }
                // Load demo data on error
                loadDemoData()
            }
        }
    }

    /**
     * Send transaction
     */
    fun sendTransaction(
        toAddress: String,
        token: String,
        amount: String,
        fee: String
    ) {
        viewModelScope.launch {
            try {
                _uiState.update { it.copy(isLoading = true, error = null) }

                val result = walletService.sendTransaction(
                    rpcUrl = rpcUrl,
                    network = network,
                    walletPath = walletPath,
                    toAddress = toAddress,
                    token = token,
                    amount = amount,
                    fee = fee
                )

                if (result.isSuccess) {
                    val txHash = result.getOrNull() ?: ""
                    Log.i(TAG, "Transaction sent: $txHash")

                    // Add to recent transactions
                    val newTx = Transaction(
                        hash = txHash,
                        from = _uiState.value.walletAddress,
                        to = toAddress,
                        token = token,
                        amount = amount,
                        fee = fee,
                        timestamp = System.currentTimeMillis(),
                        status = TransactionStatus.PENDING
                    )

                    _uiState.update { state ->
                        state.copy(
                            recentTransactions = listOf(newTx) + state.recentTransactions,
                            allTransactions = listOf(newTx) + state.allTransactions,
                            isLoading = false
                        )
                    }

                    // Refresh balances after send
                    refreshBalances()
                } else {
                    _uiState.update {
                        it.copy(
                            isLoading = false,
                            error = "Failed to send transaction: ${result.exceptionOrNull()?.message}"
                        )
                    }
                }

            } catch (e: Exception) {
                Log.e(TAG, "Error sending transaction", e)
                _uiState.update {
                    it.copy(
                        isLoading = false,
                        error = "Error: ${e.message}"
                    )
                }
            }
        }
    }

    /**
     * Load demo data for testing/preview
     */
    private fun loadDemoData() {
        val demoBalances = listOf(
            TokenBalance("CPUNK", "1,234.56", network),
            TokenBalance("CELL", "789.12", network),
            TokenBalance("KEL", "456.78", network)
        )

        val demoTransactions = listOf(
            Transaction(
                hash = "0xabc123...",
                from = "0x9876...5432",
                to = _uiState.value.walletAddress,
                token = "CPUNK",
                amount = "100.00",
                fee = "0.01",
                timestamp = System.currentTimeMillis() - 3600000, // 1 hour ago
                status = TransactionStatus.CONFIRMED
            ),
            Transaction(
                hash = "0xdef456...",
                from = _uiState.value.walletAddress,
                to = "0x1111...2222",
                token = "CELL",
                amount = "50.00",
                fee = "0.01",
                timestamp = System.currentTimeMillis() - 7200000, // 2 hours ago
                status = TransactionStatus.CONFIRMED
            ),
            Transaction(
                hash = "0xghi789...",
                from = _uiState.value.walletAddress,
                to = "0x3333...4444",
                token = "KEL",
                amount = "25.00",
                fee = "0.01",
                timestamp = System.currentTimeMillis() - 86400000, // 1 day ago
                status = TransactionStatus.PENDING
            )
        )

        _uiState.update {
            it.copy(
                tokenBalances = demoBalances,
                totalBalance = "2,480.46",
                totalBalanceUsd = "248.05",
                recentTransactions = demoTransactions,
                allTransactions = demoTransactions,
                isLoading = false,
                error = null
            )
        }
    }

    companion object {
        private const val TAG = "WalletViewModel"
    }
}

/**
 * UI state for wallet screen
 */
data class WalletUiState(
    val isLoading: Boolean = false,
    val error: String? = null,
    val walletName: String = "",
    val walletAddress: String = "",
    val totalBalance: String = "0.00",
    val totalBalanceUsd: String = "0.00",
    val tokenBalances: List<TokenBalance> = emptyList(),
    val recentTransactions: List<Transaction> = emptyList(),
    val allTransactions: List<Transaction> = emptyList()
)
