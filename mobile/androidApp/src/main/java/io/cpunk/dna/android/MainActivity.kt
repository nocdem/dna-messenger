package io.cpunk.dna.android

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.core.view.WindowCompat
import com.google.accompanist.systemuicontroller.rememberSystemUiController
import io.cpunk.dna.android.ui.theme.DNAMessengerTheme

/**
 * Main Activity for DNA Messenger
 *
 * Entry point for the Android app.
 * Sets up Jetpack Compose and navigation.
 */
class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Enable edge-to-edge display
        WindowCompat.setDecorFitsSystemWindows(window, false)

        setContent {
            val systemUiController = rememberSystemUiController()

            // TODO: Get theme from preferences
            val useDarkTheme = true // Always dark for now

            // Set status bar color based on theme
            systemUiController.setSystemBarsColor(
                color = androidx.compose.ui.graphics.Color.Transparent,
                darkIcons = false
            )

            // Apply DNA Messenger theme
            DNAMessengerTheme(darkTheme = useDarkTheme) {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    // Navigation graph
                    DNANavGraph()
                }
            }
        }
    }
}
