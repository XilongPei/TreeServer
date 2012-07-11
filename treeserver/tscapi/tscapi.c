/*------------------------------------------------------------------------------*\
 |	Module Name:																|
 |		TSCApi.c																|
 |	Abstract :																	|
 |		 TreeServer Client Application Programming Interface					|
 |	Author :																	|
 |		Jingyu Niu, Motor, Xilong Pei											    |
 |	Date :																		|
 |		06/15/1998	---	50% accomplished.										|
 |		06/16/1998	---	80% accomplished.										|
 |		06/17/1998	---	85% accomplished.										|
 |		06/22/1998	---	95%	accomplished.										|
 |		07/--/1998	--- 99% accomplished.
 |																				|
 |  Copyright (c). China Railway Software Corporation. 1998.					|
\*------------------------------------------------------------------------------*/
#include <windows.h>
#include <lm.h>
#include <crtdbg.h>
#include <stdio.h>
#include <io.h>
#include <limits.h>

#include "ts_com.h"
#include "namedpipe.h"
#include "tlimits.h"
#include "terror.h"
#include "tscapi.h"
#include "des.h"
#include "lzhuf.h"

#define TREESRV_CONTENT_LEN	(PIPE_BUFFER_SIZE-sizeof(TS_COM_PROPS))

/*------------------------------------------------------------------------------*\
 | Static Function Prototype													|
\*------------------------------------------------------------------------------*/
static BOOL	tsTableWriteRec( HANDLE hConnect, DWORD dwTableId, DWORD dwOperCode, 
						   DWORD dwRecNo, LPCSTR szRecBuf, DWORD dwBufLen );

static char szTableInfoBuf[PIPE_BUFFER_SIZE];

static BOOL tsClientWrite( HANDLE hConnect, LPCTSTR szDataBuf, DWORD dwDataLen,
						   char packetType, char msgType, long lp, 
						   char leftPactket, char endPacket );

static DWORD tsComposeStoredProc( HANDLE hConnect, LPCSTR lpProcName, 
								  LPSTR lpBuffer, DWORD dwBufLen, long lp );
static DWORD tsUserPrivilege( HANDLE hConnect, LPCSTR lpProcName, LPCSTR szUserName,
							  char cPrivilege, long lp );


/*------------------------------------------------------------------------------*\
 | Function tsLogon( LPCSTR, LPCSTR, LPCSTR, LPHANDLE )							|
 | Purpose :																	|
 |		Logon to the TreeServer													|
 | Returns:																		|
 |		User Id if succeeds.													|
 |		oxFFFFFFFF if any error occurs.											|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
DWORD tsLogon ( LPCSTR szServer, LPCSTR szUser, LPCSTR szPasswd, 
			    LPHANDLE lphConnect )
{
	CHAR	szComputerName[CNLEN+1];
	DWORD	dwLen;
	HANDLE	hPipe;
	BOOL	bSuccess;
	DWORD	dwUserId;
	CHAR	szDesPasswd[128];

	_ASSERT( ( szServer != NULL && szUser != NULL && szPasswd != NULL
			&& lphConnect != NULL ) );
	if( !( szServer != NULL && szUser != NULL && szPasswd != NULL
														&& lphConnect != NULL ) )
	{
		*lphConnect = INVALID_HANDLE_VALUE;
		return 0xFFFFFFFF;
	}


	dwLen = CNLEN + 1;
	bSuccess = GetComputerName( szComputerName, &dwLen );
	if( !bSuccess ) 
	{
		*lphConnect = INVALID_HANDLE_VALUE;
		return 0xFFFFFFFF;
	}

	hPipe = CliOpenTSNamedPipe( szServer );
	if( hPipe == INVALID_HANDLE_VALUE ) 
	{
		*lphConnect = INVALID_HANDLE_VALUE;
		return 0xFFFFFFFF;
	}

	DES(szPasswd, szPasswd, szDesPasswd);

	dwUserId = CliTSLogon( hPipe, szUser, szDesPasswd, szComputerName );
	if( dwUserId == 0xFFFFFFFF ) 
	{
		CliCloseTSNamedPipe( hPipe );
		*lphConnect = INVALID_HANDLE_VALUE;
		return 0xFFFFFFFF;
	}

	*lphConnect = hPipe;
	return dwUserId;
}


/*------------------------------------------------------------------------------*\
 | Function tsLogoff( HANDLE )													|
 | Purpose :																	|
 |		Logoff from the TreeServer												|
 | Returns:																		|
 |		TRUE if succeeds.														|
 |		FALSE if fails. Use GetLastError() to get the error number.				|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
BOOL tsLogoff ( HANDLE hConnect )
{
	BOOL bSuccess;

	//_ASSERT( hConnect != INVALID_HANDLE_VALUE );
	if( hConnect == INVALID_HANDLE_VALUE ) {
		return  FALSE;
	}
	
	bSuccess = CliTSLogoff( hConnect );
	bSuccess = CliCloseTSNamedPipe( hConnect );

	return bSuccess;
}

/*------------------------------------------------------------------------------*\
 | Function tsUserChangePwd( HANDLE, LPCSTR, LPCSTR, LPCSTR  )					|
 | Purpose :																	|
 |		Change a user's password												|
 | Returns:																		|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
DWORD tsUserChangePwd ( HANDLE hConnect, 
					   LPCSTR szOldPasswd, LPCSTR szNewPasswd )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;
	CHAR			szDesOldPasswd[128];
	CHAR			szDesNewPasswd[128];

	_ASSERT( ( hConnect != INVALID_HANDLE_VALUE
			&& szOldPasswd != NULL && szNewPasswd != NULL ) );
	if( !( hConnect != INVALID_HANDLE_VALUE
						&& szOldPasswd != NULL && szNewPasswd != NULL ) )
	{
		return ULONG_MAX;
	}

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'U';
	lptsComProps->msgType = msgTS_USER;
	lptsComProps->len = lstrlen( szOldPasswd ) + lstrlen(szNewPasswd) + 2;
	lptsComProps->lp = cmTS_USER_CHNG_PWD;
	
	DES(szOldPasswd, szOldPasswd, szDesOldPasswd);
	DES(szNewPasswd, szNewPasswd, szDesNewPasswd);

	lstrcpy( szBuffer + sizeof( TS_COM_PROPS ), szDesOldPasswd );
	lstrcpy( szBuffer + sizeof( TS_COM_PROPS ) + lstrlen(szDesOldPasswd) + 1, szDesNewPasswd );

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return ULONG_MAX;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return ULONG_MAX;

	return lptsComProps->lp;
}


/*------------------------------------------------------------------------------*\
 | Function tsTableOpen( HANDLE, LPCSTR, LPCSTR, LPTABLE_INFO * )				|
 | Purpose :																	|
 |		Open a table															|
 | Returns:																		|
 |		Table id if succeeds.													|
 |		0 if fails. Use GetLastError() to get the error number.					|
 |																				|
 | Comments:																	|
 |		Update by Jinyu Niu, 1999.08											|
 |			lppTableInfo's space was alloced by call user not use the global	|
 |			static variable szTableInfoBuf
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
/*
DWORD tsTableOpen ( HANDLE hConnect, LPCSTR szDatabase, 
				   LPCSTR szTable, LPTABLE_INFO *lppTableInfo )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( ( hConnect != INVALID_HANDLE_VALUE && szTable != NULL
			&& lppTableInfo != NULL ) );
	if( !( hConnect != INVALID_HANDLE_VALUE && szTable != NULL
													&& lppTableInfo != NULL ) )
		return  0;

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'Q';
	lptsComProps->msgType = 'Q';
	lptsComProps->lp = cmTS_OPEN_DBF;
	
	// If it succeeds, wsprintf() returns the number of characters stored in the 
	// output buffer, not counting the terminating null character. So we should 
	// add 1 to the return value to get the correct length of the string.
	lptsComProps->len = wsprintf( szBuffer + sizeof( TS_COM_PROPS), 
								  "%s`%s", szTable, szDatabase  ) + 1;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return 0;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return 0;

	// The lptsComProps->msgType variable indicates the packet's type. A value 
	// of 'D' indicates this is a normal responsive packet. A value of 'E' 
	// indicates this is a error packet. Otherwise a unknown error must occur.
	if( lptsComProps->msgType == 'D' )
	{
		memcpy( szTableInfoBuf, szBuffer + sizeof( TS_COM_PROPS ), 
				lptsComProps->len );
		*lppTableInfo = ( LPTABLE_INFO )szTableInfoBuf;

		// The lptsComProps->lp contains the table id.
		return lptsComProps->lp;
	}
	else if( lptsComProps->msgType == 'E' )
	{
		SetLastError( lptsComProps->lp );
		return 0;
	}

	// Unknown packet type
	_ASSERT( FALSE );
	return  0;
}
*/
DWORD tsTableOpen ( HANDLE hConnect, LPCSTR szDatabase, 
				   LPCSTR szTable, LPTABLE_INFO *lppTableInfo )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( ( hConnect != INVALID_HANDLE_VALUE && szTable != NULL
			&& lppTableInfo != NULL ) );
	if( !( hConnect != INVALID_HANDLE_VALUE && szTable != NULL
													&& lppTableInfo != NULL ) )
		return  0;

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'Q';
	lptsComProps->msgType = 'Q';
	lptsComProps->lp = cmTS_OPEN_DBF;
	
	// If it succeeds, wsprintf() returns the number of characters stored in the 
	// output buffer, not counting the terminating null character. So we should 
	// add 1 to the return value to get the correct length of the string.
	lptsComProps->len = wsprintf( szBuffer + sizeof( TS_COM_PROPS), 
								  "%s`%s", szTable, szDatabase  ) + 1;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return 0;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return 0;

	// The lptsComProps->msgType variable indicates the packet's type. A value 
	// of 'D' indicates this is a normal responsive packet. A value of 'E' 
	// indicates this is a error packet. Otherwise a unknown error must occur.
	if( lptsComProps->msgType == 'D' )
	{
		memcpy( *lppTableInfo, szBuffer + sizeof( TS_COM_PROPS ), 
				lptsComProps->len );

		// The lptsComProps->lp contains the table id.
		return lptsComProps->lp;
	}
	else if( lptsComProps->msgType == 'E' )
	{
		SetLastError( lptsComProps->lp );
		return 0;
	}

	// Unknown packet type
	_ASSERT( FALSE );
	return  0;
}


