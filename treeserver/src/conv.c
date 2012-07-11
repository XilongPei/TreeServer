/******************************
*	Module Name:
*		CONV.C
*
*	Abstract:
*		Tree Server User Conversation Interface module. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1997  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1997.10
*		Rewrite by NiuJingyu, 1998.02
*       Update by NiuJingyu, 1998.03
********************************************************************************/

#define STRICT
#include <windows.h>
#include <memory.h>
#include <tchar.h>

#include "tLimits.h"
#include "terror.h"
#include "conv.h"
#include "ts_com.h"
#include "tqueue.h"
#include "treesvr.h"
#include "secapi.h"
#include "tuser.h"
#include "tree_tcp.h"

#define TreeSVR_PORT 	7777

#ifndef TREESVR_STANDALONE
// Add by Jingyu Niu, 2000.08.25, for support single user mode.
long singleUserMode = 0;
DWORD singleUserThread = 0;
//
#endif

extern SYSTEM_RES_INFO SystemResInfo;
static SECURITY_ATTRIBUTES sa;
static SECURITY_DESCRIPTOR sd;

#pragma warning( disable : 4101 )    // disable the waring C4101 "unreferenced local variable"
// Initialize the User Conversation Interface.
DWORD InitConvInterface ( VOID )
{
	HANDLE	hThread, hThreadTcpip;
	DWORD	dwThreadID, dwThreadIDTcpip;
	PSID	pOwnerSid = NULL, pGroupSid = NULL;
    BOOL	fSuccess = TRUE;
    PACL	pAcl = NULL;
    DWORD	cbAcl;
	DWORD	dwRetCode;
	PSID	pSystemSid = NULL, pAnonymousSid = NULL, pInteractiveSid = NULL;

    __try {
#ifndef TREESVR_STANDALONE
		pOwnerSid = GetUserSid();
		if( pOwnerSid == NULL )
			__leave;
/*
		fSuccess = GetAccountSid( NULL, "TreeServer Users", &pGroupSid );
		if ( !fSuccess )
			__leave;
*/
		pGroupSid = CreateWorldSid();
		if( pGroupSid == NULL )
			__leave;

		pSystemSid = CreateSystemSid();
		if( pSystemSid == NULL )
			__leave;

		pAnonymousSid = CreateAnonymousSid();
		if( pAnonymousSid == NULL )
			__leave;

		pInteractiveSid = CreateInteractiveSid();
		if( pInteractiveSid == NULL )
			__leave;

		cbAcl = GetLengthSid( pOwnerSid ) + GetLengthSid( pGroupSid ) + 
			GetLengthSid( pSystemSid ) + GetLengthSid( pAnonymousSid ) + GetLengthSid( pInteractiveSid ) +
			sizeof(ACL) + (5 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));

		pAcl = (PACL) HeapAlloc(GetProcessHeap(), 0, cbAcl);
		if (NULL == pAcl)
			__leave;

		fSuccess = InitializeAcl(pAcl,
			    cbAcl,
			    ACL_REVISION);
		if (FALSE == fSuccess)
			__leave;

		fSuccess = AddAccessAllowedAce(pAcl,
			    ACL_REVISION,
			    GENERIC_ALL,
			    pOwnerSid);
		if (FALSE == fSuccess)
			__leave;

		fSuccess = AddAccessAllowedAce(pAcl,
			    ACL_REVISION,
			    GENERIC_ALL,//GENERIC_READ|GENERIC_WRITE,
			    pGroupSid);
		if (FALSE == fSuccess) 
			__leave;

		fSuccess = AddAccessAllowedAce(pAcl,
			    ACL_REVISION,
			    GENERIC_ALL,
			    pSystemSid);
		if (FALSE == fSuccess) 
			__leave;

		fSuccess = AddAccessAllowedAce(pAcl,
			    ACL_REVISION,
			    GENERIC_ALL,
			    pInteractiveSid);
		if (FALSE == fSuccess) 
			__leave;

		fSuccess = AddAccessAllowedAce(pAcl,
			    ACL_REVISION,
			    GENERIC_ALL,
			    pAnonymousSid);
		if (FALSE == fSuccess) 
			__leave;

		InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );

		fSuccess = SetSecurityDescriptorDacl(&sd,
				TRUE,
				pAcl,
				FALSE);
		if (FALSE == fSuccess) 
			__leave;

		fSuccess =  SetSecurityDescriptorOwner(
				&sd,
				pOwnerSid,
				FALSE );  
	    if ( !fSuccess )
			__leave;

		fSuccess =  SetSecurityDescriptorGroup(
				&sd,
				pGroupSid,
				FALSE );  

	    if ( !fSuccess ) 
			__leave;

		sa.nLength = sizeof( SECURITY_ATTRIBUTES );
		sa.lpSecurityDescriptor = (LPVOID)&sd;
		sa.bInheritHandle = FALSE;

#endif
		// Create the NamedPipe server thread, Process the user's connection.
		hThread = CreateThread( NULL, 
				0,
				(LPTHREAD_START_ROUTINE)PipeSelectConnectThread,
				(LPVOID)NULL,
				0,
				&dwThreadID );

		// If operation not completed, return the system error code.
		if( hThread == NULL )
		{
			fSuccess = FALSE;
			__leave;
		}

#ifndef TREESVR_STANDALONE
		hThreadTcpip = CreateThread( NULL, 
				0,
				(LPTHREAD_START_ROUTINE)TcpipSelectConnectThread,
				(LPVOID)NULL,
				0,
				&dwThreadIDTcpip );

		// If operation not completed, return the system error code.
		if( hThreadTcpip == NULL )
		{
			fSuccess = FALSE;
			__leave;
		}
