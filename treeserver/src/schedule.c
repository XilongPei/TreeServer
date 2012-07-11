/******************************
*	Module Name:
*		SCHEDULE.C
*
*	Abstract:
*		Tree Server transation schedule module. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1997  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1997.10
*		Update by NiuJingyu, 1998.03
********************************************************************************/

#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <lm.h>
#include <tchar.h>

#include "terror.h"
#include "tlimits.h"
#include "ts_com.h"
#include "tqueue.h"
#include "schedule.h"
#include "treesvr.h"

HANDLE hSemAR;
CRITICAL_SECTION CriticalSection_AR;
REQUEST RequestSpace[MAX_SERVICE_THREAD];
CHAR szRequestFlag[MAX_SERVICE_THREAD];
extern SYSTEM_RES_INFO SystemResInfo;

extern TCHAR *szTreeServerRoot;

//extern TCHAR szAuthServer[CNLEN+1];

// Initialize the transaction schedule thread.
DWORD InitSchedule ( VOID )
{
	HANDLE hThread;
	DWORD dwThreadId;
	DWORD dwRetCode;

	hSemAR = CreateSemaphore( 
			NULL, 
			(LONG)MAX_SERVICE_THREAD, 
			(LONG)MAX_SERVICE_THREAD, 
			_TEXT( "TS_SemaphoreAR" ) );
	if( hSemAR == NULL )
		return GetLastError();
	else {
		if( (dwRetCode = GetLastError()) == ERROR_ALREADY_EXISTS )
			return dwRetCode;
	}

	InitializeCriticalSection( &CriticalSection_AR );
	ZeroMemory( szRequestFlag, MAX_SERVICE_THREAD );

	InitializeCriticalSection( &CriticalSection_User );

	hThread = CreateThread ( NULL, 
			0,
			(LPTHREAD_START_ROUTINE)Schedule,
			(LPVOID)NULL,
			0,
			&dwThreadId );

	if( hThread == NULL )
		return GetLastError();
	
	SystemResInfo.hSchedulerThread = hThread;
	SystemResInfo.dwSchedulerhreadId = dwThreadId;

	return TERR_SUCCESS;
}

INT GetAvailableSpace ( VOID )
{
	int i;
	DWORD dwRetCode;

	dwRetCode = WaitForSingleObject( hSemAR, INFINITE );
	if( dwRetCode == WAIT_TIMEOUT ) {
		// During WaitForSingalObject some error occured.
		return -1;
	}
		
	EnterCriticalSection( &CriticalSection_AR );
		
	for( i = 0; i < MAX_SERVICE_THREAD; i++ ) {
		if( szRequestFlag[i] == '\x0' )
			break;
	}

	if( i >= MAX_SERVICE_THREAD )
		i = -1;
	else 
		szRequestFlag[i] = '\x1';

	LeaveCriticalSection( &CriticalSection_AR );
	
	return i;
}

DWORD FreeSpace ( LPREQUEST lpRequest )
{
	int i;

	i = ((LPSTR)lpRequest - (LPSTR)RequestSpace)/sizeof(REQUEST);
	if( i < 0 || i >= MAX_SERVICE_THREAD )
		return TERR_INVALID_ADDRESS;
		
	EnterCriticalSection( &CriticalSection_AR );
		
	szRequestFlag[i] = '\x0';
	ReleaseSemaphore( hSemAR, 1, NULL );

	LeaveCriticalSection( &CriticalSection_AR );
	
	return i;
}

// The transaction schedule thread function defination.
DWORD Schedule ( LPVOID lpvParam )
{
	DWORD dwErrorCode;
	INT i = 0;

	while( TRUE ) {
		Sleep(0);

		i = GetAvailableSpace();
		if( i == -1 )
			continue;

		dwErrorCode = ProcessRequest( &RequestSpace[i] );
		if( dwErrorCode != TERR_SUCCESS ) {
			FreeSpace( &RequestSpace[i] );
			continue;
		}
	}
	return TERR_SUCCESS;
}

DWORD GetUserWorkDir ( LPCSTR lpUserName, DWORD dwReqId, LPSTR lpHome, LPDWORD lpdwcbHome );
DWORD DeleteTree ( LPCTSTR lpDirectory, BOOL bForce );

DWORD TSRegisterUser ( LPREQUEST lpRequest )
{
	LPCLIENT_RES_INFO lpPtr;
	int i;

	EnterCriticalSection( &CriticalSection_User );

	for( i = 0, lpPtr = SystemResInfo.lpClientResInfo; 
			(DWORD)i < SystemResInfo.dwUsers; i++, lpPtr++ ) {
		if( *(lpPtr->szUser) == '\x0' )
			break;
	}

	if( i == (INT)SystemResInfo.dwUsers )
	{
		LeaveCriticalSection( &CriticalSection_User );
		return 0xFFFFFFFF;
	}

	lstrcpy( lpPtr->szUser, lpRequest->szUser );
	lstrcpy( lpPtr->szComputer, lpRequest->szComputer );
	lpPtr->hToken = (HANDLE)lpRequest->dwRequestId;

	GetLocalTime( &(lpPtr->tLogonTime) );

	lpPtr->dwAgentThreadId = (lpRequest->lpExchangeBuf)->dwCliThreadId;
	lpPtr->dwServiceThreadId = (lpRequest->lpExchangeBuf)->dwSrvThreadId;

	lprintfs( "User Register : %s\n", lpPtr->szUser );

	LeaveCriticalSection( &CriticalSection_User );

	// Add by Jingyu Niu, 1999.08
	// See GetUserWorkDir
	// 
	{
		char szWorkDir[MAX_PATH];
		DWORD dwcbDir = MAX_PATH;

		GetUserWorkDir( lpRequest->szUser, lpRequest->lpExchangeBuf->dwCliThreadId, szWorkDir, &dwcbDir );
		lprintfs( "Create User Workdir : %s\n", szWorkDir );
		CreateDirectory( szWorkDir, NULL );
	}

	return (DWORD)i;
}

