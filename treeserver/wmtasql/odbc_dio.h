/***************************************************************************\
 * FILENAME:                    odbc_dio.h
 * Copyright: (c)
 *
 * MainAuthor:
 *     Xilong Pei
 * Statements:
 *
\***************************************************************************/

#ifndef __ODBC_DIO_H_
#define __ODBC_DIO_H_

#include <windows.h>
#include <sql.h>
#include <sqlext.h>

#include "wst2mt.h"
#include "dio.h"

#define ODBC_TABLE      2

#define MAX_ODBC_NUM	100

extern WSToMT char szOdbcExecResult[512];

typedef struct OdbcAccessTag
{
    BOOL    IsUsing;
    HDBC    Hdbc;
    HENV    Henv;
    HSTMT   hstmt;
} ODBC_ACCESS, *LP_ODBC_ACCESS;

/*
about the defination of sqlFetchDirtction:
it is defined as short, but it is int now.

An SQLINTEGER bitmask enumerating the supported fetch direction options. 
The following bitmasks are used in conjunction with the flag to determine 
which options are supported:

SQL_FD_FETCH_NEXT          (ODBC 1.0)
SQL_FD_FETCH_FIRST         (ODBC 1.0)
SQL_FD_FETCH_LAST          (ODBC 1.0)
SQL_FD_FETCH_PRIOR         (ODBC 1.0)
SQL_FD_FETCH_ABSOLUTE	   (ODBC 1.0)
SQL_FD_FETCH_RELATIVE      (ODBC 1.0)
SQL_FD_FETCH_BOOKMARK      (ODBC 2.0)
*/
typedef struct tagODBC_HANDLE {
    HENV    henv;
    HDBC    hdbc;
    HSTMT   hstmt;
    LP_ODBC_ACCESS  lpOdbc;
    int	    sqlFetchDirtction;
    int	    isSeamRun;
} ODBC_HANDLE;


BOOL InitOdbcAccess( int OdbcNum, char *szDSN, char *szUser, char *szPassword );
void CloseOdbcAccess( void );
LP_ODBC_ACCESS ApplyOdbcAccess( void );
void ReleaseOdbcAccess( LP_ODBC_ACCESS pOdbcAccess );

dFILE *odbcOpen(char *tmpTableName, char *odbcStatement);
unsigned char *odbcGetRec(dFILE *df);
short odbcClose(dFILE *df);
char *odbcExecute(char *odbcStatement);

char *dioBlobSQLGetData(dFILE *df, unsigned short fldId);

#endif