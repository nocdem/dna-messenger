package io.cpunk.dna.domain.models

/**
 * User preferences and app settings
 */
data class UserPreferences(
    // User identity
    val identity: String = "",
    val seedPhrase: String = "",

    // REST API settings (SECURE - no credentials in code)
    val apiBaseUrl: String = "https://fruit-deliver-high-block.trycloudflare.com",  // C Keyserver via Cloudflare Tunnel
    val apiToken: String = "",     // JWT token (stored securely after login)

    // App settings
    val theme: AppTheme = AppTheme.SYSTEM,

    // First run flag
    val isFirstRun: Boolean = true
)

enum class AppTheme {
    LIGHT, DARK, SYSTEM
}