#endif

	}
	__finally {
		if( fSuccess ) {
			// Set the thread Prority Class.
			SetThreadPriority( hThread, THREAD_PRIORITY_ABOVE_NORMAL ); 

			SystemResInfo.hConvThread = hThread;
			SystemResInfo.dwConvThreadId = dwThreadID;

#ifndef TREESVR_STANDALONE
			// Set the thread Prority Class.
			SetThreadPriority( hThreadTcpip, THREAD_PRIORITY_ABOVE_NORMAL ); 

			SystemResInfo.hConvThreadTcpip = hThreadTcpip;
			SystemResInfo.dwConvThreadIdTcpip = dwThreadIDTcpip;
#endif

			dwRetCode = TERR_SUCCESS;
		}
		else {
			if( hThread != NULL ) {
				CloseHandle( hThread );
			}

			dwRetCode = GetLastError();

			if( pOwnerSid )
		        HeapFree( GetProcessHeap(), 0, pOwnerSid );
			if( pGroupSid )
		        HeapFree( GetProcessHeap(), 0, pGroupSid );
			if( pSystemSid )
		        HeapFree( GetProcessHeap(), 0, pSystemSid );
			if( pInteractiveSid )
		        HeapFree( GetProcessHeap(), 0, pInteractiveSid );
			if( pAnonymousSid )
		        HeapFree( GetProcessHeap(), 0, pAnonymousSid );
			if( pAcl )
		        HeapFree( GetProcessHeap(), 0, pAcl );
		}
	}
	
	return dwRetCode;
}
#pragma warning( default : 4101 )
/*
// Initialize the User Conversation Interface.
DWORD InitConvInterface ( VOID )
{
	HANDLE hThread;
	DWORD dwThreadID;
	PSID pOwnerSid = NULL, pEveryoneSid = NULL;

    BOOL fSuccess;
	BOOL bSuccess = FALSE;
    PACL pAcl = NULL;
    DWORD cbAcl;
	DWORD dwRetCode;
	HANDLE hFile;

    __try {
		pOwnerSid = GetUserSid();
		if( pOwnerSid == NULL )
			__leave;

		pEveryoneSid = CreateWorldSid();
		if ( !pEveryoneSid )
			__leave;

		cbAcl = GetLengthSid( pOwnerSid ) + GetLengthSid( pEveryoneSid ) +
			sizeof(ACL) + (2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));

		pAcl = (PACL) HeapAlloc(GetProcessHeap(), 0, cbAcl);
		if (NULL == pAcl)
			__leave;

		fSuccess = InitializeAcl(pAcl,
			    cbAcl,
			    ACL_REVISION);
		if (FALSE == fSuccess)
			__leave;

		fSuccess = AddAccessAllowedAce(pAcl,
			    ACL_REVISION,
			    GENERIC_ALL,
			    pOwnerSid);
		if (FALSE == fSuccess)
			__leave;

		fSuccess = AddAccessAllowedAce(pAcl,
			    ACL_REVISION,
			    GENERIC_READ|GENERIC_WRITE,
			    pEveryoneSid);
		if (FALSE == fSuccess) 
			__leave;

		InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );

		fSuccess = SetSecurityDescriptorDacl(&sd,
				TRUE,
				pAcl,
				FALSE);
		if (FALSE == fSuccess) 
			__leave;

		fSuccess =  SetSecurityDescriptorOwner(
				&sd,
				pOwnerSid,
				FALSE );  
	    if ( !fSuccess )
			__leave;

		sa.nLength = sizeof( SECURITY_ATTRIBUTES );
		sa.lpSecurityDescriptor = (LPVOID)&sd;
		sa.bInheritHandle = FALSE;

		hFile = CreateFile( "C:\\hello.txt",
					GENERIC_READ | GENERIC_WRITE,
					0,
					&sa,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					0 );

		WriteFile ( hFile,
				(LPCVOID)"Hello World!", 
				lstrlen( "Hello World!" ), 
				&dwRetCode,
				NULL );

		CloseHandle( hFile );

		// Create the NamedPipe server thread, Process the user's connection.
		hThread = CreateThread( NULL, 
				0,
				(LPTHREAD_START_ROUTINE)PipeSelectConnectThread,
				(LPVOID)NULL,
				0,
				&dwThreadID );

		// If operation not completed, return the system error code.
		if( hThread == NULL )
			__leave;
	
		bSuccess = TRUE;
	}
	__finally {
		if( fSuccess ) {
			// Set the thread Prority Class.
			SetThreadPriority( hThread, THREAD_PRIORITY_ABOVE_NORMAL ); 

			SystemResInfo.hConvThread = hThread;
			SystemResInfo.dwConvThreadId = dwThreadID;
			dwRetCode = TERR_SUCCESS;
		}
		else {
			dwRetCode = GetLastError();

			if( pOwnerSid )
		        HeapFree( GetProcessHeap(), 0, pOwnerSid );
			if( pEveryoneSid )
		        HeapFree( GetProcessHeap(), 0, pEveryoneSid );
			if( pAcl )
		        HeapFree( GetProcessHeap(), 0, pAcl );
		}
	}
	
	return dwRetCode;
}
*/

// NamedPipe server thread, process the user's connection.
// The parameter "lpvParam" not used.
#ifdef TREESVR_STANDALONE

