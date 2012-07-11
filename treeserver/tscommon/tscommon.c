/******************************
*	Module Name:
*		TCOMMON.C
*
*	Abstract:
*		This module is the Tree Server common routines to get 
*	the information of the server. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) 1997  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1997.11
********************************************************************************/

#define STRICT
#include <windows.h>
#include <tchar.h>

#define __TSCOMMON_DLL

#include "tscommon.h"

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

DWORD GetTreeServerRootA( LPSTR lpBuffer, LPDWORD lpccbBuffer )
{
	HKEY hKey;
	DWORD dwValueType;
	DWORD dwRetCode;

	if( lpBuffer == NULL || lpccbBuffer == NULL )
		return ERROR_INVALID_PARAMETER;

	dwRetCode = GetEnvironmentVariableA(
		"TREESVR",
		lpBuffer, 
		*lpccbBuffer );

	if( dwRetCode != 0 )
		return ERROR_SUCCESS;
 
	dwRetCode = RegOpenKeyExA( 
			HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Tree Server\\Server\\CurrentVersion\\Environment",
			0,
			KEY_READ | KEY_EXECUTE, 
			&hKey );
	
	if( dwRetCode != ERROR_SUCCESS )
		return dwRetCode;

	dwRetCode = RegQueryValueExA( 
			hKey, 
			"TreeServerRoot",
			NULL, 
			&dwValueType,
			(LPBYTE) lpBuffer, 
			lpccbBuffer );

	if( dwRetCode != ERROR_SUCCESS )
		return dwRetCode;

	if( lpBuffer[*lpccbBuffer-1] == '\\' ) {
		lpBuffer[*lpccbBuffer-1] = '\x0';
		*lpccbBuffer--;
	}

	return ERROR_SUCCESS;
}

DWORD GetTreeServerRootW( LPWSTR lpBuffer, LPDWORD lpccbBuffer )
{
	HKEY hKey;
	DWORD dwValueType;
	DWORD dwRetCode;

	if( lpBuffer == NULL || lpccbBuffer == NULL )
		return ERROR_INVALID_PARAMETER;

	dwRetCode = GetEnvironmentVariableW(
		L"TREESVR",
		lpBuffer, 
		*lpccbBuffer );

	if( dwRetCode != 0 )
		return ERROR_SUCCESS;

	dwRetCode = RegOpenKeyExW( 
			HKEY_LOCAL_MACHINE,
			L"SOFTWARE\\Tree Server\\Server\\CurrentVersion\\Environment",
			0,
			KEY_READ | KEY_EXECUTE, 
			&hKey );
	
	if( dwRetCode != ERROR_SUCCESS )
		return dwRetCode;

	dwRetCode = RegQueryValueExW( 
			hKey, 
			L"TreeServerRoot",
			NULL, 
			&dwValueType,
			(LPBYTE) lpBuffer, 
			lpccbBuffer );

	if( dwRetCode != ERROR_SUCCESS )
		return dwRetCode;

	if( lpBuffer[*lpccbBuffer-1] == L'\\' ) {
		lpBuffer[*lpccbBuffer-1] = L'\x0';
		*lpccbBuffer--;
	}

	return ERROR_SUCCESS;
}

DWORD GetTSProfilePathA( LPSTR lpBuffer, LPDWORD lpccbBuffer )
{
	CHAR szPath[MAX_PATH];
	DWORD cbPath;
	DWORD dwRetCode;

	if( lpBuffer == NULL || lpccbBuffer == NULL )
		return ERROR_INVALID_PARAMETER;

	cbPath = MAX_PATH;
	dwRetCode = GetTreeServerRootA( szPath, &cbPath );
	if( dwRetCode != ERROR_SUCCESS )
		return dwRetCode;

	lstrcatA( szPath, "\\Config" );
	if( *lpccbBuffer <= (DWORD)lstrlenA( szPath ) )
		return ERROR_MORE_DATA;

	*lpccbBuffer = lstrlenA( szPath ) + 1;
	lstrcpyA( lpBuffer, szPath );

	return ERROR_SUCCESS;
}

DWORD GetTSProfilePathW( LPWSTR lpBuffer, LPDWORD lpccbBuffer )
{
	WCHAR szPath[MAX_PATH];
	DWORD cbPath;
	DWORD dwRetCode;

	if( lpBuffer == NULL || lpccbBuffer == NULL )
		return ERROR_INVALID_PARAMETER;

	cbPath = MAX_PATH;
	dwRetCode = GetTreeServerRootW( szPath, &cbPath );
	if( dwRetCode != ERROR_SUCCESS )
		return dwRetCode;

	lstrcatW( szPath, L"\\Config" );
	if( *lpccbBuffer <= (DWORD)lstrlenW( szPath ) )
		return ERROR_MORE_DATA;

	*lpccbBuffer = lstrlenW( szPath ) + 1;
	lstrcpyW( lpBuffer, szPath );

	return ERROR_SUCCESS;
}

/*
void main( void )
{
	DWORD dwRetCode;
	DWORD cbBuffer;
	TCHAR szBuffer[MAX_PATH];

	cbBuffer = MAX_PATH;
	dwRetCode = GetTreeServerRoot( szBuffer, &cbBuffer );
	printf( "Tree Server Root: %s\nPath Length: %ld(%ld).\n",
			szBuffer, cbBuffer, lstrlen( szBuffer ) + 1 );

	cbBuffer = MAX_PATH;
	dwRetCode = GetTSProfilePath( szBuffer, &cbBuffer );
	printf( "Tree Server config path: %s\nPath Length: %ld(%ld).\n",
			szBuffer, cbBuffer, lstrlen( szBuffer ) + 1 );
}
*/