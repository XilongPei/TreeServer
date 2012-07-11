#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include "taccount.h"

void main ( void )
{
	char szHomeDir[MAX_PATH];
	DWORD dwErrorCode, cbDir;
	char UserName[80], ComputerName[80];
	WCHAR szwHomeDir[MAX_PATH];

	printf ( "User name: " );
	gets ( UserName );
	
	cbDir = MAX_PATH;
	dwErrorCode = GetUserHomeDirA( NULL, UserName, szHomeDir, &cbDir );
	if( dwErrorCode != ERROR_SUCCESS ) {
		printf ( "GetUserHomeDir() error (return code: %ld).\n", dwErrorCode );
		return;
	}

	printf ( "Home directory: %s\n", szHomeDir );

	dwErrorCode = GetUserHomeDir( NULL, UserName, szHomeDir, &cbDir );
	if( dwErrorCode != ERROR_SUCCESS ) {
		printf ( "GetUserHomeDir() error (return code: %ld).\n", dwErrorCode );
		return;
	}

	printf ( "Home directory: %s\n", szHomeDir );

	cbDir = MAX_PATH;
	dwErrorCode = GetUserHomeDirW( NULL, L"david", szwHomeDir, &cbDir );
	if( dwErrorCode != ERROR_SUCCESS ) {
		printf ( "GetUserHomeDir() error (return code: %ld).\n", dwErrorCode );
		return;
	}
	wcstombs ( szHomeDir, szwHomeDir, MAX_PATH );
	printf ( "Home directory: %s\n", szHomeDir );

	dwErrorCode = AuditUserW( L"david", L"Pentium2" );
	if( dwErrorCode == ERROR_SUCCESS ) {
		printf ( "OK.\n" );
	}
	else 
		printf ( "Error(%ld).\n", dwErrorCode );

	dwErrorCode = AuditUserA( "david", "Pentium2" );
	if( dwErrorCode == ERROR_SUCCESS ) {
		printf ( "OK.\n" );
	}
	else 
		printf ( "Error(%ld).\n", dwErrorCode );

	dwErrorCode = AuditUser( "david", "Pentium2" );
	if( dwErrorCode == ERROR_SUCCESS ) {
		printf ( "OK.\n" );
	}
	else 
		printf ( "Error(%ld).\n", dwErrorCode );

}
