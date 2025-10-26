package io.cpunk.dna.android.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

/**
 * cpunk.io Theme (Cyan)
 */
private val CpunkIODarkColorScheme = darkColorScheme(
    primary = CpunkIOPrimary,
    secondary = CpunkIOSecondary,
    tertiary = CpunkIOSecondary,
    background = CpunkIOBackground,
    surface = CpunkIOSurface,
    onPrimary = CpunkIOOnPrimary,
    onSecondary = CpunkIOOnSecondary,
    onTertiary = CpunkIOOnSecondary,
    onBackground = CpunkIOOnBackground,
    onSurface = CpunkIOOnSurface,
)

/**
 * cpunk.club Theme (Orange)
 */
private val CpunkClubDarkColorScheme = darkColorScheme(
    primary = CpunkClubPrimary,
    secondary = CpunkClubSecondary,
    tertiary = CpunkClubSecondary,
    background = CpunkClubBackground,
    surface = CpunkClubSurface,
    onPrimary = CpunkClubOnPrimary,
    onSecondary = CpunkClubOnSecondary,
    onTertiary = CpunkClubOnSecondary,
    onBackground = CpunkClubOnBackground,
    onSurface = CpunkClubOnSurface,
)

/**
 * DNA Messenger Theme
 *
 * Supports two themes:
 * - cpunk.io (cyan)
 * - cpunk.club (orange)
 *
 * @param darkTheme Force dark theme (always true for now)
 * @param useCpunkClubTheme If true, use orange theme. If false, use cyan theme.
 * @param content Composable content
 */
@Composable
fun DNAMessengerTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    useCpunkClubTheme: Boolean = false,
    content: @Composable () -> Unit
) {
    val colorScheme = when {
        useCpunkClubTheme -> CpunkClubDarkColorScheme
        else -> CpunkIODarkColorScheme
    }

    MaterialTheme(
        colorScheme = colorScheme,
        typography = Typography,
        content = content
    )
}
