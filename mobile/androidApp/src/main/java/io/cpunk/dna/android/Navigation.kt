package io.cpunk.dna.android

import androidx.compose.runtime.Composable
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import io.cpunk.dna.android.ui.screen.login.LoginScreen

/**
 * Navigation graph for DNA Messenger
 *
 * Defines all screens and navigation routes.
 */
@Composable
fun DNANavGraph() {
    val navController = rememberNavController()

    NavHost(
        navController = navController,
        startDestination = "login"
    ) {
        // Login screen
        composable("login") {
            LoginScreen(
                onNavigateToHome = {
                    navController.navigate("home") {
                        popUpTo("login") { inclusive = true }
                    }
                }
            )
        }

        // Home screen (TODO: implement)
        composable("home") {
            // TODO: Implement HomeScreen
            // Placeholder for now
            androidx.compose.material3.Text("Home Screen - TODO")
        }

        // Chat screen (TODO: implement)
        composable("chat/{contactId}") { backStackEntry ->
            val contactId = backStackEntry.arguments?.getString("contactId")
            // TODO: Implement ChatScreen
        }

        // Wallet screen (TODO: implement)
        composable("wallet") {
            // TODO: Implement WalletScreen
        }

        // Settings screen (TODO: implement)
        composable("settings") {
            // TODO: Implement SettingsScreen
        }
    }
}