/*------------------------------------------------------------------------------*\
 | Function tsTableClose( HANDLE, DWORD )										|
 | Purpose :																	|
 |		Close a table															|
 | Returns:																		|
 |		TRUE if succeeds.														|
 |		FALSE if fails. Use GetLastError() to get the error number.				|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
BOOL tsTableClose( HANDLE hConnect, DWORD dwTableId )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE  );
	if( hConnect == INVALID_HANDLE_VALUE  )
		return  FALSE;

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'Q';
	lptsComProps->msgType = 'Q';
	lptsComProps->lp = cmTS_CLOSE_DBF;
	lptsComProps->len = sizeof( DWORD );

	// Store the table id into the beginning of the actual data buffer.
	*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) ) = dwTableId;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return FALSE;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return FALSE;

	// The lptsComProps->msgType variable indicates the packet's type. A value 
	// of 'D' indicates this is a normal responsive packet. A value of 'E' 
	// indicates this is a error packet. Otherwise a unknown error must occur.
	if( lptsComProps->msgType == 'D' )
		return TRUE;
	else if( lptsComProps->msgType == 'E' )
	{
		SetLastError( lptsComProps->lp );
		return FALSE;
	}

	// Unknown packet type
	_ASSERT( FALSE );
	return  0;

}


/*------------------------------------------------------------------------------*\
 | Function tsTableLock( HANDLE, DWORD )										|
 | Purpose :																	|
 |		Lock a table															|
 | Returns:																		|
 |		TRUE if succeeds.														|
 |		FALSE if fails. Use GetLastError() to get the error number.				|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
BOOL tsTableLock( HANDLE hConnect, DWORD dwTableId )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE );
	if( hConnect == INVALID_HANDLE_VALUE )
		return  FALSE;

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'Q';
	lptsComProps->msgType = 'Q';
	lptsComProps->lp = cmTS_LOCK_DBF;
	lptsComProps->len = sizeof( DWORD );

	// Store the table id into the beginning of the actual data buffer.
	*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) ) = dwTableId;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return FALSE;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return FALSE;

	// The lptsComProps->msgType variable indicates the packet's type. A value 
	// of 'D' indicates this is a normal responsive packet. A value of 'E' 
	// indicates this is a error packet. Otherwise a unknown error must occur.
	if( lptsComProps->msgType == 'D' )
	{
		// The lptsComProps->lp indicates if the lock operation succeeds.
		// if 0 succeeds. Otherwise it contains the reason code of failure.
		if( lptsComProps->lp == 0 )
			return TRUE;
		else 
		{
			SetLastError( lptsComProps->lp );
			return FALSE;
		}
	}
	else if( lptsComProps->msgType == 'E' )
	{
		SetLastError( lptsComProps->lp );
		return FALSE;
	}

	// Unknown packet type
	_ASSERT( FALSE );
	return  0;
}


/*------------------------------------------------------------------------------*\
 | Function tsTableUnlock( HANDLE, DWORD )										|
 | Purpose :																	|
 |		Unlock a table															|
 | Returns:																		|
 |		TRUE if succeeds.														|
 |		FALSE if fails. Use GetLastError() to get the error number.				|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
BOOL tsTableUnlock( HANDLE hConnect, DWORD dwTableId )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE );
	if( hConnect == INVALID_HANDLE_VALUE )
		return  FALSE;

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'Q';
	lptsComProps->msgType = 'Q';
	lptsComProps->lp = cmTS_UNLOCK_DBF;
	lptsComProps->len = sizeof( DWORD );

	// Store the table id into the beginning of the actual data buffer.
	*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) ) = dwTableId;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return FALSE;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return FALSE;

	// The lptsComProps->msgType variable indicates the packet's type. A value 
	// of 'D' indicates this is a normal responsive packet. A value of 'E' 
	// indicates this is a error packet. Otherwise a unknown error must occur.
	if( lptsComProps->msgType == 'D' )
	{
		// The lptsComProps->lp indicates if the unlock operation succeeds.
		// if 0 succeeds. Otherwise it contains the reason code of failure.
		if( lptsComProps->lp == 0 )
			return TRUE;
		else 
		{
			SetLastError( lptsComProps->lp );
			return FALSE;
		}
	}
	else if( lptsComProps->msgType == 'E' )
	{
		SetLastError( lptsComProps->lp );
		return FALSE;
	}

	// Unknown packet type
	_ASSERT( FALSE );
	return  0;
}


/*------------------------------------------------------------------------------*\
 | Function tsTableGetRec( HANDLE, DWORD, DWORD, LPSTR, DWORD, LPRECPROPS )		|
 | Purpose :																	|
 |		Get record from table													|
 | Returns:																		|
 |		0 if succeeds. Otherwise error number.									|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
DWORD tsTableGetRec( HANDLE hConnect, DWORD dwTableId, DWORD dwRecNo, 
					 LPSTR szRecBuf, DWORD dwBufLen, LPRECPROPS lpRecProps )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE );
	if( hConnect == INVALID_HANDLE_VALUE )
		return  -1;

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'Q';
	lptsComProps->msgType = 'Q';
	lptsComProps->lp = cmTS_REC_FETCH;
	lptsComProps->len = sizeof( DWORD ) * 2;

	// Store the table id into the beginning of the actual data buffer.
	*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) ) = dwTableId;

	// Store the record No. into the data buffer.
	*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) + sizeof( DWORD ) ) = dwRecNo;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return dwRetCode;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return dwRetCode;

	// The lptsComProps->msgType variable indicates the packet's type. A value 
	// of 'D' indicates this is a normal responsive packet. A value of 'E' 
	// indicates this is a error packet. Otherwise a unknown error must occur.
	if( lptsComProps->msgType == 'D' )
	{
		DWORD dwRecLen;

		memcpy( lpRecProps, szBuffer + sizeof( TS_COM_PROPS ), sizeof( RECPROPS ) );
		
		dwRecLen = lptsComProps->len - sizeof( RECPROPS );

		// szRecBuf is too small
		if( dwRecLen > dwBufLen )
			return -1;
			
		memcpy( szRecBuf, szBuffer + sizeof( TS_COM_PROPS ) + sizeof( RECPROPS ),
				dwRecLen );

		return 0;
	}
	else if( lptsComProps->msgType == 'E' )
		return lptsComProps->lp;

	// Unknown packet type
	_ASSERT( FALSE );
	return  0;
}

/*------------------------------------------------------------------------------*\
 | Function tsTableDelRec( HANDLE, DWORD, DWORD )								|
 | Purpose :																	|
 |		Delete a record from table												|
 | Returns:																		|
 |		TRUE if succeeds.														|
 |		FALSE if fails. Use GetLastError() to get the error number.				|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
BOOL tsTableDelRec( HANDLE hConnect, DWORD dwTableId, DWORD dwRecNo )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE );
	if( hConnect == INVALID_HANDLE_VALUE )
		return  FALSE;

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'Q';
	lptsComProps->msgType = 'Q';
	lptsComProps->lp = cmTS_REC_DEL;
	lptsComProps->len = sizeof( DWORD ) * 2;

	// Store the table id into the beginning of the actual data buffer.
	*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) ) = dwTableId;

	// Store the record No into the data buffer
	*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) + sizeof( DWORD ) ) = dwRecNo;


	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return FALSE;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return FALSE;

	// The lptsComProps->msgType variable indicates the packet's type. A value 
	// of 'D' indicates this is a normal responsive packet. A value of 'E' 
	// indicates this is a error packet. Otherwise a unknown error must occur.
	if( lptsComProps->msgType == 'D' )
		return TRUE;
	else if( lptsComProps->msgType == 'E' )
	{
		SetLastError( lptsComProps->lp );
		return FALSE;
	}

	// Unknown packet type
	_ASSERT( FALSE );
	return  0;
}


/*------------------------------------------------------------------------------*\
 | Function tsTablePutRec( HANDLE, DWORD, DWORD, LPCSTR, DWORD )				|
 | Purpose :																	|
 |		Put record into table													|
 | Returns:																		|
 |		TRUE if succeeds.														|
 |		FALSE if fails. Use GetLastError() to get the error number.				|
 |																				|
 | Comments:																	|
 |		Just call tsTableWriteRec() to perform the function						|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
BOOL tsTablePutRec( HANDLE hConnect, DWORD dwTableId, DWORD dwRecNo, 
					LPCSTR szRecBuf, DWORD dwBufLen )
{
	return tsTableWriteRec( hConnect, dwTableId, cmTS_REC_PUT, dwRecNo, 
							szRecBuf, dwBufLen );
}


/*------------------------------------------------------------------------------*\
 | Function tsTableAppendRec( HANDLE, DWORD, LPCSTR, DWORD )					|
 | Purpose :																	|
 |		Append record															|
 | Returns:																		|
 |		TRUE if succeeds.														|
 |		FALSE if fails. Use GetLastError() to get the error number.				|
 |																				|
 | Comments:																	|
 |		Just call tsTableWriteRec() to perform the function						|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
BOOL tsTableAppendRec( HANDLE hConnect, DWORD dwTableId, LPCSTR szRecBuf, 
					   DWORD dwBufLen )
{
	return tsTableWriteRec( hConnect, dwTableId, cmTS_REC_APP, -1, 
							szRecBuf, dwBufLen );	
}


/*------------------------------------------------------------------------------*\
 | Function tsTableInsertRec( HANDLE, DWORD, LPCSTR, DWORD )					|
 | Purpose :																	|
 |		Insert record															|
 | Returns:																		|
 |		TRUE if succeeds.														|
 |		FALSE if fails. Use GetLastError() to get the error number.				|
 |																				|
 | Comments:																	|
 |		Just call tsTableWriteRec() to perform the function						|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
BOOL tsTableInsertRec( HANDLE hConnect, DWORD dwTableId, LPCSTR szRecBuf, 
					   DWORD dwBufLen )
{
	return tsTableWriteRec( hConnect, dwTableId, cmTS_REC_ADD, -1, 
							szRecBuf, dwBufLen );	
}


/*------------------------------------------------------------------------------*\
 | Function tsTableWriteRec( HANDLE, DWORD, DWORD, DWORD, LPCSTR, DWORD )		|
 | Purpose :																	|
 |		Write record into table													|
 | Returns:																		|
 |		TRUE if succeeds.														|
 |		FALSE if fails. Use GetLastError() to get the error number.				|
 |																				|
 | Comments:																	|
 |		This is a common function for tsTablePutRec(), tsTableAppendRec()		|
 |		and tsTableInsertRec().													|
\*------------------------------------------------------------------------------*/
static BOOL tsTableWriteRec( HANDLE hConnect, DWORD dwTableId, DWORD dwOperCode, 
						   DWORD dwRecNo, LPCSTR szRecBuf, DWORD dwBufLen )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE );

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'Q';
	lptsComProps->msgType = 'Q';
	lptsComProps->lp = dwOperCode;
	lptsComProps->len = sizeof( DWORD ) + dwBufLen;
	if( dwRecNo != -1 )
	{
		// If it is called by tsTableAppendRec() or tsTableInsertRec(), mustn't do so.
		lptsComProps->len += sizeof( DWORD );
	}


	// Store the table id into the beginning of the actual data buffer.
	*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) ) = dwTableId;

	if( dwRecNo != -1 )
	{
		// Store the record No. into the data buffer.
		// If it is called by tsTableAppendRec() or tsTableInsertRec(), mustn't do so.
		*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) + sizeof( DWORD ) ) = dwRecNo;
	}

	// Store the record content into the data buffer.
	memcpy( szBuffer + sizeof( TS_COM_PROPS ) + sizeof( DWORD ) * 
			( ( dwRecNo != -1 ) + 1 ), szRecBuf, dwBufLen );

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return FALSE;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return FALSE;

	// The lptsComProps->msgType variable indicates the packet's type. A value 
	// of 'D' indicates this is a normal responsive packet. A value of 'E' 
	// indicates this is a error packet. Otherwise a unknown error must occur.
	if( lptsComProps->msgType == 'D' )
		return TRUE;
	else if( lptsComProps->msgType == 'E' )
	{
		SetLastError( lptsComProps->lp );
		return FALSE;
	}

	// Unknown packet type
	_ASSERT( FALSE );
	return  0;
}