DWORD WINAPI PipeSelectConnectThread ( LPVOID lpvParam )
{
	DWORD		dwThreadID;
	PIPE_STRU	PipeStru;
	LPTSTR		lpMapAddress = NULL;
	HANDLE		hMapFile = NULL, hConnectionReady = NULL, 
				hConnectionMutex = NULL, hThread = NULL, 
				hConnectionCompleted = NULL;

	__try
	{
		// Create file mapping for connection buffer, which is used for 
		// exchanging connection information between client and server
		// sides.
		hMapFile = CreateFileMapping( ( HANDLE )0xFFFFFFFF,
			( LPSECURITY_ATTRIBUTES )NULL,
			PAGE_READWRITE,
			0,
			100,
			"ConnectionMappingObject" );
		if( hMapFile == NULL )
		{
			// dwRetCode = TERR_FILE_MAPPING_ERROR;
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
			// dwRetCode = TERR_MAP_VIEW_ERROR;
			__leave;
		}

		// Create hConnectionReady event object. This object is used for 
		// indicating whether the server can begin to fetch the connection 
		// information from client side.
		hConnectionReady = CreateEvent( NULL, FALSE, FALSE, "CONNECTION_READY" );
		if( hConnectionReady == NULL )
		{
			// dwRetCode = TERR_CREATE_REQUEST_EVENT_ERROR;
			__leave;
		}

		// Create hConnectionCompleted event object. This object is used for 
		// indicating whether the server has finished work for establishing 
		// connection.
		hConnectionCompleted = CreateEvent( NULL, FALSE, FALSE, "CONNECTION_COMPLETED" );
		if( hConnectionCompleted == NULL )
		{
			// dwRetCode = TERR_CREATE_COMPLETED_EVENT_ERROR
			__leave;
		}

		// Create hConnectionMutex event. This object is used for 
		// mutual-exclusion access to establishing connection.
		hConnectionMutex = CreateMutex( NULL, FALSE, "CONNECTION_MUTEX" );
		if( hConnectionMutex == NULL )
		{
			// dwRetCode = TERR_CREATE_CONNECTION_MUTEX_ERROR;
			__leave;
		}

		// Store the current process id into shared memory.
		*( PDWORD )lpMapAddress = GetCurrentProcessId();

		while( TRUE )
		{
			// Wait for request from client.
			WaitForSingleObject( hConnectionReady, INFINITE );

			// 
			PipeStru.hReadPipe	= *( PHANDLE )( lpMapAddress + sizeof( DWORD ) );
			PipeStru.hWritePipe = *( PHANDLE )( lpMapAddress + sizeof( DWORD ) + sizeof( HANDLE ) );

			// Create the client agent thread.
			hThread = CreateThread( NULL, 
					0,
					( LPTHREAD_START_ROUTINE )ClientAgentThread,
					( LPVOID )&PipeStru,
					0,
					&dwThreadID );
				
			if( hThread == NULL ) 
			{
				__leave;
			}
			else 
				CloseHandle( hThread );

			// Set the state of hConnectionCompleted to signaled.
			if( !SetEvent( hConnectionCompleted ) )
				__leave;

			// Set the state of hConnectionReady to nonsignaled.
			if( !ResetEvent( hConnectionReady ) )
				__leave;
		}
	}
	_finally
	{
		if( hConnectionMutex != NULL )
			CloseHandle( hConnectionMutex );

		if( hConnectionCompleted != NULL )
			CloseHandle( hConnectionCompleted );

		if( hConnectionReady != NULL )
			CloseHandle( hConnectionReady );

		if( lpMapAddress != NULL )
			UnmapViewOfFile( lpMapAddress );

		if( hMapFile != NULL )
			CloseHandle( hMapFile );
	}
	
	return TERR_SUCCESS;
}

#else

////////////////////NamedPipe//////////////////////
DWORD WINAPI PipeSelectConnectThread ( LPVOID lpvParam )
{
	HANDLE hPipe, hThread;
	DWORD dwThreadID;
	BOOL fSuccess;	// Indicate if the NamedPipe handle is processed successfully.

	while( TRUE ) {
		// Delay a little time, so that current thread can correctly process the 
		// user's connection.
		Sleep( 0 );

		// Reset the flag.
		fSuccess = TRUE;	

		_try {
			// Create a NamedPipe instance to wait user connect.
			//CreateFile( "d:\\hello.txt", GENERIC_READ | GENERIC_WRITE, 0, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0 );

			hPipe = CreateNamedPipe( _TEXT( "\\\\.\\pipe\\TS_ConvPipe" ),
					PIPE_ACCESS_DUPLEX | FILE_FLAG_WRITE_THROUGH,
					PIPE_TYPE_MESSAGE | PIPE_WAIT | PIPE_READMODE_MESSAGE,
					PIPE_UNLIMITED_INSTANCES,
					PIPE_BUFFER_SIZE,
					PIPE_BUFFER_SIZE,
					PIPE_TIMEOUT,
					&sa );

			if( hPipe == INVALID_HANDLE_VALUE )	{
				// Code for process error here.
				// ....
				_leave;	  
			}
		
			if( ConnectNamedPipe( hPipe, NULL ) ) {
				// If user connected, create a conversation thread to process it.
				hThread = CreateThread( NULL, 
						0,
						(LPTHREAD_START_ROUTINE)ClientAgentThread,
						(LPVOID)hPipe,
						0,
						&dwThreadID );
				
				if( hThread == NULL ) {
					fSuccess = FALSE;
					_leave;
				}
				else
					CloseHandle( hThread );
			}
			else {
				fSuccess = FALSE;
				_leave;
			}
		}

		_finally {
			if( fSuccess == FALSE )
				// if a NamedPipe instance create and not successfull processed,
				// Close it.
				if( hPipe != INVALID_HANDLE_VALUE ) {
					CloseHandle( hPipe );
				}
		}
	}
	return TERR_SUCCESS;
}


