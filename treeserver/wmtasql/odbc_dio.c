/***************************************************************************\
 * FILENAME:                    odbc_dio.c
 * Copyright: (c)
 *
 * MainAuthor:
 *     Xilong Pei
 *
 * Statements:
 *     2000.5.5 in ODBC result, both of the return value: SQL_SUCCESS, 
 *				SQL_SUCCESS_WITH_INFO
 *              are correct, withut considering the warning
 *
 *     2000.5.6 when using the ODBC runing pool, the DSN, User and Password
 *		writen in the statement must the same of the define in tengine.ini		
 *              or we will not use the runing pool
 *		the DSN, User and Password in the statement cann't be ommited
 *
 *     2001.7.6 SQLError() == SQL_SUCCESS, means get the information success
 *		but in ODBC 3.0,  this is changed
 *
 *
\***************************************************************************/

#include <limits.h>
#include <windows.h>
#include <crtdbg.h>

#include "odbc_dio.h"
#include "mistring.h"
#include "strutl.h"
#include "dioext.h"
#include "wst2mt.h"



//ODBC如果10秒钟无法提供服务,则给用户提示,通过现在这种调度方式,如果没有
//ODBC服务可以提供,其它正在服务的也被挂起,不知是为什么.
#define DWMILLISECONDS_FOR_ODBC_ELAPSE	10*1000

//ROWSET SIZE in ODBC
int iDIO_ODBC_SQL_ROWSET_SIZE = 20;


//result, static text buffer
WSToMT char szOdbcExecResult[512];

//<><><><><><><><><><><><>
char szOdbcDSN[32];
char szOdbcUser[32];
char szOdbcPassword[32];
//<><><><><><><><><><><><><><><><><><><><><><><><>

char *odbcExecute(char *odbcStatement);

int  odbcInitialized = 0;

HANDLE	hOdbcMutex = NULL;		// Snchronize the read/write of the queue.
HANDLE	hOdbcResSem = NULL;		// Resource count of the queue.
int	TotalOdbcNum;

ODBC_ACCESS	OdbcAccess[MAX_ODBC_NUM];	// Odbc access queue.


