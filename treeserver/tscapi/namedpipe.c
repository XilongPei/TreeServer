/******************************
*   Module Name:
*       NamedPipe.C
*
*   Abstract:
*   	This module contains Tree Server client agent function.
*     It connect to the tree server's service named-pipe, and
*     Process the user request.
*
*   Author:
*   	Niu Jingyu.
*
*   Copyright (c) 1997  China Railway Software Corporation
*
*   Revision History:
*   	Write by Niu Jingyu, 1997.11        一次连接，单次交互
*		Rewrite by Niu Jingyu, 1998.02      一次连接，多次交互
*		Standalone support, 1998/11			Ma Weida
*******************************************************************************/
#include <windows.h>
#include <tchar.h>
#include <crtdbg.h>

//#define _NAMEDPIPE_MAIN_

#include "namedpipe.h"
#include "ts_com.h"
#include "terror.h"
#include "tlimits.h"
#include "tree_tcp.h"


char  szTcpipPort[32] = "";

#ifdef TREESVR_STANDALONE

typedef struct
{
	HANDLE	hClientReadPipe;
	HANDLE	hClientWritePipe;
	HANDLE  hServerReadPipe;
	HANDLE  hServerWritePipe;
} PIPE_STRU;

static void ClosePipe( PIPE_STRU *PipeStru );

void ClosePipe( PIPE_STRU *PipeStru )
{
	if( PipeStru->hClientReadPipe != NULL )
		CloseHandle( PipeStru->hClientReadPipe );

	if( PipeStru->hClientWritePipe != NULL )
		CloseHandle( PipeStru->hClientWritePipe );

	if( PipeStru->hServerReadPipe != NULL )
		CloseHandle( PipeStru->hServerReadPipe );

	if( PipeStru->hServerWritePipe != NULL )
		CloseHandle( PipeStru->hServerWritePipe );
}