////////////////////TCP/IP//////////////////////
DWORD WINAPI TcpipSelectConnectThread ( LPVOID lpvParam )
{
	HANDLE 	hThread;
	DWORD 	dwThreadID;
	
	SOCKET 	iSocket;
	SOCKADDR_IN sin;
	int 	err;
	BOOL	fSuccess;
//	DWORD 	dwBytesRead;
	int 	zero;
	SOCKET 	sListener;
	WSADATA WsaData;

    //
    // Create a listening socket that we'll use to accept incoming
    // conections.
	//

	err = WSAStartup( 0x0101, &WsaData );
	if ( err == SOCKET_ERROR ) {
		lprintfs( "WSAStartup failed.\n" );
	}

	sListener = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sListener == INVALID_SOCKET ) {
		return FALSE;
    }

    //
    // Bind the socket to the POP3 well-known port.
    //

    sin.sin_family = AF_INET;
	sin.sin_port = htons( TreeSVR_PORT );
    sin.sin_addr.s_addr = INADDR_ANY;

	err = bind( sListener, (LPSOCKADDR)&sin, sizeof(sin) );
	if ( err == SOCKET_ERROR ) {
        closesocket( sListener );
        return FALSE;
    }

	//
	// Listen for incoming connections on the socket.
    //

    err = listen( sListener, 5 );
    if ( err == SOCKET_ERROR ) {
        closesocket( sListener );
        return FALSE;
    }

    //
    // Loop forever accepting connections from clients.
    //

	while( TRUE ) {
		// Delay a little time, so that current thread can correctly process the
		// user's connection.
		Sleep( 0 );

		// Reset the flag.
		fSuccess = TRUE;

		_try {

			iSocket = accept( sListener, NULL, NULL );
			if ( iSocket == INVALID_SOCKET ) {
				closesocket( sListener );
				fSuccess = FALSE;
				_leave;
			}

			zero = 0;
			err = setsockopt( iSocket, SOL_SOCKET, SO_SNDBUF, (char *)&zero, sizeof(zero) );
			if ( err == SOCKET_ERROR ) {
				closesocket( iSocket );
				closesocket( sListener );
				fSuccess = FALSE;
				_leave;
			}

			hThread = CreateThread( NULL,
									0,
									(LPTHREAD_START_ROUTINE)TcpipClientAgentThread,
									(LPVOID)iSocket,
									0,
									&dwThreadID );

			if( hThread == NULL ) {
					fSuccess = FALSE;
					_leave;
			} else
					CloseHandle( hThread );
		}

		_finally {
			if( fSuccess == FALSE ) {
				// if a NamedPipe instance create and not successfull processed,
				// Close it.
				if ( iSocket != INVALID_SOCKET ) {
					closesocket( iSocket );
				}
			}
		}
	}

	if ( iSocket != INVALID_SOCKET ) {
		closesocket( sListener );
	}

	return TERR_SUCCESS;
}


#endif

