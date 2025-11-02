package io.cpunk.dna.android

import androidx.compose.runtime.Composable
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import io.cpunk.dna.android.ui.screen.login.LoginScreen
import io.cpunk.dna.android.ui.screen.login.RestoreScreenFull
import io.cpunk.dna.android.ui.screen.home.HomeScreen
import io.cpunk.dna.android.ui.screen.settings.SettingsScreen
import io.cpunk.dna.android.ui.screen.wallet.WalletScreen

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
                },
                onNavigateToRestore = {
                    navController.navigate("restore")
                }
            )
        }

        // Restore from seed phrase screen
        composable("restore") {
            RestoreScreenFull(
                onNavigateBack = {
                    navController.popBackStack()
                },
                onRestoreSuccess = {
                    navController.navigate("home") {
                        popUpTo("login") { inclusive = true }
                    }
                }
            )
        }

        // Home screen - Contacts and Groups
        composable("home") {
            HomeScreen(
                onNavigateToChat = { contactIdentity ->
                    navController.navigate("chat/$contactIdentity")
                },
                onNavigateToGroup = { groupId ->
                    navController.navigate("group/$groupId")
                },
                onNavigateToSettings = {
                    navController.navigate("settings")
                },
                onNavigateToWallet = {
                    navController.navigate("wallet")
                }
            )
        }

        // Chat screen (1:1 conversation)
        composable("chat/{contactId}") { backStackEntry ->
            val contactId = backStackEntry.arguments?.getString("contactId") ?: return@composable
            // TODO: Implement ChatScreen
            androidx.compose.material3.Text("Chat with: $contactId (Coming soon)")
        }

        // Group chat screen
        composable("group/{groupId}") { backStackEntry ->
            val groupId = backStackEntry.arguments?.getString("groupId")?.toIntOrNull() ?: return@composable
            // TODO: Implement GroupChatScreen
            androidx.compose.material3.Text("Group chat: $groupId (Coming soon)")
        }

        // Wallet screen
        composable("wallet") {
            WalletScreen(
                onNavigateBack = {
                    navController.popBackStack()
                }
            )
        }

        // Settings screen
        composable("settings") {
            SettingsScreen(
                onNavigateBack = {
                    navController.popBackStack()
                }
            )
        }
    }
}
