/******************************
*	Module Name:
*		TACCOUNT.C
*
*	Abstract:
*		Tree Server user information module, contains the routines 
*	access the user's information.
*	
*	Build this module need the library: NETAPI32.LIB
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) 1997  China Railway Software Corporation
*
*	Revision History:
*		Write by NiuJingyu, 1997.11
********************************************************************************/

#define STRICT
#include <windows.h>
#include <lm.h>

#define __TACCOUNT_MAIN

#include "taccount.h"

BOOL WINAPI DllEntryPoint( HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	BOOL fSuccess = TRUE;

	switch( fdwReason ) {
	case DLL_PROCESS_ATTACH:
		// Allow the C runtime routines	to initialize first.
		//fSuccess = _CRT_INIT( hInstDLL, fdwReason, lpvReserved );
		//if( !fSuccess )
		//	return FALSE;
		break;

	case DLL_THREAD_ATTACH:
		// Allow the C runtime routines	to initialize first.
		//fSuccess = _CRT_INIT( hInstDLL, fdwReason, lpvReserved );
		//if( !fSuccess )
		//	return FALSE;
		break;

	case DLL_THREAD_DETACH:
		//fSuccess = _CRT_INIT( hInstDLL, fdwReason, lpvReserved );
		break;

	case DLL_PROCESS_DETACH:
		//fSuccess = _CRT_INIT( hInstDLL, fdwReason, lpvReserved );
		break;
	}

	return fSuccess;
}

DWORD MakeComputerNameW( LPCWSTR lpwComputer, LPWSTR lpwComputerName, LPDWORD lpcbwComputerName )
{
    DWORD cchComputer;

    if( lpwComputer == NULL || lpwComputerName == NULL || lpcbwComputerName == NULL )
		return ERROR_INVALID_PARAMETER;

	if( *lpwComputer == L'\0' ) 
        return ERROR_INVALID_COMPUTERNAME;

    cchComputer = lstrlenW( lpwComputer );

    if( lpwComputer[0] != L'\\' && lpwComputer[1] != L'\\' ) {
		lpwComputerName[0] =  L'\\';
		lpwComputerName[1] =  L'\\';
		lpwComputerName[2] =  L'\x0';
    }
    else {
        cchComputer -= 2;
        lpwComputerName[0] = L'\0';
    }

    if( cchComputer > CNLEN )
        return ERROR_INVALID_COMPUTERNAME;

	if( lstrcatW( lpwComputerName, lpwComputer ) == NULL ) 
		return GetLastError();

	cchComputer	= lstrlenW( lpwComputerName ) + 1;
	if( cchComputer > *lpcbwComputerName )
		return ERROR_MORE_DATA;
	
	*lpcbwComputerName = cchComputer;  
	return ERROR_SUCCESS;
}

DWORD GetUserHomeDirW( LPCWSTR lpwServer, LPCWSTR lpwUserName, LPWSTR lpwHomeDir, LPDWORD lpcbHomeDir )
{
	PUSER_INFO_1 lpUserInfo = NULL;
	WCHAR wcComputerName[CNLEN+3];
	LPWSTR _lpServer;
	DWORD cchBuffer;
	DWORD dwRetCode;

	if( lpwUserName == NULL || lpwHomeDir == NULL || lpcbHomeDir == NULL )
        return ERROR_INVALID_PARAMETER;

	if ( lstrlenW( lpwUserName ) > UNLEN )
        return ERROR_INVALID_ACCOUNT_NAME;
	
	if( lpwServer == NULL )
		_lpServer = NULL;
	else {
		cchBuffer = CNLEN+3;
		dwRetCode = MakeComputerNameW( lpwServer, (LPWSTR)wcComputerName, &cchBuffer );
		if( dwRetCode != ERROR_SUCCESS )
			return dwRetCode;
		_lpServer = wcComputerName;
	}

	dwRetCode = NetUserGetInfo( _lpServer, (LPWSTR)lpwUserName, 1, (LPBYTE *)&lpUserInfo );
	if( dwRetCode != NERR_Success )
		return dwRetCode;

	cchBuffer = lstrlenW( lpUserInfo->usri1_home_dir ) + 1;
	if( cchBuffer > *lpcbHomeDir )
		return ERROR_MORE_DATA;

	lstrcpyW( lpwHomeDir, lpUserInfo->usri1_home_dir );
	NetApiBufferFree ( (LPBYTE) lpUserInfo );

	return NERR_Success;
}

