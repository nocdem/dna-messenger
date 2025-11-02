package io.cpunk.dna.domain

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.*
import androidx.datastore.preferences.preferencesDataStore
import io.cpunk.dna.domain.models.AppTheme
import io.cpunk.dna.domain.models.UserPreferences
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

/**
 * Android implementation of preferences storage using DataStore
 */
actual class PreferencesManager(private val context: Context) {

    private val Context.dataStore: DataStore<Preferences> by preferencesDataStore(name = "dna_preferences")

    companion object {
        // User identity keys
        private val IDENTITY = stringPreferencesKey("identity")
        private val SEED_PHRASE = stringPreferencesKey("seed_phrase")

        // REST API keys (SECURE - no DB credentials)
        private val API_BASE_URL = stringPreferencesKey("api_base_url")
        private val API_TOKEN = stringPreferencesKey("api_token")

        // App settings
        private val THEME = stringPreferencesKey("theme")
        private val IS_FIRST_RUN = booleanPreferencesKey("is_first_run")
    }

    /**
     * Get current user preferences as Flow
     */
    actual fun getUserPreferences(): Flow<UserPreferences> {
        return context.dataStore.data.map { preferences ->
            val defaults = UserPreferences()
            UserPreferences(
                identity = preferences[IDENTITY] ?: defaults.identity,
                seedPhrase = preferences[SEED_PHRASE] ?: defaults.seedPhrase,
                apiBaseUrl = preferences[API_BASE_URL] ?: defaults.apiBaseUrl,
                apiToken = preferences[API_TOKEN] ?: defaults.apiToken,
                theme = AppTheme.valueOf(preferences[THEME] ?: defaults.theme.name),
                isFirstRun = preferences[IS_FIRST_RUN] ?: defaults.isFirstRun
            )
        }
    }

    /**
     * Update user preferences
     */
    actual suspend fun updateUserPreferences(update: (UserPreferences) -> UserPreferences) {
        context.dataStore.edit { preferences ->
            val defaults = UserPreferences()
            val current = UserPreferences(
                identity = preferences[IDENTITY] ?: defaults.identity,
                seedPhrase = preferences[SEED_PHRASE] ?: defaults.seedPhrase,
                apiBaseUrl = preferences[API_BASE_URL] ?: defaults.apiBaseUrl,
                apiToken = preferences[API_TOKEN] ?: defaults.apiToken,
                theme = AppTheme.valueOf(preferences[THEME] ?: defaults.theme.name),
                isFirstRun = preferences[IS_FIRST_RUN] ?: defaults.isFirstRun
            )

            val updated = update(current)

            preferences[IDENTITY] = updated.identity
            preferences[SEED_PHRASE] = updated.seedPhrase
            preferences[API_BASE_URL] = updated.apiBaseUrl
            preferences[API_TOKEN] = updated.apiToken
            preferences[THEME] = updated.theme.name
            preferences[IS_FIRST_RUN] = updated.isFirstRun
        }
    }

    /**
     * Clear all preferences
     */
    actual suspend fun clearPreferences() {
        context.dataStore.edit { it.clear() }
    }
}
