/******************************
*	Module Name:
*		TENGINE.C
*
*	Abstract:
*		Tree Server Transaction Engine management module. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1997  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1997.10
********************************************************************************/

#define STRICT
#include <windows.h>
#include <tchar.h>

#include "terror.h"
#include "tlimits.h"
#include "tengine.h"
#include "wmtasql.h"
#include "tscommon.h"

LPTE_INFO g_lpTEngineInfo = NULL;
extern TCHAR **savedArgv;
extern TCHAR szTreeServerRoot[MAX_PATH];

CHAR interTE[][MAX_TED_COMMAND+1] = {
	"ASQL",
	"FTP",
	"\x0" };

DWORD GetAvailableTED ( LPTSTR lpszTEDBuffer, LPDWORD lpcchBuffer )
{
	TCHAR szTEngineProfile[MAX_PATH];
	//DWORD cbProfile;
	DWORD dwRetCode;
	LPTSTR lpPtr;
	INT iLength;
	INT i;

	if( lpszTEDBuffer == NULL || lpcchBuffer == NULL )
		return ERROR_INVALID_PARAMETER;

	if( *lpcchBuffer == 0 )
		return ERROR_MORE_DATA;

	/*
	cbProfile = MAX_PATH;
	GetTSProfilePath( szTEngineProfile, &cbProfile );
	lstrcat( szTEngineProfile, _TEXT( "\\TEngine.ini" ) );
	*/

	lstrcpy( szTEngineProfile, szTreeServerRoot );
	lstrcat( szTEngineProfile, _TEXT( "\\Config\\TEngine.ini" ) );


	dwRetCode = GetPrivateProfileSection( 
			_TEXT( "Transaction Engine" ),
			lpszTEDBuffer,
			*lpcchBuffer,
			szTEngineProfile );

	if( dwRetCode == (*lpcchBuffer)-2 )
		return ERROR_MORE_DATA;

	if( dwRetCode == 0 )
		return TERR_NO_INVALID_TED;

	lpPtr = lpszTEDBuffer;
	i = 0;
	
	do {
		iLength = lstrlen( lpPtr );
		lpPtr = lpPtr + iLength + 1;
		i++;
	} while( *lpPtr != _TEXT( '\x0' ) );

	*lpcchBuffer = (DWORD) i;

	return TERR_SUCCESS;
}

