package io.cpunk.dna.android.ui.screen.login

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

/**
 * Simple RestoreScreen for testing navigation
 */
@Composable
fun RestoreScreenSimple(
    onNavigateBack: () -> Unit,
    onRestoreSuccess: () -> Unit
) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Text(
            text = "Restore from Seed Phrase",
            style = MaterialTheme.typography.headlineMedium
        )

        Spacer(modifier = Modifier.height(32.dp))

        Button(onClick = onNavigateBack) {
            Text("Back to Login")
        }

        Spacer(modifier = Modifier.height(16.dp))

        Text(
            text = "Restore functionality coming soon...",
            style = MaterialTheme.typography.bodyMedium
        )
    }
}
