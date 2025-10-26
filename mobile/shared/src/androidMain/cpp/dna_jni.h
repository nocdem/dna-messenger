/**
 * dna_jni.h - JNI Function Declarations for DNA Messenger
 *
 * JNI methods for cryptographic operations:
 * - Key generation (Kyber512 + Dilithium3)
 * - Message encryption/decryption
 * - Signature operations
 *
 * Package: io.cpunk.dna.domain
 */

#ifndef DNA_JNI_H
#define DNA_JNI_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize DNA context
 * Java: native fun nativeInit(): Long
 */
JNIEXPORT jlong JNICALL
Java_io_cpunk_dna_domain_DNAMessenger_nativeInit(JNIEnv* env, jobject obj);

/**
 * Free DNA context
 * Java: native fun nativeFree(contextPtr: Long)
 */
JNIEXPORT void JNICALL
Java_io_cpunk_dna_domain_DNAMessenger_nativeFree(JNIEnv* env, jobject obj, jlong contextPtr);

/**
 * Generate Kyber512 encryption keypair
 * Java: native fun nativeGenerateEncryptionKeyPair(): Pair<ByteArray, ByteArray>
 *
 * Returns: Pair of (publicKey, privateKey)
 * - publicKey: 800 bytes (Kyber512 public key)
 * - privateKey: 1632 bytes (Kyber512 private key)
 */
JNIEXPORT jobject JNICALL
Java_io_cpunk_dna_domain_DNAMessenger_nativeGenerateEncryptionKeyPair(JNIEnv* env, jobject obj, jlong contextPtr);

/**
 * Generate Dilithium3 signing keypair
 * Java: native fun nativeGenerateSigningKeyPair(): Pair<ByteArray, ByteArray>
 *
 * Returns: Pair of (publicKey, privateKey)
 * - publicKey: 1952 bytes (Dilithium3 public key)
 * - privateKey: 4032 bytes (Dilithium3 private key)
 */
JNIEXPORT jobject JNICALL
Java_io_cpunk_dna_domain_DNAMessenger_nativeGenerateSigningKeyPair(JNIEnv* env, jobject obj, jlong contextPtr);

/**
 * Encrypt message
 * Java: native fun nativeEncrypt(
 *     contextPtr: Long,
 *     plaintext: ByteArray,
 *     recipientEncPubKey: ByteArray,
 *     senderSignPubKey: ByteArray,
 *     senderSignPrivKey: ByteArray
 * ): ByteArray
 *
 * @param contextPtr DNA context pointer
 * @param plaintext Message to encrypt
 * @param recipientEncPubKey Recipient's Kyber512 public key (800 bytes)
 * @param senderSignPubKey Sender's Dilithium3 public key (1952 bytes)
 * @param senderSignPrivKey Sender's Dilithium3 private key (4032 bytes)
 * @return Encrypted ciphertext
 */
JNIEXPORT jbyteArray JNICALL
Java_io_cpunk_dna_domain_DNAMessenger_nativeEncrypt(
    JNIEnv* env, jobject obj, jlong contextPtr,
    jbyteArray plaintext,
    jbyteArray recipientEncPubKey,
    jbyteArray senderSignPubKey,
    jbyteArray senderSignPrivKey
);

/**
 * Decrypt message
 * Java: native fun nativeDecrypt(
 *     contextPtr: Long,
 *     ciphertext: ByteArray,
 *     recipientEncPrivKey: ByteArray
 * ): Pair<ByteArray, ByteArray>
 *
 * @param contextPtr DNA context pointer
 * @param ciphertext Encrypted message
 * @param recipientEncPrivKey Recipient's Kyber512 private key (1632 bytes)
 * @return Pair of (plaintext, senderSignPubKey)
 */
JNIEXPORT jobject JNICALL
Java_io_cpunk_dna_domain_DNAMessenger_nativeDecrypt(
    JNIEnv* env, jobject obj, jlong contextPtr,
    jbyteArray ciphertext,
    jbyteArray recipientEncPrivKey
);

/**
 * Get library version
 * Java: native fun nativeGetVersion(): String
 */
JNIEXPORT jstring JNICALL
Java_io_cpunk_dna_domain_DNAMessenger_nativeGetVersion(JNIEnv* env, jobject obj);

#ifdef __cplusplus
}
#endif

#endif // DNA_JNI_H
