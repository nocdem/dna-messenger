package io.cpunk.dna.android.ui.screen.wallet

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel
import io.cpunk.dna.domain.models.TokenBalance
import io.cpunk.dna.domain.models.Transaction
import io.cpunk.dna.domain.models.TransactionStatus
import java.text.SimpleDateFormat
import java.util.*

/**
 * Wallet Screen - cpunk Wallet (CPUNK, CELL, KEL tokens)
 *
 * Features:
 * - Token balances display
 * - Send/Receive transactions
 * - Transaction history
 * - Integration with Cellframe blockchain
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun WalletScreen(
    onNavigateBack: () -> Unit,
    viewModel: WalletViewModel = viewModel()
) {
    val uiState by viewModel.uiState.collectAsState()

    var showSendDialog by remember { mutableStateOf(false) }
    var showReceiveDialog by remember { mutableStateOf(false) }
    var showHistoryDialog by remember { mutableStateOf(false) }
    var showDexDialog by remember { mutableStateOf(false) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("💰 Wallet") },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "Back")
                    }
                },
                actions = {
                    IconButton(onClick = { viewModel.refreshBalances() }) {
                        Icon(Icons.Default.Refresh, contentDescription = "Refresh")
                    }
                }
            )
        }
    ) { paddingValues ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
        ) {
            when {
                uiState.isLoading && uiState.walletName.isEmpty() -> {
                    // Initial loading
                    CircularProgressIndicator(
                        modifier = Modifier.align(Alignment.Center)
                    )
                }
                uiState.error != null && uiState.walletName.isEmpty() -> {
                    // Error state
                    ErrorView(
                        message = uiState.error ?: "Unknown error",
                        onRetry = { viewModel.loadWallet() },
                        modifier = Modifier.align(Alignment.Center)
                    )
                }
                else -> {
                    // Wallet content
                    LazyColumn(
                        modifier = Modifier.fillMaxSize(),
                        contentPadding = PaddingValues(16.dp),
                        verticalArrangement = Arrangement.spacedBy(16.dp)
                    ) {
                        // Header: Wallet name and total balance
                        item {
                            WalletHeaderCard(
                                walletName = uiState.walletName,
                                totalBalance = uiState.totalBalance,
                                totalBalanceUsd = uiState.totalBalanceUsd
                            )
                        }

                        // Action buttons: Send, Receive, DEX, History
                        item {
                            ActionButtonsRow(
                                onSendClick = { showSendDialog = true },
                                onReceiveClick = { showReceiveDialog = true },
                                onDexClick = { showDexDialog = true },
                                onHistoryClick = { showHistoryDialog = true }
                            )
                        }

                        // Token balances section
                        item {
                            Text(
                                "Assets",
                                style = MaterialTheme.typography.titleMedium,
                                fontWeight = FontWeight.Bold,
                                modifier = Modifier.padding(top = 8.dp, bottom = 8.dp)
                            )
                        }

                        items(uiState.tokenBalances) { tokenBalance ->
                            TokenBalanceCard(tokenBalance = tokenBalance)
                        }

                        // Recent transactions section
                        if (uiState.recentTransactions.isNotEmpty()) {
                            item {
                                Row(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(top = 8.dp, bottom = 8.dp),
                                    horizontalArrangement = Arrangement.SpaceBetween,
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    Text(
                                        "Recent Transactions",
                                        style = MaterialTheme.typography.titleMedium,
                                        fontWeight = FontWeight.Bold
                                    )
                                    TextButton(onClick = { showHistoryDialog = true }) {
                                        Text("View All")
                                    }
                                }
                            }

                            items(uiState.recentTransactions.take(5)) { transaction ->
                                TransactionListItem(transaction = transaction)
                            }
                        }
                    }

                    // Loading indicator overlay
                    if (uiState.isLoading) {
                        LinearProgressIndicator(
                            modifier = Modifier
                                .fillMaxWidth()
                                .align(Alignment.TopCenter)
                        )
                    }
                }
            }
        }
    }

    // Dialogs
    if (showSendDialog) {
        SendTransactionDialog(
            onDismiss = { showSendDialog = false },
            onSend = { toAddress, token, amount, fee ->
                viewModel.sendTransaction(toAddress, token, amount, fee)
                showSendDialog = false
            },
            availableTokens = uiState.tokenBalances
        )
    }

    if (showReceiveDialog) {
        ReceiveDialog(
            walletAddress = uiState.walletAddress,
            onDismiss = { showReceiveDialog = false }
        )
    }

    if (showHistoryDialog) {
        TransactionHistoryDialog(
            transactions = uiState.allTransactions,
            onDismiss = { showHistoryDialog = false }
        )
    }

    if (showDexDialog) {
        DexComingSoonDialog(
            onDismiss = { showDexDialog = false }
        )
    }
}

/**
 * Wallet header card showing wallet name and total balance
 */
