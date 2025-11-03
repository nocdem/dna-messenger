package io.cpunk.dna.android.ui.screen.chat

import android.util.Log
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import io.cpunk.dna.domain.DatabaseRepository
import io.cpunk.dna.domain.models.Contact
import io.cpunk.dna.domain.models.Message
import io.cpunk.dna.domain.models.MessageStatus
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

/**
 * ViewModel for Conversation/Chat Screen
 *
 * Handles:
 * - Loading messages between two users
 * - Sending new messages
 * - Message status updates
 * - Real-time message polling
 */
class ConversationViewModel(
    private val contactIdentity: String
) : ViewModel() {
    private val repository = DatabaseRepository()

    private val _uiState = MutableStateFlow(ConversationUiState())
    val uiState: StateFlow<ConversationUiState> = _uiState.asStateFlow()

    companion object {
        private const val TAG = "ConversationViewModel"
        private const val MESSAGES_PAGE_SIZE = 50
    }

    init {
        loadContact()
        loadMessages()
    }

    /**
     * Load contact information
     */
    private fun loadContact() {
        viewModelScope.launch {
            repository.loadContact(contactIdentity)
                .onSuccess { contact ->
                    if (contact != null) {
                        _uiState.update { it.copy(contact = contact) }
                    } else {
                        Log.w(TAG, "Contact not found: $contactIdentity")
                    }
                }
                .onFailure { error ->
                    Log.e(TAG, "Failed to load contact", error)
                }
        }
    }

    /**
     * Load conversation messages
     */
    fun loadMessages(loadMore: Boolean = false) {
        viewModelScope.launch {
            _uiState.update { it.copy(isLoading = true) }

            // TODO: Get current user from session/preferences
            val currentUser = "cc" // Hardcoded for now

            val offset = if (loadMore) _uiState.value.messages.size else 0

            repository.loadConversation(
                currentUser = currentUser,
                otherUser = contactIdentity,
                limit = MESSAGES_PAGE_SIZE,
                offset = offset
            )
                .onSuccess { messages ->
                    Log.d(TAG, "Loaded ${messages.size} messages")

                    _uiState.update { state ->
                        val updatedMessages = if (loadMore) {
                            state.messages + messages
                        } else {
                            messages
                        }

                        state.copy(
                            messages = updatedMessages,
                            isLoading = false,
                            error = null,
                            hasMore = messages.size == MESSAGES_PAGE_SIZE
                        )
                    }

                    // Mark messages as read
                    markMessagesAsRead(messages)
                }
                .onFailure { error ->
                    Log.e(TAG, "Failed to load messages", error)
                    _uiState.update {
                        it.copy(
                            isLoading = false,
                            error = error.message ?: "Failed to load messages"
                        )
                    }
                }
        }
    }

    /**
     * Send a new message
     * TODO: Add encryption before saving
     */
    fun sendMessage(plaintext: String) {
        if (plaintext.isBlank()) return

        viewModelScope.launch {
            _uiState.update { it.copy(isSending = true) }

            // TODO: Get current user from session/preferences
            val currentUser = "cc" // Hardcoded for now

            // TODO: Encrypt message using DNAMessenger
            // For now, just convert to bytes (INSECURE - only for testing)
            val ciphertext = plaintext.toByteArray()

            val message = Message(
                sender = currentUser,
                recipient = contactIdentity,
                ciphertext = ciphertext,
                createdAt = System.currentTimeMillis(),
                status = MessageStatus.SENT
            )

            repository.saveMessage(message)
                .onSuccess { messageId ->
                    Log.d(TAG, "Message sent: ID=$messageId")

                    // Add to UI immediately
                    val sentMessage = message.copy(id = messageId)
                    _uiState.update { state ->
                        state.copy(
                            messages = listOf(sentMessage) + state.messages,
                            isSending = false,
                            sendError = null
                        )
                    }
                }
                .onFailure { error ->
                    Log.e(TAG, "Failed to send message", error)
                    _uiState.update {
                        it.copy(
                            isSending = false,
                            sendError = error.message ?: "Failed to send message"
                        )
                    }
                }
        }
    }

    /**
     * Mark messages as read
     */
    private fun markMessagesAsRead(messages: List<Message>) {
        viewModelScope.launch {
            // TODO: Get current user from session/preferences
            val currentUser = "cc" // Hardcoded for now

            messages.forEach { message ->
                // Only mark messages we received
                if (message.recipient == currentUser && message.status != MessageStatus.READ) {
                    repository.updateMessageStatus(message.id, "read")
                        .onSuccess {
                            Log.d(TAG, "Marked message as read: ${message.id}")
                        }
                        .onFailure { error ->
                            Log.e(TAG, "Failed to mark message as read", error)
                        }
                }
            }
        }
    }

    /**
     * Refresh messages (for pull-to-refresh)
     */
    fun refresh() {
        loadMessages(loadMore = false)
    }

    /**
     * Decrypt message for display
     * TODO: Implement actual decryption
     */
    fun decryptMessage(message: Message): String {
        return try {
            // TODO: Use DNAMessenger to decrypt
            // For now, just convert from bytes (INSECURE - only for testing)
            String(message.ciphertext)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to decrypt message", e)
            "[Decryption failed]"
        }
    }

    override fun onCleared() {
        super.onCleared()
        repository.close()
    }
}

/**
 * UI state for Conversation Screen
 */
data class ConversationUiState(
    val contact: Contact? = null,
    val messages: List<Message> = emptyList(),
    val isLoading: Boolean = false,
    val isSending: Boolean = false,
    val error: String? = null,
    val sendError: String? = null,
    val hasMore: Boolean = false
)
