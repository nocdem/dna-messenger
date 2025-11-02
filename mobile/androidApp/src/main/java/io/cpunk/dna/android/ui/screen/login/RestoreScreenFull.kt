package io.cpunk.dna.android.ui.screen.login

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp

/**
 * Full RestoreScreen - Restore identity from BIP39 seed phrase
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun RestoreScreenFull(
    onNavigateBack: () -> Unit,
    onRestoreSuccess: () -> Unit
) {
    var seedPhrase by remember { mutableStateOf("") }
    var isLoading by remember { mutableStateOf(false) }
    var errorMessage by remember { mutableStateOf<String?>(null) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Restore from Seed Phrase") },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "Back")
                    }
                }
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
                .padding(24.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(
                text = "Enter your 24-word BIP39 recovery phrase",
                style = MaterialTheme.typography.bodyLarge,
                color = MaterialTheme.colorScheme.onBackground,
                textAlign = TextAlign.Center
            )

            Spacer(modifier = Modifier.height(32.dp))

            // Seed phrase input
            OutlinedTextField(
                value = seedPhrase,
                onValueChange = {
                    seedPhrase = it
                    errorMessage = null
                },
                label = { Text("Seed Phrase") },
                placeholder = { Text("word1 word2 word3...") },
                modifier = Modifier
                    .fillMaxWidth()
                    .height(200.dp),
                maxLines = 6,
                keyboardOptions = KeyboardOptions(
                    keyboardType = KeyboardType.Text,
                    imeAction = ImeAction.Done
                ),
                enabled = !isLoading
            )

            Spacer(modifier = Modifier.height(8.dp))

            // Word count indicator
            val wordCount = if (seedPhrase.isBlank()) 0 else seedPhrase.trim().split("\\s+".toRegex()).size
            Text(
                text = "$wordCount / 24 words",
                style = MaterialTheme.typography.bodySmall,
                color = if (wordCount == 24)
                    MaterialTheme.colorScheme.primary
                else
                    MaterialTheme.colorScheme.onBackground.copy(alpha = 0.6f)
            )

            Spacer(modifier = Modifier.height(32.dp))

            // Restore button
            Button(
                onClick = {
                    isLoading = true
                    errorMessage = null

                    // TODO: Implement actual BIP39 seed phrase restoration
                    // For now, just simulate and show error
                    errorMessage = "Restore functionality coming soon. Using stub crypto for now."
                    isLoading = false
                },
                enabled = !isLoading && wordCount == 24,
                modifier = Modifier
                    .fillMaxWidth()
                    .height(56.dp)
            ) {
                if (isLoading) {
                    CircularProgressIndicator(
                        modifier = Modifier.size(24.dp),
                        color = MaterialTheme.colorScheme.onPrimary
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    Text("Restoring...")
                } else {
                    Text("Restore Identity")
                }
            }

            // Error message
            if (errorMessage != null) {
                Spacer(modifier = Modifier.height(16.dp))
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(
                        containerColor = MaterialTheme.colorScheme.errorContainer
                    )
                ) {
                    Text(
                        text = errorMessage ?: "",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onErrorContainer,
                        modifier = Modifier.padding(16.dp)
                    )
                }
            }

            Spacer(modifier = Modifier.height(24.dp))

            // Help text
            Card(
                modifier = Modifier.fillMaxWidth(),
                colors = CardDefaults.cardColors(
                    containerColor = MaterialTheme.colorScheme.surfaceVariant
                )
            ) {
                Column(modifier = Modifier.padding(16.dp)) {
                    Text(
                        text = "How to restore",
                        style = MaterialTheme.typography.titleSmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    Text(
                        text = "Enter all 24 words from your recovery phrase, separated by spaces. Make sure the words are spelled correctly and in the right order.",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }
        }
    }
}
