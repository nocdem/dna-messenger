/**
 * wallet_jni.cpp - JNI Implementation for Cellframe Wallet
 *
 * Bridges Kotlin code to C wallet library.
 */

#include "wallet_jni.h"
#include "jni_utils.h"
#include "wallet.h"
#include "cellframe_rpc.h"
#include <cstdlib>
#include <cstring>

/**
 * Read Cellframe wallet from file
 */
JNIEXPORT jobject JNICALL
Java_io_cpunk_dna_domain_WalletService_nativeReadWallet(JNIEnv* env, jobject obj, jstring walletPath) {
    LOGI("nativeReadWallet: Reading wallet");

    const char* path = jstringToString(env, walletPath);
    if (path == nullptr) {
        throwException(env, "java/lang/IllegalArgumentException", "Wallet path is null");
        return nullptr;
    }

    LOGI("nativeReadWallet: Path: %s", path);

    cellframe_wallet_t* wallet = nullptr;
    int result = wallet_read_cellframe_path(path, &wallet);

    releaseString(env, walletPath, path);

    if (result != 0 || wallet == nullptr) {
        LOGE("nativeReadWallet: Failed to read wallet: %d", result);
        throwException(env, "java/io/IOException", "Failed to read wallet file");
        return nullptr;
    }

    // Create WalletData object (simplified version - just return wallet name for now)
    // Full implementation would create a proper Kotlin data class
    jclass stringClass = env->FindClass("java/lang/String");
    jstring walletName = stringToJstring(env, wallet->name);

    wallet_free(wallet);

    if (walletName == nullptr) {
        LOGE("nativeReadWallet: Failed to create wallet name string");
        return nullptr;
    }

    LOGI("nativeReadWallet: Wallet read successfully: %s", wallet->name);

    // For now, just return the wallet name as a String
    // TODO: Create proper WalletData Kotlin class and return structured data
    return walletName;
}

/**
 * List all Cellframe wallets
 */
JNIEXPORT jobjectArray JNICALL
Java_io_cpunk_dna_domain_WalletService_nativeListWallets(JNIEnv* env, jobject obj) {
    LOGI("nativeListWallets: Listing wallets");

    wallet_list_t* list = nullptr;
    int result = wallet_list_cellframe(&list);

    if (result != 0 || list == nullptr) {
        LOGE("nativeListWallets: Failed to list wallets: %d", result);
        throwException(env, "java/io/IOException", "Failed to list wallets");
        return nullptr;
    }

    LOGI("nativeListWallets: Found %zu wallets", list->count);

    // Create String array for wallet names
    jclass stringClass = env->FindClass("java/lang/String");
    if (stringClass == nullptr) {
        LOGE("nativeListWallets: Failed to find String class");
        wallet_list_free(list);
        return nullptr;
    }

    jobjectArray walletArray = env->NewObjectArray(list->count, stringClass, nullptr);
    if (walletArray == nullptr) {
        LOGE("nativeListWallets: Failed to create wallet array");
        wallet_list_free(list);
        return nullptr;
    }

    // Populate array with wallet names
    for (size_t i = 0; i < list->count; i++) {
        jstring walletName = stringToJstring(env, list->wallets[i].name);
        if (walletName != nullptr) {
            env->SetObjectArrayElement(walletArray, i, walletName);
        }
    }

    wallet_list_free(list);

    LOGI("nativeListWallets: Wallet list created successfully");
    return walletArray;
}

/**
 * Get wallet address for network
 */
JNIEXPORT jstring JNICALL
Java_io_cpunk_dna_domain_WalletService_nativeGetAddress(JNIEnv* env, jobject obj, jstring walletName, jstring network) {
    LOGI("nativeGetAddress: Getting wallet address");

    const char* wallet_name = jstringToString(env, walletName);
    const char* network_name = jstringToString(env, network);

    if (wallet_name == nullptr || network_name == nullptr) {
        releaseString(env, walletName, wallet_name);
        releaseString(env, network, network_name);
        throwException(env, "java/lang/IllegalArgumentException", "Wallet name or network is null");
        return nullptr;
    }

    LOGI("nativeGetAddress: wallet=%s, network=%s", wallet_name, network_name);

    // Read wallet
    cellframe_wallet_t* wallet = nullptr;
    int result = wallet_read_cellframe(wallet_name, &wallet);

    if (result != 0 || wallet == nullptr) {
        LOGE("nativeGetAddress: Failed to read wallet: %d", result);
        releaseString(env, walletName, wallet_name);
        releaseString(env, network, network_name);
        throwException(env, "java/io/IOException", "Failed to read wallet");
        return nullptr;
    }

    // Get address for network
    char address[WALLET_ADDRESS_MAX] = {0};
    result = wallet_get_address(wallet, network_name, address);

    releaseString(env, walletName, wallet_name);
    releaseString(env, network, network_name);

    if (result != 0) {
        LOGE("nativeGetAddress: Failed to get address: %d", result);
        wallet_free(wallet);
        throwException(env, "java/lang/RuntimeException", "Failed to get wallet address");
        return nullptr;
    }

    jstring jaddress = stringToJstring(env, address);
    wallet_free(wallet);

    if (jaddress == nullptr) {
        LOGE("nativeGetAddress: Failed to create address string");
        return nullptr;
    }

    LOGI("nativeGetAddress: Address: %s", address);
    return jaddress;
}

