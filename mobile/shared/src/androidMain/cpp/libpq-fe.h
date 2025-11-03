/*
 * PostgreSQL stub header for Android
 * Mobile doesn't use PostgreSQL, but messenger.h includes this header
 * This is just a stub to make it compile
 */

#ifndef LIBPQ_FE_H
#define LIBPQ_FE_H

#ifdef __cplusplus
extern "C" {
#endif

// Stub PostgreSQL connection type (unused on mobile)
typedef struct pg_conn PGconn;
typedef struct pg_result PGresult;
typedef unsigned int Oid;

// Stub connection status
typedef enum {
    CONNECTION_OK,
    CONNECTION_BAD
} ConnStatusType;

// Stub exec status
typedef enum {
    PGRES_COMMAND_OK,
    PGRES_TUPLES_OK,
    PGRES_FATAL_ERROR
} ExecStatusType;

// Stub function declarations (implementations in messenger_mobile_stubs.c)
PGconn* PQconnectdb(const char *conninfo);
void PQfinish(PGconn *conn);
ConnStatusType PQstatus(const PGconn *conn);
char* PQerrorMessage(const PGconn *conn);
PGresult* PQexec(PGconn *conn, const char *command);
ExecStatusType PQresultStatus(const PGresult *res);
void PQclear(PGresult *res);
int PQntuples(const PGresult *res);
int PQnfields(const PGresult *res);
char* PQgetvalue(const PGresult *res, int row, int col);
char* PQfname(const PGresult *res, int col);

// PQexecParams - used by messenger_p2p.c
PGresult* PQexecParams(
    PGconn *conn,
    const char *command,
    int nParams,
    const Oid *paramTypes,
    const char * const *paramValues,
    const int *paramLengths,
    const int *paramFormats,
    int resultFormat
);

#ifdef __cplusplus
}
#endif

#endif // LIBPQ_FE_H