DWORD TSUnregisterUser ( HANDLE hUserToken )
{
	LPCLIENT_RES_INFO lpPtr;
	int i;
	char szWorkDir[MAX_PATH];
	DWORD dwcbDir = MAX_PATH;

	EnterCriticalSection( &CriticalSection_User );

	for( i = 0, lpPtr = SystemResInfo.lpClientResInfo; 
			(DWORD)i < SystemResInfo.dwUsers; i++, lpPtr++ ) {
		if( lpPtr->hToken == hUserToken )
			break;
	}

	if( i == (INT)SystemResInfo.dwUsers )
	{
		LeaveCriticalSection( &CriticalSection_User );
		return 0xFFFFFFFF;
	}

	// Add by Jingyu Niu, 1999.08
	// See GetUserWorkDir
	// 
	{
		if(GetUserWorkDir( lpPtr->szUser, (DWORD)(lpPtr->dwAgentThreadId), szWorkDir, &dwcbDir ))
			*szWorkDir = 0;
	}

	lprintfs( "User Unregister : %s\n", lpPtr->szUser );
	lprintfs( "Remove User Workdir : %s\n", szWorkDir );

	ZeroMemory( lpPtr, sizeof(CLIENT_RES_INFO) );

	LeaveCriticalSection( &CriticalSection_User );

	// Add by Jingyu Niu, 1999.08
	// See GetUserWorkDir
	// 
	/*
	{
		TCHAR szBin[MAX_PATH];
		TCHAR szData[MAX_PATH];
		TCHAR szConfig[MAX_PATH];
		TCHAR szCode[MAX_PATH];

		wsprintf( szBin, "%s\\%s", szTreeServerRoot, "bin" );
		wsprintf( szData, "%s\\%s", szTreeServerRoot, "Data" );
		wsprintf( szConfig, "%s\\%s", szTreeServerRoot, "Config" );
		wsprintf( szCode, "%s\\%s", szTreeServerRoot, "Code" );


		if( *szWorkDir &&
				lstrcmpi( szWorkDir, szTreeServerRoot ) &&
				lstrcmpi( szWorkDir, szBin ) &&
				lstrcmpi( szWorkDir, szData ) &&
				lstrcmpi( szWorkDir, szCode ) &&
				lstrcmpi( szWorkDir, szConfig ) )
			DeleteTree( szWorkDir, TRUE );
	}
	*/
	return (DWORD)i;
}

DWORD DeleteTree ( LPCTSTR lpDirectory, BOOL bForce )
{
	WIN32_FIND_DATA wfd;
	HANDLE hFind;
	BOOL bSuccess;
	LPTSTR p;
	TCHAR szCurrentDir[MAX_PATH];
	DWORD dwAttrib; 

	if( !lpDirectory || *lpDirectory == TEXT('\x0') )
		return ERROR_INVALID_PARAMETER;

	GetCurrentDirectory( MAX_PATH, szCurrentDir );
	bSuccess = SetCurrentDirectory( lpDirectory );
	if( !bSuccess )
		return GetLastError();

	hFind = FindFirstFile( "*.*", &wfd );
	if( INVALID_HANDLE_VALUE == hFind )
		return GetLastError();

	while( TRUE ) {
		if( lstrcmp( wfd.cFileName, "." ) != 0 &&
				lstrcmp( wfd.cFileName, ".." ) != 0 ) {
			if( bForce ) {
				dwAttrib = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM;
				dwAttrib = dwAttrib & (~dwAttrib);
				SetFileAttributes( wfd.cFileName, dwAttrib );
			}

			if( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY )
				DeleteTree( wfd.cFileName, bForce );
			else 
				DeleteFile( wfd.cFileName );
		}			

		bSuccess = FindNextFile( hFind, &wfd );
		if( !bSuccess )
			if( GetLastError() == ERROR_NO_MORE_FILES )
				break;
	}

	FindClose( hFind );

	SetCurrentDirectory( TEXT("..") );

	for( p = (LPTSTR)lpDirectory + lstrlen( lpDirectory ); (DWORD)p > (DWORD)lpDirectory; p-- )
		if( *p == TEXT('\\') ) {
			p++;
			break;
		}

	RemoveDirectory( p );

	SetCurrentDirectory( szCurrentDir ); 

	return ERROR_SUCCESS;
}
