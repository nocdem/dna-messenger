package io.cpunk.dna.domain

/**
 * Platform-specific HTTP implementations
 *
 * These expect functions are implemented in:
 * - androidMain: ApiHttpImpl.android.kt (using HttpURLConnection)
 * - iosMain: ApiHttpImpl.ios.kt (using NSURLSession)
 */

/**
 * HTTP POST implementation
 * @return Response body as String
 */
internal expect suspend fun httpPostImpl(url: String, jsonBody: String, timeout: Int, authToken: String? = null): String

/**
 * HTTP GET implementation
 * @return Response body as String
 */
internal expect suspend fun httpGetImpl(url: String, timeout: Int, authToken: String? = null): String

/**
 * HTTP PATCH implementation
 * @return Response body as String
 */
internal expect suspend fun httpPatchImpl(url: String, jsonBody: String, timeout: Int, authToken: String? = null): String

/**
 * HTTP DELETE implementation
 * @return Response body as String
 */
internal expect suspend fun httpDeleteImpl(url: String, timeout: Int, authToken: String? = null): String
