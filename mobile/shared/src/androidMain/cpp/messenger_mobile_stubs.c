/**
 * Mobile stubs for messenger functions that depend on PostgreSQL
 * These provide minimal implementations for Android that don't use PostgreSQL
 */

#include <stdlib.h>
#include <string.h>
#include <android/log.h>
#include "libpq-fe.h"
#include "messenger.h"

#define LOG_TAG "MessengerStubs"

/**
 * Stub: Load public keys (mobile version - returns error)
 *
 * On mobile, keys should be loaded from local storage or DNA context,
 * not from PostgreSQL. This stub returns an error to indicate the key
 * couldn't be loaded from the database.
 */
int messenger_load_pubkey(
    messenger_context_t *ctx,
    const char *identity,
    uint8_t **signing_pubkey_out,
    size_t *signing_len_out,
    uint8_t **encryption_pubkey_out,
    size_t *encryption_len_out
) {
    (void)ctx;  // Suppress unused warning
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG,
                       "messenger_load_pubkey() called on mobile - not implemented (identity: %s)",
                       identity);

    // On mobile, we don't have PostgreSQL to load keys from
    // Keys should be managed through DNA context or local storage
    // For now, return error to indicate key not found
    *signing_pubkey_out = NULL;
    *signing_len_out = 0;
    *encryption_pubkey_out = NULL;
    *encryption_len_out = 0;

    return -1;  // Key not found
}

/**
 * Stub: PostgreSQL connection functions
 */
PGconn* PQconnectdb(const char *conninfo) {
    (void)conninfo;  // Suppress unused warning
    return NULL;
}

void PQfinish(PGconn *conn) {
    (void)conn;  // Suppress unused warning
}

ConnStatusType PQstatus(const PGconn *conn) {
    (void)conn;  // Suppress unused warning
    return CONNECTION_BAD;
}

char* PQerrorMessage(const PGconn *conn) {
    (void)conn;  // Suppress unused warning
    return (char*)"PostgreSQL not available on mobile";
}

/**
 * Stub: PostgreSQL query execution functions
 */
PGresult* PQexec(PGconn *conn, const char *command) {
    (void)conn;
    (void)command;
    return NULL;
}

PGresult* PQexecParams(
    PGconn *conn,
    const char *command,
    int nParams,
    const Oid *paramTypes,
    const char * const *paramValues,
    const int *paramLengths,
    const int *paramFormats,
    int resultFormat
) {
    (void)conn;
    (void)command;
    (void)nParams;
    (void)paramTypes;
    (void)paramValues;
    (void)paramLengths;
    (void)paramFormats;
    (void)resultFormat;
    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,
                       "PQexecParams() stub called - PostgreSQL not available on mobile");
    return NULL;
}

ExecStatusType PQresultStatus(const PGresult *res) {
    (void)res;
    return PGRES_FATAL_ERROR;
}

void PQclear(PGresult *res) {
    (void)res;
}

/**
 * Stub: PostgreSQL result access functions
 */
int PQntuples(const PGresult *res) {
    (void)res;
    return 0;
}

int PQnfields(const PGresult *res) {
    (void)res;
    return 0;
}

char* PQgetvalue(const PGresult *res, int row, int col) {
    (void)res;
    (void)row;
    (void)col;
    return (char*)"";
}

char* PQfname(const PGresult *res, int col) {
    (void)res;
    (void)col;
    return (char*)"";
}