/*------------------------------------------------------------------------------*\
 | Function BOOL tsTableBlobGet( HANDLE, DWORD, LPCSTR, DWORD, LPCSTR )			|
 | Purpose:																		|
 |		Get a BLOB object from table and store it into a local file				|
 | Returns:																		|
 |		TRUE if succeeds.														|
 |		FALSE if fails. Use GetLastError() to get the error number.				|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
BOOL tsTableBlobGet( HANDLE hConnect, DWORD dwTableId, LPCSTR szFieldName, 
					 DWORD dwRecNo, LPCSTR szFileName )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;
	FILE			*fp;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE && szFieldName != NULL 
			 && szFieldName != NULL );
	if( hConnect == INVALID_HANDLE_VALUE || szFieldName == NULL 
													|| szFieldName == NULL )
		return  FALSE;

	if( ( fp = fopen( szFileName, "wb" ) ) == NULL )
	{
		// Fail to open the specified file
		return FALSE;
	}

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'Q';
	lptsComProps->msgType = 'Q';
	lptsComProps->lp = cmTS_BLOB_MEM_FETCH;
	lptsComProps->len = sizeof( DWORD ) * 2 + 12;	// 12 is the maxiamal length
													// of the field name.

	// Store the table id into the beginning of the actual data buffer.
	*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) ) = dwTableId;

	// Store the record No into the data buffer
	*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) + sizeof( DWORD ) ) = dwRecNo;

	// Store the blob field name into the data buffer, the maximal length of the 
	// field name is 12
	strncpy( szBuffer + sizeof( TS_COM_PROPS ) + sizeof( DWORD ) * 2, szFieldName, 12 );

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
	{
		fclose( fp );
		return FALSE;
	}

	while( 1 )
	{
		dwBufSize = PIPE_BUFFER_SIZE;
		if( CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize ) != TERR_SUCCESS ) {
			fclose( fp );
			return 3;
		}

		switch( lptsComProps->msgType )
		{
		case 'D' :
			goto _EXIT;

		case 'B' :
			fwrite( szBuffer + sizeof( TS_COM_PROPS ), lptsComProps->len, 1, fp );
			break;

		case 'E' :
			fclose( fp );
			SetLastError( lptsComProps->lp );
			return FALSE;

		default :
			fclose( fp );
			// Unknown packet type
			return  FALSE;
			//_ASSERT( FALSE );
		}
	}

_EXIT :
	fclose( fp );
	return TRUE;
}


/*------------------------------------------------------------------------------*\
 | Function BOOL tsTableBlobPut( HANDLE, DWORD, LPCSTR, DWORD, LPCSTR )			|
 | Purpose:																		|
 |		Get a table's record number												|
 | Returns:																		|
 |		TRUE if succeeds.														|
 |		FALSE if fails. Use GetLastError() to get the error number.				|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
BOOL tsTableBlobPut( HANDLE hConnect, DWORD dwTableId, LPCSTR szFieldName, 
					 DWORD dwRecNo, LPCSTR szFileName )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;
	FILE			*fp;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE && szFieldName != NULL 
			 && szFieldName != NULL );
	if( hConnect == INVALID_HANDLE_VALUE || szFieldName == NULL 
													|| szFieldName == NULL )
		return  FALSE;

	if( ( fp = fopen( szFileName, "rb" ) ) == NULL )
	{
		// Fail to open the specified file
		return FALSE;
	}

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'Q';
	lptsComProps->msgType = 'Q';
	lptsComProps->leftPacket = '\1';
	lptsComProps->endPacket = '\1';
	lptsComProps->lp = cmTS_BLOB_MEM_PUT;
	lptsComProps->len = sizeof( DWORD ) * 2 + 12;	// 12 is the maxiamal length
													// of the field name.

	// Store the table id into the beginning of the actual data buffer.
	*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) ) = dwTableId;

	// Store the record No into the data buffer
	*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) + sizeof( DWORD ) ) = dwRecNo;

	// Store the blob field name into the data buffer, the maximal length of the 
	// field name is 12
	strncpy( szBuffer + sizeof( TS_COM_PROPS ) + sizeof( DWORD ) * 2, szFieldName, 12 );

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
	{
		fclose( fp );
		return FALSE;
	}

	lptsComProps->lp = _filelength( _fileno( fp ) );

	do
	{
		lptsComProps->packetType = 'Q';
		lptsComProps->msgType = 'B';
		lptsComProps->len = fread( szBuffer + sizeof( TS_COM_PROPS ), 1, 
								   TREESRV_CONTENT_LEN, fp );

		if( feof( fp ) || lptsComProps->len < TREESRV_CONTENT_LEN )
		{
			lptsComProps->leftPacket = '\0';
			lptsComProps->endPacket = '\0';
		}
		else
		{
			lptsComProps->leftPacket = '\1';
			lptsComProps->endPacket = '\1';
		}

		dwBufSize = PIPE_BUFFER_SIZE;
		dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
		if( dwRetCode != TERR_SUCCESS ) 
		{
			fclose( fp );
			return FALSE;
		}
	} while( !(feof( fp ) || lptsComProps->len < TREESRV_CONTENT_LEN) );
	
	fclose( fp );

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return FALSE;

	switch( lptsComProps->msgType )
	{
	case 'D' :
		return TRUE;

	case 'E' :
		return FALSE;

	default :
		return FALSE;
	}
}

/*------------------------------------------------------------------------------*\
 | Function tsTableRecNum( HANDLE, DWORD )										|
 | Purpose :																	|
 |		Get a table's record number												|
 | Returns:																		|
 |		record number if succeeds.												|
 |		-1 if fails. Use GetLastError() to get the error number.				|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
LONG tsTableRecNum( HANDLE hConnect, DWORD dwTableId )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE );

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'Q';
	lptsComProps->msgType = 'Q';
	lptsComProps->lp = cmTS_DBF_RECNUM;
	lptsComProps->len = sizeof( DWORD );

	// Store the table id into the beginning of the actual data buffer.
	*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) ) = dwTableId;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return -1;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return -1;

	// The lptsComProps->msgType variable indicates the packet's type. A value 
	// of 'D' indicates this is a normal responsive packet. A value of 'E' 
	// indicates this is a error packet. Otherwise a unknown error must occur.
	if( lptsComProps->msgType == 'D' )
	{
		// The lptsComProps->lp contains the returned record number.
		return lptsComProps->lp;
	}
	else if( lptsComProps->msgType == 'E' )
	{
		SetLastError( lptsComProps->lp );
		return FALSE;
	}

	// Unknown packet type
	_ASSERT( FALSE );
	return  0;
}


/*------------------------------------------------------------------------------*\
 | Function DWORD tsTableGetFieldInfo( HANDLE, DWORD, LPDIO_FIELD, DWORD )		|
 | Purpose :																	|
 |		Get a table's field infomation											|
 | Returns:																		|
 |		number of field.														|
 |		0xffffffff : error occurs												|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
DWORD tsTableGetFieldInfo( HANDLE hConnect, DWORD dwTableId, 
						   LPDIO_FIELD lpFieldInfo, DWORD dwFieldLen )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE );

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'Q';
	lptsComProps->msgType = 'Q';
	lptsComProps->lp = cmTS_FIELD_INFO;
	lptsComProps->len = sizeof( DWORD );

	// Store the table id into the beginning of the actual data buffer.
	*( DWORD * )( szBuffer + sizeof( TS_COM_PROPS ) ) = dwTableId;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return -1;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return -1;

	// The lptsComProps->msgType variable indicates the packet's type. A value 
	// of 'D' indicates this is a normal responsive packet. A value of 'E' 
	// indicates this is a error packet. Otherwise a unknown error must occur.
	if( lptsComProps->msgType == 'D' )
	{
		if( ( DWORD )( lptsComProps->len ) > dwFieldLen ) {
			return 0xffffffff;
		}

		//_ASSERT( IsBadWritePtr( lpFieldInfo, dwFieldLen ) );
		memcpy( lpFieldInfo, szBuffer + sizeof( TS_COM_PROPS ), lptsComProps->len );

		return lptsComProps->lp;
	}
	else if( lptsComProps->msgType == 'E' )
	{
		SetLastError( lptsComProps->lp );
		return 0xffffffff;
	}

	// Unknown packet type
	_ASSERT( FALSE );
	return  0;
}

/*------------------------------------------------------------------------------*\
 | Function LPSTR tsTableGetFieldByName( LPDIO_FIELD, LPCSTR, LPCSTR,			|
 |										LPSTR, DWORD dwResultSize)				|
 | Purpose: 																	|
 |		Get a field content from record buffer by specified name				|
 | Returns:																		|
 |		Field pointer															|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
LPSTR tsTableGetFieldByName( LPDIO_FIELD lpFieldInfo, LPCSTR szRecBuf,
							 LPCSTR szFieldName,LPSTR szResult, DWORD dwResultSize)
{
	for( ; *(lpFieldInfo->field) != '\0'; lpFieldInfo++ )
	{
		if( !stricmp( lpFieldInfo->field, szFieldName ) )
			return ( LPSTR )( szRecBuf + lpFieldInfo->fieldstart );
	}

	// If we can't find the specified field, there must be a programming 
	// error. So let's abort the program to indicate that.
	_ASSERT( FALSE );
	return  0;
}


/*------------------------------------------------------------------------------*\
 | Function LPSTR tsTableGetfieldById( LPDIO_FIELD, LPCSTR, DWORD )				|
 | Purpose :																	|
 |		Get a field content from record buffer by specified name				|
 | Returns:																		|
 |		Field pointer															|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
LPSTR tsTableGetfieldById( LPDIO_FIELD lpdioField, LPCSTR szRecBuf,
						   DWORD dwFieldId )
{
	return ( LPSTR )( szRecBuf + lpdioField[dwFieldId].fieldstart );
}

/*------------------------------------------------------------------------------*\
 | Function DWORD tsCallAsqlSvr( HANDLE, LPCSTR, LPCSTR, LPSTR, DWORD )			|
 | Purpose :																	|
 |		Perform ASQL scripts file												|
 | Returns :																	|
 |		0 if succeeds.															|
 |		other values if fails. Use GetLastError() to get the error number.		|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
DWORD tsCallAsqlSvr( HANDLE hConnect, LPCSTR szAsqlScriptFileName, 
					 LPCSTR szResultFileName, LPSTR szInstResult, 
					 DWORD dwInstResultSize )
{
	FILE	*fpScript, *fpResult;
	char	*szAsqlScript, szResult[10000];
	DWORD	dwLen = 10000, dwScriptLen;
	DWORD	dwRetCode;

	if( ( fpScript = fopen( szAsqlScriptFileName, "r" ) ) == NULL )
		return 1;
	if( ( fpResult = fopen( szResultFileName, "w" ) ) == NULL )
	{
		fclose( fpScript );
		return 1;
	}

	dwScriptLen = _filelength( _fileno( fpScript ) );
	if( ( szAsqlScript = ( char * )malloc( dwScriptLen + 1 ) ) == NULL )
		return 2;

	fread( szAsqlScript, dwScriptLen, 1, fpScript );
	szAsqlScript[dwScriptLen] = '\0';

	dwRetCode = tsCallAsqlSvrM( hConnect, szAsqlScript, szResult, &dwLen, 
								szInstResult, dwInstResultSize );

	fwrite( szResult, dwLen, 1, fpResult );

	fclose( fpScript );
	fclose( fpResult );

	free( szAsqlScript );

	return dwRetCode;
}


/*------------------------------------------------------------------------------*\
 | Function DWORD tsCallAsqlSvrM( HANDLE, LPCSTR, LPVOID, LPDWORD, LPSTR,		|
 |								  DWORD )										|
 | Purpose :																	|
 |		Perform 'in-memory' ASQL scripts										|
 | Returns :																	|
 |		1 : intact outcome.														|
 |		2 : incomplete outcome.													|
 |		3 : fails. Use GetLastError() to get the error number.					|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
DWORD tsCallAsqlSvrM( HANDLE hConnect, LPCSTR szAsqlScript, LPVOID lpRsutMem, 
					  LPDWORD lpdwLen, LPSTR szInstResult, DWORD dwInstResultSize )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwCurrentPos = 0;
	BOOL			isComplete = TRUE;
	DWORD			dwScriptLen;

	dwScriptLen = strlen( szAsqlScript ) + 1;
	if( tsClientWrite( hConnect, szAsqlScript, dwScriptLen, 'Q', 'A', dwScriptLen,
					   '\0', '\0' ) == FALSE )
		return 3;

	lptsComProps = ( LPTS_COM_PROPS )szBuffer;

	while( 1 )
	{
		dwBufSize = PIPE_BUFFER_SIZE;
		if( CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize ) != TERR_SUCCESS ) 
			return 3;

		// The lptsComProps->msgType variable indicates the packet's type. A value 
		// of 'D' indicates this is a normal responsive packet. A value of 'E' 
		// indicates this is a error packet. A value of 'X' indicates this is a 
		// instant outcome pactet. Otherwise a unknown error must occur.
		switch( lptsComProps->msgType )
		{
		case 'D' :
			if( ( DWORD )lptsComProps->len > dwInstResultSize )
			{
				SetLastError( TERR_UNKNOWN_ERROR );
				return 3;
			}

			memcpy( szInstResult, szBuffer + sizeof( TS_COM_PROPS ), 
					lptsComProps->len );
			*lpdwLen = dwCurrentPos;

			if( isComplete == TRUE )
				return 1;
			else 
				return 2;

		case 'X' :
		case 'Z':
/*			if( ( dwCurrentPos + lptsComProps->len ) <= *lpdwLen )
			{
				memcpy( ( char * )lpRsutMem + dwCurrentPos, szBuffer + 
						sizeof( TS_COM_PROPS ), lptsComProps->len );
				dwCurrentPos += lptsComProps->len;
			}
			else 
				isComplete = FALSE;
			break;
*/
		case 'W' :
			if( ( dwCurrentPos + lptsComProps->len ) <= *lpdwLen )
			{
				memcpy( ( char * )lpRsutMem + dwCurrentPos, szBuffer + 
						sizeof( TS_COM_PROPS ), lptsComProps->len );
				dwCurrentPos += lptsComProps->len;
			}
			else 
			{
				isComplete = FALSE;
				memcpy( ( char * )lpRsutMem + dwCurrentPos, szBuffer +
						sizeof( TS_COM_PROPS ), *lpdwLen - dwCurrentPos );
				dwCurrentPos = *lpdwLen;
			}
			break;

		case 'E' :
			SetLastError( lptsComProps->lp );
			return 3;

		default :
			// Unknown packet type
			_ASSERT( FALSE );
		}
	}
}


