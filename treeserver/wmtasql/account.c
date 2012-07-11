/******************************
*	Module Name:
*		ACCOUNT.C
*
*	Abstract:
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) 1997  China Railway Software Corporation
*
*	Revision History:
*		Write by NiuJingyu, 1997.10
********************************************************************************/

#define STRICT

#include <windows.h>
#include <lm.h>

#include "account.h"

LPWSTR MakeComputerName ( LPWSTR lpwServer, LPWSTR lpwComputerName )
{
    DWORD cchServer;

    if ( lpwServer == NULL || *lpwServer == L'\0' ) {
        SetLastError ( ERROR_INVALID_COMPUTERNAME );
        return NULL;
    }

    if ( lpwComputerName == NULL ) {
        SetLastError ( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    cchServer = lstrlenW ( lpwServer );

    if ( lpwServer[0] != L'\\' && lpwServer[1] != L'\\' ) {
		lpwComputerName[0] =  L'\\';
		lpwComputerName[1] =  L'\\';
		lpwComputerName[2] =  L'\x0';
    }
    else {
        cchServer -= 2;
        lpwComputerName[0] = L'\0';
    }

    if ( cchServer > CNLEN ) {
        SetLastError ( ERROR_INVALID_COMPUTERNAME );
        return NULL;
    }

	if ( lstrcatW ( lpwComputerName, lpwServer ) == NULL ) 
		return NULL;

	return lpwComputerName;
}

BOOL GetUserHomeDir ( LPCSTR lpServer, LPCSTR lpUserName, LPSTR lpHomeDir )
{
	PUSER_INFO_1 lpUserInfo;
    NET_API_STATUS nas;
	WCHAR wcComputerName[CNLEN+3];
	WCHAR wcServer[CNLEN], wcUserName[UNLEN];
	BOOL isLocal = TRUE;
	DWORD cchBuffer;

	if ( lpServer == NULL || *lpServer == '\0' ) {
        SetLastError ( ERROR_INVALID_ACCOUNT_NAME );
        return FALSE;
	}

	if ( lpUserName == NULL || *lpUserName == '\0' ) {
        SetLastError ( ERROR_INVALID_ACCOUNT_NAME );
        return FALSE;
	}

	mbstowcs ( wcServer, lpServer, lstrlen ( lpServer )+1 );
	mbstowcs ( wcUserName, lpUserName, lstrlen ( lpUserName )+1 );

	if ( !MakeComputerName ( wcServer, (LPWSTR)wcComputerName ) )
		return FALSE;

	if ( lstrlenW ( wcUserName ) > UNLEN ) {
        SetLastError ( ERROR_INVALID_ACCOUNT_NAME );
        return FALSE;
	}

    cchBuffer = CNLEN+3;

	nas = NetUserGetInfo ( wcComputerName, wcUserName, 1, (LPBYTE *) &lpUserInfo );
    
	if ( nas != NERR_Success ) {
		SetLastError ( nas );
		return FALSE;
	}

	wcstombs ( lpHomeDir, lpUserInfo->usri1_home_dir, MAX_PATH );
	NetApiBufferFree ( (LPBYTE) lpUserInfo );
	return TRUE;
}

/*
/////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>

void main ( void )
{
	char szHomeDir[MAX_PATH];
	DWORD dwErrorCode;
	char UserName[80], ComputerName[80];

	printf ( "Computer name: " );
	gets ( ComputerName	);
	printf ( "User name: " );
	gets ( UserName );
	if ( !GetUserHomeDir ( ComputerName, UserName, szHomeDir ) ) {
		dwErrorCode = GetLastError ();
		printf ( "GetUserHomeDir() error (return code: %ld).\n", dwErrorCode );
		return;
	}

	printf ( "Home directory: %s\n", szHomeDir );
}
*/