// The thread process the user's conversation: Get user's request command, send it
// to scheduler, Get the response code and return it to user.
// The parameter "lpvParam" if the NamedPipe instance handle value.
DWORD WINAPI ClientAgentThread ( LPVOID lpvParam )
{
	LPTS_COM_PROPS lptsComProps;
	REQUEST Request;
	CHAR szBuffer[PIPE_BUFFER_SIZE];
	DWORD cbWriten, cbReaded;
	DWORD dwRetCode;
	BOOL fSuccess;
	BOOL fDirect = TRUE;
	MSG Msg;

#ifdef TREESVR_STANDALONE
	HANDLE hReadPipe, hWritePipe;

	hReadPipe  = ( ( PIPE_STRU * )lpvParam )->hReadPipe;
	hWritePipe = ( ( PIPE_STRU * )lpvParam )->hWritePipe;
#else
	HANDLE hPipe;

	hPipe = (HANDLE) lpvParam;

	// If the parameter lpvParam is an invalid pipe handle.
	if( hPipe == 0 || hPipe == INVALID_HANDLE_VALUE )
		ExitThread( ERROR_INVALID_PARAMETER );
#endif

	GetQueueStatus( QS_SENDMESSAGE ); 
	
	lprintfs( "Client agent thread starts.\n" );
    
	_try {
	// Wait and read the message send by the client, until the client post.

#ifdef TREESVR_STANDALONE
	fSuccess = ReadFile ( hReadPipe,
			szBuffer, 
			PIPE_BUFFER_SIZE,
			&cbReaded,
			NULL );
#else
	fSuccess = ReadFile ( hPipe,
			szBuffer, 
			PIPE_BUFFER_SIZE,
			&cbReaded,
			NULL );
#endif

	if( !fSuccess ) {
#ifdef TREESVR_STANDALONE
//		FlushFileBuffers( hWritePipe );
#else
		FlushFileBuffers( hPipe );
		DisconnectNamedPipe ( hPipe );
		CloseHandle( hPipe );
#endif
		ExitThread( GetLastError() );
	}

	// Get the packet header, analyse the packet information. 
	lptsComProps = (LPTS_COM_PROPS)szBuffer;
	
	// If the user's first packet is not logon information packet,
	// Reject the user's request.
	if( lptsComProps->packetType != pkTS_SYNC_PIPE || 
			lptsComProps->msgType != msgTS_CLIENT_LOGON ) {
		ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
		lptsComProps->packetType = pkTS_ERROR;
		lptsComProps->msgType = msgTS_ERROR_MSG;
		
		lptsComProps->len = wsprintfA( (LPSTR)(szBuffer+sizeof( TS_COM_PROPS )),
				"User can not access the server before logon." );

#ifdef TREESVR_STANDALONE
		fSuccess = WriteFile ( hWritePipe,
				(LPCVOID)szBuffer, 
//				lptsComProps->len+sizeof( TS_COM_PROPS ), 
				PIPE_BUFFER_SIZE,
				&cbWriten,
				NULL );

//		FlushFileBuffers( hWritePipe );
#else
		fSuccess = WriteFile ( hPipe,
				(LPCVOID)szBuffer, 
				lptsComProps->len+sizeof( TS_COM_PROPS ), 
				&cbWriten,
				NULL );

		FlushFileBuffers( hPipe );
		DisconnectNamedPipe ( hPipe );
		CloseHandle( hPipe );
#endif
		
		ExitThread( TERR_NOT_LOGON );
	}

#ifndef TREESVR_STANDALONE
	// Add by Jingyu Niu, 2000.08.25, for support single user mode.
	if( singleUserMode ) {
		ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
		lptsComProps->packetType = pkTS_SYNC_PIPE;
		lptsComProps->msgType = msgTS_LOGON_ERROR;
		lptsComProps->lp = 0x10000;
		
		lptsComProps->len = wsprintfA( (LPSTR)(szBuffer+sizeof( TS_COM_PROPS )),
			"Server now running in single user mode, not accept connect request." );

#ifdef TREESVR_STANDALONE
		fSuccess = WriteFile ( hWritePipe,
				(LPCVOID)szBuffer, 
//				lptsComProps->len+sizeof( TS_COM_PROPS ), 
				PIPE_BUFFER_SIZE,
				&cbWriten,
				NULL );

//		FlushFileBuffers( hWritePipe );
#else
		fSuccess = WriteFile ( hPipe,
				(LPCVOID)szBuffer, 
				lptsComProps->len+sizeof( TS_COM_PROPS ), 
				&cbWriten,
				NULL );

		FlushFileBuffers( hPipe );
		DisconnectNamedPipe ( hPipe );
		CloseHandle( hPipe );
#endif

		ExitThread( 0x10000 );
	}
#endif

	// If the user's logon information packet is invalid,
	// Reject the user's request.
	dwRetCode = MsgToRequest( (LPCSTR)(szBuffer+sizeof( TS_COM_PROPS )), &Request );
	if( dwRetCode != TERR_SUCCESS ) {
		ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
		lptsComProps->packetType = pkTS_SYNC_PIPE;
		lptsComProps->msgType = msgTS_LOGON_ERROR;
		lptsComProps->lp = dwRetCode;
		
		lptsComProps->len = wsprintfA( (LPSTR)(szBuffer+sizeof( TS_COM_PROPS )),
			"Logon information error(error code:%ld).", dwRetCode );

#ifdef TREESVR_STANDALONE
		fSuccess = WriteFile ( hWritePipe,
				(LPCVOID)szBuffer, 
//				lptsComProps->len+sizeof( TS_COM_PROPS ), 
				PIPE_BUFFER_SIZE,
				&cbWriten,
				NULL );

//		FlushFileBuffers( hWritePipe );
#else
		fSuccess = WriteFile ( hPipe,
				(LPCVOID)szBuffer, 
				lptsComProps->len+sizeof( TS_COM_PROPS ), 
				&cbWriten,
				NULL );

		FlushFileBuffers( hPipe );
		DisconnectNamedPipe ( hPipe );
		CloseHandle( hPipe );
#endif

		ExitThread( dwRetCode );
	}

	// Send the request to scheduler.
	dwRetCode = SendRequest( &Request );
	if( dwRetCode != TERR_SUCCESS ) {
		ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
		lptsComProps->packetType = pkTS_ERROR;
		lptsComProps->msgType = msgTS_LOGON_ERROR;
		lptsComProps->lp = dwRetCode;
		
		lptsComProps->len = wsprintfA( (LPSTR)(szBuffer+sizeof( TS_COM_PROPS )),
			"Can not logon the server(error code:%ld).", dwRetCode );

#ifdef TREESVR_STANDALONE
		fSuccess = WriteFile ( hWritePipe,
				(LPCVOID)szBuffer, 
//				lptsComProps->len+sizeof( TS_COM_PROPS ), 
				PIPE_BUFFER_SIZE,
				&cbWriten,
				NULL );

//		FlushFileBuffers( hWritePipe );
#else
		fSuccess = WriteFile ( hPipe,
				(LPCVOID)szBuffer, 
				lptsComProps->len+sizeof( TS_COM_PROPS ), 
				&cbWriten,
				NULL );

		FlushFileBuffers( hPipe );
		DisconnectNamedPipe ( hPipe );
		CloseHandle( hPipe );
#endif

		ExitThread( dwRetCode );
	}
	

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps->packetType = pkTS_SYNC_PIPE;

	if( Request.dwRequest == TERR_SUCCESS ) {
		lptsComProps->msgType = msgTS_LOGON_OK;
		lptsComProps->lp = Request.dwRequestId;
	}
	else { 
		lptsComProps->msgType = msgTS_LOGON_ERROR;
		lptsComProps->lp = Request.dwRequest;
	}
	
#ifdef TREESVR_STANDALONE
	fSuccess = WriteFile ( hWritePipe,
			(LPCVOID)szBuffer, 
//			sizeof( TS_COM_PROPS ), 
			PIPE_BUFFER_SIZE,
			&cbWriten,
			NULL );
#else
	fSuccess = WriteFile ( hPipe,
			(LPCVOID)szBuffer, 
			sizeof( TS_COM_PROPS ), 
			&cbWriten,
			NULL );
#endif

	if( !fSuccess ) {
#ifdef TREESVR_STANDALONE
//		FlushFileBuffers( hWritePipe );
#else
		FlushFileBuffers( hPipe );
		DisconnectNamedPipe ( hPipe );
		CloseHandle( hPipe );
#endif

		ExitThread( GetLastError() );
	}
	
	if( Request.dwRequest != TERR_SUCCESS ) {
#ifdef TREESVR_STANDALONE		
//		FlushFileBuffers( hWritePipe );
#else		
		FlushFileBuffers( hPipe );
		DisconnectNamedPipe ( hPipe );
		CloseHandle ( hPipe );
#endif

		ExitThread( TERR_SUCCESS );
	}

	// Add by NiuJingyu, 1998.03
	GetMessage( &Msg, NULL, TS_SERVICE_START, TS_SERVICE_START );

	dwRetCode = TERR_SUCCESS;

//	lprintfs( "########################## Begin transfer loop.\n" );
	while( 1 ) {
		if( fDirect ) {
			// Client is voluntary.
//		lprintfs( "########################## Read namedpipe.\n" );
#ifdef TREESVR_STANDALONE		
			fSuccess = ReadFile ( hReadPipe,
					szBuffer, 
					PIPE_BUFFER_SIZE,
					&cbReaded,
					NULL );
#else
			fSuccess = ReadFile ( hPipe,
					szBuffer, 
					PIPE_BUFFER_SIZE,
					&cbReaded,
					NULL );
#endif

			if( !fSuccess ) {
				dwRetCode = GetLastError();
				if( dwRetCode == ERROR_BROKEN_PIPE )  { //The pipe has been ended.
					CliSetExchBufError( Request.lpExchangeBuf );
					dwRetCode = TERR_CLIENT_CLOSE;
					break;
				}
			}

#ifdef TREESVR_STANDALONE		
//			FlushFileBuffers ( hWritePipe );
#else
			FlushFileBuffers ( hPipe );
#endif
		
			lptsComProps = (LPTS_COM_PROPS)szBuffer;
			if( lptsComProps->endPacket == '\x0' )
				fDirect = FALSE;

			if( lptsComProps->packetType == pkTS_SYNC_PIPE ) {
				if( lptsComProps->msgType == msgTS_PIPE_CLOSE ) {
					dwRetCode = TERR_SUCCESS;
					break;
				}
			} 

#ifndef TREESVR_STANDALONE
			// Add by Jingyu Niu, 2000.08.25, for support single user mode.
			if( singleUserMode && singleUserThread != GetCurrentThreadId() ) {
				if( fDirect )
					continue;
				else {
					dwRetCode = TERR_SUCCESS;
					break;
				}
			}
#endif

//			lprintfs( "########################## Client write exchange buffer.\n" );
			dwRetCode = CliWriteExchngBuf( Request.lpExchangeBuf, szBuffer, 4096 );
			if( dwRetCode != TERR_SUCCESS ) {
				break;
			}
		}
		else {
			// Service thread is voluntary.
			// Read the packet from service thread.
			dwRetCode = CliReadExchngBuf( Request.lpExchangeBuf, szBuffer, 4096 );
			if( dwRetCode != TERR_SUCCESS )
				break;

			lptsComProps = (LPTS_COM_PROPS)szBuffer;
			if( lptsComProps->endPacket == '\x0' )
				fDirect = TRUE;
	
			if( lptsComProps->packetType == pkTS_SYNC_PIPE ) {
				if( lptsComProps->msgType == msgTS_NO_DATA )
					continue;
			}

#ifndef TREESVR_STANDALONE
			// Add by Jingyu Niu, 2000.08.25, for support single user mode.
			if( singleUserMode && singleUserThread != GetCurrentThreadId() && fDirect ) {
				lptsComProps->packetType = pkTS_SYNC_PIPE;
				lptsComProps->msgType = msgTS_PIPE_CLOSE;
				lptsComProps->lp = 0;
				lptsComProps->len = 0;
				lptsComProps->leftPacket = '\x0';
				lptsComProps->endPacket = '\x0';

				dwRetCode = CliWriteExchngBuf( Request.lpExchangeBuf, szBuffer, 4096 );
				break;
			}
#endif

#ifdef TREESVR_STANDALONE
			fSuccess = WriteFile ( hWritePipe,
					(LPCVOID)szBuffer, 
//					lptsComProps->len+sizeof( TS_COM_PROPS ), 
					PIPE_BUFFER_SIZE,
					&cbWriten,
					NULL );
#else
			fSuccess = WriteFile ( hPipe,
					(LPCVOID)szBuffer, 
					lptsComProps->len+sizeof( TS_COM_PROPS ), 
					&cbWriten,
					NULL );
#endif
			
			if( !fSuccess ) {
				dwRetCode = GetLastError();
				if( dwRetCode == ERROR_NO_DATA ) { // Pipe close in progress.
					dwRetCode = TERR_CLIENT_CLOSE;
					CliSetExchBufError( Request.lpExchangeBuf );
					break;
				}
			}
#ifdef TREESVR_STANDALONE		
//			FlushFileBuffers ( hWritePipe );
#else
			FlushFileBuffers ( hPipe );
#endif
		}
		
		dwRetCode = TERR_SUCCESS;
	}

	if( dwRetCode == TERR_SUCCESS ) {
		ZeroMemory( szBuffer, 4096 );
		lptsComProps->packetType = pkTS_SYNC_PIPE;
		lptsComProps->msgType = msgTS_CLIENT_LOGOFF;
		dwRetCode = CliWriteExchngBuf( Request.lpExchangeBuf, szBuffer, 4096 );
		if( dwRetCode != TERR_SUCCESS )
			CliSetExchBufError( Request.lpExchangeBuf );
	}

	lprintfs( "Client agent thread ends.\n" );

#ifdef TREESVR_STANDALONE		
//	FlushFileBuffers ( hWritePipe );
#else
	FlushFileBuffers ( hPipe );
	DisconnectNamedPipe ( hPipe );
	CloseHandle ( hPipe );
#endif
	}
	_except ( EXCEPTION_EXECUTE_HANDLER ) {
		char szBuffer[256];

		wsprintf( szBuffer, "Client adent thread cause an exception.\n" );
		MessageBox( NULL, szBuffer, "Inspector", MB_OK | MB_ICONSTOP );
	}

	ExitThread( TERR_SUCCESS );

	return  0;

}