DWORD GetTEDInfo ( LPCTSTR lpszAlias, LPTED_INFO lpTEDInfo )
{
	TCHAR szTEngineProfile[MAX_PATH];
	DWORD cbProfile;
	TCHAR szBuffer[512];
	DWORD dwRetCode;

	if( lpszAlias == NULL || lpTEDInfo == NULL )
		return ERROR_INVALID_PARAMETER;

	cbProfile = MAX_PATH;
	GetTSProfilePath( szTEngineProfile, &cbProfile );
	lstrcat( szTEngineProfile, _TEXT( "\\TEngine.ini" ) );

	lstrcpy( lpTEDInfo->szAlias, lpszAlias );

	dwRetCode = GetPrivateProfileString( 
			lpszAlias,
			_TEXT( "Engine" ),
			_TEXT( "\x0" ),
			szBuffer,
			512,
			szTEngineProfile );
	
	if( *szBuffer == _TEXT( '\x0' ) )
		return TERR_INVALID_TED_LIBRARY;

	lstrcpy( lpTEDInfo->szEngine, szBuffer );
	
	dwRetCode = GetPrivateProfileString( 
			lpszAlias,
			_TEXT( "Command" ),
			_TEXT( "\x0" ),
			szBuffer,
			512,
			szTEngineProfile );

	if( *szBuffer == _TEXT( '\x0' ) || lstrlen( szBuffer ) > MAX_TED_COMMAND )
		return TERR_INVALID_TED_COMMAND;

	lstrcpy( lpTEDInfo->szCommand, szBuffer );

	dwRetCode = GetPrivateProfileString( 
			lpszAlias,
			_TEXT( "EntryFunction" ),
			_TEXT( "\x0" ),
			szBuffer,
			512,
			szTEngineProfile );

	if( *szBuffer == _TEXT( '\x0' ) || lstrlen( szBuffer ) > MAX_TED_FUNCTION )
		return TERR_INVALID_TED_ENTRY;

	lstrcpy( lpTEDInfo->szEntryProc, szBuffer );
	
	lpTEDInfo->dwCapability = 0;

	dwRetCode = GetPrivateProfileString( 
			lpszAlias,
			_TEXT( "Capability" ),
			_TEXT( "\x0" ),
			szBuffer,
			512,
			szTEngineProfile );

	if( lstrlen( szBuffer ) != MAX_TED_CAPABILITY )
		return TERR_INVALID_TED_CAPABILITY;

	if( szBuffer[0] == _TEXT( 'm' ) || szBuffer[0] == _TEXT( 'M' ) )
		lpTEDInfo->dwCapability |= TED_MULTI_THREAD;
	else if( szBuffer[0] != _TEXT( '-' ) )
		return TERR_INVALID_TED_CAPABILITY;

	if( szBuffer[1] == _TEXT( 'e' ) || szBuffer[1] == _TEXT( 'E' ) )
		lpTEDInfo->dwCapability |= TED_EXCLUSIVE;
	else if( szBuffer[1] != _TEXT( '-' ) )
		return TERR_INVALID_TED_CAPABILITY;

	if( szBuffer[2] == _TEXT( 's' ) || szBuffer[2] == _TEXT( 'S' ) )
		lpTEDInfo->dwCapability |= TED_SCHEDULE;
	else if( szBuffer[2] != _TEXT( '-' ) )
		return TERR_INVALID_TED_CAPABILITY;

	dwRetCode = (DWORD)GetPrivateProfileInt( 
			lpszAlias,
			_TEXT( "EnableTimer" ),
			0,
			szTEngineProfile );

	if( dwRetCode == 0 )
		lpTEDInfo->fEnableTimer = FALSE;
	else if( dwRetCode == 1 )
		lpTEDInfo->fEnableTimer = TRUE;
	else 
		return TERR_INVALID_TED_TIMER;

	dwRetCode = (DWORD)GetPrivateProfileInt( 
			lpszAlias,
			_TEXT( "EnableIdle" ),
			0,
			szTEngineProfile );

	if( dwRetCode == 0 )
		lpTEDInfo->fEnableIdle = FALSE;
	else if( dwRetCode == 1 )
		lpTEDInfo->fEnableIdle = TRUE;
	else 
		return TERR_INVALID_TED_IDLE;

	lpTEDInfo->dwTimeOut = (DWORD)GetPrivateProfileInt( 
			lpszAlias,
			_TEXT( "TimeOut" ),
			60000,
			szTEngineProfile );

	if( (LONG)lpTEDInfo->dwTimeOut < 0 )
		return TERR_INVALID_TED_TIMEOUT;

	dwRetCode = (DWORD)GetPrivateProfileInt( 
			lpszAlias,
			_TEXT( "Enable" ),
			1,
			szTEngineProfile );

	if( dwRetCode == 0 )
		lpTEDInfo->fEnable = FALSE;
	else if( dwRetCode == 1 )
		lpTEDInfo->fEnable = TRUE;
	else 
		return TERR_INVALID_TED_ENABLE;


	return TERR_SUCCESS;
}