/*------------------------------------------------------------------------------*\
 | Function DWORD tsCallAsqlSvrMVar( HANDLE, LPCSTR, LPSysVarOFunTyper, DWORD,	|
 |									 LPSTR, DWORD, LPVOID, LPDWORD )			|
 | Purpose :																	|
 |		Perform 'in-memory' ASQL scripts with parameters						|
 | Returns :																	|
 |		1 : intact outcome.														|
 |		2 : incomplete outcome.													|
 |		3 : fails. Use GetLastError() to get the error number.					|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
DWORD tsCallAsqlSvrMVar( HANDLE hConnect, LPCSTR szAsqlScript, 
						 LPSysVarOFunType lpXexpVar, DWORD dwXexpVarNum, 
						 LPSTR szInstResult, DWORD dwInstResultSize, 
						 LPVOID lpRsutMem, LPDWORD lpdwLen )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize;
	DWORD			dwCurrentPos = 0, dwXexpCurrPos = 0;
	BOOL			isComplete = TRUE;
	DWORD			dwScriptLen;
	DWORD			i;
	DWORD			incAllocMem = 0, iAllocMem;
	LPSTR			varReallocMem;

	if( dwXexpVarNum >= 256 )
		return 11;

	for( i = 0; i < dwXexpVarNum; i++ )
	{
		/*_ASSERT( lpXexpVar[i].type == 1051 || lpXexpVar[i].type == 1052 || 
			lpXexpVar[i].type == 1053 || lpXexpVar[i].type == 1054 || 
			lpXexpVar[i].type == 1055 || lpXexpVar[i].type == 1056 );

		_ASSERT( lpXexpVar[i].length <= 31 );
		_ASSERT( lpXexpVar[i].length > 0 );
		*/
		if( (lpXexpVar[i].type == 1051 || lpXexpVar[i].type == 1052 || 
			lpXexpVar[i].type == 1053 || lpXexpVar[i].type == 1054 || 
			lpXexpVar[i].type == 1055 || lpXexpVar[i].type == 1056) ) 
		{

			if( lpXexpVar[i].length < 0 )
				return  1;
			else if( lpXexpVar[i].length >= 32 ) {
				incAllocMem += lpXexpVar[i].length;
			}
		} else {
			return  2;
		}
	}

	if( incAllocMem > 0 ) {
		iAllocMem = incAllocMem + sizeof(SysVarOFunType) * dwXexpVarNum;
		varReallocMem = malloc( iAllocMem + 64 );		//add 64 for safe
		if( varReallocMem == NULL )
			return  10;

		incAllocMem = sizeof(SysVarOFunType) * dwXexpVarNum;
		memcpy(varReallocMem, (LPSTR)lpXexpVar, incAllocMem);
		
		for( i = 0; i < dwXexpVarNum; i++ )
		{
			if( lpXexpVar[i].length >= 32 ) {
				memcpy(varReallocMem+incAllocMem, 
							(char *)*(long *)(lpXexpVar[i].values), lpXexpVar[i].length);
				incAllocMem += lpXexpVar[i].length;
			}
		}
	} else {
		varReallocMem = (LPSTR)lpXexpVar;
		iAllocMem = sizeof(SysVarOFunType) * dwXexpVarNum;
	}

	dwScriptLen = strlen( szAsqlScript ) + 1;
	if( tsClientWrite( hConnect, szAsqlScript, dwScriptLen, 'Q', 'E', dwScriptLen,
					   '\0', '\1' ) == FALSE )
	{
		if( incAllocMem > 0 )
			free(varReallocMem);
		return 3;
	}

	if( tsClientWrite( hConnect, varReallocMem, iAllocMem, 'Q', 'E', 
			((dwXexpVarNum & 0xFF)<<24)|(iAllocMem & 0xFFFFFF), '\0', '\0' ) == FALSE )
	{
		if( incAllocMem > 0 )
			free(varReallocMem);
		return 3;	
	}

	lptsComProps = ( LPTS_COM_PROPS )szBuffer;

	while( 1 )
	{
		dwBufSize = PIPE_BUFFER_SIZE;
		if( CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize ) != TERR_SUCCESS ) 
		{
			if( incAllocMem > 0 )
				free(varReallocMem);
			return 3;
		}

		// The lptsComProps->msgType variable indicates the packet's type. A value 
		// of 'D' indicates this is a normal responsive packet. A value of 'E' 
		// indicates this is a error packet. A value of 'X' indicates this is a 
		// instant outcome pactet. Otherwise a unknown error must occur.
		switch( lptsComProps->msgType )
		{
		case 'D' :
			//
			// the end packet
			//
			// the ASQL run information
			//
			if( ( DWORD )lptsComProps->len > dwInstResultSize )
			{
				SetLastError( TERR_UNKNOWN_ERROR );
				if( incAllocMem > 0 )
					free(varReallocMem);
				return 3;
			}

			memcpy( szInstResult, szBuffer + sizeof( TS_COM_PROPS ), lptsComProps->len );
			*lpdwLen = dwCurrentPos;

			if( incAllocMem > 0 ) 
			{
				incAllocMem = sizeof(SysVarOFunType) * dwXexpVarNum;
				for( i = 0;   i < dwXexpVarNum;   i++ )
				{
					if( lpXexpVar[i].length >= 32 ) {
						memcpy((char *)*(long *)(lpXexpVar[i].values), 
								varReallocMem+incAllocMem, lpXexpVar[i].length);
						incAllocMem += lpXexpVar[i].length;
					} else {
						memcpy((char *)(lpXexpVar[i].values), 
								varReallocMem + i*sizeof(SysVarOFunType), lpXexpVar[i].length);
					}
				}
				free(varReallocMem);
			}
	
			if( isComplete == TRUE )
				return 1;
			else 
				return 2;

		case 'X' :
		case 'Z':
/*			if( ( dwCurrentPos + lptsComProps->len ) <= *lpdwLen )
			{
				memcpy( ( char * )lpRsutMem + dwCurrentPos, szBuffer + 
						sizeof( TS_COM_PROPS ), lptsComProps->len );
				dwCurrentPos += lptsComProps->len;
			}
			else 
				isComplete = FALSE;
			break;
*/
		case 'W' :
			if( ( dwCurrentPos + lptsComProps->len ) <= *lpdwLen )
			{
				memcpy( ( char * )lpRsutMem + dwCurrentPos, szBuffer + 
						sizeof( TS_COM_PROPS ), lptsComProps->len );
				dwCurrentPos += lptsComProps->len;
			}
			else 
				isComplete = FALSE;
			break;

		case 'V' :
			if( ( dwXexpCurrPos + lptsComProps->len ) <= iAllocMem )
			{
				memcpy( varReallocMem + dwXexpCurrPos, szBuffer + 
								sizeof( TS_COM_PROPS ),	lptsComProps->len );
				dwXexpCurrPos += lptsComProps->len;
			}
			else
				isComplete = FALSE;
			break;

		case 'E' :
			if( incAllocMem > 0 )
				free(varReallocMem);
			SetLastError( lptsComProps->lp );
			return FALSE;

		default :
			// Unknown packet type
			_ASSERT( FALSE );
		}
	}

	return 0;
} //end of tsCallAsqlSvrMVar()