@Composable
fun WalletHeaderCard(
    walletName: String,
    totalBalance: String,
    totalBalanceUsd: String
) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 8.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.primaryContainer
        ),
        shape = RoundedCornerShape(16.dp)
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(24.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(
                text = walletName,
                style = MaterialTheme.typography.titleMedium,
                fontWeight = FontWeight.Bold,
                color = MaterialTheme.colorScheme.onPrimaryContainer
            )

            Spacer(modifier = Modifier.height(16.dp))

            Text(
                text = totalBalance,
                style = MaterialTheme.typography.displayMedium,
                fontWeight = FontWeight.Bold,
                color = MaterialTheme.colorScheme.onPrimaryContainer,
                fontSize = 48.sp
            )

            Spacer(modifier = Modifier.height(8.dp))

            Text(
                text = "≈ $$totalBalanceUsd USD",
                style = MaterialTheme.typography.bodyLarge,
                color = MaterialTheme.colorScheme.onPrimaryContainer.copy(alpha = 0.7f)
            )
        }
    }
}

/**
 * Action buttons: Send, Receive, DEX, History
 */
@Composable
fun ActionButtonsRow(
    onSendClick: () -> Unit,
    onReceiveClick: () -> Unit,
    onDexClick: () -> Unit,
    onHistoryClick: () -> Unit
) {
    Column(
        modifier = Modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        // First row: Send & Receive
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            ActionButton(
                text = "💸 Send",
                onClick = onSendClick,
                modifier = Modifier.weight(1f)
            )
            ActionButton(
                text = "📥 Receive",
                onClick = onReceiveClick,
                modifier = Modifier.weight(1f)
            )
        }

        // Second row: DEX & History
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            ActionButton(
                text = "🔄 DEX",
                onClick = onDexClick,
                modifier = Modifier.weight(1f)
            )
            ActionButton(
                text = "📜 History",
                onClick = onHistoryClick,
                modifier = Modifier.weight(1f)
            )
        }
    }
}

/**
 * Individual action button
 */
@Composable
fun ActionButton(
    text: String,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    Button(
        onClick = onClick,
        modifier = modifier.height(80.dp),
        colors = ButtonDefaults.buttonColors(
            containerColor = MaterialTheme.colorScheme.primaryContainer.copy(alpha = 0.5f),
            contentColor = MaterialTheme.colorScheme.primary
        ),
        shape = RoundedCornerShape(12.dp)
    ) {
        Text(
            text = text,
            style = MaterialTheme.typography.titleMedium,
            fontWeight = FontWeight.Bold,
            fontSize = 16.sp
        )
    }
}

/**
 * Token balance card (CPUNK, CELL, KEL)
 */
@Composable
fun TokenBalanceCard(tokenBalance: TokenBalance) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .clickable { /* TODO: Show token details */ },
        shape = RoundedCornerShape(12.dp)
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                // Token icon
                Surface(
                    modifier = Modifier.size(48.dp),
                    shape = RoundedCornerShape(24.dp),
                    color = getTokenColor(tokenBalance.token)
                ) {
                    Box(contentAlignment = Alignment.Center) {
                        Text(
                            text = getTokenEmoji(tokenBalance.token),
                            fontSize = 24.sp
                        )
                    }
                }

                // Token name and network
                Column {
                    Text(
                        text = tokenBalance.token,
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold
                    )
                    Text(
                        text = tokenBalance.network,
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }

            // Balance
            Text(
                text = tokenBalance.balance,
                style = MaterialTheme.typography.titleLarge,
                fontWeight = FontWeight.Bold
            )
        }
    }
}

/**
 * Transaction list item
 */
