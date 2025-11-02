package io.cpunk.dna.domain

import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.net.HttpURLConnection
import java.net.URL

/**
 * Android-specific HTTP implementations for API clients
 *
 * Provides POST, GET, PATCH, and DELETE methods for Messages, Contacts, and Groups API clients
 */

private const val TAG = "ApiHttpImpl"

/**
 * HTTP POST that returns response body
 */
internal actual suspend fun httpPostImpl(url: String, jsonBody: String, timeout: Int, authToken: String?): String {
    return withContext(Dispatchers.IO) {
        try {
            val urlConnection = URL(url).openConnection() as HttpURLConnection
            urlConnection.apply {
                requestMethod = "POST"
                connectTimeout = timeout * 1000
                readTimeout = timeout * 1000
                doOutput = true
                setRequestProperty("Content-Type", "application/json")
                setRequestProperty("Accept", "application/json")
                if (authToken != null) {
                    setRequestProperty("Authorization", "Bearer $authToken")
                }
            }

            // Write request body
            OutputStreamWriter(urlConnection.outputStream).use { writer ->
                writer.write(jsonBody)
                writer.flush()
            }

            // Check response code
            val responseCode = urlConnection.responseCode
            if (responseCode != HttpURLConnection.HTTP_OK && responseCode != HttpURLConnection.HTTP_CREATED) {
                val errorStream = urlConnection.errorStream
                val errorMessage = if (errorStream != null) {
                    BufferedReader(InputStreamReader(errorStream)).use { it.readText() }
                } else {
                    "HTTP $responseCode"
                }
                Log.e(TAG, "HTTP POST failed: $responseCode - $errorMessage")
                throw RuntimeException("HTTP POST failed: $responseCode - $errorMessage")
            }

            // Read response
            val response = BufferedReader(InputStreamReader(urlConnection.inputStream)).use {
                it.readText()
            }

            urlConnection.disconnect()
            response
        } catch (e: Exception) {
            Log.e(TAG, "HTTP POST error to $url: ${e.message}", e)
            throw e
        }
    }
}

/**
 * HTTP GET
 */
internal actual suspend fun httpGetImpl(url: String, timeout: Int, authToken: String?): String {
    return withContext(Dispatchers.IO) {
        try {
            val urlConnection = URL(url).openConnection() as HttpURLConnection
            urlConnection.apply {
                requestMethod = "GET"
                connectTimeout = timeout * 1000
                readTimeout = timeout * 1000
                setRequestProperty("Accept", "application/json")
                if (authToken != null) {
                    setRequestProperty("Authorization", "Bearer $authToken")
                }
            }

            // Check response code
            val responseCode = urlConnection.responseCode
            if (responseCode != HttpURLConnection.HTTP_OK) {
                val errorStream = urlConnection.errorStream
                val errorMessage = if (errorStream != null) {
                    BufferedReader(InputStreamReader(errorStream)).use { it.readText() }
                } else {
                    "HTTP $responseCode"
                }
                Log.e(TAG, "HTTP GET failed: $responseCode - $errorMessage")
                throw RuntimeException("HTTP GET failed: $responseCode - $errorMessage")
            }

            // Read response
            val response = BufferedReader(InputStreamReader(urlConnection.inputStream)).use {
                it.readText()
            }

            urlConnection.disconnect()
            response
        } catch (e: Exception) {
            Log.e(TAG, "HTTP GET error from $url: ${e.message}", e)
            throw e
        }
    }
}

/**
 * HTTP PATCH
 */
internal actual suspend fun httpPatchImpl(url: String, jsonBody: String, timeout: Int, authToken: String?): String {
    return withContext(Dispatchers.IO) {
        try {
            val urlConnection = URL(url).openConnection() as HttpURLConnection
            urlConnection.apply {
                requestMethod = "PATCH"
                connectTimeout = timeout * 1000
                readTimeout = timeout * 1000
                doOutput = true
                setRequestProperty("Content-Type", "application/json")
                setRequestProperty("Accept", "application/json")
                if (authToken != null) {
                    setRequestProperty("Authorization", "Bearer $authToken")
                }
            }

            // Write request body
            OutputStreamWriter(urlConnection.outputStream).use { writer ->
                writer.write(jsonBody)
                writer.flush()
            }

            // Check response code
            val responseCode = urlConnection.responseCode
            if (responseCode != HttpURLConnection.HTTP_OK) {
                val errorStream = urlConnection.errorStream
                val errorMessage = if (errorStream != null) {
                    BufferedReader(InputStreamReader(errorStream)).use { it.readText() }
                } else {
                    "HTTP $responseCode"
                }
                Log.e(TAG, "HTTP PATCH failed: $responseCode - $errorMessage")
                throw RuntimeException("HTTP PATCH failed: $responseCode - $errorMessage")
            }

            // Read response
            val response = BufferedReader(InputStreamReader(urlConnection.inputStream)).use {
                it.readText()
            }

            urlConnection.disconnect()
            response
        } catch (e: Exception) {
            Log.e(TAG, "HTTP PATCH error to $url: ${e.message}", e)
            throw e
        }
    }
}

/**
 * HTTP DELETE
 */
internal actual suspend fun httpDeleteImpl(url: String, timeout: Int, authToken: String?): String {
    return withContext(Dispatchers.IO) {
        try {
            val urlConnection = URL(url).openConnection() as HttpURLConnection
            urlConnection.apply {
                requestMethod = "DELETE"
                connectTimeout = timeout * 1000
                readTimeout = timeout * 1000
                setRequestProperty("Accept", "application/json")
                if (authToken != null) {
                    setRequestProperty("Authorization", "Bearer $authToken")
                }
            }

            // Check response code
            val responseCode = urlConnection.responseCode
            if (responseCode != HttpURLConnection.HTTP_OK && responseCode != HttpURLConnection.HTTP_NO_CONTENT) {
                val errorStream = urlConnection.errorStream
                val errorMessage = if (errorStream != null) {
                    BufferedReader(InputStreamReader(errorStream)).use { it.readText() }
                } else {
                    "HTTP $responseCode"
                }
                Log.e(TAG, "HTTP DELETE failed: $responseCode - $errorMessage")
                throw RuntimeException("HTTP DELETE failed: $responseCode - $errorMessage")
            }

            // Read response (if any)
            val response = try {
                BufferedReader(InputStreamReader(urlConnection.inputStream)).use {
                    it.readText()
                }
            } catch (e: Exception) {
                "{}" // Empty JSON for successful DELETE with no content
            }

            urlConnection.disconnect()
            response
        } catch (e: Exception) {
            Log.e(TAG, "HTTP DELETE error to $url: ${e.message}", e)
            throw e
        }
    }
}