/**
 * Get token balance via RPC
 */
JNIEXPORT jstring JNICALL
Java_io_cpunk_dna_domain_WalletService_nativeGetBalance(
    JNIEnv* env, jobject obj,
    jstring rpcUrl,
    jstring network,
    jstring address,
    jstring token
) {
    LOGI("nativeGetBalance: Getting token balance");

    const char* rpc_url = jstringToString(env, rpcUrl);
    const char* network_name = jstringToString(env, network);
    const char* addr = jstringToString(env, address);
    const char* token_name = jstringToString(env, token);

    if (rpc_url == nullptr || network_name == nullptr || addr == nullptr || token_name == nullptr) {
        releaseString(env, rpcUrl, rpc_url);
        releaseString(env, network, network_name);
        releaseString(env, address, addr);
        releaseString(env, token, token_name);
        throwException(env, "java/lang/IllegalArgumentException", "Invalid arguments");
        return nullptr;
    }

    LOGI("nativeGetBalance: rpc=%s, network=%s, address=%s, token=%s",
         rpc_url, network_name, addr, token_name);

    // Call RPC to get balance
    char* response = nullptr;
    int result = cellframe_rpc_call(rpc_url, "wallet", "info",
                                    network_name, addr, token_name, &response);

    releaseString(env, rpcUrl, rpc_url);
    releaseString(env, network, network_name);
    releaseString(env, address, addr);
    releaseString(env, token, token_name);

    if (result != 0 || response == nullptr) {
        LOGE("nativeGetBalance: RPC call failed: %d", result);
        free(response);
        throwException(env, "java/io/IOException", "Failed to get balance via RPC");
        return nullptr;
    }

    // Parse balance from response (simplified - assumes response is just the balance)
    // TODO: Parse JSON response properly
    jstring jbalance = stringToJstring(env, response);
    free(response);

    if (jbalance == nullptr) {
        LOGE("nativeGetBalance: Failed to create balance string");
        return nullptr;
    }

    LOGI("nativeGetBalance: Balance retrieved successfully");
    return jbalance;
}

/**
 * Send transaction via RPC
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
) {
    LOGI("nativeSendTransaction: Sending transaction");

    const char* rpc_url = jstringToString(env, rpcUrl);
    const char* network_name = jstringToString(env, network);
    const char* wallet_path = jstringToString(env, walletPath);
    const char* to_addr = jstringToString(env, toAddress);
    const char* token_name = jstringToString(env, token);
    const char* amount_str = jstringToString(env, amount);
    const char* fee_str = jstringToString(env, fee);

    if (rpc_url == nullptr || network_name == nullptr || wallet_path == nullptr ||
        to_addr == nullptr || token_name == nullptr || amount_str == nullptr || fee_str == nullptr) {

        releaseString(env, rpcUrl, rpc_url);
        releaseString(env, network, network_name);
        releaseString(env, walletPath, wallet_path);
        releaseString(env, toAddress, to_addr);
        releaseString(env, token, token_name);
        releaseString(env, amount, amount_str);
        releaseString(env, fee, fee_str);

        throwException(env, "java/lang/IllegalArgumentException", "Invalid arguments");
        return nullptr;
    }

    LOGI("nativeSendTransaction: rpc=%s, network=%s, to=%s, token=%s, amount=%s, fee=%s",
         rpc_url, network_name, to_addr, token_name, amount_str, fee_str);

    // Read wallet for signing
    cellframe_wallet_t* wallet = nullptr;
    int result = wallet_read_cellframe_path(wallet_path, &wallet);

    if (result != 0 || wallet == nullptr) {
        LOGE("nativeSendTransaction: Failed to read wallet: %d", result);

        releaseString(env, rpcUrl, rpc_url);
        releaseString(env, network, network_name);
        releaseString(env, walletPath, wallet_path);
        releaseString(env, toAddress, to_addr);
        releaseString(env, token, token_name);
        releaseString(env, amount, amount_str);
        releaseString(env, fee, fee_str);

        throwException(env, "java/io/IOException", "Failed to read wallet");
        return nullptr;
    }

    // Build and send transaction via RPC
    // TODO: Implement proper transaction builder integration
    char* tx_hash = nullptr;

    // Simplified RPC call (actual implementation would build transaction and sign it)
    result = cellframe_rpc_call(rpc_url, "tx", "create",
                                network_name, to_addr, token_name, &tx_hash);

    wallet_free(wallet);

    releaseString(env, rpcUrl, rpc_url);
    releaseString(env, network, network_name);
    releaseString(env, walletPath, wallet_path);
    releaseString(env, toAddress, to_addr);
    releaseString(env, token, token_name);
    releaseString(env, amount, amount_str);
    releaseString(env, fee, fee_str);

    if (result != 0 || tx_hash == nullptr) {
        LOGE("nativeSendTransaction: Transaction failed: %d", result);
        free(tx_hash);
        throwException(env, "java/io/IOException", "Failed to send transaction");
        return nullptr;
    }

    jstring jtx_hash = stringToJstring(env, tx_hash);
    free(tx_hash);

    if (jtx_hash == nullptr) {
        LOGE("nativeSendTransaction: Failed to create tx hash string");
        return nullptr;
    }

    LOGI("nativeSendTransaction: Transaction sent successfully");
    return jtx_hash;
}