//////////////////////////////////////////////////////////////////////////////
BOOL InitOdbcAccess( int OdbcNum, char *szDSN, char *szUser, char *szPassword )
{
	int  i;
	RETCODE rc;

	if( OdbcNum > MAX_ODBC_NUM || OdbcNum <= 0 )
	{
		strcpy(szOdbcExecResult, "并发数量错");
		return FALSE;
	}

	
	//<>save the ODBC environment
	strZcpy(szOdbcDSN, lrtrim(szDSN), 32);
	strZcpy(szOdbcUser, lrtrim(szUser), 32);
	strZcpy(szOdbcPassword, lrtrim(szPassword), 32);

	TotalOdbcNum = OdbcNum;

	// Create the read/write synchronization object.
	// This mutex is not owned by any thread.
	hOdbcMutex = CreateMutex( NULL,	FALSE, "$OdbcMutex$" );
	if( hOdbcMutex == NULL )
	{
		strcpy(szOdbcExecResult, "$OdbcMutex$错");
		return FALSE;
	}

	// Initialize the semaphore which indicate how many resources are available.
	hOdbcResSem = CreateSemaphore( NULL, OdbcNum, OdbcNum, "$OdbcResSem$" );
	if( hOdbcResSem == NULL ) 
	{
		strcpy(szOdbcExecResult, "$OdbcResSem$错");
		return FALSE;
	}

	ZeroMemory( OdbcAccess, sizeof( ODBC_ACCESS ) * MAX_ODBC_NUM );

	for( i = 0; i < OdbcNum; i++ )
	{
		rc = SQLAllocEnv( &(OdbcAccess[i].Henv) );
		if( (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) )
		{
			strcpy(szOdbcExecResult, "SQLAllocEnv()错");
			return  FALSE;
		}
		
		/*
		//2001.7.7 Xilong
		// Let ODBC know this is an ODBC 3.0 app.
		rc = SQLSetEnvAttr(&(OdbcAccess[i].Henv), SQL_ATTR_ODBC_VERSION,
				(SQLPOINTER) SQL_OV_ODBC3, SQL_IS_INTEGER);
		*/

		rc = SQLAllocConnect( OdbcAccess[i].Henv, &(OdbcAccess[i].Hdbc));
		if( (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) )
		{
			strcpy(szOdbcExecResult, "SQLAllocConnect()错");
			return  FALSE;
		}

		rc = SQLConnect( OdbcAccess[i].Hdbc, szOdbcDSN, SQL_NTS, szOdbcUser, SQL_NTS, 
						 szOdbcPassword, SQL_NTS);
		if( (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) )
		{
			//error process

			char szSQLState[6];
			SDWORD nErr;
			char msg[100+1];
			SWORD cbmsg;
			char  buf[256];

			szOdbcExecResult[0] = '\0';
			while( SQLError( OdbcAccess[i].Henv, OdbcAccess[i].Hdbc, NULL, szSQLState,
				   &nErr, msg, sizeof(msg), &cbmsg) != SQL_NO_DATA_FOUND )
			{
				sprintf(buf, "%s %ld, %s\n", szSQLState, nErr, msg);
				strcat(szOdbcExecResult, buf);
			}
	    
			SQLFreeConnect(OdbcAccess[i].Hdbc);
			SQLFreeEnv(OdbcAccess[i].Henv);
			return  FALSE;
		}
		
		SQLAllocStmt(OdbcAccess[i].Hdbc, &(OdbcAccess[i].hstmt));

		// ##################################
		
		//this setting cause ODBC get data very slowly
		//2001.7.10 Xilong
		//rc = SQLSetStmtOption(OdbcAccess[i].hstmt, SQL_CURSOR_TYPE, SQL_CURSOR_DYNAMIC );

		//rc = SQLSetStmtOption(OdbcAccess[i].hstmt, SQL_CURSOR_TYPE, SQL_CURSOR_STATIC);
		// ##################################
		/*
		SQLSetStmtOption(OdbcAccess[i].hstmt, SQL_CONCURRENCY, SQL_CONCUR_READ_ONLY);
		SQLSetStmtOption(OdbcAccess[i].hstmt, SQL_CURSOR_TYPE, SQL_CURSOR_KEYSET_DRIVEN);
		*/
		
		if ( iDIO_ODBC_SQL_ROWSET_SIZE > 0 )
		    SQLSetStmtOption(OdbcAccess[i].hstmt, SQL_ROWSET_SIZE, iDIO_ODBC_SQL_ROWSET_SIZE);

	} //end of for

	odbcInitialized = 1;
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
void CloseOdbcAccess( void )
{
	int i;

	if( odbcInitialized == 0 )
		return;

	if( hOdbcMutex != NULL )
	{
	        CloseHandle( hOdbcMutex );
		hOdbcMutex = NULL;
	}

	if( hOdbcResSem != NULL )
	{
		CloseHandle( hOdbcResSem );
		hOdbcResSem = NULL;
	}

	for( i = 0; i < TotalOdbcNum; i++ )
	{
	    SQLFreeStmt( OdbcAccess[i].hstmt, SQL_DROP );
	    SQLDisconnect( OdbcAccess[i].Hdbc );
	    SQLFreeConnect( OdbcAccess[i].Hdbc);
	    SQLFreeEnv( OdbcAccess[i].Henv );
	}

	odbcInitialized = 0;
}


	
//////////////////////////////////////////////////////////////////////////////
LP_ODBC_ACCESS ApplyOdbcAccess( void )
{
	int i;

	//wait until there is enough resource to continue
	//WaitForSingleObject( hOdbcResSem, INFINITE );
	if( WaitForSingleObject( hOdbcResSem, DWMILLISECONDS_FOR_ODBC_ELAPSE ) != WAIT_OBJECT_0 )
	{
	    strcpy(szOdbcExecResult, "ODBC无足够并发");
	    return  NULL;
	}

	//lock the queue
	WaitForSingleObject( hOdbcMutex, INFINITE );

	for( i = 0; i < TotalOdbcNum; i++ )
		if( OdbcAccess[i].IsUsing == FALSE )
			break;

	_ASSERT( i != TotalOdbcNum );
	
	OdbcAccess[i].IsUsing = TRUE;

	ReleaseMutex( hOdbcMutex );

	return &OdbcAccess[i];
}


//////////////////////////////////////////////////////////////////////////////
void ReleaseOdbcAccess( LP_ODBC_ACCESS pOdbcAccess )
{
	int i;

	WaitForSingleObject( hOdbcMutex, INFINITE );

	i = ( (char *)pOdbcAccess - (char *)OdbcAccess ) / sizeof( ODBC_ACCESS );

	_ASSERT( i >= 0 && i < TotalOdbcNum );

	OdbcAccess[i].IsUsing = FALSE;

	ReleaseSemaphore( hOdbcResSem, 1, NULL );
	
	ReleaseMutex( hOdbcMutex );
}


/*
-----------------------------------------------------------------------
		odbcOpen()
-----------------------------------------------------------------------*/
dFILE *odbcOpen(char *tmpTableName, char *odbcStatement)
{
    dFIELD *field;
    dFILE  *df;
    char   *szDSN;
    char   *szUser;
    char   *szPassword;
    char   *szSQL;
    int    i, j;
    ODBC_HANDLE *hOdbc;
    short  isSeamRun;

    RETCODE	rc;
    HENV	henv;
    HDBC	hdbc;
    HSTMT	hstmt;
    LP_ODBC_ACCESS  lpOdbc;
    SWORD	cbData;
    SWORD	cCols;
    SWORD	fSQLType;
    UDWORD	cbPrec;
    SWORD	cbScale;
    SWORD	fNullable;

    if( odbcStatement == NULL )
	return  NULL;

    i = strlen(odbcStatement);
    szDSN = odbcStatement;
    szUser = strnchr(odbcStatement, i, (unsigned char)',', 1);
    szPassword = strnchr(odbcStatement, i, (unsigned char)',', 2);
    szSQL = strnchr(odbcStatement, i, (unsigned char)',', 3);

    if( szUser == NULL || szPassword == NULL || szSQL == NULL ) {
	//if( !odbcInitialized ) {
		return  NULL;
	//}
    } else {
	*szUser++ = '\0';
	*szPassword++ = '\0';
	*szSQL++ = '\0';

	lrtrim(szDSN);
	lrtrim(szUser);
	lrtrim(szPassword);
	lrtrim(szSQL);

    }

    if( odbcInitialized && \
		stricmp(szDSN, szOdbcDSN) == 0 && stricmp(szUser, szOdbcUser) == 0 && \
					    stricmp(szPassword, szOdbcPassword) == 0 )
    { //can use the runing pool to run
	    isSeamRun = 1;
	    lpOdbc = ApplyOdbcAccess();

	    if( lpOdbc == NULL )
		return  NULL;

	    hdbc = lpOdbc->Hdbc; 
	    henv = lpOdbc->Henv;

	    hstmt = lpOdbc->hstmt;
	    
	    /*
	    How to use a statement (ODBC)
	    ================================
	    To use a statement 

	    Call SQLAllocHandle with a HandleType of SQL_HANDLE_STMT to allocate a statement handle. 
	    Optionally, call SQLSetStmtAttr to set statement options or SQLGetStmtAttr to get statement attributes. 
	    To use server cursors, you must set cursor attributes to values other than their defaults. 

	    Optionally, if the statement will be executed several times, prepare the statement for execution with SQLPrepare. 
	    Optionally, if the statement has bound parameter markers, bind the parameter markers to program variables by using SQLBindParameter. If the statement was prepared, you can call SQLNumParams and SQLDescribeParam to find the number and characteristics of the parameters. 
	    Execute a statement directly by using SQLExecDirect. 
	    Or 

	    If the statement was prepared, execute it multiple times by using SQLExecute. 

	    Or 

	    Call a catalog function, which returns results. 

	    Process the results by binding the result set columns to program variables, by moving data from the result set columns to program variables by using SQLGetData, or a combination of the two methods. 
	    Fetch through the result set of a statement one row at a time. 

	    Or 

	    Fetch through the result set several rows at a time by using a block cursor. 

	    Or 

	    Call SQLRowCount to determine the number of rows affected by an INSERT, UPDATE, or DELETE statement. 

	    If the SQL statement can have multiple result sets, call SQLMoreResults at the end of each result set to see if there are additional result sets to process. 

	    After results are processed, the following actions may be necessary to make the statement handle available to execute a new statement: 
	    If you did not call SQLMoreResults until it returned SQL_NO_DATA, call SQLCloseCursor to close the cursor. 
	    If you bound parameter markers to program variables, call SQLFreeStmt with Option set to SQL_RESET_PARAMS to free the bound parameters. 
	    If you bound result set columns to program variables, call SQLFreeStmt with Option set to SQL_UNBIND to free the bound columns. 
	    To reuse the statement handle, go to step 2. 
	    Call SQLFreeHandle with a HandleType of SQL_HANDLE_STMT to free the statement handle. 
	    */

	    //
	    //SQLCloseCursor replaces SQLFreeStmt with an Option value of SQL_CLOSE. 
	    //On receipt of SQLCloseCursor, the Microsoft SQL Server ODBC driver 
	    //discards pending result set rows. 
	    //Note that the statement's column and parameter bindings (if any exist) 
	    //are left unaltered by SQLCloseCursor
	    rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	    if( (rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO) )
		rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	    if( (rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO) )
		rc = SQLFreeStmt(hstmt, SQL_UNBIND );
	    
	    if( (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) ) {
		//error process
			
		char   szSQLState[6];
		SDWORD nErr;
		char   msg[100+1];
		SWORD  cbmsg;
		int    i;
		char   buf[120];

		szOdbcExecResult[0] = '\0';
		i = 0;	//for szOdbcExecResult can only hold 4 buf
		while(i++ < 4 && \
		    SQLError(lpOdbc->Henv, lpOdbc->Hdbc, NULL, szSQLState, &nErr, msg, sizeof(msg), &cbmsg) != SQL_NO_DATA_FOUND ) 
		{
		    sprintf(buf, "%s %ld, %s\n", szSQLState, nErr, msg);
		    strcat(szOdbcExecResult, buf);
		}
		
		ReleaseOdbcAccess( lpOdbc );
		
		return  NULL;
	    } //end of if rc

    } else {
	isSeamRun = 0;
	rc = SQLAllocEnv(&henv);
	if( (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) ) {
		return  NULL;
	}
    
	/*
	//2001.7.7 Xilong
	// Let ODBC know this is an ODBC 3.0 app.
	rc = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                            (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_INTEGER);
	*/

	rc = SQLAllocConnect(henv, &hdbc);
	if( (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) ) {
		return  NULL;
	}

	rc = SQLConnect(hdbc, szDSN, SQL_NTS, szUser, SQL_NTS, szPassword, SQL_NTS);
	//rc could be: SQL_SUCCESS, SQL_SUCCESS_WITH_INFO, SQL_ERROR, 
	//or SQL_INVALID_HANDLE.

	if( (rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO) )
	    rc = SQLAllocStmt(hdbc, &hstmt);

	if( (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) ) {
		//error process
			
		char   szSQLState[6];
		SDWORD nErr;
		char   msg[100+1];
		SWORD  cbmsg;
		int    i;
		char   buf[120];

		szOdbcExecResult[0] = '\0';
		i = 0;	//for szOdbcExecResult can only hold 4 buf

		//SQLError() == SQL_SUCCESS, means get the information success
		//but in ODBC 3.0,  this is changed
		//2001.7.6 Xilong Pei

		while(i++ < 4 && \
		    SQLError(henv, hdbc, NULL, szSQLState, &nErr, msg, sizeof(msg), &cbmsg) != SQL_NO_DATA_FOUND ) 
		{
		    sprintf(buf, "%s %ld, %s\n", szSQLState, nErr, msg);
		    strcat(szOdbcExecResult, buf);
		}
		SQLFreeConnect(hdbc);
		SQLFreeEnv(henv);
		return  NULL;
	}
	
	// ##################################
	//this setting cause ODBC get data very slowly
	//2001.7.10 Xilong
	//rc = SQLSetStmtOption(hstmt, SQL_CURSOR_TYPE, SQL_CURSOR_DYNAMIC );
	//rc = SQLSetStmtOption(hstmt, SQL_CURSOR_TYPE, SQL_CURSOR_STATIC);
	// ##################################
	/*
	SQLSetStmtOption(hstmt, SQL_CONCURRENCY, SQL_CONCUR_READ_ONLY);
	SQLSetStmtOption(hstmt, SQL_CURSOR_TYPE, SQL_CURSOR_KEYSET_DRIVEN);
	*/
	
	if ( iDIO_ODBC_SQL_ROWSET_SIZE > 0 )
	    SQLSetStmtOption(hstmt, SQL_ROWSET_SIZE, iDIO_ODBC_SQL_ROWSET_SIZE);
    }

    
    rc = SQLExecDirect(hstmt, szSQL, SQL_NTS);
    if( (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) ) {
		//error process
		char   szSQLState[6];
		SDWORD nErr;
		char   msg[100+1];
		SWORD  cbmsg;
		char   buf[120];
		int    i;

		szOdbcExecResult[0] = '\0';
		i = 0;	    //for szOdbcExecResult can only hold 4 buf
		while(i++ < 4 && \
		    SQLError(henv, hdbc, hstmt, szSQLState, &nErr, msg, sizeof(msg), &cbmsg) != SQL_NO_DATA_FOUND )
		{
		    sprintf(buf, "%s %ld, %s\n", szSQLState, nErr, msg);
		    strcat(szOdbcExecResult, buf);
		}
		
		if( isSeamRun ) {
		    ReleaseOdbcAccess( lpOdbc );
		} else {
		    SQLFreeStmt(hstmt, SQL_DROP);
		    SQLDisconnect(hdbc);
		    SQLFreeConnect(hdbc);
		    SQLFreeEnv(henv);
		}
		return  NULL;
    }

    rc = SQLNumResultCols(hstmt, &cCols);

    /*if( cCols <= 0 ) 
    {   //
	//this SQL statement may be a INSERT,CREATE TABLE etc
	//
	qsError = 100;
	return  NULL;
    }*/

    field = malloc(sizeof(dFIELD) * (cCols+1));
    if( field == NULL ) {
	if( isSeamRun ) {
	    ReleaseOdbcAccess( lpOdbc );
	} else {
	    SQLFreeStmt(hstmt, SQL_DROP);
	    SQLDisconnect(hdbc);
	    SQLFreeConnect(hdbc);
	    SQLFreeEnv(henv);
	}
	return  NULL;
    }

    hOdbc = (ODBC_HANDLE *)malloc(sizeof(ODBC_HANDLE));
    if( hOdbc == NULL ) {
	free(field);
	if( isSeamRun ) {
	    ReleaseOdbcAccess( lpOdbc );
	} else {
	    SQLFreeStmt(hstmt, SQL_DROP);
	    SQLDisconnect(hdbc);
	    SQLFreeConnect(hdbc);
	    SQLFreeEnv(henv);
	}
	return  NULL;
    }

    //2000.5.6
    hOdbc->isSeamRun = isSeamRun;


    for(j = 0, i = 1;   i <= cCols;   i++, j++) {
	rc = SQLDescribeCol(hstmt, (UWORD)i, (char *)&field[j].field, (SWORD)FIELDNAMELEN, \
			   &cbData, &fSQLType, &cbPrec, &cbScale, &fNullable);
	switch( fSQLType ) {
	    case SQL_NUMERIC:
	    case SQL_DECIMAL:
	    case SQL_FLOAT:
	    case SQL_REAL:
	    case SQL_DOUBLE:
		field[j].fieldtype = 'N';

		if( cbScale <= 0 ) {	    //no scale defined
		    field[j].fielddec = 4;
		    field[j].fieldlen = cbPrec+4;
		} else {
		    field[j].fielddec = (unsigned char)cbScale;
		    field[j].fieldlen = cbPrec+cbScale+2;
		}
		break;
	    case SQL_INTEGER:
	    case SQL_SMALLINT:
		field[j].fieldtype = 'N';
		field[j].fieldlen = cbPrec+cbScale+2;
		field[j].fielddec = (unsigned char)cbScale;
		break;
	    case SQL_CHAR:
	    case SQL_VARCHAR:
	    //case SQL_LONGVARCHAR:
	    //case SQL_UNICODE:
	    //case SQL_UNICODE_VARCHAR:
	    //case SQL_UNICODE_LONGVARCHAR:
		field[j].fieldtype = 'C';
		field[j].fieldlen = cbPrec+1;
		field[j].fielddec = (unsigned char)cbScale;
		break;
	    case SQL_DATE:
	    case SQL_TIME:
	    case SQL_TIMESTAMP:
#if (ODBCVER >= 0x0300)
	    case SQL_TYPE_DATE:
	    case SQL_TYPE_TIME:
	    case SQL_TYPE_TIMESTAMP:
#endif
		field[j].fieldtype = 'D';
		field[j].fieldlen = cbPrec+1;
		field[j].fielddec = (unsigned char)cbScale;
		break;

// 2000.07.06 By Jingyu Niu
// For support BLOB columns. All BOLB columns convert to memo/text fields.
/*
#define SQL_LONGVARCHAR                         (-1)
#define SQL_BINARY                              (-2)
#define SQL_VARBINARY                           (-3)
#define SQL_LONGVARBINARY                       (-4)
#define SQL_BIGINT                              (-5)
#define SQL_TINYINT                             (-6)
#define SQL_BIT                                 (-7)
*/
	    case SQL_LONGVARCHAR:
	    case SQL_BINARY:
	    case SQL_VARBINARY:
	    case SQL_LONGVARBINARY:
	    case SQL_UNICODE:
	    case SQL_UNICODE_VARCHAR:
	    case SQL_UNICODE_LONGVARCHAR:
		field[j].fieldtype = 'M';
		field[j].fieldlen = 10;
		field[j].fielddec = (unsigned char)0;
		break;


	    case SQL_BIGINT:
	    case SQL_TINYINT:
	    case SQL_BIT:
	    case SQL_INTERVAL_YEAR:
	    case SQL_INTERVAL_MONTH:
	    case SQL_INTERVAL_YEAR_TO_MONTH:
	    case SQL_INTERVAL_DAY:
	    case SQL_INTERVAL_HOUR:
	    case SQL_INTERVAL_MINUTE:
	    case SQL_INTERVAL_SECOND:
	    case SQL_INTERVAL_DAY_TO_HOUR:
	    case SQL_INTERVAL_DAY_TO_MINUTE:
	    case SQL_INTERVAL_DAY_TO_SECOND:
	    case SQL_INTERVAL_HOUR_TO_MINUTE:
	    case SQL_INTERVAL_HOUR_TO_SECOND:
	    case SQL_INTERVAL_MINUTE_TO_SECOND:
	    default:
		field[j].fieldtype = 'C';
		field[j].fieldlen = cbPrec+1;
		field[j].fielddec = (unsigned char)cbScale;
		break;
	}
	/*rc = SQLColAttributes(hstmt, i, SQL_COLUMN_TYPE_NAME,
				szTypeName, sizeof(szTypeName), &cbTypeName, 0);
	*/
    }
    field[j].field[0] = '\0';

    df = dTmpCreate(field);
    free(field);

    df->dbf_flag = ODBC_TABLE;

    // #########################################
    field = df->field;
    for(j = 0, i = 1;   i <= cCols;   i++, j++) {
	SQLBindCol(hstmt, (UWORD)i, SQL_C_CHAR, field[j].fieldstart, field[j].fieldlen, &(field[j].sdword));
    }
    // #########################################

    SQLGetInfo(hdbc, SQL_FETCH_DIRECTION, (PTR)&(hOdbc->sqlFetchDirtction),
	       sizeof(hOdbc->sqlFetchDirtction),
	       NULL);

    hOdbc->henv = henv;
    hOdbc->hdbc = hdbc;
    hOdbc->hstmt = hstmt;
    hOdbc->lpOdbc = lpOdbc;
    (void *)(df->sqlDesc) = (void *)hOdbc;

    /*if( SQLRowCount(hstmt, &(df->rec_num)) != SQL_SUCCESS )
	df->rec_num = LONG_MAX;
	*/
    if( df->rec_num < 0 )
	df->rec_num = LONG_MAX;

    return  df;

} //end of odbcOpen()


/*
-----------------------------------------------------------------------
		odbcGetRec()
-----------------------------------------------------------------------*/
unsigned char *odbcGetRec(dFILE *df)
{
    long	lrow;
    int		i;
    dFIELD	*field;
    SQLUSMALLINT rgfRowStatus;
    RETCODE	rc;
    ODBC_HANDLE *hOdbc = (ODBC_HANDLE *)df->sqlDesc;
    dFILE	*pdf;

    if( df->pdf != NULL ) {
	pdf = df->pdf;
	df->rec_num = pdf->rec_num;
    } else
	pdf = df;

    if( hOdbc->sqlFetchDirtction == SQL_FETCH_ABSOLUTE )
	rc = SQLExtendedFetch(hOdbc->hstmt, SQL_FETCH_ABSOLUTE, df->rec_p, &lrow, &rgfRowStatus);
    else
	rc = SQLFetch(hOdbc->hstmt);

    if( (rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO) ) {
	field = pdf->field;
	for( i = pdf->field_num-1;  i >= 0;  i-- ) {
	    if( field[i].sdword <= 0 ) {
		//NULL value, set it to '\0'
		memset(field[i].fieldstart, 0, field[i].fieldlen);
	    } else if ( field[i].fieldtype == 'D' ) {
		char *s = field[i].fieldstart;
		//ODBC date type is as: 1998-12-30
		/*s[4] = s[5];
		s[5] = s[6];
		s[6] = s[8];
		s[7] = s[9];
		s[8] = '\0'; */
		*(short *)&s[4] = *(short *)&s[5];
		
		*(short *)&s[6] = *(short *)&s[8];
		
		s[8] = '\0';
	    }
	}

	if( pdf != df )
	{
	    return  memcpy(df->rec_buf, pdf->rec_buf, pdf->rec_len);
	}

	return  df->rec_buf;
    } else {
	//error process
	char   szSQLState[6];
	SDWORD nErr;
	char   msg[100+1];
	SWORD  cbmsg;
	char   buf[120];
	int    i;

	//2001.7.7 Xilong
	if( rc == SQL_NO_DATA ) {
	    df->error = 0;
	    return  NULL;
	}

	szOdbcExecResult[0] = '\0';
	i = 0;	    //for szOdbcExecResult can only hold 4 buf
	while(i++ < 4 && \
	    SQLError(hOdbc->henv, hOdbc->hdbc, hOdbc->hstmt, szSQLState, &nErr, msg, sizeof(msg), &cbmsg) != SQL_NO_DATA_FOUND )
	{
	    sprintf(buf, "%s %ld, %s\n", szSQLState, nErr, msg);
	    strcat(szOdbcExecResult, buf);
	}
	df->error = i;
	
	//printf("%s %ld, %s\n", szSQLState, nErr, msg);
    
    }

    return  NULL;

} //end of odbcGetRec()


/*
-----------------------------------------------------------------------
		odbcClose()
-----------------------------------------------------------------------*/
short odbcClose(dFILE *df)
{
    ODBC_HANDLE *hOdbc = (ODBC_HANDLE *)(df->sqlDesc);

    if( !(hOdbc->isSeamRun) ) {
	SQLFreeStmt(hOdbc->hstmt, SQL_DROP);
	SQLDisconnect(hOdbc->hdbc);
	SQLFreeConnect(hOdbc->hdbc);
	SQLFreeEnv(hOdbc->henv);
    } else {
	ReleaseOdbcAccess( hOdbc->lpOdbc );
    }
    free(hOdbc);

    free(df);

    return  0;

} //end of odbcClose()




/*
-----------------------------------------------------------------------
		odbcExecute()
-----------------------------------------------------------------------*/
char *odbcExecute(char *odbcStatement)
{
    char   *szDSN;
    char   *szUser;
    char   *szPassword;
    char   *szSQL;
    int    i;
    short  isSeamRun = 0;

    RETCODE rc;
    HENV henv;
    HDBC hdbc;
    HSTMT hstmt;
    LP_ODBC_ACCESS  lpOdbc;

    if( odbcStatement == NULL )
    {
	return  strcpy(szOdbcExecResult, "odbcStatement");
    }

    i = strlen(odbcStatement);
    szDSN = odbcStatement;
    szUser = strnchr(odbcStatement, i, (unsigned char)',', 1);
    szPassword = strnchr(odbcStatement, i, (unsigned char)',', 2);
    szSQL = strnchr(odbcStatement, i, (unsigned char)',', 3);

    if( szUser == NULL || szPassword == NULL || szSQL == NULL ) {
	//error
	//if( !odbcInitialized ) {
	return  strcpy(szOdbcExecResult, "szUser,szPassword,szSQL");
	//}
    } else {
	*szUser++ = '\0';
	*szPassword++ = '\0';
	*szSQL++ = '\0';

	lrtrim(szDSN);
	lrtrim(szUser);
	lrtrim(szPassword);
	lrtrim(szSQL);
    }

    
    if( odbcInitialized && stricmp(szDSN, szOdbcDSN) == 0 && \
					    stricmp(szUser, szOdbcUser) == 0 && \
					    stricmp(szPassword, szOdbcPassword) == 0 )
    {
	isSeamRun = 1;
	lpOdbc = ApplyOdbcAccess();

	if( lpOdbc == NULL )
	    return  NULL;
	
	hstmt = lpOdbc->hstmt;
	hdbc = lpOdbc->Hdbc; 
	henv = lpOdbc->Henv;

    } else {
	isSeamRun = 0;
	rc = SQLAllocEnv(&henv);
	if( (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) ) {
	    return  strcpy(szOdbcExecResult, "SQLAllocEnv()");
	}

	/*
	//2001.7.7 Xilong
	// Let ODBC know this is an ODBC 3.0 app.
	rc = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                            (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_INTEGER);
	*/

	rc = SQLAllocConnect(henv, &hdbc);
	if( (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) ) {
	    return  strcpy(szOdbcExecResult, "SQLAllocConnect()");
	}

	rc = SQLConnect(hdbc, szDSN, SQL_NTS, szUser, SQL_NTS, szPassword, SQL_NTS);
	if( (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) ) {
			//error process

			char szSQLState[6];
			SDWORD nErr;
			char msg[100+1];
			SWORD cbmsg;
			char  buf[256];

			szOdbcExecResult[0] = '\0';
			while(SQLError(henv, hdbc, NULL, szSQLState, &nErr, msg, sizeof(msg), &cbmsg) != SQL_NO_DATA_FOUND ) {
				sprintf(buf, "%s %ld, %s\n", szSQLState, nErr, msg);
				strcat(szOdbcExecResult, buf);
			}
			SQLFreeConnect(hdbc);
			SQLFreeEnv(henv);
			return  szOdbcExecResult;
	}
	SQLAllocStmt(hdbc, &hstmt);

	// ##################################
	//this setting cause ODBC get data very slowly
	//2001.7.10 Xilong
	//rc = SQLSetStmtOption(hstmt, SQL_CURSOR_TYPE, SQL_CURSOR_DYNAMIC );
	//rc = SQLSetStmtOption(hstmt, SQL_CURSOR_TYPE, SQL_CURSOR_STATIC);
	// ##################################
	/*
	SQLSetStmtOption(hstmt, SQL_CONCURRENCY, SQL_CONCUR_READ_ONLY);
	SQLSetStmtOption(hstmt, SQL_CURSOR_TYPE, SQL_CURSOR_KEYSET_DRIVEN);
	*/
	
	if ( iDIO_ODBC_SQL_ROWSET_SIZE > 0 )
	    SQLSetStmtOption(hstmt, SQL_ROWSET_SIZE, iDIO_ODBC_SQL_ROWSET_SIZE);
    }


    rc = SQLExecDirect(hstmt,szSQL, SQL_NTS);
    if( (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) ) {
		//error process
		char szSQLState[6];
		SDWORD nErr;
		char msg[100+1];
		SWORD cbmsg;
		char  buf[256];

		szOdbcExecResult[0] = '\0';
		while(SQLError(henv, hdbc, hstmt, szSQLState, &nErr, msg, sizeof(msg), &cbmsg) != SQL_NO_DATA_FOUND ) {
			sprintf(buf, "%s %ld, %s\n", szSQLState, nErr, msg);
			strcat(szOdbcExecResult, buf);
		}
		
		if( isSeamRun ) {
			ReleaseOdbcAccess( lpOdbc );
		} else {
			SQLFreeStmt(hstmt, SQL_DROP);
			SQLDisconnect(hdbc);
			SQLFreeConnect(hdbc);
			SQLFreeEnv(henv);
		}

		return  szOdbcExecResult;
    }

    if( isSeamRun ) {
	ReleaseOdbcAccess( lpOdbc );
    }

    return NULL;

} //end of odbcExecute()



/*
-----------------------------------------------------------------------
		dioSQLGetData()
-----------------------------------------------------------------------*/
char *dioBlobSQLGetData(dFILE *df, unsigned short fldId)
{
  
    PBYTE       pPicture = "";
    SQLINTEGER  pIndicators;
    char	szSQLState[6];
    SDWORD	nErr;
    char	msg[100+1];
    SWORD	cbmsg;
    char	buf[256];
    ODBC_HANDLE *hOdbc = (ODBC_HANDLE *)df->sqlDesc;
    SQLRETURN   rc;

    //
    //   SQLRETURN  SQL_API SQLGetData(SQLHSTMT StatementHandle,
    //				    SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
    //				    SQLPOINTER TargetValue, SQLINTEGER BufferLength,
    //				    SQLINTEGER *StrLen_or_Ind);
    //

    // Call SQLGetData to determine the amount of data that's waiting.
    rc = SQLGetData(hOdbc->hstmt, (SQLUSMALLINT)(fldId+1), SQL_C_BINARY, pPicture, 0, &pIndicators);
    if ( rc == SQL_SUCCESS_WITH_INFO || rc == SQL_SUCCESS )
    {
	// Get all the data at once.
	//add 1 for null-terminator and safe
	pIndicators++;
        pPicture = malloc(pIndicators+1);
	if( pPicture == NULL )
	    return  NULL;


        rc = SQLGetData(hOdbc->hstmt, (SQLUSMALLINT)(fldId+1), SQL_C_DEFAULT, pPicture,
			pIndicators, &pIndicators);
	if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        {
            // Handle error and continue.
	    free(pPicture);

	    szOdbcExecResult[0] = '\0';
	    while(SQLError(hOdbc->henv, hOdbc->hdbc, NULL, szSQLState, &nErr, msg, sizeof(msg), &cbmsg) != SQL_NO_DATA ) {
		sprintf(buf, "%s %ld, %s\n", szSQLState, nErr, msg);
		strcat(szOdbcExecResult, buf);
	    }

	    return  NULL;
        }

	return  pPicture;
    }
    
    // Handle error on attempt to get data length.
    szOdbcExecResult[0] = '\0';
    while(SQLError(hOdbc->henv, hOdbc->hdbc, NULL, szSQLState, &nErr, msg, sizeof(msg), &cbmsg) != SQL_NO_DATA_FOUND ) {
	sprintf(buf, "%s %ld, %s\n", szSQLState, nErr, msg);
	strcat(szOdbcExecResult, buf);
    }

    return  NULL;

} //end of dioSQLGetData()



////////////////////////////// end of this odbc_dio.c ////////////////////////////////////////