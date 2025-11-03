package io.cpunk.dna.android.ui.screen.settings

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import io.cpunk.dna.domain.PreferencesManager
import io.cpunk.dna.domain.models.AppTheme
import io.cpunk.dna.domain.models.UserPreferences
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

/**
 * ViewModel for Settings Screen
 *
 * Manages user preferences and app settings
 */
class SettingsViewModel(application: Application) : AndroidViewModel(application) {
    private val preferencesManager = PreferencesManager(application.applicationContext)

    val userPreferences: StateFlow<UserPreferences> = preferencesManager.getUserPreferences()
        .stateIn(
            scope = viewModelScope,
            started = SharingStarted.WhileSubscribed(5000),
            initialValue = UserPreferences()
        )

    /**
     * Update user identity
     */
    fun updateIdentity(identity: String) {
        viewModelScope.launch {
            preferencesManager.updateUserPreferences { prefs ->
                prefs.copy(identity = identity)
            }
        }
    }

    /**
     * Update API configuration settings
     */
    fun updateApiSettings(
        apiBaseUrl: String
    ) {
        viewModelScope.launch {
            preferencesManager.updateUserPreferences { prefs ->
                prefs.copy(
                    apiBaseUrl = apiBaseUrl
                )
            }
        }
    }

    /**
     * Update app theme
     */
    fun updateTheme(theme: AppTheme) {
        viewModelScope.launch {
            preferencesManager.updateUserPreferences { prefs ->
                prefs.copy(theme = theme)
            }
        }
    }

    /**
     * Update seed phrase (for export/backup)
     */
    fun updateSeedPhrase(seedPhrase: String) {
        viewModelScope.launch {
            preferencesManager.updateUserPreferences { prefs ->
                prefs.copy(seedPhrase = seedPhrase)
            }
        }
    }

    /**
     * Clear all app data and preferences
     */
    fun clearAllData() {
        viewModelScope.launch {
            preferencesManager.clearPreferences()
        }
    }
}