@Composable
fun TransactionListItem(transaction: Transaction) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(8.dp)
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(12.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                // Transaction type icon
                Icon(
                    imageVector = if (transaction.from.contains("...")) Icons.Default.CallReceived else Icons.Default.CallMade,
                    contentDescription = null,
                    tint = if (transaction.from.contains("...")) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.error,
                    modifier = Modifier.size(32.dp)
                )

                // Transaction details
                Column {
                    Text(
                        text = "${transaction.token} • ${formatTransactionDate(transaction.timestamp)}",
                        style = MaterialTheme.typography.bodyMedium,
                        fontWeight = FontWeight.Bold
                    )
                    Text(
                        text = if (transaction.from.contains("...")) "From ${transaction.from}" else "To ${transaction.to}",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }

            // Amount and status
            Column(
                horizontalAlignment = Alignment.End
            ) {
                Text(
                    text = if (transaction.from.contains("...")) "+${transaction.amount}" else "-${transaction.amount}",
                    style = MaterialTheme.typography.bodyLarge,
                    fontWeight = FontWeight.Bold,
                    color = if (transaction.from.contains("...")) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.error
                )
                TransactionStatusBadge(status = transaction.status)
            }
        }
    }
}

/**
 * Transaction status badge
 */
@Composable
fun TransactionStatusBadge(status: TransactionStatus) {
    val (color, text) = when (status) {
        TransactionStatus.PENDING -> MaterialTheme.colorScheme.tertiary to "Pending"
        TransactionStatus.CONFIRMED -> MaterialTheme.colorScheme.primary to "Confirmed"
        TransactionStatus.FAILED -> MaterialTheme.colorScheme.error to "Failed"
    }

    Surface(
        color = color.copy(alpha = 0.15f),
        shape = RoundedCornerShape(4.dp)
    ) {
        Text(
            text = text,
            modifier = Modifier.padding(horizontal = 8.dp, vertical = 4.dp),
            style = MaterialTheme.typography.labelSmall,
            color = color,
            fontWeight = FontWeight.Bold
        )
    }
}

/**
 * Error view with retry button
 */
@Composable
fun ErrorView(
    message: String,
    onRetry: () -> Unit,
    modifier: Modifier = Modifier
) {
    Column(
        modifier = modifier.padding(32.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Icon(
            Icons.Default.Error,
            contentDescription = null,
            modifier = Modifier.size(64.dp),
            tint = MaterialTheme.colorScheme.error
        )
        Spacer(modifier = Modifier.height(16.dp))
        Text(
            "Error Loading Wallet",
            style = MaterialTheme.typography.titleMedium,
            color = MaterialTheme.colorScheme.error
        )
        Spacer(modifier = Modifier.height(8.dp))
        Text(
            message,
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
            textAlign = TextAlign.Center
        )
        Spacer(modifier = Modifier.height(16.dp))
        Button(onClick = onRetry) {
            Text("Retry")
        }
    }
}

/**
 * Send transaction dialog
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SendTransactionDialog(
    onDismiss: () -> Unit,
    onSend: (toAddress: String, token: String, amount: String, fee: String) -> Unit,
    availableTokens: List<TokenBalance>
) {
    var toAddress by remember { mutableStateOf("") }
    var selectedToken by remember { mutableStateOf(availableTokens.firstOrNull()?.token ?: "CPUNK") }
    var amount by remember { mutableStateOf("") }
    var fee by remember { mutableStateOf("0.01") }
    var expanded by remember { mutableStateOf(false) }

    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text("💸 Send Transaction") },
        text = {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(vertical = 8.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                // To address
                OutlinedTextField(
                    value = toAddress,
                    onValueChange = { toAddress = it },
                    label = { Text("To Address") },
                    modifier = Modifier.fillMaxWidth(),
                    singleLine = true
                )

                // Token selector
                ExposedDropdownMenuBox(
                    expanded = expanded,
                    onExpandedChange = { expanded = !expanded }
                ) {
                    OutlinedTextField(
                        value = selectedToken,
                        onValueChange = {},
                        readOnly = true,
                        label = { Text("Token") },
                        trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = expanded) },
                        modifier = Modifier
                            .fillMaxWidth()
                            .menuAnchor()
                    )

                    ExposedDropdownMenu(
                        expanded = expanded,
                        onDismissRequest = { expanded = false }
                    ) {
                        availableTokens.forEach { tokenBalance ->
                            DropdownMenuItem(
                                text = { Text("${tokenBalance.token} (${tokenBalance.balance})") },
                                onClick = {
                                    selectedToken = tokenBalance.token
                                    expanded = false
                                }
                            )
                        }
                    }
                }

                // Amount
                OutlinedTextField(
                    value = amount,
                    onValueChange = { amount = it },
                    label = { Text("Amount") },
                    modifier = Modifier.fillMaxWidth(),
                    singleLine = true
                )

                // Fee
                OutlinedTextField(
                    value = fee,
                    onValueChange = { fee = it },
                    label = { Text("Fee") },
                    modifier = Modifier.fillMaxWidth(),
                    singleLine = true
                )
            }
        },
        confirmButton = {
            TextButton(
                onClick = {
                    if (toAddress.isNotEmpty() && amount.isNotEmpty()) {
                        onSend(toAddress, selectedToken, amount, fee)
                    }
                },
                enabled = toAddress.isNotEmpty() && amount.isNotEmpty()
            ) {
                Text("Send")
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) {
                Text("Cancel")
            }
        }
    )
}

/**
 * Receive dialog showing wallet address and QR code (placeholder)
 */
@Composable
fun ReceiveDialog(
    walletAddress: String,
    onDismiss: () -> Unit
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text("📥 Receive") },
        text = {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(vertical = 8.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                Text(
                    "Your wallet address:",
                    style = MaterialTheme.typography.bodyMedium
                )

                // Address display
                Surface(
                    modifier = Modifier.fillMaxWidth(),
                    color = MaterialTheme.colorScheme.surfaceVariant,
                    shape = RoundedCornerShape(8.dp)
                ) {
                    Text(
                        text = walletAddress.ifEmpty { "Loading..." },
                        modifier = Modifier.padding(16.dp),
                        style = MaterialTheme.typography.bodySmall,
                        textAlign = TextAlign.Center
                    )
                }

                // QR code placeholder
                Surface(
                    modifier = Modifier.size(200.dp),
                    color = MaterialTheme.colorScheme.surfaceVariant,
                    shape = RoundedCornerShape(8.dp)
                ) {
                    Box(contentAlignment = Alignment.Center) {
                        Text(
                            "QR Code\nComing Soon",
                            textAlign = TextAlign.Center,
                            style = MaterialTheme.typography.bodyMedium
                        )
                    }
                }
            }
        },
        confirmButton = {
            TextButton(onClick = onDismiss) {
                Text("Close")
            }
        }
    )
}