/////////////////
// CliOpenTSNamedPipe()
/////////////////////////////////////////////////////////////////////////////
__declspec( dllexport )HANDLE CliOpenTSNamedPipe ( LPCSTR lpServer )
{
	BOOL		bSuccess = TRUE;
	PIPE_STRU	*PipeStru = NULL;
	LPTSTR		lpMapAddress = NULL;
	DWORD		dwServerProcessId;
	HANDLE		hServerProcess = NULL;
	HANDLE		hClientReadPipe = NULL, hClientWritePipe = NULL,
				hServerReadPipe = NULL, hServerWritePipe = NULL,
				hDupServerReadPipe, hDupServerWritePipe;
	HANDLE		hMapFile = NULL, hConnectionMutex = NULL, 
				hConnectionReady = NULL, hConnectionCompleted = NULL;

	__try
	{
		// Open file mapping for connection buffer, which is used for 
		// exchanging connection information between client and server
		// sides. The file mapping object is created by server side.
		hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS,
			FALSE,
			"ConnectionMappingObject" );
		if( hMapFile == NULL )
		{
			bSuccess = FALSE;
			__leave;
		}

		// This makes the specified portion of the file ( In this case,
		// our file-mapping object is backed by the operating-system paging 
		// file ) visible in the address space of the calling process. 
		lpMapAddress = MapViewOfFile( hMapFile,
			FILE_MAP_ALL_ACCESS,
			0, 
			0,
			0 );
		if( lpMapAddress == NULL )
		{
			bSuccess = FALSE;
			__leave;
		}

		// Open hConnectionMutex event which is created by server side.
		// This object is used for mutual-exclusion access to establishing 
		// connection.
		hConnectionMutex = OpenMutex( MUTEX_ALL_ACCESS, FALSE, "CONNECTION_MUTEX" );
		if( hConnectionMutex == NULL )
		{
			bSuccess = FALSE;
			__leave;
		}

		// Open hConnectionReady event object which is created by server side.
		// This object is used for indicating whether the server can begin to 
		// fetch the connection information from client side.
		hConnectionReady = OpenEvent( EVENT_ALL_ACCESS, FALSE, "CONNECTION_READY" );
		if( hConnectionReady == NULL )
		{
			bSuccess = FALSE;
			__leave;
		}

		// Open hConnectionCompleted event object which is created by server side.
		// This object is used for indicating whether the server has finished 
		// work for establishing connection.
		hConnectionCompleted = OpenEvent( EVENT_ALL_ACCESS, FALSE, "CONNECTION_COMPLETED" );
		if( hConnectionCompleted == NULL )
		{
			bSuccess = FALSE;
			__leave;
		}

		// Allocte memory for storing pipe handles. The memory will be freed 
		// when Client logoff.
		PipeStru = ( PIPE_STRU * )malloc( sizeof( PIPE_STRU ) );
		if( PipeStru == NULL )
		{
			bSuccess = FALSE;
			_leave;
		}

		// Fill the PipeStru with zero.
		ZeroMemory( PipeStru, sizeof( PIPE_STRU ) );

		// Enter into critical section.
		if( WaitForSingleObject( hConnectionMutex, INFINITE ) != WAIT_OBJECT_0 )
		{
			bSuccess = FALSE;
			__leave;
		}

		// Fetch the server process id from the shared memory.
		dwServerProcessId = *( PDWORD )lpMapAddress;
		
		// Fetch the handle of the server process by its id.
		hServerProcess = OpenProcess( PROCESS_DUP_HANDLE, 
			FALSE,
			dwServerProcessId );
		if( hServerProcess == NULL )
		{
			bSuccess = FALSE;
			__leave;
		}

		// Create anonymous pipe used for server-to-client data transfer.
		if( CreatePipe( &hClientReadPipe, &hServerWritePipe, NULL, 0 ) == 0 )
		{
			bSuccess = FALSE;
			__leave;
		}

		// Create anonymous pipe used for client-to-server data transfer.
		if( CreatePipe( &hServerReadPipe, &hClientWritePipe, NULL, 0 ) == 0 )
		{
			bSuccess = FALSE;
			__leave;
		}

		// Get the the duplicated handle of hServerReadPipe.
		if( DuplicateHandle( GetCurrentProcess(),
				hServerReadPipe,
				hServerProcess,
				&hDupServerReadPipe,
				0,
				FALSE,
				DUPLICATE_SAME_ACCESS ) == 0 ) 
		{
			bSuccess = FALSE;
			__leave;
		}

		// Get the the duplicated handle of hServerWritePipe.
		if( DuplicateHandle( GetCurrentProcess(),
				hServerWritePipe,
				hServerProcess,
				&hDupServerWritePipe,
				0,
				FALSE,
				DUPLICATE_SAME_ACCESS ) == 0 ) 
		{
			bSuccess = FALSE;
			__leave;
		}

		// Store all the pipe handles into PipeStru.
		PipeStru->hClientReadPipe  = hClientReadPipe;
		PipeStru->hClientWritePipe = hClientWritePipe;
		PipeStru->hServerReadPipe  = hServerReadPipe;
		PipeStru->hServerWritePipe = hServerWritePipe;

		// Store hDupServerReadPipe and hDupServerWritePipe into shared memory.
		*( PHANDLE )( lpMapAddress + sizeof( DWORD ) ) = hDupServerReadPipe;
		*( PHANDLE )( lpMapAddress + sizeof( DWORD ) + sizeof( HANDLE ) ) = hDupServerWritePipe;

		// Set the state of hConnectionReady to signaled
		if( SetEvent( hConnectionReady ) == FALSE )
		{
			bSuccess = FALSE;
			__leave;
		}
		
		// Wait for server side completing the process.
		if( WaitForSingleObject( hConnectionCompleted, INFINITE ) != WAIT_OBJECT_0 )
		{
			bSuccess = FALSE;
			__leave;
		}

		// Set the state of hConnectionCompleted to nonsignaled.
		if( ResetEvent( hConnectionCompleted ) == FALSE )
		{
			bSuccess = FALSE;
			__leave;
		}

		// Leave the critical section.
		if( ReleaseMutex( hConnectionMutex ) == FALSE)
		{
			bSuccess = FALSE;
			__leave;
		}
	}
	__finally
	{
		if( hConnectionReady != NULL )
			CloseHandle( hConnectionReady );

		if( hConnectionCompleted != NULL )
			CloseHandle( hConnectionCompleted );

		if( hConnectionMutex != NULL )
			CloseHandle( hConnectionMutex );

		if( lpMapAddress != NULL )
			UnmapViewOfFile( lpMapAddress );

		if( hMapFile != NULL )
			CloseHandle( hMapFile );

		if( bSuccess == TRUE )
			return PipeStru;
		else
		{
			if( PipeStru != NULL ) 
			{
				ClosePipe( PipeStru );
				free( PipeStru );
			}

			return INVALID_HANDLE_VALUE;
		}
	}	
}

