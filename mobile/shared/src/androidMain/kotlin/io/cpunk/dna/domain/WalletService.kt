package io.cpunk.dna.domain

import android.util.Log
import io.cpunk.dna.domain.models.Wallet

/**
 * Android implementation of WalletService using JNI
 */
actual class WalletService {
    init {
        try {
            System.loadLibrary("dna_jni")
            Log.d(TAG, "WalletService initialized")
        } catch (e: UnsatisfiedLinkError) {
            Log.e(TAG, "Failed to load native library", e)
            throw RuntimeException("Failed to load DNA native library", e)
        }
    }

    /**
     * Read Cellframe wallet from file
     */
    actual fun readWallet(walletPath: String): Result<Wallet> {
        return runCatching {
            val walletName = nativeReadWallet(walletPath)
            // TODO: Return full Wallet object when JNI returns structured data
            // For now, return a simplified Wallet object
            Wallet(
                name = walletName,
                address = "",
                network = "backbone",
                sigType = io.cpunk.dna.domain.models.WalletSignatureType.DILITHIUM,
                isProtected = false
            )
        }
    }

    /**
     * List all Cellframe wallets
     */
    actual fun listWallets(): Result<List<String>> {
        return runCatching {
            nativeListWallets().toList()
        }
    }

    /**
     * Get wallet address for network
     */
    actual fun getAddress(walletName: String, network: String): Result<String> {
        return runCatching {
            nativeGetAddress(walletName, network)
        }
    }

    /**
     * Get token balance
     */
    actual fun getBalance(
        rpcUrl: String,
        network: String,
        address: String,
        token: String
    ): Result<String> {
        return runCatching {
            nativeGetBalance(rpcUrl, network, address, token)
        }
    }

    /**
     * Send transaction
     */
    actual fun sendTransaction(
        rpcUrl: String,
        network: String,
        walletPath: String,
        toAddress: String,
        token: String,
        amount: String,
        fee: String
    ): Result<String> {
        return runCatching {
            nativeSendTransaction(
                rpcUrl,
                network,
                walletPath,
                toAddress,
                token,
                amount,
                fee
            )
        }
    }

    // Native method declarations
    private external fun nativeReadWallet(walletPath: String): String
    private external fun nativeListWallets(): Array<String>
    private external fun nativeGetAddress(walletName: String, network: String): String
    private external fun nativeGetBalance(
        rpcUrl: String,
        network: String,
        address: String,
        token: String
    ): String
    private external fun nativeSendTransaction(
        rpcUrl: String,
        network: String,
        walletPath: String,
        toAddress: String,
        token: String,
        amount: String,
        fee: String
    ): String

    companion object {
        private const val TAG = "WalletService"
    }
}