DWORD GetUserHomeDirA( LPCSTR lpServer, LPCSTR lpUserName, LPSTR lpHomeDir, LPDWORD lpcbHomeDir )
{
	PUSER_INFO_1 lpUserInfo = NULL;
	WCHAR wcComputerName[CNLEN+3];
	WCHAR wcServer[CNLEN], wcUserName[UNLEN];
	LPWSTR _lpServer;
	DWORD cchBuffer;
	DWORD dwRetCode;

	if( lpUserName == NULL || lpHomeDir == NULL || lpcbHomeDir == NULL )
        return ERROR_INVALID_PARAMETER;

	if( lpServer )
		mbstowcs( wcServer, lpServer, lstrlen( lpServer )+1 );
	
	mbstowcs( wcUserName, lpUserName, lstrlen( lpUserName )+1 );

	if ( lstrlenW( wcUserName ) > UNLEN )
        return ERROR_INVALID_ACCOUNT_NAME;
	
	if( lpServer == NULL )
		_lpServer = NULL;
	else {
		cchBuffer = CNLEN+3;
		dwRetCode = MakeComputerNameW( wcServer, (LPWSTR)wcComputerName, &cchBuffer );
		if( dwRetCode != ERROR_SUCCESS )
			return dwRetCode;
		_lpServer = wcComputerName;
	}

	dwRetCode = NetUserGetInfo( _lpServer, wcUserName, 1, (LPBYTE *)&lpUserInfo );
	if( dwRetCode != NERR_Success )
		return dwRetCode;

	cchBuffer = lstrlenW( lpUserInfo->usri1_home_dir ) + 1;
	if( cchBuffer > *lpcbHomeDir )
		return ERROR_MORE_DATA;

	wcstombs ( lpHomeDir, lpUserInfo->usri1_home_dir, MAX_PATH );
	NetApiBufferFree ( (LPBYTE) lpUserInfo );

	return NERR_Success;
}

DWORD AuditUserA( LPCSTR lpUser, LPCSTR lpPasswd )
{
	DWORD dwRetCode;
	WCHAR szwComputer[CNLEN+1];
	WCHAR szwComputerName[CNLEN+3];
	DWORD cbComputer;
	WCHAR szwUser[UNLEN];
	WCHAR szwOldPasswd[PWLEN];
	WCHAR szwNewPasswd[PWLEN];

	if( lpUser == NULL || lpPasswd == NULL )
		return ERROR_INVALID_PARAMETER;

	cbComputer = CNLEN+1;
	GetComputerNameW( szwComputer, &cbComputer );

	cbComputer = CNLEN+3;
	MakeComputerNameW( szwComputer, szwComputerName, &cbComputer );

	mbstowcs( szwUser, lpUser, UNLEN );
	mbstowcs( szwOldPasswd, lpPasswd, PWLEN );
	mbstowcs( szwNewPasswd, lpPasswd, UNLEN );

	dwRetCode = NetUserChangePassword(
		szwComputerName,
        szwUser,
		szwOldPasswd,
        szwNewPasswd );

	return dwRetCode;
}

DWORD AuditUserW( LPWSTR lpUser, LPWSTR lpPasswd )
{
	DWORD dwRetCode;
	WCHAR szwComputer[CNLEN+1];
	WCHAR szwComputerName[CNLEN+3];
	DWORD cbComputer;

	if( lpUser == NULL || lpPasswd == NULL )
		return ERROR_INVALID_PARAMETER;

	cbComputer = CNLEN+1;
	GetComputerNameW( szwComputer, &cbComputer );

	cbComputer = CNLEN+3;
	MakeComputerNameW( szwComputer, szwComputerName, &cbComputer );

	dwRetCode = NetUserChangePassword(
		szwComputerName,
        lpUser,
		lpPasswd,
        lpPasswd );

	return dwRetCode;
}


/*
/////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>

void main ( void )
{
	char szHomeDir[MAX_PATH];
	DWORD dwErrorCode, cbDir;
	char UserName[80], ComputerName[80];
	WCHAR szwHomeDir[MAX_PATH];

	printf ( "Computer name: " );
	gets ( ComputerName	);
	printf ( "User name: " );
	gets ( UserName );
	
	cbDir = MAX_PATH;
	dwErrorCode = GetUserHomeDirA( ComputerName, UserName, szHomeDir, &cbDir );
	if( dwErrorCode != NERR_Success ) {
		printf ( "GetUserHomeDir() error (return code: %ld).\n", dwErrorCode );
		return;
	}

	printf ( "Home directory: %s\n", szHomeDir );

	cbDir = MAX_PATH;
	dwErrorCode = GetUserHomeDirW( L"Treesvr", L"david", szwHomeDir, &cbDir );
	if( dwErrorCode != NERR_Success ) {
		printf ( "GetUserHomeDir() error (return code: %ld).\n", dwErrorCode );
		return;
	}
	wcstombs ( szHomeDir, szwHomeDir, MAX_PATH );
	printf ( "Home directory: %s\n", szHomeDir );

	dwErrorCode = AuditUserW( L"david", L"" );
	if( dwErrorCode == ERROR_SUCCESS ) {
		printf ( "OK.\n" );
	}
	else 
		printf ( "Error(%ld).\n", dwErrorCode );

	dwErrorCode = AuditUserA( "david", "" );
	if( dwErrorCode == ERROR_SUCCESS ) {
		printf ( "OK.\n" );
	}
	else 
		printf ( "Error(%ld).\n", dwErrorCode );


}
*/