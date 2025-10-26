/**
 * wallet_jni.h - JNI Function Declarations for Cellframe Wallet
 *
 * JNI methods for wallet operations:
 * - Read wallet files
 * - Get balance
 * - Send transactions (CPUNK, CELL, KEL tokens)
 *
 * Package: io.cpunk.dna.domain
 */

#ifndef WALLET_JNI_H
#define WALLET_JNI_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read Cellframe wallet from file
 * Java: native fun nativeReadWallet(walletPath: String): WalletData
 *
 * @param walletPath Full path to .dwallet file
 * @return WalletData object with wallet information
 */
JNIEXPORT jobject JNICALL
Java_io_cpunk_dna_domain_WalletService_nativeReadWallet(JNIEnv* env, jobject obj, jstring walletPath);

/**
 * List all Cellframe wallets
 * Java: native fun nativeListWallets(): Array<WalletData>
 *
 * @return Array of WalletData objects
 */
JNIEXPORT jobjectArray JNICALL
Java_io_cpunk_dna_domain_WalletService_nativeListWallets(JNIEnv* env, jobject obj);

/**
 * Get wallet address for network
 * Java: native fun nativeGetAddress(walletName: String, network: String): String
 *
 * @param walletName Wallet name
 * @param network Network name (e.g., "backbone", "subzero")
 * @return Wallet address for the specified network
 */
JNIEXPORT jstring JNICALL
Java_io_cpunk_dna_domain_WalletService_nativeGetAddress(JNIEnv* env, jobject obj, jstring walletName, jstring network);

/**
 * Get token balance via RPC
 * Java: native fun nativeGetBalance(
 *     rpcUrl: String,
 *     network: String,
 *     address: String,
 *     token: String
 * ): String
 *
 * @param rpcUrl RPC endpoint URL
 * @param network Network name
 * @param address Wallet address
 * @param token Token name (CPUNK, CELL, KEL)
 * @return Balance as string (e.g., "1234.56")
 */
JNIEXPORT jstring JNICALL
Java_io_cpunk_dna_domain_WalletService_nativeGetBalance(
    JNIEnv* env, jobject obj,
    jstring rpcUrl,
    jstring network,
    jstring address,
    jstring token
);

/**
 * Send transaction via RPC
 * Java: native fun nativeSendTransaction(
 *     rpcUrl: String,
 *     network: String,
 *     walletPath: String,
 *     toAddress: String,
 *     token: String,
 *     amount: String,
 *     fee: String
 * ): String
 *
 * @param rpcUrl RPC endpoint URL
 * @param network Network name
 * @param walletPath Path to wallet file
 * @param toAddress Recipient address
 * @param token Token name (CPUNK, CELL, KEL)
 * @param amount Amount to send
 * @param fee Transaction fee
 * @return Transaction hash
 */
JNIEXPORT jstring JNICALL
Java_io_cpunk_dna_domain_WalletService_nativeSendTransaction(
    JNIEnv* env, jobject obj,
    jstring rpcUrl,
    jstring network,
    jstring walletPath,
    jstring toAddress,
    jstring token,
    jstring amount,
    jstring fee
);

#ifdef __cplusplus
}
#endif

#endif // WALLET_JNI_H