LPTE_INFO TELoader ( VOID )
{
	TED_INFO TEDInfo;
	HLOCAL hTEInfo;
	LPTE_INFO lpTEInfo;
	INT iCurLoadTED;

	TCHAR szBuffer[2048];
	DWORD cchBuffer;
	TCHAR szAlias[512];
	INT iAlias;
	LPTSTR lpTEDAlias;
	INT iTEDAliasLen;
	INT iTEDNum;

	DWORD dwRetCode;
	
	HANDLE hDll;
	FARPROC lpfnEntryProc;

	INT i;

	cchBuffer = 2048;
	dwRetCode = GetAvailableTED( szBuffer, &cchBuffer );
	if( dwRetCode == TERR_NO_INVALID_TED ) {
		SetLastError( TERR_NO_INVALID_TED );
		return NULL;
	}

	iTEDNum = (INT)cchBuffer;

	hTEInfo = LocalAlloc( LPTR, (iTEDNum+2) * sizeof( TE_INFO ) );
	if( hTEInfo == NULL ) {
		return NULL;
	}

	lpTEInfo = (LPTE_INFO)LocalLock( hTEInfo );
	if( lpTEInfo == NULL ) {
		LocalFree( hTEInfo );
		return NULL;
	}

	if( ProcessAttach() != ERROR_SUCCESS ) {
		LocalUnlock( hTEInfo );
		LocalFree( hTEInfo );
		return NULL;
	}

	iCurLoadTED = 0;
	lpTEDAlias = szBuffer;
	
	for( i = 0; i <= iTEDNum; i++ ) {
		iTEDAliasLen = lstrlen( lpTEDAlias );

		for( iAlias = 0; lpTEDAlias[iAlias] != _TEXT( '=' ) 
				&& lpTEDAlias[iAlias] != _TEXT( '\x0' ); iAlias++ );

		lstrcpyn( szAlias, lpTEDAlias, iAlias+1 );

		lpTEDAlias = lpTEDAlias + iTEDAliasLen + 1;

		dwRetCode = GetTEDInfo( szAlias, &TEDInfo );
		 
		if( dwRetCode != TERR_SUCCESS )
			continue;
	
		if( TEDInfo.fEnable == TRUE ) {
			hDll = LoadLibrary( TEDInfo.szEngine );
			if( hDll == NULL ) {
				dwRetCode = GetLastError();
				lpfnEntryProc = NULL;
			}
			else {
				lpfnEntryProc = (FARPROC)GetProcAddress( hDll, _TEXT( "ProcessAttach" ) );
				if( lpfnEntryProc != NULL )	{
					if( (*lpfnEntryProc)() != ERROR_SUCCESS ) {
						dwRetCode = GetLastError();
						FreeLibrary( hDll );
						lpfnEntryProc = NULL;
						goto __Failure;
					}
				}
				
#ifdef  _UNICODE
				{
					CHAR szTempBuffer[MAX_TED_FUNCTION+1];
			
					wcstombs( szTempBuffer, TEDInfo.szEntryProc, MAX_TED_FUNCTION+1 );
					lpfnEntryProc = GetProcAddress( hDll, szTempBuffer );
				}
#else
				{
					lpfnEntryProc = GetProcAddress( hDll, TEDInfo.szEntryProc );
				}
#endif

				if( lpfnEntryProc == NULL ) {
					dwRetCode = GetLastError();
					FreeLibrary( hDll );
				}
			}
			
__Failure:
			lstrcpy( lpTEInfo[iCurLoadTED].szAlias, TEDInfo.szAlias );
			lstrcpy( lpTEInfo[iCurLoadTED].szCommand, TEDInfo.szCommand );
			lpTEInfo[iCurLoadTED].dwRequest = (DWORD)iCurLoadTED + TED_USER_BASE_REQUEST;
			lpTEInfo[iCurLoadTED].dwCapability = TEDInfo.dwCapability;
			lpTEInfo[iCurLoadTED].lpfnEntryProc = lpfnEntryProc;
			lpTEInfo[iCurLoadTED].hLibrary = hDll;
			lpTEInfo[iCurLoadTED].fEnableTimer = TEDInfo.fEnableTimer; 
			lpTEInfo[iCurLoadTED].fEnableIdle = TEDInfo.fEnableIdle; 
			lpTEInfo[iCurLoadTED].dwTimeOut = TEDInfo.dwTimeOut;
			lpTEInfo[iCurLoadTED].fEnable = TEDInfo.fEnable;
		}
		else {
			lstrcpy( lpTEInfo[iCurLoadTED].szAlias, TEDInfo.szAlias );
			lstrcpy( lpTEInfo[iCurLoadTED].szCommand, TEDInfo.szCommand );
			lpTEInfo[iCurLoadTED].dwRequest = (DWORD)iCurLoadTED + TED_USER_BASE_REQUEST;
			lpTEInfo[iCurLoadTED].dwCapability = TEDInfo.dwCapability;
			lpTEInfo[iCurLoadTED].lpfnEntryProc = NULL;
			lpTEInfo[iCurLoadTED].hLibrary = NULL;
			lpTEInfo[iCurLoadTED].fEnableTimer = TEDInfo.fEnableTimer; 
			lpTEInfo[iCurLoadTED].fEnableIdle = TEDInfo.fEnableIdle; 
			lpTEInfo[iCurLoadTED].dwTimeOut = TEDInfo.dwTimeOut;
			lpTEInfo[iCurLoadTED].fEnable = TEDInfo.fEnable;
		}

		iCurLoadTED++;
	}

	return lpTEInfo;
}

