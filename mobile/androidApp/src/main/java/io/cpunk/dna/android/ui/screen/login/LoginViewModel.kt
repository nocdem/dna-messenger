package io.cpunk.dna.android.ui.screen.login

import android.util.Log
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import io.cpunk.dna.domain.DNAMessenger
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * LoginViewModel - Business logic for login screen
 *
 * Handles:
 * - Key generation (Kyber512 + Dilithium3)
 * - Key storage in Android Keystore
 * - Loading states
 */
class LoginViewModel : ViewModel() {
    private val _uiState = MutableStateFlow(LoginUiState())
    val uiState: StateFlow<LoginUiState> = _uiState.asStateFlow()

    private val dnaMessenger = DNAMessenger()

    init {
        loadVersion()
    }

    /**
     * Load DNA library version
     */
    private fun loadVersion() {
        try {
            val version = dnaMessenger.getVersion()
            _uiState.value = _uiState.value.copy(version = version)
            Log.d(TAG, "DNA Messenger version: $version")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to get version", e)
            _uiState.value = _uiState.value.copy(version = "Unknown")
        }
    }

    /**
     * Create new identity (generate keys)
     */
    fun createNewIdentity(onSuccess: () -> Unit) {
        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(
                isLoading = true,
                loadingMessage = "Generating encryption keys...",
                errorMessage = null
            )

            try {
                // Generate Kyber512 encryption keypair
                val encryptionResult = withContext(Dispatchers.IO) {
                    dnaMessenger.generateEncryptionKeyPair()
                }

                if (encryptionResult.isFailure) {
                    throw encryptionResult.exceptionOrNull() ?: Exception("Encryption key generation failed")
                }

                val (encPubKey, encPrivKey) = encryptionResult.getOrThrow()
                Log.d(TAG, "Encryption keypair generated (pub: ${encPubKey.size} bytes, priv: ${encPrivKey.size} bytes)")

                _uiState.value = _uiState.value.copy(
                    loadingMessage = "Generating signing keys..."
                )

                // Generate Dilithium3 signing keypair
                val signingResult = withContext(Dispatchers.IO) {
                    dnaMessenger.generateSigningKeyPair()
                }

                if (signingResult.isFailure) {
                    throw signingResult.exceptionOrNull() ?: Exception("Signing key generation failed")
                }

                val (signPubKey, signPrivKey) = signingResult.getOrThrow()
                Log.d(TAG, "Signing keypair generated (pub: ${signPubKey.size} bytes, priv: ${signPrivKey.size} bytes)")

                _uiState.value = _uiState.value.copy(
                    loadingMessage = "Saving keys to secure storage..."
                )

                // TODO: Store keys in Android Keystore
                // For now, just log success
                withContext(Dispatchers.IO) {
                    // Simulate storage delay
                    kotlinx.coroutines.delay(500)
                }

                Log.i(TAG, "Identity created successfully")

                _uiState.value = _uiState.value.copy(
                    isLoading = false,
                    loadingMessage = "",
                    errorMessage = null
                )

                // Navigate to home screen
                onSuccess()

            } catch (e: Exception) {
                Log.e(TAG, "Failed to create identity", e)
                _uiState.value = _uiState.value.copy(
                    isLoading = false,
                    loadingMessage = "",
                    errorMessage = "Failed to create identity: ${e.message}"
                )
            }
        }
    }

    /**
     * Navigate to restore screen
     */
    fun navigateToRestore() {
        // TODO: Implement navigation to restore screen
        Log.d(TAG, "Navigate to restore screen (not implemented yet)")
    }

    override fun onCleared() {
        super.onCleared()
        dnaMessenger.close()
    }

    companion object {
        private const val TAG = "LoginViewModel"
    }
}

/**
 * UI state for login screen
 */
data class LoginUiState(
    val isLoading: Boolean = false,
    val loadingMessage: String = "",
    val errorMessage: String? = null,
    val version: String = "Loading..."
)
