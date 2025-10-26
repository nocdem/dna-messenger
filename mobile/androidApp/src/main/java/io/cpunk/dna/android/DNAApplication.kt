package io.cpunk.dna.android

import android.app.Application
import android.app.NotificationChannel
import android.app.NotificationManager
import android.os.Build
import android.util.Log

/**
 * DNA Messenger Application class
 *
 * Initializes:
 * - Native libraries
 * - Notification channels
 * - Application-wide configurations
 */
class DNAApplication : Application() {

    companion object {
        private const val TAG = "DNAApplication"

        // Notification channels
        const val CHANNEL_MESSAGES = "messages"
        const val CHANNEL_TRANSACTIONS = "transactions"
    }

    override fun onCreate() {
        super.onCreate()
        Log.d(TAG, "Initializing DNA Messenger")

        // Load native libraries
        loadNativeLibraries()

        // Create notification channels (Android 8.0+)
        createNotificationChannels()

        Log.d(TAG, "Initialization complete")
    }

    private fun loadNativeLibraries() {
        try {
            // Load C libraries in correct order (dependencies first)
            System.loadLibrary("crypto")      // OpenSSL
            System.loadLibrary("ssl")         // OpenSSL
            System.loadLibrary("kyber512")    // Post-quantum KEM
            System.loadLibrary("dilithium")   // Post-quantum signatures
            System.loadLibrary("dna_lib")     // DNA Messenger core
            System.loadLibrary("dna_jni")     // JNI wrapper

            Log.d(TAG, "Native libraries loaded successfully")
        } catch (e: UnsatisfiedLinkError) {
            Log.e(TAG, "Failed to load native libraries", e)
            // TODO: Show error dialog to user
        }
    }

    private fun createNotificationChannels() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val notificationManager = getSystemService(NotificationManager::class.java)

            // Messages channel
            val messagesChannel = NotificationChannel(
                CHANNEL_MESSAGES,
                "Messages",
                NotificationManager.IMPORTANCE_HIGH
            ).apply {
                description = "New message notifications"
                enableVibration(true)
                setShowBadge(true)
            }

            // Transactions channel
            val transactionsChannel = NotificationChannel(
                CHANNEL_TRANSACTIONS,
                "Transactions",
                NotificationManager.IMPORTANCE_DEFAULT
            ).apply {
                description = "Transaction status notifications"
                enableVibration(false)
                setShowBadge(false)
            }

            notificationManager.createNotificationChannels(
                listOf(messagesChannel, transactionsChannel)
            )

            Log.d(TAG, "Notification channels created")
        }
    }
}