/////////////////////////////////////////////////////////////////////////
// The thread process the user's conversation: Get user's request command, send it
// to scheduler, Get the response code and return it to user.
// The parameter "lpvParam" if the NamedPipe instance handle value.
DWORD WINAPI TcpipClientAgentThread( LPVOID lpvParam )
{
	LPTS_COM_PROPS lptsComProps;
	REQUEST 	Request;
	CHAR 		szBuffer[PIPE_BUFFER_SIZE];
	DWORD 		cbWriten, cbReaded;
	DWORD 		dwRetCode;
	int 		iRet;
	BOOL 		fDirect = TRUE;
	MSG 		Msg;
	SOCKET 		iSocket;

	iSocket = (SOCKET)lpvParam;

	// If the parameter lpvParam is an invalid pipe handle.
	//if( iSocket == SOCKET_ERROR )
	//	ExitThread( ERROR_INVALID_PARAMETER );

	GetQueueStatus( QS_SENDMESSAGE );

	lprintfs( "Client agent thread starts.\n" );

	_try {
	// Wait and read the message send by the client, until the client post.

	cbReaded = recvPacket(iSocket, szBuffer, PIPE_BUFFER_SIZE);
	//if( cbReaded == SOCKET_ERROR ) {
	if( cbReaded <= 0 ) {
		ExitThread( GetLastError() );
	}

	// Get the packet header, analyse the packet information.
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	// If the user's first packet is not logon information packet,
	// Reject the user's request.
	if( lptsComProps->packetType != pkTS_SYNC_PIPE ||
						lptsComProps->msgType != msgTS_CLIENT_LOGON ) {
		ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
		lptsComProps->packetType = pkTS_ERROR;
		lptsComProps->msgType = msgTS_ERROR_MSG;

		lptsComProps->len = wsprintfA( (LPSTR)(szBuffer+sizeof( TS_COM_PROPS )),
				"User can not access the server before logon." );

		iRet = sendPacket(iSocket, szBuffer,
							 lptsComProps->len+sizeof( TS_COM_PROPS ) );
		//if( iRet == SOCKET_ERROR ) {
		if( iRet <= 0 ) {
			ExitThread( GetLastError() );
		}

		ExitThread( TERR_NOT_LOGON );
	}

#ifndef TREESVR_STANDALONE
	// Add by Jingyu Niu, 2000.08.25, for support single user mode.
	if( singleUserMode ) {
		ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
		lptsComProps->packetType = pkTS_SYNC_PIPE;
		lptsComProps->msgType = msgTS_LOGON_ERROR;
		lptsComProps->lp = 0x10000;
		
		lptsComProps->len = wsprintfA( (LPSTR)(szBuffer+sizeof( TS_COM_PROPS )),
			"Server now running in single user mode, not accept connect request." );

		cbWriten = sendPacket( iSocket, szBuffer,
							 lptsComProps->len+sizeof(TS_COM_PROPS) );
		//if( cbWriten == SOCKET_ERROR ) {
		if( cbWriten <= 0 ) {
			ExitThread( GetLastError() );
		}

		ExitThread( 0x10000 );
	}
#endif
	
	// If the user's logon information packet is invalid,
	// Reject the user's request.
	dwRetCode = MsgToRequest( (LPCSTR)(szBuffer+sizeof( TS_COM_PROPS )), &Request );
	if( dwRetCode != TERR_SUCCESS ) {
		ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
		lptsComProps->packetType = pkTS_SYNC_PIPE;
		lptsComProps->msgType = msgTS_LOGON_ERROR;
		lptsComProps->lp = dwRetCode;
		
		lptsComProps->len = wsprintfA( (LPSTR)(szBuffer+sizeof( TS_COM_PROPS )),
			"Logon information error(error code:%ld).", dwRetCode );

		cbWriten = sendPacket( iSocket, szBuffer,
							 lptsComProps->len+sizeof(TS_COM_PROPS) );
		//if( cbWriten == SOCKET_ERROR ) {
		if( cbWriten <= 0 ) {
			ExitThread( GetLastError() );
		}

		ExitThread( dwRetCode );
	}
	
	// Send the request to scheduler.
	dwRetCode = SendRequest( &Request );
	if( dwRetCode != TERR_SUCCESS ) {
		ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
		lptsComProps->packetType = pkTS_ERROR;
		lptsComProps->msgType = msgTS_LOGON_ERROR;
		lptsComProps->lp = dwRetCode;
		
		lptsComProps->len = wsprintfA( (LPSTR)(szBuffer+sizeof( TS_COM_PROPS )),
			"Can not logon the server(error code:%ld).", dwRetCode );

		cbWriten = sendPacket( iSocket, szBuffer,
							 lptsComProps->len+sizeof(TS_COM_PROPS) );
		//if( cbWriten == SOCKET_ERROR ) {
		if( cbWriten <= 0 ) {
			ExitThread( GetLastError() );
		}

		ExitThread( dwRetCode );
	}


	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	lptsComProps->packetType = pkTS_SYNC_PIPE;

	if( Request.dwRequest == TERR_SUCCESS ) {
		lptsComProps->msgType = msgTS_LOGON_OK;
		lptsComProps->lp = Request.dwRequestId;
	}
	else {
		lptsComProps->msgType = msgTS_LOGON_ERROR;
		lptsComProps->lp = Request.dwRequest;
	}

	cbWriten = sendPacket( iSocket, szBuffer, sizeof(TS_COM_PROPS) );
	//if( cbWriten == SOCKET_ERROR ) {
	if( cbWriten <= 0 ) {
		ExitThread( GetLastError() );
	}

	if( Request.dwRequest != TERR_SUCCESS ) {
		ExitThread( TERR_SUCCESS );
	}

	// Add by NiuJingyu, 1998.03
	GetMessage( &Msg, NULL, TS_SERVICE_START, TS_SERVICE_START );

	dwRetCode = TERR_SUCCESS;

	while( 1 ) {
		if( fDirect ) {
			cbReaded = recvPacket(iSocket, szBuffer, PIPE_BUFFER_SIZE);
			//if( cbReaded == SOCKET_ERROR ) {
			if( cbReaded <= 0 ) {
				dwRetCode = GetLastError();
				CliSetExchBufError( Request.lpExchangeBuf );
				dwRetCode = TERR_CLIENT_CLOSE;
				break;
			}

			lptsComProps = (LPTS_COM_PROPS)szBuffer;
			if( lptsComProps->endPacket == '\x0' )
				fDirect = FALSE;

			if( lptsComProps->packetType == pkTS_SYNC_PIPE ) {
				if( lptsComProps->msgType == msgTS_PIPE_CLOSE ) {
					dwRetCode = TERR_SUCCESS;
					break;
				}
			} 

#ifndef TREESVR_STANDALONE
			// Add by Jingyu Niu, 2000.08.25, for support single user mode.
			if( singleUserMode && singleUserThread != GetCurrentThreadId() ) {
				if( fDirect )
					continue;
				else {
					dwRetCode = TERR_SUCCESS;
					break;
				}
			}

#endif
			
//			lprintfs( "########################## Client write exchange buffer.\n" );
			dwRetCode = CliWriteExchngBuf( Request.lpExchangeBuf, szBuffer, 4096 );
			if( dwRetCode != TERR_SUCCESS ) {
				break;
			}
		}
		else {
			// Service thread is voluntary.
			// Read the packet from service thread.
			dwRetCode = CliReadExchngBuf( Request.lpExchangeBuf, szBuffer, 4096 );
			if( dwRetCode != TERR_SUCCESS )
				break;

			lptsComProps = (LPTS_COM_PROPS)szBuffer;
			if( lptsComProps->endPacket == '\x0' )
				fDirect = TRUE;
	
			if( lptsComProps->packetType == pkTS_SYNC_PIPE ) {
				if( lptsComProps->msgType == msgTS_NO_DATA )
					continue;
			}

#ifndef TREESVR_STANDALONE
			// Add by Jingyu Niu, 2000.08.25, for support single user mode.
			if( singleUserMode && singleUserThread != GetCurrentThreadId() && fDirect ) {
				lptsComProps->packetType = pkTS_SYNC_PIPE;
				lptsComProps->msgType = msgTS_PIPE_CLOSE;
				lptsComProps->lp = 0;
				lptsComProps->len = 0;
				lptsComProps->leftPacket = '\x0';
				lptsComProps->endPacket = '\x0';

				dwRetCode = CliWriteExchngBuf( Request.lpExchangeBuf, szBuffer, 4096 );
				break;
			}
#endif

			cbWriten = sendPacket( iSocket, szBuffer,
								 lptsComProps->len+sizeof(TS_COM_PROPS) );
			//if( cbWriten == SOCKET_ERROR ) {
			if( cbWriten <= 0 ) {
				dwRetCode = GetLastError();
				CliSetExchBufError( Request.lpExchangeBuf );
				break;
			}
		}

		dwRetCode = TERR_SUCCESS;
	}

	if( dwRetCode == TERR_SUCCESS ) {
		ZeroMemory( szBuffer, 4096 );
		lptsComProps->packetType = pkTS_SYNC_PIPE;
		lptsComProps->msgType = msgTS_CLIENT_LOGOFF;
		dwRetCode = CliWriteExchngBuf( Request.lpExchangeBuf, szBuffer, 4096 );
		if( dwRetCode != TERR_SUCCESS )
			CliSetExchBufError( Request.lpExchangeBuf );
	}

	lprintfs( "Client agent thread ends.\n" );

	}
	_except ( EXCEPTION_EXECUTE_HANDLER ) {
		char szBuffer[256];

		wsprintf( szBuffer, "Client adent thread cause an exception.\n" );
		MessageBox( NULL, szBuffer, "Inspector", MB_OK | MB_ICONSTOP );
	}

	closesocket( iSocket );
	ExitThread( TERR_SUCCESS );

	return  0;

}

