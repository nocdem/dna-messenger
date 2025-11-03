package io.cpunk.dna.android.ui.screen.home

import android.app.Application
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import io.cpunk.dna.domain.AuthApiClient
import io.cpunk.dna.domain.ContactsApiClient
import io.cpunk.dna.domain.GroupsApiClient
import io.cpunk.dna.domain.PreferencesManager
import io.cpunk.dna.domain.models.Contact
import io.cpunk.dna.domain.models.Group
import io.cpunk.dna.domain.models.Message
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

/**
 * ViewModel for Home Screen
 *
 * Displays:
 * - List of contacts (from API)
 * - List of groups (from API)
 * - Recent conversations
 *
 * Uses REST API instead of direct database access
 */
class HomeViewModel(application: Application) : AndroidViewModel(application) {
    private val preferencesManager = PreferencesManager(application.applicationContext)
    private lateinit var contactsApi: ContactsApiClient
    private lateinit var groupsApi: GroupsApiClient
    private var apiToken: String = ""

    private val _uiState = MutableStateFlow(HomeUiState())
    val uiState: StateFlow<HomeUiState> = _uiState.asStateFlow()

    companion object {
        private const val TAG = "HomeViewModel"
    }

    init {
        // Initialize API clients with backend URL
        viewModelScope.launch {
            val prefs = preferencesManager.getUserPreferences().first()
            val apiBaseUrl = prefs.apiBaseUrl
            apiToken = prefs.apiToken

            Log.d(TAG, "📡 API Base URL: $apiBaseUrl")

            // Auto-login if no token exists
            if (apiToken.isEmpty()) {
                Log.d(TAG, "⚠️ No auth token, performing auto-login as 'cc'...")
                try {
                    val authApi = AuthApiClient(apiBaseUrl)
                    val token = authApi.login("cc").getOrThrow()
                    apiToken = token
                    Log.d(TAG, "✅ Auto-login successful, token: ${apiToken.take(20)}...")
                    // Save token to preferences
                    preferencesManager.updateUserPreferences { it.copy(apiToken = apiToken, identity = "cc") }
                } catch (e: Exception) {
                    Log.e(TAG, "❌ Auto-login failed", e)
                }
            } else {
                Log.d(TAG, "✅ Using existing token: ${apiToken.take(20)}...")
            }

            contactsApi = ContactsApiClient(apiBaseUrl, apiToken)
            groupsApi = GroupsApiClient(apiBaseUrl, apiToken)

            Log.d(TAG, "✅ API clients initialized with auth")

            // Load data from API
            loadContacts()
            loadGroups()
        }
    }

    /**
     * Load all contacts from API
     */
    fun loadContacts() {
        viewModelScope.launch {
            _uiState.update { it.copy(isLoadingContacts = true) }

            contactsApi.loadAllContacts()
                .onSuccess { contacts ->
                    Log.d(TAG, "✅ Loaded ${contacts.size} contacts from API")
                    _uiState.update {
                        it.copy(
                            contacts = contacts,
                            isLoadingContacts = false,
                            contactsError = null
                        )
                    }
                }
                .onFailure { error ->
                    Log.e(TAG, "❌ Failed to load contacts from API", error)
                    _uiState.update {
                        it.copy(
                            isLoadingContacts = false,
                            contactsError = error.message ?: "Failed to load contacts"
                        )
                    }
                }
        }
    }

    /**
     * Load all groups for current user from API
     */
    fun loadGroups() {
        viewModelScope.launch {
            _uiState.update { it.copy(isLoadingGroups = true) }

            // TODO: Get current user identity from session/preferences
            val currentUser = "cc" // Hardcoded for now

            groupsApi.loadUserGroups(currentUser)
                .onSuccess { groups ->
                    Log.d(TAG, "✅ Loaded ${groups.size} groups from API")
                    _uiState.update {
                        it.copy(
                            groups = groups,
                            isLoadingGroups = false,
                            groupsError = null
                        )
                    }
                }
                .onFailure { error ->
                    Log.e(TAG, "❌ Failed to load groups from API", error)
                    _uiState.update {
                        it.copy(
                            isLoadingGroups = false,
                            groupsError = error.message ?: "Failed to load groups"
                        )
                    }
                }
        }
    }

    /**
     * Search contacts by identity
     */
    fun searchContacts(query: String) {
        _uiState.update { state ->
            state.copy(
                searchQuery = query,
                filteredContacts = if (query.isBlank()) {
                    state.contacts
                } else {
                    state.contacts.filter { contact ->
                        contact.identity.contains(query, ignoreCase = true)
                    }
                }
            )
        }
    }

    /**
     * Refresh all data
     */
    fun refresh() {
        loadContacts()
        loadGroups()
    }

    override fun onCleared() {
        super.onCleared()
        // No cleanup needed for API clients
    }
}

/**
 * UI state for Home Screen
 */
data class HomeUiState(
    val contacts: List<Contact> = emptyList(),
    val filteredContacts: List<Contact> = emptyList(),
    val groups: List<Group> = emptyList(),
    val searchQuery: String = "",
    val isLoadingContacts: Boolean = false,
    val isLoadingGroups: Boolean = false,
    val contactsError: String? = null,
    val groupsError: String? = null
) {
    val displayedContacts: List<Contact>
        get() = if (searchQuery.isBlank()) contacts else filteredContacts
}