DWORD TEFree ( VOID )
{
	LPTE_INFO lpPtr;
	HLOCAL hTEInfo;
	DWORD dwRetCode;
	FARPROC lpfn;

	dwRetCode = TERR_SUCCESS;

	if( g_lpTEngineInfo == NULL )
		return ERROR_INVALID_PARAMETER;

	ProcessDetach();
	lpPtr = g_lpTEngineInfo;
	for( ++lpPtr; *(lpPtr->szAlias) !=	_TEXT( '\x0' ); lpPtr++ ) {
		if( lpPtr->hLibrary ) {
			lpfn = (FARPROC)GetProcAddress( lpPtr->hLibrary, _TEXT( "ProcessDetach" ) );
			if( lpfn != NULL ) {
				(*lpfn)();
				FreeLibrary( lpPtr->hLibrary );
			}
		}
	}

	hTEInfo = LocalHandle( g_lpTEngineInfo );
	if( hTEInfo == NULL )
		dwRetCode = GetLastError();
	else
		LocalFree( hTEInfo );

	return dwRetCode;
}

DWORD GetTERequest ( LPSTR lpCommand )
{
	TCHAR szTempBuffer[MAX_TED_COMMAND+1];
	LPTE_INFO lpPtr;
	int i;

	if( lpCommand == NULL ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return 0;
	}

	for( i = 0; interTE[i][0] != '\x0'; i++ ) {
		if( strcmpi( lpCommand, interTE[i] ) == 0 )
			return i+1;
	}

	if( g_lpTEngineInfo == NULL ) {
		SetLastError( TERR_NO_TEINFO_ENTRY );
		return 0;
	}

#ifdef  _UNICODE
	mbstowcs( szTempBuffer, lpCommand, MAX_TED_COMMAND+1 );
#else
	lstrcpy(  szTempBuffer, lpCommand );
#endif

	for( lpPtr = g_lpTEngineInfo; *(lpPtr->szAlias) != _TEXT( '\x0' ); lpPtr++ ) {
		if( strcmpi( lpPtr->szCommand, lpCommand ) == 0 )
			break;
	}

	if( *(lpPtr->szAlias) == _TEXT( '\x0' ) ) {
		SetLastError( TERR_INVALID_COMMAND );
		return 0;
	}

	return lpPtr->dwRequest;
}

FARPROC GetTEEntryAddress ( DWORD dwRequest )
{
	LPTE_INFO lpPtr;

	if( dwRequest < TED_USER_BASE_REQUEST ) {
		SetLastError( TERR_INVALID_REQUEST );
		return NULL;
	}

	if( g_lpTEngineInfo == NULL ) {
		SetLastError( TERR_NO_TEINFO_ENTRY );
		return NULL;
	}

	SetLastError( TERR_SUCCESS );
	
	for( lpPtr = g_lpTEngineInfo; *(lpPtr->szAlias) != _TEXT( '\x0' ); lpPtr++ ) {
		if( lpPtr->dwRequest == dwRequest )
			break;
	}

	if( *(lpPtr->szAlias) == _TEXT( '\x0' ) ) {
		SetLastError( TERR_INVALID_REQUEST );
		return NULL;
	}

	if( lpPtr->lpfnEntryProc == NULL ) {
		if( lpPtr->fEnable == FALSE )
			SetLastError( TERR_TED_DISABLE );
		else {
			if( lpPtr->hLibrary == NULL )
				SetLastError( TERR_TED_LAOD_FAILURE );
			else
				SetLastError( TERR_TED_ERROR_ENTRY );
		}
	}
	
	return lpPtr->lpfnEntryProc;
}

/*
#include <stdio.h>

void main( void )
{
	DWORD dwRetCode;
	FARPROC lpfnProc;
	CHAR szCommand[80];
	DWORD dwRequest;

	lpTEngineInfo = TELoader();
	
	if( lpTEngineInfo == NULL )
		dwRetCode = GetLastError();

	while( 1 ) {
		gets( szCommand );

		dwRequest = GetTERequest( szCommand );
		if( dwRequest == 0 ) {
			printf( "ErrorCode: %ld.\n", GetLastError() );
			continue;
		}

		lpfnProc = GetTEEntryAddress( dwRequest );
		if( lpfnProc ) {
			printf( "Current entry address: %ld.\n", lpfnProc );
//			(*lpfnProc)();
		}
		else {
			printf( "Error cdoe: %ld.\n", GetLastError() );
		}

	}

	if( lpTEngineInfo )
		TEFree( lpTEngineInfo );
}
*/