DWORD MsgToRequest ( LPCSTR lpMessage, LPREQUEST lpRequest )
{
	//char *lpPtr1;
	//char *lpPtr2;
	//int i;

	// Check the pointer parameter.
	if( lpMessage == NULL || lpRequest == NULL )
		return ERROR_INVALID_PARAMETER;

	lstrcpyn( lpRequest->szUser, lpMessage, 16 );
	_strupr( lpRequest->szUser );

	lstrcpyn( lpRequest->szPasswd, lpMessage+16, 16 );
	
	lstrcpyn( lpRequest->szComputer, lpMessage+32, 16 );
	_strupr( lpRequest->szComputer );

	// Use the current thread id to indicate the request(It's unique).
	lpRequest->dwRequestId = GetCurrentThreadId();

	/*
	lpPtr1 = (char *)lpMessage;
	lpPtr2 = strchr( lpPtr1, '\n' );
	if( lpPtr2 == NULL )
		return TERR_INVALID_MESSAGE;
	i = lpPtr2 - lpPtr1;
	if( i > MAX_USER )
		return TERR_INVALID_USER;
	lstrcpyn( lpRequest->szUser, lpPtr1, i+1 );
	_strupr( lpRequest->szUser );

	lpPtr1 = ++lpPtr2;
	lpPtr2 = strchr( lpPtr1, '\n' );
	if( lpPtr2 == NULL )
		return TERR_INVALID_MESSAGE;
	i = lpPtr2 - lpPtr1;
	if( i > MAX_PASSWD )
		return TERR_INVALID_PASSWORD;
	lstrcpyn( lpRequest->szPasswd, lpPtr1, i+1 );

	i = lstrlen( ++lpPtr2 );
	if( i <= 0 )
		return TERR_INVALID_MESSAGE;
	if( i > MAX_COMPUTER )
		return TERR_INVALID_COMPUTER;
	lstrcpyn( lpRequest->szComputer, lpPtr2, i+1 );
	_strupr( lpRequest->szComputer );
	*/
	
	GetLocalTime( &(lpRequest->Time) );

	return TERR_SUCCESS;
}