#else

/////////////////
// CliOpenTSNamedPipe()
//
// enchance it with tcp/ip
/////////////////////////////////////////////////////////////////////////////
__declspec( dllexport )HANDLE CliOpenTSNamedPipe ( LPCSTR lpServer )
{
	HANDLE 	hPipe;
	TCHAR 	szPipeName[MAX_PATH];
	BOOL 	fSuccess;
	int		i;

	if( szTcpipPort[0] != '\0' )
	{
		//TCP/IP chanel
		return  (HANDLE)tcpipStartup((char *)lpServer, szTcpipPort);
	}

	if( lpServer == NULL ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return INVALID_HANDLE_VALUE;
	}

    wsprintf( szPipeName, _TEXT( "\\\\%s\\pipe\\TS_ConvPipe" ), lpServer );
    fSuccess = WaitNamedPipe( szPipeName, 180000 );
    if( !fSuccess ) {
  		SetLastError( TERR_NAMEDPIPE_TIMEOUT );
		return INVALID_HANDLE_VALUE;
	}

    for( i = 0; i < 5; i++ )
	{
		hPipe = CreateFile ( szPipeName,
		        GENERIC_READ | GENERIC_WRITE,
				0,
				NULL,
				OPEN_EXISTING,
				0,
				NULL );

		if ( hPipe != INVALID_HANDLE_VALUE )
			break;

		Sleep( 200 );
	}

	if( hPipe == INVALID_HANDLE_VALUE )  {
		return INVALID_HANDLE_VALUE;
	}

	SetLastError( TERR_SUCCESS );
	return hPipe;
}

#endif



#ifdef TREESVR_STANDALONE

__declspec( dllexport )BOOL CliCloseTSNamedPipe ( HANDLE hPipe )
{
	// Oops, Can it be NULL?! The silly programmer must be wrong.
	_ASSERT( hPipe != NULL );

	// Close all pipe handles.
	ClosePipe( ( PIPE_STRU * )hPipe );

	// There are inconsistent semantics of the standalone and non-standalone versions.
	// I wanna keep the program simple so here just return TRUE simply.
	return TRUE;
}

#else

