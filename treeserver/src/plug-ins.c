//
// by Jingyu Niu 1999.09
//

#include <windows.h>
#include <tchar.h>
#include <io.h>
#include "TSCommon.h"
#include "tplug-ins.h"

static LPTS_PLUGINS_ENTRY lpPlugInsEntry;

VOID RegisterPlugIns ( LPTS_PLUGINS lpPlugIns )
{
	lpPlugIns->lpSysUse2 = (LPVOID)lpPlugInsEntry;
	if( lpPlugInsEntry ) 
		lpPlugInsEntry->lpSysUse1 = lpPlugIns;
	lpPlugInsEntry = lpPlugIns;
}

DWORD InitPlugIns ( VOID )
{
	TCHAR szPlugInsRoot[MAX_PATH];
	TCHAR szPlugInsProfile[MAX_PATH];
	DWORD cbPlugInsRoot = MAX_PATH;
	TCHAR szCurrentDir[MAX_PATH];
	LPTSTR lpSectionData;
	DWORD cbSectionData;
	DWORD cbReaded;
	DWORD dwRetCode;
	LPTSTR lpKey, lpValue;

	GetTreeServerRoot( szPlugInsRoot, &cbPlugInsRoot );
	lstrcat( szPlugInsRoot, TEXT("\\plug-ins") );
	if( _taccess( szPlugInsRoot, 0 ) == -1 ) 
		return TERR_NO_PLUGINS;

	GetCurrentDirectory( MAX_PATH, szCurrentDir );
	SetCurrentDirectory( szPlugInsRoot );

	wsprintf( szPlugInsProfile, TEXT("%s\\%s"), szPlugInsRoot, TEXT("plug-ins.ini") );
	if( _taccess( szPlugInsProfile, 0 ) == -1 ) 
		return TERR_NO_PLUGINS;

	cbSectionData = 2048;
	lpSectionData = (LPTSTR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, cbSectionData );
	if( !lpSectionData )
		return GetLastError();

	while( TRUE ) {
		cbReaded = GetPrivateProfileSection(  // Get the entry section data
				TEXT("plug-ins"), 
				lpSectionData, 
				cbSectionData, 
				szPlugInsProfile );
		
		if( cbReaded == cbSectionData-2 ) {	// Data buffer to small
			LPTSTR p;

			cbSectionData += 2048;
			p = (LPTSTR)HeapReAlloc( 
					GetProcessHeap,
					HEAP_ZERO_MEMORY,
					lpSectionData,
					cbSectionData );
			if( !p ) {
				dwRetCode = GetLastError();
				HeapFree( GetProcessHeap(), 0, lpSectionData );
				return dwRetCode;
			}

			lpSectionData = p;
		}
		else
			break;
	}

	// Now read each plug-ins startup information and load the it
	lpKey = lpSectionData;
	while( TRUE ) {
		LPTS_PLUGINS lpPlugIns;
		LPTSTR lpFileName;
		int pos;
		CHAR szProcName[MAX_PLUGINS_ENTRY_NAME];

		lpPlugIns = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( TS_PLUGINS ) );
		if( !lpPlugIns ) {
			dwRetCode = GetLastError();
			HeapFree( GetProcessHeap(), 0, lpSectionData );
			return dwRetCode;
		}

		pos = _tcscspn( lpKey, TEXT("=") );
		lpValue = lpKey + pos + 1; // Plug-ins startup information section name

		GetPrivateProfileString( 
				lpValue,
				TEXT("Name"),
				lpValue,
				lpPlugIns->szName,
				MAX_PLUGINS_NAME,
				szPlugInsProfile );

		GetPrivateProfileString( 
				lpValue,
				TEXT("Image"),
				TEXT("\x0"),
				lpPlugIns->szImage,
				MAX_PATH,
				szPlugInsProfile );

		GetPrivateProfileString( 
				lpValue,
				TEXT("Parameter"),
				TEXT("\x0"),
				lpPlugIns->szParameter,
				MAX_PATH,
				szPlugInsProfile );

		GetPrivateProfileString( 
				lpValue,
				TEXT("Provider"),
				TEXT("(Unknown)"),
				lpPlugIns->szProvider,
				MAX_PLUGINS_PROVIDER,
				szPlugInsProfile );

		GetPrivateProfileString( 
				lpValue,
				TEXT("Entry"),
				TEXT("PlugInEntry"),
				szProcName,
				MAX_PLUGINS_ENTRY_NAME,
				szPlugInsProfile );

		GetFullPathName( lpPlugIns->szImage, MAX_PATH, lpPlugIns->szImage, &lpFileName );

		lpPlugIns->hLibrary = LoadLibrary( lpPlugIns->szImage );
		if( lpPlugIns->hLibrary ) {
			lpPlugIns->lpPlugInsMain = (PLUGINS_PROC)GetProcAddress( lpPlugIns->hLibrary, szProcName );
			if( lpPlugIns->lpPlugInsMain ) {
				__try {
					if( lpPlugIns->lpPlugInsMain( lpPlugIns ) == 0 ) {
						FreeLibrary( lpPlugIns->hLibrary );
						lpPlugIns->hLibrary = NULL;
						lpPlugIns->tspStatus = tspLoadFail;
					}
					else {
						lpPlugIns->tspStatus = tspLoadSucceed;
					}						
				}
				__except( EXCEPTION_EXECUTE_HANDLER ) {
					FreeLibrary( lpPlugIns->hLibrary );
					lpPlugIns->hLibrary = NULL;
					lpPlugIns->tspStatus = tspLoadFail;
				}
			}
			else {
				FreeLibrary( lpPlugIns->hLibrary );
				lpPlugIns->hLibrary = NULL;
				lpPlugIns->tspStatus = tspIncorrectImage;
			}
		}
		else {
			lpPlugIns->tspStatus = tspIncorrectImage;
		}

		RegisterPlugIns( lpPlugIns );

		// Next plugins
		lpKey += lstrlen( lpKey ) + 1;
		if( *lpKey == 0 )	// End of all plug-ins 
			break;
	}

	SetCurrentDirectory( szCurrentDir );
	HeapFree( GetProcessHeap(), 0, lpSectionData );
	return TRUE;
}

VOID UnloadPlugIns ( VOID )
{
	LPTS_PLUGINS p;

	while( lpPlugInsEntry ) {
		if( lpPlugInsEntry->hLibrary )
			FreeLibrary( lpPlugInsEntry->hLibrary );

		p = lpPlugInsEntry;
		lpPlugInsEntry = (LPTS_PLUGINS)lpPlugInsEntry->lpSysUse2;
		HeapFree( GetProcessHeap(), 0, p );
	}
}