/*------------------------------------------------------------------------------*\
 | Function DWORD tsHistDataCollect( HANDLE, LPCSTR, LPSysVarOFunTyper, DWORD,	|
 |									 LPSTR, DWORD, LPVOID, LPDWORD )			|
 | Purpose :																	|
 |		Perform 'in-memory' ASQL scripts with parameters						|
 | Returns :																	|
 |		1 : intact outcome.														|
 |		2 : incomplete outcome.													|
 |		3 : fails. Use GetLastError() to get the error number.					|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
DWORD tsHistDataCollect( HANDLE hConnect, LPCSTR szHistcondPath, LPCSTR szRem,
						 LPSysVarOFunType lpXexpVar, DWORD dwXexpVarNum, 
						 LPSTR szInstResult, DWORD dwInstResultSize, 
						 LPVOID lpRsutMem, LPDWORD lpdwLen )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize;
	DWORD			dwCurrentPos = 0, dwXexpCurrPos = 0;
	BOOL			isComplete = TRUE;
	DWORD			dwScriptLen;
	DWORD			i;
	CHAR			szAsqlScript[512];

	sprintf(szAsqlScript, "%s*%s", szHistcondPath, szRem);

	for( i = 0; i < dwXexpVarNum; i++ )
	{
		_ASSERT( lpXexpVar[i].type == 1051 || lpXexpVar[i].type == 1052 || 
			lpXexpVar[i].type == 1053 || lpXexpVar[i].type == 1054 || 
			lpXexpVar[i].type == 1055 || lpXexpVar[i].type == 1056 );

		_ASSERT( lpXexpVar[i].length <= 31 );
		_ASSERT( lpXexpVar[i].length > 0 );
	}

	dwScriptLen = strlen( szAsqlScript ) + 1;
	if( tsClientWrite( hConnect, szAsqlScript, dwScriptLen, 'Q', 'D', dwScriptLen, 
					   '\0', '\1' ) == FALSE )
		return 3;

	if( tsClientWrite( hConnect, ( LPCTSTR )lpXexpVar, sizeof( SysVarOFunType ) * 
			dwXexpVarNum, 'Q', 'D', dwXexpVarNum, '\0', '\0' ) == FALSE )
		return 3;	

	lptsComProps = ( LPTS_COM_PROPS )szBuffer;

	while( 1 )
	{
		dwBufSize = PIPE_BUFFER_SIZE;
		if( CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize ) != TERR_SUCCESS ) 
			return 3;

		// The lptsComProps->msgType variable indicates the packet's type. A value 
		// of 'D' indicates this is a normal responsive packet. A value of 'E' 
		// indicates this is a error packet. A value of 'X' indicates this is a 
		// instant outcome pactet. Otherwise a unknown error must occur.
		switch( lptsComProps->msgType )
		{
		case 'D' :
			if( ( DWORD )lptsComProps->len > dwInstResultSize )
			{
				SetLastError( TERR_UNKNOWN_ERROR );
				return 3;
			}

			memcpy( szInstResult, szBuffer + sizeof( TS_COM_PROPS ), lptsComProps->len );
			*lpdwLen = dwCurrentPos;

			if( isComplete == TRUE )
				return 1;
			else 
				return 2;

		case 'X' :
/*			if( ( dwCurrentPos + lptsComProps->len ) <= *lpdwLen )
			{
				memcpy( ( char * )lpRsutMem + dwCurrentPos, szBuffer + 
						sizeof( TS_COM_PROPS ), lptsComProps->len );
				dwCurrentPos += lptsComProps->len;
			}
			else 
				isComplete = FALSE;
			break;
*/
		case 'W' :
			if( ( dwCurrentPos + lptsComProps->len ) <= *lpdwLen )
			{
				memcpy( ( char * )lpRsutMem + dwCurrentPos, szBuffer + 
						sizeof( TS_COM_PROPS ), lptsComProps->len );
				dwCurrentPos += lptsComProps->len;
			}
			else 
				isComplete = FALSE;
			break;

		case 'V' :
			if( ( dwXexpCurrPos + lptsComProps->len ) <= ( sizeof( SysVarOFunType ) * 
				dwXexpVarNum ) )
			{
				memcpy( ( char * )lpXexpVar + dwXexpCurrPos, szBuffer + 
						sizeof( TS_COM_PROPS ),	lptsComProps->len );
				dwXexpCurrPos += lptsComProps->len;
			}
			else
				isComplete = FALSE;
			break;

		case 'E' :
			SetLastError( lptsComProps->lp );
			return FALSE;

		default :
			// Unknown packet type
			_ASSERT( FALSE );
		}
	}
	return 0;
}


/*------------------------------------------------------------------------------*\
 | Function tsClientWrite( HANDLE, LPCTSTR, DWORD, DWORD, char, char )			|
 | Purpose :																	|
 |		Writes data into named pipe.											|
 | Returns :																	|
 |		TRUE if succeeds.														|
 |		FALSE if fails. Use GetLastError() to get the error number.				|
 |																				|
 | Comments:																	|
 |		It is a common function which can transform large size packet			|
\*------------------------------------------------------------------------------*/
static BOOL tsClientWrite( HANDLE hConnect, LPCTSTR szDataBuf, DWORD dwDataLen,
						   char packetType, char msgType, long lp, 
						   char leftPactket, char endPacket )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize;
	DWORD			i;

	lptsComProps = ( LPTS_COM_PROPS )szBuffer;

	ZeroMemory( lptsComProps, sizeof( TS_COM_PROPS));

	for( i = 0; i < dwDataLen; i += TREESRV_CONTENT_LEN )
	{
		lptsComProps->packetType = packetType;
		lptsComProps->msgType = msgType;
	
		if( ( i + TREESRV_CONTENT_LEN ) < dwDataLen )
		{
			lptsComProps->len = TREESRV_CONTENT_LEN;
			lptsComProps->leftPacket = '\1';
			lptsComProps->endPacket = '\1';
		}
		else
		{
			lptsComProps->len = ( short )( dwDataLen - i );
			lptsComProps->leftPacket = '\0';
			lptsComProps->endPacket = endPacket;
		}
		
		lptsComProps->lp = lp;

		memcpy( szBuffer + sizeof( TS_COM_PROPS ), szDataBuf + i, 
				lptsComProps->len );

		dwBufSize = PIPE_BUFFER_SIZE;
		
		if( CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize ) != 0 )
			return FALSE;
	}

	return TRUE;
}