/////////////////
// CliCloseTSNamedPipe()
//
// enchance it with tcp/ip
/////////////////////////////////////////////////////////////////////////////
__declspec( dllexport )BOOL CliCloseTSNamedPipe ( HANDLE hPipe )
{
	if( hPipe == INVALID_HANDLE_VALUE ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if( szTcpipPort[0] != '\0' )
	{
		//TCP/IP channel
		closeTcpip( (SOCKET)hPipe );
		return  TRUE;
	}

	return CloseHandle( hPipe );
}

#endif

__declspec( dllexport )DWORD CliTSLogon ( HANDLE hPipe, LPCSTR lpszUser, LPCSTR lpszPasswd, 
					LPCSTR lpszComputer )
{
	CHAR	szBuffer[PIPE_BUFFER_SIZE];
	DWORD	dwcbBuffer;
	LPTS_COM_PROPS lptsComProps;
	DWORD	dwRetCode;

	if( hPipe == INVALID_HANDLE_VALUE || lpszUser == NULL ||
			lpszPasswd == NULL || lpszComputer == NULL ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return 0xFFFFFFFF;
	}

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	lptsComProps->packetType = pkTS_SYNC_PIPE;
	lptsComProps->msgType = msgTS_CLIENT_LOGON;
	
	/*
	lptsComProps->len = wsprintf( (LPSTR)(szBuffer+sizeof(TS_COM_PROPS)),
			"%s\n%s\n%s", lpszUser, lpszPasswd, lpszComputer );
	*/
	lptsComProps->len = 50;
	lstrcpyn( (LPSTR)(szBuffer+sizeof(TS_COM_PROPS)), lpszUser, 16);
	lstrcpyn( (LPSTR)(szBuffer+sizeof(TS_COM_PROPS)+16), lpszPasswd, 16);
	lstrcpyn( (LPSTR)(szBuffer+sizeof(TS_COM_PROPS)+32), lpszComputer, 16);

	dwcbBuffer = PIPE_BUFFER_SIZE;
	dwRetCode = CliWriteTSNamedPipe( hPipe, szBuffer, &dwcbBuffer );
	if( dwRetCode != TERR_SUCCESS )
		return 0xFFFFFFFF;
		
	dwcbBuffer = PIPE_BUFFER_SIZE;
	dwRetCode = CliReadTSNamedPipe( hPipe, szBuffer, &dwcbBuffer );
	if( dwRetCode != TERR_SUCCESS )
		return 0xFFFFFFFF;
	
	if( lptsComProps->msgType != msgTS_LOGON_OK ) {
		SetLastError( lptsComProps->lp );
		return 0xFFFFFFFF;
	}
	
	return lptsComProps->lp;
}

__declspec( dllexport )BOOL CliTSLogoff ( HANDLE hPipe )
{
	TS_COM_PROPS tsComProps;
	DWORD dwBufSize;
	DWORD dwResult;

	if( hPipe == INVALID_HANDLE_VALUE ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	ZeroMemory( (LPVOID)&tsComProps, sizeof(TS_COM_PROPS) );
	tsComProps.packetType = pkTS_SYNC_PIPE;
	tsComProps.msgType = msgTS_PIPE_CLOSE;
	tsComProps.len = 0;

	dwBufSize = sizeof( TS_COM_PROPS );

	dwResult = CliWriteTSNamedPipe ( hPipe, (LPVOID)&tsComProps, &dwBufSize );
	if( dwResult != TERR_SUCCESS )
		return FALSE;
	return TRUE;
}

/////////////////
// CliReadTSNamedPipe()
//
// enchance it with tcp/ip
/////////////////////////////////////////////////////////////////////////////
__declspec( dllexport )DWORD CliReadTSNamedPipe ( HANDLE hPipe, LPVOID lpBuffer, LPDWORD lpdwcbBuffer )
{
	BOOL 	fSuccess;
	DWORD 	dwcbBuffer;
#ifdef TREESVR_STANDALONE
	char	TempBuf[PIPE_BUFFER_SIZE];
#endif

	if( hPipe == INVALID_HANDLE_VALUE || lpBuffer == NULL ||
			lpdwcbBuffer == NULL || *lpdwcbBuffer < PIPE_BUFFER_SIZE )
		return ERROR_INVALID_PARAMETER;

	dwcbBuffer = *lpdwcbBuffer;

#ifdef TREESVR_STANDALONE

	fSuccess = ReadFile ( ( ( PIPE_STRU * )hPipe )->hClientReadPipe,
			TempBuf,
			PIPE_BUFFER_SIZE,
			lpdwcbBuffer,
			NULL );

	CopyMemory( lpBuffer, TempBuf, dwcbBuffer );

#else

	if( szTcpipPort[0] != '\0' )
	{
		/////////////////////////////////
		//TCP/IP chanel
		/////////////////////////////////

		*lpdwcbBuffer = recvPacket((SOCKET)hPipe, lpBuffer, dwcbBuffer);
		if( *lpdwcbBuffer <= 0 )
			return 1;
		else
			return TERR_SUCCESS;
	}

	fSuccess = ReadFile ( hPipe,
			lpBuffer,
			dwcbBuffer,
			lpdwcbBuffer,
			NULL );

#endif

	if ( !fSuccess || *lpdwcbBuffer == 0 )
		return GetLastError();

#ifdef TREESVR_STANDALONE
	FlushFileBuffers ( ( ( PIPE_STRU * )hPipe )->hClientReadPipe );
#else
	FlushFileBuffers ( hPipe );
#endif

	return TERR_SUCCESS;
}



/////////////////
// CliWriteTSNamedPipe()
//
// enchance it with tcp/ip
/////////////////////////////////////////////////////////////////////////////
__declspec( dllexport )DWORD CliWriteTSNamedPipe ( HANDLE hPipe, LPVOID lpBuffer, LPDWORD lpdwcbBuffer )
{
	BOOL fSuccess;
	LPTS_COM_PROPS lptsComProps;
	DWORD dwcbBuffer;

	if( hPipe == INVALID_HANDLE_VALUE || lpBuffer == NULL ||
			lpdwcbBuffer == NULL || *lpdwcbBuffer <= 0 ||
			*lpdwcbBuffer > PIPE_BUFFER_SIZE )
		return ERROR_INVALID_PARAMETER;

	lptsComProps = (LPTS_COM_PROPS)lpBuffer;
	dwcbBuffer = lptsComProps->len + sizeof(TS_COM_PROPS);

#ifdef TREESVR_STANDALONE
	// ReadFile function will block until it has read specified number of bytes, or all the pipe handles of the write-end
	// have been closed. That is why here we use 4096 instead of the actual length of the packet.
	dwcbBuffer = 4096;
	fSuccess = WriteFile ( ( ( PIPE_STRU * )hPipe )->hClientWritePipe,
			lpBuffer,
			dwcbBuffer,
			lpdwcbBuffer,
			NULL );

#else

	if( szTcpipPort[0] != '\0' )
	{
		/////////////////////////////////
		//TCP/IP chanel
		/////////////////////////////////

		*lpdwcbBuffer = sendPacket((SOCKET)hPipe, lpBuffer, dwcbBuffer);
		if( *lpdwcbBuffer <= 0 )
			return 1;
		else
			return TERR_SUCCESS;
	}

	fSuccess = WriteFile ( hPipe,
			lpBuffer,
			dwcbBuffer,
			lpdwcbBuffer,
			NULL );

#endif

    if ( !fSuccess || *lpdwcbBuffer != dwcbBuffer )
        return GetLastError();

#ifdef TREESVR_STANDALONE
	FlushFileBuffers( ( ( PIPE_STRU * )hPipe )->hClientWritePipe );
#else    
	FlushFileBuffers ( hPipe );
#endif

	return TERR_SUCCESS;
}


/////////////////
// cliSetTcpipPort()
/////////////////////////////////////////////////////////////////////////////
//#ifndef TREESVR_STANDALONE
__declspec( dllexport )
int cliSetTcpipPort( char *szPortOrService )
{
	if( szPortOrService == NULL ) {
		szTcpipPort[0] = '\0';
		return  1;
	}

	strncpy(szTcpipPort, szPortOrService, 32);
	szTcpipPort[31] = '\0';

	return  0;
}

//#endif



__declspec( dllexport )DWORD justRunASQL ( HANDLE hPipe, LPSTR lpParam,
						LPSTR lpResponse, DWORD dwParam )
{
	DWORD dwRetCode, dwErrorCode;
	DWORD dwcbBuffer;

	dwRetCode = TERR_SUCCESS;

	if(	hPipe == INVALID_HANDLE_VALUE ||
			lpParam == NULL || lpResponse == NULL || 
			dwParam <= 0 || dwParam > 4096 )
		return ERROR_INVALID_PARAMETER;

	dwcbBuffer = dwParam;
	dwErrorCode = CliWriteTSNamedPipe( hPipe, lpParam, &dwcbBuffer );
	if( dwErrorCode != TERR_SUCCESS )
		dwRetCode = dwErrorCode;

	dwcbBuffer = dwParam;
	dwErrorCode = CliReadTSNamedPipe( hPipe, lpParam, &dwcbBuffer );
	if( dwErrorCode != TERR_SUCCESS )
		dwRetCode = dwErrorCode;

	return dwRetCode;
}

__declspec( dllexport )DWORD CliTSPipeFTPPut ( HANDLE hPipe, 
						LPSTR lpLocalFile, LPSTR lpRemoteFile )
{
	HANDLE hFile;
	CHAR szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS lptsComProps;
	DWORD dwcbBuffer;
	DWORD dwcbRDWR;
	BOOL fEof = FALSE;
	BOOL fRDWR_OK = TRUE;
    BOOL fSuccess;
	DWORD dwRetCode;
    
	if( hPipe == INVALID_HANDLE_VALUE || lpLocalFile == NULL ||
			lpRemoteFile == NULL || lpLocalFile[0] == '\x0' ||
			lpRemoteFile[0] == '\x0' )
        return ERROR_INVALID_PARAMETER;
	
	lptsComProps = (LPTS_COM_PROPS)szBuffer;
	
	hFile = CreateFile(
		lpLocalFile, 
		GENERIC_READ, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, 
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL );
	if( hFile == INVALID_HANDLE_VALUE )
		return GetLastError();

	lptsComProps->packetType = pkTS_PIPE_FTP;
	lptsComProps->msgType = msgTS_FTP_FILE;
	lptsComProps->lp = cmFTP_FILE_PUT;
	lptsComProps->leftPacket = '\x0';
	lptsComProps->endPacket = '\x0';
	lptsComProps->len = wsprintf( (LPSTR)(szBuffer+sizeof(TS_COM_PROPS)),
			"%s\x0", lpRemoteFile );

	dwcbBuffer = lptsComProps->len + sizeof(TS_COM_PROPS);
	fSuccess = WriteFile ( hPipe,
			szBuffer,
			dwcbBuffer,
			&dwcbRDWR,
			NULL );
    if ( !fSuccess || dwcbRDWR != dwcbBuffer ) {
		CloseHandle( hFile );
        return GetLastError();
	}

	dwcbBuffer = PIPE_BUFFER_SIZE;
	fSuccess = ReadFile ( hPipe,
			szBuffer,
			dwcbBuffer,
			&dwcbRDWR,
			NULL );
    if( !fSuccess || dwcbRDWR == 0 ) {
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

	SetFilePointer( hFile, 0, NULL, FILE_BEGIN );
	lptsComProps->packetType = pkTS_PIPE_FTP;
	lptsComProps->msgType = msgTS_FTP_FILE;
	lptsComProps->lp = cmFTP_FILE_DATA;
	lptsComProps->leftPacket = '\x1';
	lptsComProps->endPacket = '\x1';

	while( !fEof ) {
		SetLastError( 0 );
		dwcbBuffer = PIPE_BUFFER_SIZE - sizeof(TS_COM_PROPS);
		fSuccess = ReadFile ( hFile,
				(LPVOID)(szBuffer+sizeof(TS_COM_PROPS)),
				dwcbBuffer,
				&dwcbRDWR,
				NULL );
		if ( !fSuccess || dwcbRDWR != dwcbBuffer ) {
			dwRetCode = GetLastError();
			if( dwRetCode == ERROR_HANDLE_EOF || dwRetCode == 0 ) {
				if( dwcbRDWR <= dwcbBuffer )
					fEof = TRUE;
			}
			else {
				fRDWR_OK = FALSE;
				break;
			}
		}

		lptsComProps->len = (short)dwcbRDWR;

		dwcbBuffer = dwcbRDWR + sizeof(TS_COM_PROPS);
		fSuccess = WriteFile ( hPipe,
				szBuffer,
				dwcbBuffer,
				&dwcbRDWR,
				NULL );
		if ( !fSuccess || dwcbRDWR != dwcbBuffer ) {
			dwRetCode = GetLastError();
			fRDWR_OK = FALSE;
			break;
		}
	}

	CloseHandle( hFile );

	if( fRDWR_OK ) {
		lptsComProps->packetType = pkTS_PIPE_FTP;
		lptsComProps->msgType = msgTS_FTP_FILE;
		lptsComProps->len = 0;
		lptsComProps->lp = cmFTP_FILE_END;
		lptsComProps->leftPacket = '\x0';
		lptsComProps->endPacket = '\x0';
		dwRetCode = TERR_SUCCESS;
	}
	else {
		lptsComProps->packetType = pkTS_PIPE_FTP;
		lptsComProps->msgType = msgTS_FTP_FILE;
		lptsComProps->len = 0;
		lptsComProps->lp = cmFTP_FILE_ERROR;
		lptsComProps->leftPacket = '\x0';
		lptsComProps->endPacket = '\x0';
	}

	dwcbBuffer = sizeof(TS_COM_PROPS);
	fSuccess = WriteFile ( hPipe,
			szBuffer,
			dwcbBuffer,
			&dwcbRDWR,
			NULL );
    if ( !fSuccess || dwcbRDWR != dwcbBuffer )
        return GetLastError();

    FlushFileBuffers( hPipe );

	dwcbBuffer = PIPE_BUFFER_SIZE;
	fSuccess = ReadFile ( hPipe,
			szBuffer,
			dwcbBuffer,
			&dwcbRDWR,
			NULL );

    return dwRetCode;
}

__declspec( dllexport )DWORD CliTSPipeFTPGet ( HANDLE hPipe, 
						LPSTR lpRemoteFile, LPSTR lpLocalFile )
{
	HANDLE hFile;
	CHAR szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS lptsComProps;
	DWORD dwcbBuffer;
	DWORD dwcbRDWR;
	BOOL fEof = FALSE;
	BOOL fRDWR_OK = TRUE;
    BOOL fSuccess;
	DWORD dwRetCode;
    
	if( hPipe == INVALID_HANDLE_VALUE || lpLocalFile == NULL ||
			lpRemoteFile == NULL || lpLocalFile[0] == '\x0' ||
			lpRemoteFile[0] == '\x0' )
        return ERROR_INVALID_PARAMETER;
	
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	hFile = CreateFile(
		lpLocalFile, 
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, 
		NULL,
		CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL );
	if( hFile == INVALID_HANDLE_VALUE )
		return GetLastError();

	lptsComProps->packetType = pkTS_PIPE_FTP;
	lptsComProps->msgType = msgTS_FTP_FILE;
	lptsComProps->lp = cmFTP_FILE_GET;
	lptsComProps->leftPacket = '\x0';
	lptsComProps->endPacket = '\x0';
	lptsComProps->len = wsprintf( (LPSTR)(szBuffer+sizeof(TS_COM_PROPS)),
			"%s\x0", lpRemoteFile );

	dwcbBuffer = lptsComProps->len + sizeof(TS_COM_PROPS);
	fSuccess = WriteFile ( hPipe,
			szBuffer,
			dwcbBuffer,
			&dwcbRDWR,
			NULL );
    if ( !fSuccess || dwcbRDWR != dwcbBuffer ) {
		CloseHandle( hFile );
		DeleteFile( lpLocalFile );
		return GetLastError();
	}

	dwcbBuffer = PIPE_BUFFER_SIZE;
	fSuccess = ReadFile ( hPipe,
			szBuffer,
			dwcbBuffer,
			&dwcbRDWR,
			NULL );
    if( !fSuccess || dwcbRDWR == 0 ) {
		CloseHandle( hFile );
		DeleteFile( lpLocalFile );
        return GetLastError();
	}

	if( lptsComProps->packetType == pkTS_ERROR ) {
		CloseHandle( hFile );
		DeleteFile( lpLocalFile );
        return 0xFFFFFFFF;
	}

	if( lptsComProps->lp != cmFTP_FILE_OK ) {
		CloseHandle( hFile );
		DeleteFile( lpLocalFile );
		return (DWORD)lptsComProps->lp;
	}

	SetFilePointer( hFile, 0, NULL, FILE_BEGIN );

	while( !fEof ) {
		dwcbBuffer = PIPE_BUFFER_SIZE;
		fSuccess = ReadFile ( hPipe,
				(LPVOID)szBuffer,
				dwcbBuffer,
				&dwcbRDWR,
				NULL );
		if ( !fSuccess || dwcbRDWR == 0 ) {
			dwRetCode = GetLastError();
			fRDWR_OK = FALSE;
			break;
		}
		
		if( lptsComProps->packetType == pkTS_ERROR ) {
			dwRetCode = 0xFFFFFFFF;
			fRDWR_OK = FALSE;
			break;
		}

		if( lptsComProps->lp == cmFTP_FILE_ERROR ) {
			dwRetCode = (DWORD)*(szBuffer+sizeof(TS_COM_PROPS));
			fRDWR_OK = FALSE;
			break;
		}

		if( lptsComProps->endPacket == '\x0' ) {
			fEof = TRUE;
		}

		dwcbBuffer = lptsComProps->len;
		fSuccess = WriteFile ( hFile,
				(LPVOID)(szBuffer+sizeof(TS_COM_PROPS)),
				dwcbBuffer,
				&dwcbRDWR,
				NULL );
		if ( !fSuccess || dwcbRDWR != dwcbBuffer ) {
			dwRetCode = GetLastError();
			fRDWR_OK = FALSE;
		}
	}

    FlushFileBuffers( hFile );
	CloseHandle( hFile );
	
	if( fRDWR_OK )
		dwRetCode = TERR_SUCCESS;

	FlushFileBuffers( hPipe );

    return dwRetCode;
}

__declspec( dllexport )DWORD RunASQL ( HANDLE hPipe, LPSTR lpParam, LPSTR szGetFiles, 
						LPSTR szLocalPath, LPSTR lpResponse, DWORD dwParam )
{
	DWORD dwRetCode;
	LPSTR lpFileName;
	CHAR szLocalFile[MAX_PATH];
    BOOL bFtpOK = TRUE;

	if(	hPipe == INVALID_HANDLE_VALUE ||
			lpParam == NULL || lpResponse == NULL ||
			dwParam <= 0 || dwParam > 4096 )
		return ERROR_INVALID_PARAMETER;

	dwRetCode = justRunASQL( hPipe, lpParam, lpResponse, dwParam );
	if( dwRetCode != TERR_SUCCESS )
		return dwRetCode;

	if( szGetFiles == NULL )
		return dwRetCode;

	for( lpFileName = szGetFiles; *lpFileName != '\x0'; lpFileName += lstrlen( lpFileName )+1 ) {
		if( szLocalPath == NULL )
			wsprintf( szLocalFile, "%s", lpFileName );
		else
			wsprintf( szLocalFile, "%s\\%s", szLocalPath, lpFileName );
		
		dwRetCode = CliTSPipeFTPGet( hPipe, lpFileName, szLocalFile );
		if( dwRetCode != TERR_SUCCESS )
			bFtpOK = FALSE;
	}
	
	if( bFtpOK )
		return TERR_SUCCESS;
	else
		return TERR_NOT_ALL_COPYED;
}