/**
 * Transaction history dialog
 */
@Composable
fun TransactionHistoryDialog(
    transactions: List<Transaction>,
    onDismiss: () -> Unit
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text("📜 Transaction History") },
        text = {
            if (transactions.isEmpty()) {
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(32.dp),
                    horizontalAlignment = Alignment.CenterHorizontally
                ) {
                    Icon(
                        Icons.Default.Receipt,
                        contentDescription = null,
                        modifier = Modifier.size(64.dp),
                        tint = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                    Spacer(modifier = Modifier.height(16.dp))
                    Text(
                        "No transactions yet",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            } else {
                LazyColumn(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(400.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    items(transactions) { transaction ->
                        TransactionListItem(transaction = transaction)
                    }
                }
            }
        },
        confirmButton = {
            TextButton(onClick = onDismiss) {
                Text("Close")
            }
        }
    )
}

/**
 * DEX coming soon dialog
 */
@Composable
fun DexComingSoonDialog(onDismiss: () -> Unit) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text("🔄 DEX") },
        text = {
            Text("Decentralized exchange integration coming soon!\n\nThis will allow you to swap CPUNK, CELL, and KEL tokens directly within the app.")
        },
        confirmButton = {
            TextButton(onClick = onDismiss) {
                Text("OK")
            }
        }
    )
}

/**
 * Utility functions
 */
private fun getTokenColor(token: String): Color {
    return when (token.uppercase()) {
        "CPUNK" -> Color(0xFF00D9FF)  // Cyan
        "CELL" -> Color(0xFFFF6B35)   // Orange
        "KEL" -> Color(0xFF14A098)    // Teal
        else -> Color.Gray
    }
}

private fun getTokenEmoji(token: String): String {
    return when (token.uppercase()) {
        "CPUNK" -> "🔷"
        "CELL" -> "🔶"
        "KEL" -> "💎"
        else -> "💰"
    }
}

private fun formatTransactionDate(timestamp: Long): String {
    val date = Date(timestamp)
    val now = Date()
    val diff = now.time - date.time

    return when {
        diff < 60_000 -> "Just now"
        diff < 3600_000 -> "${diff / 60_000}m ago"
        diff < 86400_000 -> "${diff / 3600_000}h ago"
        diff < 604800_000 -> "${diff / 86400_000}d ago"
        else -> SimpleDateFormat("MMM d, yyyy", Locale.getDefault()).format(date)
    }
}
