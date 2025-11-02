package io.cpunk.dna.domain

import io.cpunk.dna.domain.models.UserPreferences
import kotlinx.coroutines.flow.Flow

/**
 * Platform-specific preferences storage
 */
expect class PreferencesManager {
    /**
     * Get current user preferences as Flow
     */
    fun getUserPreferences(): Flow<UserPreferences>

    /**
     * Update user preferences
     */
    suspend fun updateUserPreferences(update: (UserPreferences) -> UserPreferences)

    /**
     * Clear all preferences
     */
    suspend fun clearPreferences()
}