/*------------------------------------------------------------------------------*\
 | Function tsGetUserNetPath ()													|
 | Purpose :																	|
 |		Get user's net path														|
 | Returns:																		|
 |		0 if succeeds.															|
 |		otherwise fails.														|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
DWORD tsGetUserNetPath ( HANDLE hConnect, LPSTR lpNetPath, DWORD dwBuffer )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE );

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'U';
	lptsComProps->msgType = 'U';
	lptsComProps->lp = 0x4013;
	lptsComProps->len = 0;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return -1;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return -1;

	if( lptsComProps->lp == 0 )
	{
		if( ( DWORD )lptsComProps->len > dwBuffer )
			return -1;

		memcpy( lpNetPath, szBuffer + sizeof( TS_COM_PROPS ), 
				lptsComProps->len );

		return 0;
	}

	SetLastError( lptsComProps->lp );
	return lptsComProps->lp;
}

/*------------------------------------------------------------------------------*\
 | Function tsGetRegisteredUser()												|
 | Purpose :																	|
 |		Get registered user(s)													|
 | Returns:																		|
 |		true if succeeds.														|
 |		false if fails.															|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
BOOL tsGetRegisteredUser( HANDLE hConnect, LPCLIENT_RES_INFO lpClientResInfo, 
						  LPDWORD lpdwMaxClientNum )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;
	DWORD			dwMaxInfoBufSize;

	_ASSERT( lpClientResInfo != NULL && *lpdwMaxClientNum > 0 );

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'U';
	lptsComProps->msgType = 'Q';

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return FALSE;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return FALSE;

	if( lptsComProps->lp == 0 )
	{
		dwMaxInfoBufSize = *lpdwMaxClientNum * sizeof( CLIENT_RES_INFO );

		if( ( DWORD )lptsComProps->len > dwMaxInfoBufSize )
		{
			SetLastError( ERROR_INSUFFICIENT_BUFFER  );
			return FALSE;
		}

		memcpy( lpClientResInfo, szBuffer + sizeof( TS_COM_PROPS ), 
				lptsComProps->len );

		*lpdwMaxClientNum = ( DWORD )lptsComProps->len / sizeof( CLIENT_RES_INFO );

		return TRUE;
	}
	else
	{
		SetLastError( lptsComProps->lp );
		return FALSE;
	}
}

/*------------------------------------------------------------------------------*\
 | Function GetIdenticalUserNum()												|
 | Purpose :																	|
 |		Get the number of the identical users.									|
 | Returns:																		|
 |		the number if succeeds													|
 |		ULONG_MAX if fails
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
DWORD tsGetIdenticalUserNum( HANDLE hConnect, LPCSTR szUser )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'U';
	lptsComProps->msgType = 'N';
	lptsComProps->len = lstrlen( szUser ) + 1;
	
	lstrcpy( szBuffer + sizeof( TS_COM_PROPS ), szUser );

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return ULONG_MAX;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return ULONG_MAX;

	return ( DWORD )( lptsComProps->lp );
}


/////////////////////////////////////////////////////////////////////////////////
// Function : DWORD tsCreateStoredProc()
// Purpose : 
//		Create a stored procedure
// Parameters :
//		hConnect			[IN]	TreeServer connection handle
//		lpProcName			[IN]	Stored procedure name : UserName.ProcName
//		lpBuffer			[IN]	Pointer of the buffer which contains the 
//									contents of the stored procedure
//		dwBufLen			[IN]	Length of the stored procedure's contents
// Returns :
//		0 if succeeds
//		Nonzero, which is a error number, if fails 
//
// Comments :
//		User who creates the procedure owns all the privileges of the procedure
/////////////////////////////////////////////////////////////////////////////////
DWORD tsCreateStoredProc( HANDLE hConnect,	LPCSTR lpProcName, LPSTR lpBuffer, 
						  DWORD	dwBufLen )
{
	return tsComposeStoredProc( hConnect, lpProcName, lpBuffer, dwBufLen, 
								cmTS_NEW_PROC );
}


/////////////////////////////////////////////////////////////////////////////////
// Function DWORD tsDeleteStoredProc()
// Purpose	:
//		Delete a stored procedure
// Parameters :
//		hConnect			[IN]	TreeServer connection handle
//		lpProcName			[IN]	Stored procedure name : UserName.ProcName
//
// Returns :
//		0 if succeeds
//		Nonzero, which is a error number, if fails
//
// Comments :
//		User who wants to perform the operation must have the privilege to write 
//		the destination stored procedure
/////////////////////////////////////////////////////////////////////////////////
DWORD tsDeleteStoredProc( HANDLE hConnect, LPCSTR lpProcName )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE && lpProcName != NULL );

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'P';
	lptsComProps->msgType = 'P';
	lptsComProps->lp = cmTS_DELETE_PROC;
	lptsComProps->len = lstrlen( lpProcName ) + 1;	

	lstrcpy( szBuffer + sizeof( TS_COM_PROPS ), lpProcName );

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return dwRetCode;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return dwRetCode;

	if( lptsComProps->lp == 0 )
		return 0;
	else
	{
		SetLastError( lptsComProps->lp );
		return lptsComProps->lp;
	}
}


/////////////////////////////////////////////////////////////////////////////////
// Function : DWORD tsGetStoredProc()
// Purpose :
//		Get a Stored procedure
// Parameters :
//		hConnect			[IN]	TreeServer connection handle
//		lpProcName			[IN]	Stored procedure name : UserName.ProcName
//		lpBuffer			[OUT]	Pointer of the buffer which receives the 
//									contents of the stored procedure
//		lpdwBufLen			[IN]	Length of the lpBuffer
//							[OUT]	Actual length of the stored procedure
// Returns :
//		0 if succeeds
//		Nonzero, which is a error number, if fails 
//
// Comments :
//		User who wants to perform the operation must have the privilege to wirte 
//		or execute the destination stored procedure
/////////////////////////////////////////////////////////////////////////////////
DWORD tsGetStoredProc( HANDLE hConnect, LPCSTR lpProcName, LPSTR lpBuffer,
					   LPDWORD lpdwBufLen )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;
	DWORD			dwCurrentPos = 0;
	BOOL			isComplete = TRUE;
	
	_ASSERT( hConnect != INVALID_HANDLE_VALUE && lpProcName != NULL );

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'P';
	lptsComProps->msgType = 'P';
	lptsComProps->lp = cmTS_GET_PROC;
	lptsComProps->len = lstrlen( lpProcName ) + 1;	

	lstrcpyn( szBuffer + sizeof( TS_COM_PROPS ), lpProcName, MAX_PROC_NAME );
	
	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return dwRetCode;

	do
	{
		dwBufSize = PIPE_BUFFER_SIZE;
		dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
		if( dwRetCode != TERR_SUCCESS ) 
			return dwRetCode;

		if( lptsComProps->lp != 0 )
		{
			SetLastError( lptsComProps->lp );
			return lptsComProps->lp;
		}

		if( ( dwCurrentPos + lptsComProps->len ) <= *lpdwBufLen )
		{
			CopyMemory( ( PVOID )( lpBuffer + dwCurrentPos ), szBuffer + 
					sizeof( TS_COM_PROPS ), lptsComProps->len );
			dwCurrentPos += lptsComProps->len;
		}
		else 
			isComplete = FALSE;
	} while( lptsComProps->endPacket != '\0' );

	return  0;
}


/////////////////////////////////////////////////////////////////////////////////
// Fucntion : DWORD tsUpdateStoredProc()
// Purpose :
//		Update a stored procedure
// Parameters :
//		hConnect			[IN]	TreeServer connection handle
//		lpProcName			[IN]	Stored procedure name : UserName.ProcName
//		lpBuffer			[IN]	Pointer of the buffer which contains the 
//									new contents of the stored procedure
//		dwBufLen			[IN]	Length of the lpBuffer
// Returns :
//		0 if succeeds
//		Nonzero, which is a error number, if fails 
//
// Comments :
//		User who wants to perform the operation must have the privilege to wirte 
//		the destination stored procedure
/////////////////////////////////////////////////////////////////////////////////
DWORD tsUpdateStoredProc( HANDLE hConnect, LPCSTR lpProcName, LPSTR	lpBuffer,
						  DWORD dwBufLen )
{
	return tsComposeStoredProc( hConnect, lpProcName, lpBuffer, dwBufLen, 
								cmTS_UPDATE_PROC );
}


/////////////////////////////////////////////////////////////////////////////////
// Function : DWORD tsExecuteStoredProc()
// Purpose :
//		Execute a stored procedure
// Parameters :
//		hConnect			[IN]	TreeServer connection handle
//		lpProcName			[IN]	Stored procedure name : UserName.ProcName
// Returns :
//		0 if succeeds
//		Nonzero, which is a error number, if fails 
//
// Comments :
//		User who wants to perform the operation must have the privilege to 
//		execute the destination stored procedure
/////////////////////////////////////////////////////////////////////////////////
/*------------------------------------------------------------------------------*\
 | Function DWORD tsHistDataCollect( HANDLE, LPCSTR, LPSysVarOFunTyper, DWORD,	|
 |									 LPSTR, DWORD, LPVOID, LPDWORD )			|
 | Purpose :																	|
 |		Perform 'in-memory' ASQL scripts with parameters						|
 | Returns :																	|
 |		1 : intact outcome.														|
 |		2 : incomplete outcome.													|
 |		3 : fails. Use GetLastError() to get the error number.					|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
DWORD tsExecuteStoredProc( HANDLE hConnect, LPCSTR lpProcName,
						 LPSysVarOFunType lpXexpVar, DWORD dwXexpVarNum, 
						 LPSTR szInstResult, DWORD dwInstResultSize, 
						 LPVOID lpRsutMem, LPDWORD lpdwLen )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize;
	DWORD			dwCurrentPos = 0, dwXexpCurrPos = 0;
	BOOL			isComplete = TRUE;
	DWORD			dwScriptLen;
	DWORD			i;
	DWORD			dwRetCode;
	
	for( i = 0; i < dwXexpVarNum; i++ )
	{
		_ASSERT( lpXexpVar[i].type == 1051 || lpXexpVar[i].type == 1052 || 
			lpXexpVar[i].type == 1053 || lpXexpVar[i].type == 1054 || 
			lpXexpVar[i].type == 1055 || lpXexpVar[i].type == 1056 );

		_ASSERT( lpXexpVar[i].length <= 31 );
		_ASSERT( lpXexpVar[i].length > 0 );
	}

	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	*( LPDWORD )szBuffer = dwXexpVarNum;
	strcpy( szBuffer + sizeof( DWORD ), lpProcName );
	dwScriptLen = sizeof( DWORD ) + strlen( lpProcName ) + 1;
	if( tsClientWrite( hConnect, szBuffer, dwScriptLen, 'P', 'P', cmTS_EXECUTE_PROC, 
					   '\0', '\0' ) == FALSE )
		return 3;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return dwRetCode;

	if( lptsComProps->lp != 0 ) 
	{ //proc not ready
		return  1;
	}

	if( tsClientWrite( hConnect, ( LPCTSTR )lpXexpVar, sizeof( SysVarOFunType ) * 
			dwXexpVarNum, 'P', 'P', cmTS_EXECUTE_PROC, '\0', '\0' ) == FALSE )
		return 3;	

	lptsComProps = ( LPTS_COM_PROPS )szBuffer;

	while( 1 )
	{
		dwBufSize = PIPE_BUFFER_SIZE;
		if( CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize ) != TERR_SUCCESS ) 
			return 3;

		// The lptsComProps->msgType variable indicates the packet's type. A value 
		// of 'D' indicates this is a normal responsive packet. A value of 'E' 
		// indicates this is a error packet. A value of 'X' indicates this is a 
		// instant outcome pactet. Otherwise a unknown error must occur.
		switch( lptsComProps->msgType )
		{
		case 'D' :
			if( ( DWORD )lptsComProps->len > dwInstResultSize )
			{
				SetLastError( TERR_UNKNOWN_ERROR );
				return 3;
			}

			memcpy( szInstResult, szBuffer + sizeof( TS_COM_PROPS ), lptsComProps->len );
			*lpdwLen = dwCurrentPos;

			if( isComplete == TRUE )
				return 1;
			else 
				return 2;

		case 'X' :
/*			if( ( dwCurrentPos + lptsComProps->len ) <= *lpdwLen )
			{
				memcpy( ( char * )lpRsutMem + dwCurrentPos, szBuffer + 
						sizeof( TS_COM_PROPS ), lptsComProps->len );
				dwCurrentPos += lptsComProps->len;
			}
			else 
				isComplete = FALSE;
			break;
*/
		case 'W' :
			if( ( dwCurrentPos + lptsComProps->len ) <= *lpdwLen )
			{
				memcpy( ( char * )lpRsutMem + dwCurrentPos, szBuffer + 
						sizeof( TS_COM_PROPS ), lptsComProps->len );
				dwCurrentPos += lptsComProps->len;
			}
			else 
				isComplete = FALSE;
			break;

		case 'V' :
			if( ( dwXexpCurrPos + lptsComProps->len ) <= ( sizeof( SysVarOFunType ) * 
				dwXexpVarNum ) )
			{
				memcpy( ( char * )lpXexpVar + dwXexpCurrPos, szBuffer + 
						sizeof( TS_COM_PROPS ),	lptsComProps->len );
				dwXexpCurrPos += lptsComProps->len;
			}
			else
				isComplete = FALSE;
			break;

		case 'E' :
			SetLastError( lptsComProps->lp );
			return FALSE;

		default :
			// Unknown packet type
			_ASSERT( FALSE );
		}
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////
// Function : DWORD tsGrantUserPrivilege()
// Purpose :
//		Grant user privilege(s)
// Parameters :
//		hConnect
//		lpProcName			[IN]	Stored procedure name : UserName.ProcName
//		cPrivilege			[IN]	The privilege mask which can be combine 
//									of PRIVILEGE_EXECUTE, PRIVILEGE and 
//									PRIVILEGE_WRITE
// Returns :
//		0 if succeeds
//		Nonzero, which is a error number, if fails
// Comments :
//		User who wants to perform the operation must have the privilege to
//		write the destination stored procedure
/////////////////////////////////////////////////////////////////////////////////
DWORD tsGrantUserPrivilege( HANDLE hConnect, LPCSTR	lpProcName, LPCSTR szUserName, char cPrivilege )
{
	return tsUserPrivilege( hConnect, lpProcName, szUserName, cPrivilege, cmTS_PROC_GRANT );
}


/////////////////////////////////////////////////////////////////////////////////
// Function : DWORD tsRevokeUserPrivilege()
// Purpose :
//		Revoke the specified privilege(s) of a user
// Parameters :
//		hConnect,			[IN]	TreeServer connection handle
//		lpProcName			[IN]	Stored procedure name : UserName.ProcName
//		cPrivilege			[IN]	The privilege mask which can be combine 
//									of PRIVILEGE_EXECUTE, PRIVILEGE and 
//									PRIVILEGE_WRITE
// Returns :
//		0 if succeeds
//		Nonzero, which is a error number, if fails 
//
// Comments :
//		User who wants to perform the operation must have the privilege to 
//		write the destination stored procedure
/////////////////////////////////////////////////////////////////////////////////
DWORD tsRevokeUserPrivilege( HANDLE	hConnect, LPCSTR lpProcName, LPCSTR szUserName, char cPrivilege )
{
	return tsUserPrivilege( hConnect, lpProcName, szUserName, cPrivilege, cmTS_PROC_REVOKE );
}


