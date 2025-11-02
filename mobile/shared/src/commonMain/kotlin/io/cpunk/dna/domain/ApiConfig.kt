package io.cpunk.dna.domain

/**
 * API Configuration
 *
 * Change BASE_URL to match your backend server.
 */
object ApiConfig {
    // Select the appropriate URL for your environment:

    // Cloudflare Tunnel (public HTTPS access)
    const val BASE_URL = "https://fruit-deliver-high-block.trycloudflare.com"

    // Local testing (C keyserver via Apache proxy)
    // const val BASE_URL = "http://192.168.0.198"

    // Production (after DNS and HTTPS setup)
    // const val BASE_URL = "https://cpunk.io"

    // C Keyserver API endpoints
    const val KEYSERVER_REGISTER = "$BASE_URL/api/keyserver/register"
    const val KEYSERVER_LOOKUP = "$BASE_URL/api/keyserver/lookup"
    const val KEYSERVER_LIST = "$BASE_URL/api/keyserver/list"
    const val KEYSERVER_HEALTH = "$BASE_URL/api/keyserver/health"

    // Legacy Node.js backend endpoints (deprecated)
    const val AUTH_ENDPOINT = "$BASE_URL/api/auth"
    const val CONTACTS_ENDPOINT = "$BASE_URL/api/keyserver"  // Now points to C keyserver
    const val MESSAGES_ENDPOINT = "$BASE_URL/api/messages"
    const val GROUPS_ENDPOINT = "$BASE_URL/api/groups"
    const val HEALTH_ENDPOINT = "$BASE_URL/api/keyserver/health"
}
