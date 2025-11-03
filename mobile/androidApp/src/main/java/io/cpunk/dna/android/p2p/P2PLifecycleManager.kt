package io.cpunk.dna.android.p2p

import android.util.Log
import io.cpunk.dna.domain.MessengerContext
import io.cpunk.dna.domain.P2PTransport
import kotlinx.coroutines.*
import kotlin.time.Duration.Companion.minutes

/**
 * P2P Lifecycle Manager
 *
 * Manages P2P transport lifecycle and periodic tasks:
 * - Presence refresh (every 5 minutes)
 * - Offline message checking (every 2 minutes)
 *
 * Usage:
 * ```kotlin
 * val manager = P2PLifecycleManager()
 * manager.start(messengerContext, p2pTransport)
 * // ... when done
 * manager.stop()
 * ```
 */
class P2PLifecycleManager {
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private var presenceJob: Job? = null
    private var offlineCheckJob: Job? = null

    private var messengerContext: MessengerContext? = null
    private var p2pTransport: P2PTransport? = null

    private var isRunning = false

    /**
     * Start P2P lifecycle management
     *
     * @param messengerCtx Messenger context
     * @param transport P2P transport
     */
    fun start(messengerCtx: MessengerContext, transport: P2PTransport) {
        if (isRunning) {
            Log.w(TAG, "P2P lifecycle manager already running")
            return
        }

        this.messengerContext = messengerCtx
        this.p2pTransport = transport

        val ctxPtr = messengerCtx.getContextPtr()
        if (ctxPtr == 0L) {
            Log.e(TAG, "Cannot start: Messenger context not initialized")
            return
        }

        Log.i(TAG, "Starting P2P lifecycle management for identity: ${messengerCtx.getIdentity()}")

        // Start presence refresh timer (every 5 minutes)
        presenceJob = scope.launch {
            while (isActive) {
                try {
                    delay(PRESENCE_REFRESH_INTERVAL.inWholeMilliseconds)

                    Log.d(TAG, "Refreshing presence...")
                    transport.refreshPresence(ctxPtr).onSuccess {
                        Log.d(TAG, "Presence refreshed successfully")
                    }.onFailure { e ->
                        Log.e(TAG, "Failed to refresh presence", e)
                    }
                } catch (e: CancellationException) {
                    throw e  // Propagate cancellation
                } catch (e: Exception) {
                    Log.e(TAG, "Error in presence refresh loop", e)
                }
            }
        }

        // Start offline message check timer (every 2 minutes)
        offlineCheckJob = scope.launch {
            while (isActive) {
                try {
                    delay(OFFLINE_CHECK_INTERVAL.inWholeMilliseconds)

                    Log.d(TAG, "Checking for offline messages...")
                    transport.checkOfflineMessages(ctxPtr).onSuccess { count ->
                        if (count > 0) {
                            Log.i(TAG, "Retrieved $count offline message(s)")
                            // TODO: Trigger UI update to fetch new messages from storage
                        } else {
                            Log.d(TAG, "No offline messages")
                        }
                    }.onFailure { e ->
                        Log.e(TAG, "Failed to check offline messages", e)
                    }
                } catch (e: CancellationException) {
                    throw e  // Propagate cancellation
                } catch (e: Exception) {
                    Log.e(TAG, "Error in offline message check loop", e)
                }
            }
        }

        isRunning = true
        Log.i(TAG, "P2P lifecycle management started")
    }

    /**
     * Stop P2P lifecycle management
     */
    fun stop() {
        if (!isRunning) {
            return
        }

        Log.i(TAG, "Stopping P2P lifecycle management...")

        presenceJob?.cancel()
        offlineCheckJob?.cancel()

        presenceJob = null
        offlineCheckJob = null

        isRunning = false

        Log.i(TAG, "P2P lifecycle management stopped")
    }

    /**
     * Cleanup all resources
     */
    fun destroy() {
        stop()
        scope.cancel()
        messengerContext = null
        p2pTransport = null
    }

    companion object {
        private const val TAG = "P2PLifecycleManager"

        // Presence refresh interval (5 minutes as per desktop implementation)
        private val PRESENCE_REFRESH_INTERVAL = 5.minutes

        // Offline message check interval (2 minutes as per desktop implementation)
        private val OFFLINE_CHECK_INTERVAL = 2.minutes
    }
}