/////////////////////////////////////////////////////////////////////////////////
// Function : DWORD tsComposeStoredProc()
// Purpose :
//		Compose a stored procedure. This function is used by tsCreateStoredProc()
//		and tsUpdateStoredProc()
// Parameters :
//		hConnect			[IN]	TreeServer connection handle
//		lpProcName			[IN]	Stored procedure name : UserName.ProcName
//		lpBuffer			[IN]	Pointer of the buffer which contains the 
//									new contents of the stored procedure
//		dwBufLen			[IN]	Length of the lpBuffer
//		lp					[IN]	cmTS_NEW_PROC or cmTS_UPDATE_PROC
// Returns :
//		0 if succeeds
//		Nonzero, which is a error number, if fails 
//
// Comments :
/////////////////////////////////////////////////////////////////////////////////
static DWORD tsComposeStoredProc( HANDLE hConnect, LPCSTR lpProcName, 
								  LPSTR lpBuffer, DWORD dwBufLen, long lp )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode, i;
	LPSTR			lpParam;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE && lpProcName != NULL );

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'P';
	lptsComProps->msgType = 'P';
	lptsComProps->leftPacket = '\1';
	lptsComProps->endPacket = '\1';
	lptsComProps->lp = lp;
	lptsComProps->len = sizeof( DWORD ) + lstrlen( lpProcName ) + 1;	

	lpParam = szBuffer + sizeof( TS_COM_PROPS );

	*( LPDWORD )lpParam = dwBufLen;
	lstrcpyn( lpParam + sizeof( DWORD ), lpProcName, MAX_PROC_NAME );
	
	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return dwRetCode;

	for( i = 0; i < dwBufLen; i += TREESRV_CONTENT_LEN )
	{
		lptsComProps->packetType = 'P';
		lptsComProps->msgType = 'P';
		lptsComProps->lp = lp;
	
		if( ( i + TREESRV_CONTENT_LEN ) < dwBufLen )
		{
			lptsComProps->len = TREESRV_CONTENT_LEN;
			lptsComProps->leftPacket = '\1';
			lptsComProps->endPacket = '\1';
		}
		else
		{
			lptsComProps->len = ( short )( dwBufLen - i );
			lptsComProps->leftPacket = '\0';
			lptsComProps->endPacket = '\0';
		}
		
		memcpy( szBuffer + sizeof( TS_COM_PROPS ), lpBuffer + i, 
				lptsComProps->len );

		dwBufSize = PIPE_BUFFER_SIZE;
		dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
		if( dwRetCode != TERR_SUCCESS ) 
			return dwRetCode;
	}

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return dwRetCode;

	if( lptsComProps->lp == 0 )
		return 0;
	else
	{
		SetLastError( lptsComProps->lp );
		return lptsComProps->lp;
	}
}


/////////////////////////////////////////////////////////////////////////////////
// Function : DWORD tsUserPrivilege()
// Purpose :
//		Grant or Revoke specified privilege(s) of a user
// Parameters :
//		hConnect,			[IN]	TreeServer connection handle
//		lpProcName			[IN]	Stored procedure name : UserName.ProcName
//		cPrivilege			[IN]	The privilege mask which can be combine 
//									of PRIVILEGE_EXECUTE, PRIVILEGE and 
//									PRIVILEGE_WRITE
//		lp					[IN]	cmTS_PROC_GRANT or cmTS_PROC_REVOKE
// Returns :
//		0 if succeeds
//		Nonzero, which is a error number, if fails 
//
// Comments :
//		User who wants to perform the operation must have the privilege to 
//		write the destination stored procedure
/////////////////////////////////////////////////////////////////////////////////
static DWORD tsUserPrivilege( HANDLE hConnect, LPCSTR lpProcName, LPCSTR szUserName,
							  char cPrivilege, long lp )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE && lpProcName != NULL );

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'P';
	lptsComProps->msgType = 'P';
	lptsComProps->lp = lp;
	lptsComProps->len = lstrlen( lpProcName ) + 1 + sizeof( char );	

	lstrcpyn( szBuffer + sizeof( TS_COM_PROPS ), lpProcName, EXCHANGE_CONTENT_SIZE-64 );
	*( szBuffer + sizeof( TS_COM_PROPS ) + lstrlen( lpProcName ) + 1 ) = cPrivilege;
	lstrcpyn( szBuffer + sizeof( TS_COM_PROPS ) + lstrlen( lpProcName ) + 2, szUserName, 32 );


	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return dwRetCode;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return dwRetCode;

	if( lptsComProps->lp == 0 )
		return 0;
	else
	{
		SetLastError( lptsComProps->lp );
		return lptsComProps->lp;
	}
}

/////////////////////////////////////////////////////////////////////////////////
// Function : DWORD tsPrivateEngCall()
// Purpose :
//		Call a private engine
// Parameters :
//		hConnect,			[IN]	TreeServer connection handle
//		lpCommand			[IN]	specify the engine to be called
//		lpSwapBuf			[IN]	Swap buffer
//		lpdwSwapBufLen		[IN]	Length of swap buffer
//							[OUT]	Length of the result set
// Returns :
//		0 if succeeds
//		Nonzero, which is a error number, if fails 
//
// Comments :
/////////////////////////////////////////////////////////////////////////////////
DWORD tsPrivateEngCall( HANDLE hConnect, LPCSTR lpCommand, LPSTR lpSwapBuf, 
					    LPDWORD lpdwSwapBufLen )
{

	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode, i, dwCurrentPos = 0;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE && lpCommand != NULL && 
			 lpSwapBuf != NULL );

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'A';
	lptsComProps->leftPacket = '\0';
	lptsComProps->endPacket = '\0';
	lptsComProps->lp = *lpdwSwapBufLen;


	lptsComProps->len = lstrlen(
		lstrcpyn(szBuffer + sizeof( TS_COM_PROPS ), lpCommand, EXCHANGE_CONTENT_SIZE ) );
	
	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return dwRetCode;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return dwRetCode;

	if( lptsComProps->lp != 0 ) 
	{ //engine not ready
		return  1;
	}


	//send request
	for( i = 0;   i < *lpdwSwapBufLen;   i += TREESRV_CONTENT_LEN )
	{
		ZeroMemory( szBuffer, sizeof( TS_COM_PROPS ) );
		lptsComProps->packetType = 'A';
	
		if( ( i + TREESRV_CONTENT_LEN ) < *lpdwSwapBufLen )
		{
			lptsComProps->len = TREESRV_CONTENT_LEN;
			lptsComProps->leftPacket = '\1';
			lptsComProps->endPacket = '\1';
		}
		else
		{
			lptsComProps->len = ( short )( *lpdwSwapBufLen - i );
			lptsComProps->leftPacket = '\0';
			lptsComProps->endPacket = '\0';
		}
		
		memcpy( szBuffer + sizeof( TS_COM_PROPS ), lpSwapBuf + i, 
				lptsComProps->len );

		dwBufSize = PIPE_BUFFER_SIZE;
		dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
		if( dwRetCode != TERR_SUCCESS ) 
			return dwRetCode;
	}

	//get the result
	do
	{
		dwBufSize = PIPE_BUFFER_SIZE;
		dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
		if( dwRetCode != TERR_SUCCESS ) 
			return dwRetCode;

		if( lptsComProps->lp != 0 )
		{
			SetLastError( lptsComProps->lp );
			return lptsComProps->lp;
		}

		if( ( dwCurrentPos + lptsComProps->len ) <= *lpdwSwapBufLen )
		{
			CopyMemory( ( PVOID )( lpSwapBuf + dwCurrentPos ), szBuffer + 
					sizeof( TS_COM_PROPS ), lptsComProps->len );
			dwCurrentPos += lptsComProps->len;
		}
		else 
		{
			CopyMemory( ( PVOID )( lpSwapBuf + dwCurrentPos ), szBuffer + 
					sizeof( TS_COM_PROPS ), *lpdwSwapBufLen - dwCurrentPos );
			
			dwCurrentPos = *lpdwSwapBufLen;
			break;
		}
	} while( lptsComProps->endPacket != '\0' );

	*lpdwSwapBufLen = dwCurrentPos;

	return  0;

}



/*------------------------------------------------------------------------------*\
 | Function tsGetSprocList ()													|
 | Purpose :																	|
 |		Get sproc's list   														|
 | Returns:																		|
 |		0 if succeeds.															|
 |		otherwise fails.														|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
DWORD tsGetSprocList ( HANDLE hConnect, LPSTR szListBuffer, DWORD dwBuffer )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;
	DWORD			i;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE );

	ZeroMemory( szBuffer, sizeof( TS_COM_PROPS ) );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = pkTS_SPROC;
	lptsComProps->msgType = msgTS_SPROC;
	lptsComProps->lp = cmTS_PROC_ENUM;
	lptsComProps->len = 0;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return -1;

	i = 0;
	while( 1 ) {
		dwBufSize = PIPE_BUFFER_SIZE;
		dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
		if( dwRetCode != TERR_SUCCESS ) 
			return -1;

		if( i + lptsComProps->len < dwBuffer ) {
			CopyMemory(szListBuffer+i, szBuffer+sizeof( TS_COM_PROPS ), lptsComProps->len);
			//szListBuffer[lptsComProps->len] = '\0';
			i += lptsComProps->len;
		} //else
		  //return  i;

		if( lptsComProps->endPacket == '\0' || 
									lptsComProps->leftPacket == '\0' )
			break;
	} //while

	return i;
}




/*------------------------------------------------------------------------------*\
 | Function tsGetDictInfo( HANDLE, LPCSTR, LPCSTR, LPCSTR, LPSTR )				|
 | Purpose :																	|
 |		get a dictionary content												|
 | Returns:																		|
 |		content if succeeds.													|
 |		0 if fails. Use GetLastError() to get the error number.					|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
DWORD tsGetDictInfo( HANDLE hConnect, LPCSTR szGroup, 
				   LPCSTR szLabel, LPCSTR szKey, LPSTR *szCont, int dwContSize )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( ( hConnect != INVALID_HANDLE_VALUE ) );

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'Q';
	lptsComProps->msgType = 'Q';
	lptsComProps->lp = cmTS_GET_DICT_INFO;
	
	// If it succeeds, wsprintf() returns the number of characters stored in the 
	// output buffer, not counting the terminating null character. So we should 
	// add 1 to the return value to get the correct length of the string.
	lptsComProps->len = wsprintf( szBuffer + sizeof( TS_COM_PROPS), 
								  "%s`%s`%s", szGroup, szLabel, szKey ) + 1;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return 0;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return 0;

	// The lptsComProps->msgType variable indicates the packet's type. A value 
	// of 'D' indicates this is a normal responsive packet. A value of 'E' 
	// indicates this is a error packet. Otherwise a unknown error must occur.
	if( lptsComProps->msgType == 'D' )
	{
		memcpy( szCont, szBuffer + sizeof( TS_COM_PROPS ), 
											min(lptsComProps->len, dwContSize) );
		return lptsComProps->len;
	}
	else if( lptsComProps->msgType == 'E' )
	{
		SetLastError( lptsComProps->lp );
		return 0;
	}

	// Unknown packet type
	_ASSERT( FALSE );
	return  0;
}


/*------------------------------------------------------------------------------*\
 | Function tsPutDictInfo( HANDLE, LPCSTR, LPCSTR, LPCSTR, LPCSTR )				|
 | Purpose :																	|
 |		put a dictionary content												|
 | Returns:																		|
 |		content if succeeds.													|
 |		0 if fails. Use GetLastError() to get the error number.					|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
DWORD tsPutDictInfo( HANDLE hConnect, LPCSTR szGroup, 
							LPCSTR szLabel, LPCSTR szKey, LPCSTR *szCont )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;

	_ASSERT( ( hConnect != INVALID_HANDLE_VALUE ) );

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = 'Q';
	lptsComProps->msgType = 'Q';
	lptsComProps->lp = cmTS_PUT_DICT_INFO;
	
	// If it succeeds, wsprintf() returns the number of characters stored in the 
	// output buffer, not counting the terminating null character. So we should 
	// add 1 to the return value to get the correct length of the string.
	lptsComProps->len = wsprintf( szBuffer + sizeof( TS_COM_PROPS), 
								  "%s`%s`%s`%s", szGroup, szLabel, szKey, szCont ) + 1;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return 0;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return 0;

	// The lptsComProps->msgType variable indicates the packet's type. A value 
	// of 'D' indicates this is a normal responsive packet. A value of 'E' 
	// indicates this is a error packet. Otherwise a unknown error must occur.
	if( lptsComProps->msgType == 'D' )
	{
		// The lptsComProps->lp contains the table id.
		return lptsComProps->lp;
	}
	else if( lptsComProps->msgType == 'E' )
	{
		SetLastError( lptsComProps->lp );
		return 0;
	}

	// Unknown packet type
	_ASSERT( FALSE );
	return  0;
}





/*------------------------------------------------------------------------------*\
 | Function tsGeneralFunCall()													|
 | Purpose :																	|
 |		geteral function call													|
 | Returns:																		|
 |		0 if succeeds.															|
 |		otherwise fails.														|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
DWORD tsGeneralFunCall ( HANDLE hConnect, char packetType, 
										  char msgType,
										  long lp,
										  LPSTR szListBuffer, DWORD dwBuffer )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwBufSize, dwRetCode;
	DWORD			i;

	_ASSERT( hConnect != INVALID_HANDLE_VALUE );

	ZeroMemory( szBuffer, sizeof( TS_COM_PROPS ) );
	ZeroMemory( szListBuffer, dwBuffer );

	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = packetType;
	lptsComProps->msgType = msgType;
	lptsComProps->lp = lp;
	lptsComProps->len = 0;

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) 
		return -1;

	i = 0;
	while( 1 ) {
		dwBufSize = PIPE_BUFFER_SIZE;
		dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
		if( dwRetCode != TERR_SUCCESS ) 
			return -1;

		if( i + lptsComProps->len < dwBuffer ) {
			CopyMemory(szListBuffer+i, szBuffer+sizeof( TS_COM_PROPS ), lptsComProps->len);
			//szListBuffer[lptsComProps->len] = '\0';
			i += lptsComProps->len;
		} //else
		  //return  i;

		if( lptsComProps->endPacket == '\0' || 
									lptsComProps->leftPacket == '\0' )
			break;
	} //while

	return i;
}


/*------------------------------------------------------------------------------*\
 | Function tsFtpPut()															|
 | Purpose :																	|
 |		ftp																		|
 | Returns:																		|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
DWORD tsFtpPut( HANDLE hConnect, LPSTR lpLocalFile, LPSTR lpRemoteFile )
{
	CHAR 	szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS lptsComProps;
	BOOL 	fEof = FALSE;
	BOOL 	fRDWR_OK = TRUE;
	DWORD 	dwRetCode;
	DWORD	dwBufSize;
	HANDLE hFile;

	if( hConnect == INVALID_HANDLE_VALUE || lpLocalFile == NULL ||
				lpRemoteFile == NULL || lpLocalFile[0] == '\x0' ||
				lpRemoteFile[0] == '\x0' )
		return ERROR_INVALID_PARAMETER;

	if( lpLocalFile == NULL || lpRemoteFile == NULL ) {
		return ERROR_INVALID_PARAMETER;
	}

	hFile = CreateFile( 
			lpLocalFile,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL );
	if( hFile == INVALID_HANDLE_VALUE ) {
		return GetLastError();
	}

	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	//tell them my file information
    ///////////////////////////////////////////////////////////////////////
	lptsComProps->packetType = pkTS_PIPE_FTP;
	lptsComProps->msgType = msgTS_FTP_FILE;
	lptsComProps->lp = cmFTP_FILE_PUT;
	lptsComProps->leftPacket = '\x0';
	lptsComProps->endPacket = '\x0';
	lptsComProps->len = wsprintf( (LPSTR)(szBuffer+sizeof(TS_COM_PROPS)),
			"%s\x0", lpRemoteFile );

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) {
		return GetLastError();
	}

	//are you ready?
	///////////////////////////////////////////////////////////////////////
	dwBufSize = PIPE_BUFFER_SIZE;
	if( CliReadTSNamedPipe(hConnect, szBuffer, &dwBufSize) != TERR_SUCCESS ) {
		CloseHandle( hFile );
		return GetLastError();
	}

	if( lptsComProps->packetType == pkTS_ERROR ) {
		CloseHandle( hFile );
		return 0xFFFFFFFF;
	}

	if( lptsComProps->lp != cmFTP_FILE_OK ) {
		CloseHandle( hFile );
		return (DWORD)lptsComProps->lp;
	}

	/////////////////////////////////////////////////////////////
	// by Jingyu Niu 1999.12.27
	//
	// FileCompressToSend(lpLocalFile, hConnect);
	/////////////////////////////////////////////////////////////
	SetFilePointer( hFile, 0, NULL, FILE_BEGIN );
	lptsComProps->packetType = pkTS_PIPE_FTP;
	lptsComProps->msgType = msgTS_FTP_FILE;
	lptsComProps->lp = cmFTP_FILE_DATA;
	lptsComProps->leftPacket = '\x1';
	lptsComProps->endPacket = '\x1';

	while( !fEof ) {
		DWORD dwcbRDWR;
		BOOL fSuccess;
		DWORD dwcbBuffer = PIPE_BUFFER_SIZE - sizeof(TS_COM_PROPS);
		fSuccess = ReadFile ( hFile,
				(LPVOID)(szBuffer+sizeof(TS_COM_PROPS)),
				dwcbBuffer,
				&dwcbRDWR,
				NULL );
		if ( !fSuccess ) {
			dwRetCode = GetLastError();
			if( dwRetCode == ERROR_HANDLE_EOF || dwRetCode == 0 ) {
				if( dwcbRDWR == 0 )
					break;
				else
					fEof = TRUE;
			}
			else {
				fRDWR_OK = FALSE;
				break;
			}
		}
		else {
			if( dwcbRDWR < dwcbBuffer )
				fEof = TRUE;
		}

		lptsComProps->len = (short)dwcbRDWR;
		dwBufSize = 4096;
		dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
		if( dwRetCode != TERR_SUCCESS ) {
			fRDWR_OK = FALSE;
			break;
		}
	}

	CloseHandle( hFile );

	ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
	lptsComProps->packetType = pkTS_PIPE_FTP;
	lptsComProps->msgType = msgTS_FTP_FILE;

	if( fRDWR_OK )
		lptsComProps->lp = cmFTP_FILE_END;
	else {
		lptsComProps->lp = cmFTP_FILE_ERROR;
		CopyMemory( (LPVOID)(szBuffer+sizeof(TS_COM_PROPS)), (LPVOID)&dwRetCode, sizeof(DWORD) );
	}

	dwBufSize = 4096;
	CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );

	//get an answer
	///////////////////////////////////////////////////////////////////////
	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS )
		return GetLastError();

	return dwRetCode;

} //tsFtpPut()


/*------------------------------------------------------------------------------*\
 | Function tsFtpGet()															|
 | Purpose :																	|
 |		ftp																		|
 | Returns:																		|
 |																				|
 | Comments:																	|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
DWORD tsFtpGet( HANDLE hConnect,
						LPSTR lpRemoteFile, LPSTR lpLocalFile )
{
	HANDLE hLocalFile;
	CHAR 	szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS lptsComProps;
	BOOL 	fEof = FALSE;
	BOOL 	fRDWR_OK = TRUE;
	DWORD 	dwRetCode = 0;
	DWORD	dwBufSize;
	DWORD cbWrite;
	int	i;

	if( hConnect == INVALID_HANDLE_VALUE || lpLocalFile == NULL ||
				lpRemoteFile == NULL || lpLocalFile[0] == '\x0' ||
				lpRemoteFile[0] == '\x0' )
		return ERROR_INVALID_PARAMETER;

	if( (hLocalFile = CreateFile( lpLocalFile,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL )) == INVALID_HANDLE_VALUE )
		return GetLastError();

	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	//tell them my file information
	///////////////////////////////////////////////////////////////////////
	lptsComProps->packetType = pkTS_PIPE_FTP;
	lptsComProps->msgType = msgTS_FTP_FILE;
	lptsComProps->lp = cmFTP_FILE_GET;
	lptsComProps->leftPacket = '\x0';
	lptsComProps->endPacket = '\x0';
	lptsComProps->len = wsprintf( (LPSTR)(szBuffer+sizeof(TS_COM_PROPS)),
			"%s\x0", lpRemoteFile );

	dwBufSize = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hConnect, szBuffer, &dwBufSize );
	if( dwRetCode != TERR_SUCCESS ) {
		CloseHandle( hLocalFile );
		return GetLastError();
	}

	//are you ready?
	///////////////////////////////////////////////////////////////////////
	dwBufSize = PIPE_BUFFER_SIZE;
	if( CliReadTSNamedPipe(hConnect, szBuffer, &dwBufSize) != TERR_SUCCESS ) {
		CloseHandle( hLocalFile );
		return GetLastError();
	}

	if( lptsComProps->packetType == pkTS_ERROR ) {
		CloseHandle( hLocalFile );
		return 0xFFFFFFFF;
	}

	if( lptsComProps->lp != cmFTP_FILE_OK ) {
		CloseHandle( hLocalFile );
		return (DWORD)lptsComProps->lp;
	}

	/////////////////////////////////////////////////////////////
	// by Jingyu Niu 1999.12.27
	/*
	if( (i = FileDecompressFromGet(lpLocalFile, hConnect)) < 0 )
		return GetLastError();
	*/

	while( 1 ) {
		dwBufSize = PIPE_BUFFER_SIZE;
		if( CliReadTSNamedPipe(hConnect, szBuffer, &dwBufSize) != TERR_SUCCESS ) {
			return GetLastError();
		}

		if( lptsComProps->lp == cmFTP_FILE_END )
			break;

		if( lptsComProps->lp == cmFTP_FILE_ERROR ) {
			dwRetCode = *(LPDWORD)(szBuffer+sizeof(TS_COM_PROPS));
			break;
		}

		if( lptsComProps->lp == cmFTP_FILE_DATA )
			WriteFile( hLocalFile,
					szBuffer+sizeof(TS_COM_PROPS),
					lptsComProps->len,
					&cbWrite,
					NULL );
	}

	CloseHandle( hLocalFile );

	return dwRetCode;
} //end of tsFtpGet()



/*------------------------------------------------------------------------------*\
 | Function tsSetTcpipPort()													|
 | Purpose :																	|
 |		set TCP/IP port															| 
 | Returns:																		|
 |																				|
 | Comments:																	|
 |    port or servicename is import												|
\*------------------------------------------------------------------------------*/
//__declspec( dllexport )
int tsSetTcpipPort( char *szPortOrService )
{
	return  cliSetTcpipPort( szPortOrService );
}




//////////////////////////////////end of this file ///////////////////////////////////////////