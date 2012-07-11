/****************************** 
*	Module Name:
*		TREESVR.C
*
*	Abstract:
*		Tree Server application entry module. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1997  China Railway Software Corporation
*
*	Revision History:
*		Write by NiuJingyu, 1997.10
*		Update by NiuJingyu, 1998.03
********************************************************************************
* 
* compiled defination:
* WIN32,NDEBUG,_MT,_NTSDK,_CONSOLE,aTREESVR_STANDALONE
*						-------------
*
* link defination:
* kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib 
* ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console 
*																----------------------
* /incremental:no /pdb:"Release/TreeSvr.pdb" /machine:I386 /out:"Release/TreeSvr.exe" 
*
* console or _CONSOLE means that it is SERVER version
* windows means that it is STANDALONE version
********************************************************************************/



#define STRICT

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <lm.h>

#include "resource.h"
#include "tuser.h"
#include "treesvr.h"
#include "des.h"
#include "tplug-ins.h"

//#define TREESVR_STANDALONE 

#ifdef TRIAL_VERSION
typedef int (*TRIALCK)( void );
#endif

#ifdef TREESVR_STANDALONE
#include "Notify.h"
#endif

//////////////////////////////////////////////////////////////////////////////
//// todo: change to desired strings
////
// name of the executable
#define SZAPPNAME            "TBroker"
// internal name of the service
#define SZSERVICENAME        "TBroker"
// displayed name of the service
#define SZSERVICEDISPLAYNAME "TBroker Service"
// list of service dependencies - "dep1\0dep2\0\0"
#define SZDEPENDENCIES       ""
//////////////////////////////////////////////////////////////////////////////

// Add by Jingyu Niu 1999.09
// This is for support plug-ins


extern LPQUEUE g_lpSharedQueue;
extern LPEXCHNG_BUF_INFO g_lpExchBufInfo;
extern LPTE_INFO g_lpTEngineInfo;
SYSTEM_RES_INFO SystemResInfo;
TCHAR szTreeServerRoot[MAX_PATH];
TCHAR szAuthServer[CNLEN+1];
TCHAR **savedArgv;

HANDLE hServerStopEvent;
SERVICE_STATUS          ssStatus;       // current status of the service
SERVICE_STATUS_HANDLE   sshStatusHandle;
TCHAR                   szErr[256];

BOOL bDebug = FALSE;
static CRITICAL_SECTION cs;
HANDLE hLogFile;

// Added by Motor, 8 August, 1998
// This variable is used for accessing SYSTEM_RES_INFO::lpClientResInfo
CRITICAL_SECTION CriticalSection_User;

int lprintfs(char *format, ...)
{
    char buf[1024];
    int cb;
    va_list arg_ptr;

    va_start(arg_ptr, format);
	cb = _vsnprintf(buf, sizeof(buf), format, arg_ptr);

#ifdef TREESVR_STANDALONE 

	
	DlgShowMsg(hSvrDlg, buf);

#else

	EnterCriticalSection( &cs );
	if( !bDebug ) {
		DWORD dwWr;
		WriteFile( 
				hLogFile,
				buf,
				cb,
				&dwWr,
				NULL );

		FlushFileBuffers( hLogFile );
	}
	else 
		_tprintf( buf );

	LeaveCriticalSection( &cs );

#endif

	return cb;
}

DWORD CreateTreeSVRUserDB ( VOID ) 
{
	CHAR szUserDB[MAX_PATH];
	DWORD dwBufferLen;
	DWORD dwRetCode;
	USER_INFO UserInfo;

	dwBufferLen = MAX_PATH;
	dwRetCode = GetTreeServerRoot( szUserDB, &dwBufferLen );
	if( dwRetCode != 0 )
		return dwRetCode;

	lstrcat( szUserDB, _TEXT( "\\Config\\User.DB" ) );

	if( !TCreateUserDB ( szUserDB ) ) {
		return GetLastError();
	}

	if( !TOpenUserDB ( szUserDB ) ) {
		return GetLastError();
	}

	ZeroMemory( (PVOID)&UserInfo, sizeof( USER_INFO ) );
	
	lstrcpy( UserInfo.szUser, "Admin" );

	{
		CHAR sztemp[32];

		DES("TREEADMIN", "TREEADMIN", sztemp);
		lstrcpy( UserInfo.szPasswd, sztemp );
	}

	//UserInfo.szHomeDir;
	FillMemory( (PVOID)UserInfo.bLogonHours, MAX_LOGON_HOURS, 0xFF );
	FillMemory( (PVOID)UserInfo.szAccessMask, MAX_ACCESS_MASK, 0xFF );
	lstrcpy( UserInfo.szDescription, "TBroker administrator account." );
	UserInfo.cLogonFail = '\x0';
	UserInfo.cLocked = '\x0';

	dwRetCode = TUserAdd( &UserInfo );

	TCloseUserDB();
	return dwRetCode;
}

//
//  FUNCTION: ServiceStart
//
//  PURPOSE: Actual code of the service
//           that does the work.
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    Start the services and begain to work.



DWORD ServiceStart ( DWORD dwArgc, LPTSTR *lpszArgv )
{
	DWORD	dwBufferLen;
	TCHAR	szBuffer[MAX_PATH];
	CHAR	szUserDB[MAX_PATH], szSProc[MAX_PATH];
	DWORD	dwUsers;
	static  HLOCAL	hMemory;
	TCHAR	szComputer[CNLEN+1];
	DWORD	dwErrorCode = TERR_SUCCESS;

#if defined(TRIAL_VERSION) && !defined(TREESVR_STANDALONE)
	HMODULE hLib;
	TRIALCK p;
	int d;

	hLib = LoadLibrary( "TRIALCK.DLL" );
	if( hLib == NULL ) {
#ifdef TREESVR_STANDALONE 
		MessageBox( 0, "软件的试用协议被破坏，试用协议失效。", "Trial Check", MB_OK | MB_ICONINFORMATION );
#endif
		ExitProcess( 0 );
	}

	p = (TRIALCK)GetProcAddress( hLib, (LPTSTR)102 );
	d = (*p)();
	FreeLibrary( hLib );
	if( d < 0 ) {
#ifdef TREESVR_STANDALONE 
		MessageBox( 0, "软件的试用协议被破坏，试用协议失效。", "Trial Check", MB_OK | MB_ICONINFORMATION );
#endif
		ExitProcess( 0 );
	}

	if( d == 0 ) {
#ifdef TREESVR_STANDALONE 
		MessageBox( 0, "软件的试用协议期限已满，请购买正式版。", "Trial Check", MB_OK | MB_ICONINFORMATION );
#endif
		ExitProcess( 0 );
	}
#endif /* TRIAL_VERSION */

	__try {

	if (dwArgc==666666)
		__leave;
		///////////////////////////////////////////////////
		//
		// Service initialization
		//

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				3000) )                // wait hint
			__leave;
#endif

		dwBufferLen = CNLEN+1;
		if( !GetComputerName( szComputer, &dwBufferLen ) ) {
			*szComputer = _TEXT( '\x0' );
		}
	
#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				3000) )                // wait hint
			__leave;
#endif

		// create the event object. The control handler function signals
		// this event when it receives the "stop" control code.
		//
		hServerStopEvent = CreateEvent(
				NULL,    // no security attributes
				TRUE,    // manual reset event
				FALSE,   // not-signalled
				TEXT( "$TS_STOP$" ) );   // no name

		if( hServerStopEvent == NULL ) {
			dwErrorCode = GetLastError();
			__leave;
		}
	
#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				3000) )                // wait hint
			__leave;
#endif
		
		dwBufferLen = MAX_PATH;
		dwErrorCode = GetTreeServerRoot( szTreeServerRoot, &dwBufferLen );
		if( dwErrorCode != TERR_SUCCESS	) {
			char  *sz;

			_fullpath(szTreeServerRoot, savedArgv[0], MAX_PATH);
			//lstrcpy(szTreeServerRoot, savedArgv[0]);
			sz = strrchr(szTreeServerRoot, '\\');
			if( sz != NULL ) {
				*sz = '\0';
			}
			sz = strrchr(szTreeServerRoot, '\\');
			if( sz != NULL ) {
				*sz = '\0';
			}

			lprintfs( "Failed to get TBroker root directory from environment: TREESVR=?.\n"
					  "\tuse default path    [%s]\n",szTreeServerRoot);
			//__leave;
		}

		// Create Log file.
		lstrcpy( szBuffer, szTreeServerRoot );
		lstrcat( szBuffer, _TEXT( "\\TBroker.Service.Log" ) );
		hLogFile = CreateFile( 
				szBuffer,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL );

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				3000) )                // wait hint
			__leave;
#endif
		
		lstrcpy( szBuffer, szTreeServerRoot );
		lstrcat( szBuffer, _TEXT( "\\bin" ) );
		if( !SetCurrentDirectory( szBuffer ) ) {
			dwErrorCode = GetLastError();
			lprintfs( "Failed to initialize Tree Server root directory.\n" );
			__leave;
		}
	
#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				3000) )                // wait hint
			__leave;
#endif
		
		lstrcpy( szBuffer, szTreeServerRoot );
		lstrcat( szBuffer, _TEXT( "\\Config\\TreeSvr.ini" ) );
	
		dwErrorCode = GetPrivateProfileString( 
				_TEXT( "Server" ),
				_TEXT( "AuthServer" ),
				szComputer,
				szAuthServer,
				CNLEN,
				szBuffer );
	
		if( *szAuthServer == _TEXT( '\x0' ) ) {
			dwErrorCode = TERR_INVALID_AUTHSERVER;
			__leave;
		}
	
		dwUsers = (DWORD)GetPrivateProfileInt( 
				_TEXT( "Server" ),
				_TEXT( "Users" ),
				20,
				szBuffer );
	
		SystemResInfo.dwCurrentUsers = 0;
		SystemResInfo.dwUsers = dwUsers;
		
#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				3000) )                // wait hint
			__leave;
#endif

		hMemory = LocalAlloc( LPTR, sizeof(CLIENT_RES_INFO) * dwUsers );
		if( hMemory == NULL ) {
			dwErrorCode = GetLastError();
			lprintfs( "Failed to initialize user's information structure.\n" );
			__leave;
		}

		SystemResInfo.lpClientResInfo = (LPCLIENT_RES_INFO)LocalLock( hMemory );
		if( SystemResInfo.lpClientResInfo == NULL ) {
			dwErrorCode = GetLastError();
			lprintfs( "Failed to initialize user's information structure.\n" );
			__leave;
		}
		ZeroMemory( SystemResInfo.lpClientResInfo, sizeof( CLIENT_RES_INFO ) * dwUsers );
		lprintfs( "Initialize user's information structure successfully.\n" );

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				3000) )                // wait hint
			__leave;
#endif

		g_lpSharedQueue = InitQueue( dwUsers, &dwErrorCode );
		if( dwErrorCode != TERR_SUCCESS	) {
			lprintfs( "Failed to initialize shared queue.\n" );
			__leave;
		}
		SystemResInfo.lpSharedQueue = g_lpSharedQueue;

		lprintfs( "Initialize shared queue successfully.\n" );

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				3000) )                // wait hint
			__leave;
#endif
	
		g_lpExchBufInfo = InitExchngBuf( dwUsers, &dwErrorCode );
		if( g_lpExchBufInfo == NULL ) {
			dwErrorCode = GetLastError();
			lprintfs( "Failed to initialize exchange buffer.\n" );
			__leave;
		}
		SystemResInfo.lpExchBufInfo = g_lpExchBufInfo;

		lprintfs( "Initialize exchange buffer successfully.\n" );

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				5000) )                // wait hint
			__leave;
#endif

		g_lpTEngineInfo = TELoader();
		if( g_lpTEngineInfo == NULL ) {
			dwErrorCode = GetLastError();
			lprintfs( "Failed to load Transaction Engine.\n" );
			__leave;
		}
		SystemResInfo.lpTEngineInfo = g_lpTEngineInfo;

		lprintfs( "Load Transaction Engine successfully.\n" );

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				10000) )                // wait hint
			__leave;
#endif

		dwErrorCode = InitSchedule();
		if( dwErrorCode != TERR_SUCCESS ) {
			lprintfs( "Failed to initialize schedule thread.\n" );
			__leave;
		}

		lprintfs( "Initialize schedule thread successfully.\n" );

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				3000) )                // wait hint
			__leave;
#endif

		lstrcpy( szUserDB, szTreeServerRoot );
		lstrcat( szUserDB, _TEXT( "\\Config\\User.DB" ) );
		if( !TOpenUserDB ( szUserDB ) ) {
			dwErrorCode = GetLastError();
			lprintfs( "Failed to open user account database (Error code: %ld).\n", dwErrorCode );
			__leave;
		}
		lprintfs( "Open user account database successfully.\n", dwErrorCode );
		
#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				3000) )                // wait hint
			__leave;
#endif

		lstrcpy( szSProc, szTreeServerRoot );
		lstrcat( szSProc, _TEXT( "\\Config\\SProc.DB" ) );
		if( !OpenStoredProcFile( szSProc ) )
		{
			dwErrorCode = GetLastError();
			lprintfs( "Failed to open stored procedure database(Error code: %ld).\n", dwErrorCode );
			_leave;
		}
		lprintfs( "Open stored procedure database successfully.\n", dwErrorCode );

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				3000) )                // wait hint
			__leave;
#endif
		lprintfs( "Load plug-ins...\n" );
		InitPlugIns();

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				3000) )                // wait hint
			__leave;
#endif

		dwErrorCode = InitConvInterface();
		if( dwErrorCode != TERR_SUCCESS ) {
			lprintfs( "Failed to initialize User conversation interface.\n" );
			__leave;
		}

		lprintfs( "Initialize user conversation interface successfully.\n" );

		lprintfs( "Initialize treeServer service successfully.\n\n" );

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		if( !ReportStatusToSCMgr(
				SERVICE_RUNNING, // service state
				NO_ERROR,              // exit code
				3000) )                // wait hint
			__leave;
#endif



#ifndef TREESVR_STANDALONE 

		//
		// End of initialization
		//
		////////////////////////////////////////////////////////

		WaitForSingleObject( hServerStopEvent, INFINITE ); 
#endif
	}
	__finally {

#ifdef TREESVR_STANDALONE 
		if (dwArgc!=666666)
			return  dwErrorCode;
#endif

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		ReportStatusToSCMgr(
				SERVICE_STOP_PENDING, // service state
				NO_ERROR,              // exit code
				3000);                // wait hint
#endif
	UnloadPlugIns();

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		ReportStatusToSCMgr(
				SERVICE_STOP_PENDING, // service state
				NO_ERROR,              // exit code
				3000);                // wait hint
#endif

		if( dwErrorCode != TERR_SUCCESS ) {
			lprintfs( "Failed to initialize TBroker service.\n" );
			lprintfs( "TBroker service is now stoping ." );
		}

		CloseHandle( hServerStopEvent );
		lprintfs( "." );

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		ReportStatusToSCMgr(
				SERVICE_STOP_PENDING, // service state
				NO_ERROR,              // exit code
				3000);                // wait hint
#endif

		if( SystemResInfo.hConvThread ) {
			TerminateThread( SystemResInfo.hConvThread, 0 );
			CloseHandle( SystemResInfo.hConvThread );
		}

		if( SystemResInfo.hConvThreadTcpip ) {
			TerminateThread( SystemResInfo.hConvThreadTcpip, 0 );
			CloseHandle( SystemResInfo.hConvThreadTcpip );
		}

		lprintfs( "..." );

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		ReportStatusToSCMgr(
				SERVICE_STOP_PENDING, // service state
				NO_ERROR,              // exit code
				3000);                // wait hint
#endif

		if( SystemResInfo.hSchedulerThread ) {
			extern HANDLE hSemAR;		//defined in schedule.c

			TerminateThread( SystemResInfo.hSchedulerThread, 0 );
			CloseHandle( SystemResInfo.hSchedulerThread );

			CloseHandle( hSemAR );
		}
		lprintfs( "..." );
	
		if( hMemory ) {
			LocalUnlock( hMemory );
			LocalFree( hMemory );
		}
		lprintfs( "." );

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		ReportStatusToSCMgr(
				SERVICE_STOP_PENDING, // service state
				NO_ERROR,              // exit code
				3000);                // wait hint
#endif

		if( g_lpSharedQueue )
			ReleaseQueue();
		lprintfs( ".." );
	
		if( g_lpExchBufInfo )
			ReleaseExchngBuf();
		lprintfs( ".." );
	
		// report the status to the service control manager.
		//
		ReportStatusToSCMgr(
				SERVICE_STOP_PENDING, // service state
				NO_ERROR,              // exit code
				3000);                // wait hint

		if( g_lpTEngineInfo )
			TEFree();
		lprintfs( "...." );

#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		ReportStatusToSCMgr(
				SERVICE_STOP_PENDING, // service state
				NO_ERROR,              // exit code
				3000);                // wait hint

#endif
		TCloseUserDB();
		lprintfs( ".." );
#ifndef TREESVR_STANDALONE 
		// report the status to the service control manager.
		//
		ReportStatusToSCMgr(
				SERVICE_STOP_PENDING, // service state
				NO_ERROR,              // exit code
				3000);                // wait hint

#endif
		CloseStoredProcFile();
		lprintfs( "..stoped\n" );

		CloseHandle( hLogFile );
		
		return  dwErrorCode;
		//ExitProcess( dwErrorCode );
	}
}

//
//  FUNCTION: ServiceStop
//
//  PURPOSE: Stops the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    If a ServiceStop procedure is going to
//    take longer than 3 seconds to execute,
//    it should spawn a thread to execute the
//    stop code, and return.  Otherwise, the
//    ServiceControlManager will believe that
//    the service has stopped responding.
//    
VOID ServiceStop( void )
{
    if ( hServerStopEvent )
        SetEvent( hServerStopEvent );
}

/*
void main ( )
{
	DWORD dwErrorCode;
	
	bConsole = TRUE;
	InitializeCriticalSection( &cs );

	SetConsoleTitle( "TreeServer Console Monitor" );

	dwErrorCode = ServerStartup();
	if( dwErrorCode != TERR_SUCCESS )
		lprintfs( "Server initialize failure.\n\n" );
	else {
		lprintfs( "Server initialize successfully\n." );
		lprintfs( "System ready.\n\n" );
	}

	Sleep( 6000000 );

	// ServerShutdown();
}
*/

//
//  FUNCTION: main
//
//  PURPOSE: entrypoint for service
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    main() either performs the command line task, or
//    call StartServiceCtrlDispatcher to register the
//    main service thread.  When the this call returns,
//    the service has stopped, so exit.
//

#ifdef TREESVR_STANDALONE

BOOL CALLBACK ServerDlgProc(HWND hDlg,UINT uMsg,
								WPARAM wParam,LPARAM lParam);

char *ag[2]={"start",NULL};
int nDlgShow=0;

int WINAPI WinMain(HINSTANCE hInstexe,
				   HINSTANCE hInstprev,
				   LPSTR lpszCmdLine,
				   int nCmdShow)
{
	static char *argv[2];

#ifdef TRIAL_VERSION
	HMODULE hLib;
	TRIALCK p;
	int d;

	hLib = LoadLibrary( "TRIALCK.DLL" );
	if( hLib == NULL ) {
#ifdef TREESVR_STANDALONE 
		MessageBox( 0, "软件的试用协议被破坏，试用协议失效。", "Trial Check", MB_OK | MB_ICONINFORMATION );
#endif
		ExitProcess( 0 );
	}

	p = (TRIALCK)GetProcAddress( hLib, (LPTSTR)102 );
	d = (*p)();
	FreeLibrary( hLib );
	if( d < 0 ) {
#ifdef TREESVR_STANDALONE 
		MessageBox( 0, "软件的试用协议被破坏，试用协议失效。", "Trial Check", MB_OK | MB_ICONINFORMATION );
#endif
		ExitProcess( 0 );
	}

	if( d == 0 ) {
#ifdef TREESVR_STANDALONE 
		MessageBox( 0, "软件的试用协议期限已满，请购买正式版。", "Trial Check", MB_OK | MB_ICONINFORMATION );
#endif
		ExitProcess( 0 );
	}
#endif /* TRIAL_VERSION */

	hSTANDALONEMutex=CreateMutex(NULL,FALSE,"TREESVR_STANDALONE");
	
	if (GetLastError()==ERROR_ALREADY_EXISTS) {
		if( *lpszCmdLine == '-' || *lpszCmdLine == '/' ) {
			if( !lstrcmpi( (lpszCmdLine+1), "STOP" ) ) {
				HWND hwnd = FindWindow( NULL, "TBroker 服务管理器" );
				if( hwnd ) {
					SendMessage( hwnd, WM_COMMAND, IDM_EXIT, 1 );
					return 0;
				}
			}
		}

		return(-1);
	}
	if (GetLastError()==ERROR_INVALID_HANDLE) {

		return(-2);
	}

	argv[0] = lpszCmdLine;
	hInst=hInstexe;
	nServerStatus=1;

	InitializeCriticalSection( &cs );

	savedArgv = argv;

	if (DialogBox(NULL,MAKEINTRESOURCE(IDD_SERVERDIALOG),NULL,
		(DLGPROC)ServerDlgProc)==-1) 
		MessageBeep(100);

	CloseHandle(hSTANDALONEMutex);
	return(0);
}

BOOL CALLBACK ServerDlgProc(HWND hDlg,UINT uMsg,
								WPARAM wParam,LPARAM lParam)
{
	HMENU hMenuPopup;
	POINT point;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		hSvrDlg=hDlg;
		EnableWindow(GetDlgItem(hDlg,IDC_START),nServerStatus==1?TRUE:FALSE);
		EnableWindow(GetDlgItem(hDlg,IDC_STOP),nServerStatus==0?TRUE:FALSE);
		EnableWindow(GetDlgItem(hDlg,IDC_PAUSE),nServerStatus==0?TRUE:FALSE);
		NotifyAdd(hDlg);
		nServerStatus=3;
		StateChange(hDlg);
		if (RunStandalone(1, NULL)==TERR_SUCCESS)
			nServerStatus=0;
		else
			nServerStatus=1;
		StateChange(hDlg);
		break;
	case WM_DESTROY:
		NotifyDelete(hDlg);
		break;
	case WM_PAINT:
		Dlg_OnPaint(hDlg);
		if(nDlgShow==0)
			PostMessage(hDlg,WM_COMMAND,IDCANCEL,0);
		break;
	case WM_COMMAND:
		ChangeMenuItem(GetMenu(hDlg)); 
		nDlgShow=1;
		switch(LOWORD(wParam))
		{
		case IDM_EXIT:
			if( !lParam ) {
			    if (MessageBox(hDlg, _T("是否要退出TBroker服务管理器 ?"), 
						_T("提示框"),
						MB_YESNO | MB_ICONQUESTION |MB_DEFBUTTON2 )==IDYES )
					EndDialog(hDlg,TRUE);
			}
			EndDialog(hDlg,TRUE);
			break;
		case IDCANCEL:
			ShowWindow(hDlg,SW_HIDE);
			break;
		case IDC_START:
			nServerStatus=3;
			StateChange(hDlg);
			if (RunStandalone(1, NULL)==TERR_SUCCESS)
				nServerStatus=0;
			else
				nServerStatus=1;
			StateChange(hDlg);
			break;
		case IDC_PAUSE:
			nServerStatus=3;
			StateChange(hDlg);
			if (RunStandalone(666666, NULL)==TERR_SUCCESS)
				nServerStatus=2;
			else
				nServerStatus=1;
			StateChange(hDlg);
			break;
		case IDC_STOP:
			nServerStatus=3;
			StateChange(hDlg);
			RunStandalone(666666, NULL);
			nServerStatus=1;
			StateChange(hDlg);
			break;
		case IDM_SHOW:
			ShowWindow(hDlg,SW_SHOW);
			break;
		case IDABORT:
			ShowWindow(hDlg,SW_HIDE);
			break;
		case IDM_ABOUT:
			DialogBox(hInst,MAKEINTRESOURCE(IDD_ABOUTBOX),hDlg,(DLGPROC)AboutProc);
			break;
		}
		ChangeMenuItem(GetMenu(hDlg)); 
		break;
	case WM_MOUSEMOVE:
		PostMessage(hDlg, WM_LBUTTONDOWN, 0,MAKELONG(738,511) );
		break;
	case MYWM_NOTIFYICON:
		switch(lParam)
		{
		case WM_LBUTTONDBLCLK:
			ShowWindow(hDlg,SW_SHOW);
			SetForegroundWindow(hDlg);
			break;
		case WM_RBUTTONDOWN:
			BringWindowToTop(hDlg);
			SetForegroundWindow(hDlg);

			hMenuPopup=LoadMenu(hInst,MAKEINTRESOURCE(POPUPMENU));
			ChangeMenuItem(hMenuPopup); 
			GetCursorPos(&point);

			TrackPopupMenu(GetSubMenu(hMenuPopup,0),
				TPM_RIGHTALIGN|TPM_LEFTBUTTON |TPM_RIGHTBUTTON,
				point.x+10,
				GetSystemMetrics(SM_CYSCREEN),
				0,hDlg,NULL);
			ChangeMenuItem(hMenuPopup); 
			DestroyMenu(hMenuPopup);
			ReleaseCapture();
			break;
		case WM_MOUSEMOVE:
			PostMessage(hDlg, WM_LBUTTONDOWN, 0,MAKELONG(738,511) );
			break;
		default:
			break;
		}
		break;
	default:
		return(FALSE);
	}
	return(TRUE);
}

/*void _CRTAPI1 main ( int argc, char **argv )
{
	_tprintf( "TreeServer standalone version 2.6.\n" );
	_tprintf( "Copyright (C) 1998 Shanghai Tiedao University.\n\n"  );
	_tprintf( "NOTICE: press Ctrl+Break to stop the service!\n\n"  );


	InitializeCriticalSection( &cs );

	savedArgv = argv;

	if( (argc > 1) && ((*argv[1] == '-') || (*argv[1] == '/')) ) {
		if( _stricmp( "Start", argv[1]+1 ) == 0 ) {
			bDebug = TRUE;
			RunStandalone(argc, argv);
		}
		else if( _stricmp( "UserDB", argv[1]+1 ) == 0 ) {
			DWORD dwRetCode;

			dwRetCode = CreateTreeSVRUserDB();
			if( dwRetCode != 0 )
				printf( "Failed to create user account database (Error code: %ld).\n", dwRetCode );
			else
				printf( "Create user account database successfully.\n" );
		}
		else if( _stricmp( "SProcDB", argv[1]+1 ) == 0 ) {
			DWORD dwRetCode;

			if( (dwRetCode = spCreateFile ( "SProc.DB" ) ) )
				printf( "Create stored procedure database successfully.\n" );
			else
				printf( "Failed to create stored procedure database (Error code: %ld).\n", dwRetCode );
		}
		else {
			bDebug = TRUE;
			goto dispatch;
		}

		ExitProcess( 0 );
	}

dispatch:
	// this is just to be friendly
	_tprintf( "%s -Start <params>   Run Treesvr standalone version 2.6\n", SZAPPNAME );
	_tprintf( "%s -UserDB           Create the default user account database\n", SZAPPNAME );
	_tprintf( "%s -SProcDB          Create the default stored procedure database\n", SZAPPNAME );
}*/

#else

void _CRTAPI1 main( int argc, char **argv )
{
	OSVERSIONINFO osv;

	SERVICE_TABLE_ENTRY dispatchTable[] = {
		{ TEXT(SZSERVICENAME), (LPSERVICE_MAIN_FUNCTION)service_main },
		{ NULL, NULL }
	};

	osv.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	GetVersionEx( &osv );

	_tprintf( "TBroker service image files.\n" );
	_tprintf( "Copyright (C) 1989-2001 Shanghai Withub Technology Co., Ltd.\n\n"  );

	if( osv.dwPlatformId != VER_PLATFORM_WIN32_NT ) {
		_tprintf( "The service only runs on Windows NT.\n"  );
		ExitProcess( 0 );
	}

	if( osv.dwMajorVersion < 4 ) {
		_tprintf( "The service only runs on Windows NT 4.00 or above version.\n"  );
		ExitProcess( 0 );
	}

	savedArgv = argv;

	InitializeCriticalSection( &cs );

	if( (argc > 1) && ((*argv[1] == '-') || (*argv[1] == '/')) ) {
		if ( _stricmp( "install", argv[1]+1 ) == 0 ) {
			CmdInstallService();
		}
		else if( _stricmp( "remove", argv[1]+1 ) == 0 ) {
			CmdRemoveService();
		}
		else if( _stricmp( "start", argv[1]+1 ) == 0 ) {
			bDebug = TRUE;
			CmdStartService(argc, argv);
		}
		else if( _stricmp( "stop", argv[1]+1 ) == 0 ) {
			bDebug = TRUE;
			CmdStopService();
		}
		else if( _stricmp( "status", argv[1]+1 ) == 0 ) {
			bDebug = TRUE;
			CmdQueryService();
		}
		else if( _stricmp( "debug", argv[1]+1 ) == 0 ) {
			bDebug = TRUE;
			CmdDebugService(argc, argv);
		}
		else if( _stricmp( "UserDB", argv[1]+1 ) == 0 ) {
			DWORD dwRetCode;

			dwRetCode = CreateTreeSVRUserDB();
			if( dwRetCode != 0 )
				printf( "Failed to create user account database (Error code: %ld).\n", dwRetCode );
			else
				printf( "Created user account database successfully.\n" );
		}
		else if( _stricmp( "SProcDB", argv[1]+1 ) == 0 ) {
			DWORD dwRetCode;

			if( (dwRetCode = spCreateFile ( "SProc.DB" ) ) )
				printf( "Created stored procedure database successfully.\n" );
			else
				printf( "Failed to create stored procedure database (Error code: %ld).\n", dwRetCode );
		}
		else {
			bDebug = TRUE;
			goto dispatch;
		}

		ExitProcess( 0 );
	}

	// if it doesn't match any of the above parameters
	// the service control manager may be starting the service
	// so we must call StartServiceCtrlDispatcher
dispatch:
	// this is just to be friendly
	_tprintf( "%s -install          Install the service\n", SZAPPNAME );
	_tprintf( "%s -remove           Remove the service\n", SZAPPNAME );
	_tprintf( "%s -start            Start the service\n", SZAPPNAME );
	_tprintf( "%s -stop             Stop the service\n", SZAPPNAME );
	_tprintf( "%s -status           Query the service status\n", SZAPPNAME );
	_tprintf( "%s -debug <params>   Run as a console app for debugging\n", SZAPPNAME );
	_tprintf( "%s -UserDB           Create the default user account database\n", SZAPPNAME );
	_tprintf( "%s -SProcDB          Create the default stored procedure database\n", SZAPPNAME );
	_tprintf( "\nStartServiceCtrlDispatcher being called.\n" );
	_tprintf( "This may take several seconds.  Please wait.\n" );

	if( !StartServiceCtrlDispatcher( dispatchTable ) ) {
		char __buffer[128];

		wsprintf( __buffer, TEXT("StartServiceCtrlDispatcher failed(%ld)."), GetLastError() );
		AddToMessageLog( __buffer );
	}
}

#endif

///////////////////////////////////////////////////////////////////
//
//  The following code handles service installation and removal
//
///////////////////////////////////////////////////////////////////

//
//  FUNCTION: CmdInstallService()
//
//  PURPOSE: Installs the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
void CmdInstallService( void )
{
	SC_HANDLE   schService;
	SC_HANDLE   schSCManager;
	TCHAR		szPath[512];

	if( GetModuleFileName( NULL, szPath, 512 ) == 0 ) {
		_tprintf( TEXT("Unable to install %s - %s\n"), TEXT(SZSERVICEDISPLAYNAME), GetLastErrorText( szErr, 256 ) );
		return;
	}

	schSCManager = OpenSCManager(
			NULL,                   // machine (NULL == local)
			NULL,                   // database (NULL == default)
			SC_MANAGER_ALL_ACCESS );// access required
 
	if( schSCManager ) {
		schService = CreateService(
				schSCManager,               // SCManager database
				TEXT(SZSERVICENAME),        // name of service
				TEXT(SZSERVICEDISPLAYNAME), // name to display
				SERVICE_ALL_ACCESS,         // desired access
				SERVICE_WIN32_OWN_PROCESS,  // service type
				SERVICE_AUTO_START,			// start type
				SERVICE_ERROR_NORMAL,       // error control type
				szPath,                     // service's binary
				NULL,                       // no load ordering group
				NULL,                       // no tag identifier
				TEXT(SZDEPENDENCIES),       // dependencies
				NULL,                       // LocalSystem account
				NULL );                      // no password

		if( schService ) {
			_tprintf( TEXT("%s installed.\n"), TEXT(SZSERVICEDISPLAYNAME) );
			CloseServiceHandle(schService);
		}
        else {
			_tprintf( TEXT("CreateService failed - %s\n"), GetLastErrorText(szErr, 256) );
		}

		CloseServiceHandle( schSCManager );
	}
	else
		_tprintf( TEXT("OpenSCManager failed - %s\n"), GetLastErrorText(szErr,256) );
}

//
//  FUNCTION: CmdRemoveService()
//
//  PURPOSE: Stops and removes the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
void CmdRemoveService ( void )
{
	SC_HANDLE   schService;
	SC_HANDLE   schSCManager;

	schSCManager = OpenSCManager(
			NULL,                   // machine (NULL == local)
			NULL,                   // database (NULL == default)
			SC_MANAGER_ALL_ACCESS );// access required

	if( schSCManager ) {
		schService = OpenService( schSCManager, TEXT(SZSERVICENAME), SERVICE_ALL_ACCESS );
		if( schService ) {
			// try to stop the service
			if( ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) ) {
				_tprintf( TEXT("Stopping %s."), TEXT(SZSERVICEDISPLAYNAME) );
				Sleep( 1000 );

				while( QueryServiceStatus( schService, &ssStatus ) ) {
					if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING ) {
						_tprintf( TEXT("^") );
						Sleep( 1000 );
					}
					else
						break;
				}

				if( ssStatus.dwCurrentState == SERVICE_STOPPED )
					 _tprintf( TEXT("\n%s stopped.\n"), TEXT(SZSERVICEDISPLAYNAME) );
                else
                    _tprintf( TEXT("\n%s failed to stop.\n"), TEXT(SZSERVICEDISPLAYNAME) );
			}

			// now remove the service
			if( DeleteService( schService ) )
				_tprintf( TEXT("%s removed.\n"), TEXT(SZSERVICEDISPLAYNAME) );
			else
				_tprintf( TEXT("DeleteService failed - %s\n"), GetLastErrorText(szErr,256) );

			CloseServiceHandle( schService );
		}
		else
			_tprintf( TEXT("OpenService failed - %s\n"), GetLastErrorText(szErr,256) );

        CloseServiceHandle( schSCManager );
	}
    else
		_tprintf( TEXT("OpenSCManager failed - %s\n"), GetLastErrorText(szErr,256) );
}

void CmdStartService ( int argc, char ** argv )
{
	SC_HANDLE   schService;
	SC_HANDLE   schSCManager;

	schSCManager = OpenSCManager(
			NULL,                   // machine (NULL == local)
			NULL,                   // database (NULL == default)
			SC_MANAGER_ALL_ACCESS );// access required

	if( schSCManager ) {
		schService = OpenService( schSCManager, TEXT(SZSERVICENAME), SERVICE_ALL_ACCESS );
		if( schService ) {
			// try to start the service
			if( StartService( schService, argc, argv ) ) {
				_tprintf( TEXT("Now %s is starting."), TEXT(SZSERVICEDISPLAYNAME) );
				Sleep( 100 );

				while( QueryServiceStatus( schService, &ssStatus ) ) {
					if ( ssStatus.dwCurrentState == SERVICE_START_PENDING ) {
						_tprintf( TEXT(".") );
						Sleep( 100 );
					}
					else
						break;
				}

				if( ssStatus.dwCurrentState == SERVICE_STOPPED )
					 _tprintf( TEXT("\n%s start failed - %s\n.\n"), TEXT(SZSERVICEDISPLAYNAME), GetLastErrorText(szErr,256) );
                else
                    _tprintf( TEXT("\n%s started.\n"), TEXT(SZSERVICEDISPLAYNAME) );
			}

			CloseServiceHandle( schService );
		}
		else
			_tprintf( TEXT("OpenService failed - %s\n"), GetLastErrorText(szErr,256) );

        CloseServiceHandle( schSCManager );
	}
    else
		_tprintf( TEXT("OpenSCManager failed - %s\n"), GetLastErrorText(szErr,256) );
}

void CmdStopService ( void )
{
	SC_HANDLE   schService;
	SC_HANDLE   schSCManager;

	schSCManager = OpenSCManager(
			NULL,                   // machine (NULL == local)
			NULL,                   // database (NULL == default)
			SC_MANAGER_ALL_ACCESS );// access required

	if( schSCManager ) {
		schService = OpenService( schSCManager, TEXT(SZSERVICENAME), SERVICE_ALL_ACCESS );
		if( schService ) {
			// try to stop the service
			if( ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) ) {
				_tprintf( TEXT("Now %s is Stopping."), TEXT(SZSERVICEDISPLAYNAME) );
				Sleep( 100 );

				while( QueryServiceStatus( schService, &ssStatus ) ) {
					if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING ) {
						_tprintf( TEXT("^") );
						Sleep( 100 );
					}
					else
						break;
				}

				if( ssStatus.dwCurrentState == SERVICE_STOPPED )
					 _tprintf( TEXT("\n%s stopped.\n"), TEXT(SZSERVICEDISPLAYNAME) );
                else
                    _tprintf( TEXT("\n%s failed to stop.\n"), TEXT(SZSERVICEDISPLAYNAME) );
			}

			CloseServiceHandle( schService );
		}
		else
			_tprintf( TEXT("OpenService failed - %s\n"), GetLastErrorText(szErr,256) );

        CloseServiceHandle( schSCManager );
	}
    else
		_tprintf( TEXT("OpenSCManager failed - %s\n"), GetLastErrorText(szErr,256) );
}

void CmdQueryService ( void )
{
	SC_HANDLE   schService;
	SC_HANDLE   schSCManager;

	schSCManager = OpenSCManager(
			NULL,                   // machine (NULL == local)
			NULL,                   // database (NULL == default)
			SC_MANAGER_ALL_ACCESS );// access required

	if( schSCManager ) {
		schService = OpenService( schSCManager, TEXT(SZSERVICENAME), SERVICE_ALL_ACCESS );
		if( schService ) {
			if( QueryServiceStatus( schService, &ssStatus ) ) {
				if( ssStatus.dwCurrentState == SERVICE_STOPPED )
					_tprintf( TEXT( "%s now stopped.\n" ), TEXT(SZSERVICEDISPLAYNAME) );
				else if( ssStatus.dwCurrentState == SERVICE_RUNNING )
					_tprintf( TEXT( "%s now running.\n" ), TEXT(SZSERVICEDISPLAYNAME) );
				else if( ssStatus.dwCurrentState == SERVICE_PAUSED )
					_tprintf( TEXT( "%s now paused.\n" ), TEXT(SZSERVICEDISPLAYNAME) );
			}
			else 
				_tprintf( TEXT("QueryServiceStatus failed - %s\n"), GetLastErrorText(szErr,256) );

			CloseServiceHandle( schService );
		}
		else
			_tprintf( TEXT("OpenService failed - %s\n"), GetLastErrorText(szErr,256) );

        CloseServiceHandle( schSCManager );
	}
    else
		_tprintf( TEXT("OpenSCManager failed - %s\n"), GetLastErrorText(szErr,256) );
}


///////////////////////////////////////////////////////////////////
//
//  The following code is for running the service as a console app
//
///////////////////////////////////////////////////////////////////

//
//  FUNCTION: CmdDebugService(int argc, char ** argv)
//
//  PURPOSE: Runs the service as a console application
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
void CmdDebugService ( int argc, char ** argv )
{
	DWORD dwArgc;
	LPTSTR *lpszArgv;

#ifdef UNICODE
	lpszArgv = CommandLineToArgvW( GetCommandLineW(), &(dwArgc) );
#else
	dwArgc   = (DWORD)argc;
	lpszArgv = argv;
#endif

	_tprintf( TEXT("Debugging %s.\n"), TEXT(SZSERVICEDISPLAYNAME) );

	SetConsoleCtrlHandler( ControlHandler, TRUE );

	ServiceStart( dwArgc, lpszArgv );
}

#ifdef TREESVR_STANDALONE

DWORD RunStandalone( int argc, char ** argv )
{
	DWORD dwArgc;
	LPTSTR *lpszArgv;

#ifdef UNICODE
	lpszArgv = CommandLineToArgvW( GetCommandLineW(), &(dwArgc) );
#else
	dwArgc   = (DWORD)argc;
	lpszArgv = argv;
#endif

	SetConsoleCtrlHandler( ControlHandler, TRUE );

	return  ServiceStart( dwArgc, lpszArgv );
}

#endif

///////////////////////////////////////////////////////////////////
//
//  The following code is handle the service control.
//
///////////////////////////////////////////////////////////////////

//
//  FUNCTION: ReportStatusToSCMgr()
//
//  PURPOSE: Sets the current status of the service and
//           reports it to the Service Control Manager
//
//  PARAMETERS:
//    dwCurrentState - the state of the service
//    dwWin32ExitCode - error code to report
//    dwWaitHint - worst case estimate to next checkpoint
//
//  RETURN VALUE:
//    TRUE  - success
//    FALSE - failure
//
//  COMMENTS:
//
BOOL ReportStatusToSCMgr ( DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;
	BOOL fResult = TRUE;

    if( !bDebug ) { // when debugging we don't report to the SCM
		if( dwCurrentState == SERVICE_START_PENDING)
			ssStatus.dwControlsAccepted = 0;
		else
			ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

		ssStatus.dwCurrentState = dwCurrentState;
		ssStatus.dwWin32ExitCode = dwWin32ExitCode;
		ssStatus.dwWaitHint = dwWaitHint;

		if( ( dwCurrentState == SERVICE_RUNNING ) || ( dwCurrentState == SERVICE_STOPPED ) )
			ssStatus.dwCheckPoint = 0;
		else
			ssStatus.dwCheckPoint = dwCheckPoint++;

        // Report the status of the service to the service control manager.
		if( !( fResult = SetServiceStatus( sshStatusHandle, &ssStatus ) ) ) {
			AddToMessageLog( TEXT( "SetServiceStatus" ) );
		}
	}
	
	return fResult;
}

//
//  FUNCTION: service_ctrl
//
//  PURPOSE: This function is called by the SCM whenever
//           ControlService() is called on this service.
//
//  PARAMETERS:
//    dwCtrlCode - type of control requested
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID WINAPI service_ctrl ( DWORD dwCtrlCode )
{
	// Handle the requested control code.
	switch(dwCtrlCode) {
	// Stop the service.
	//
	// SERVICE_STOP_PENDING should be reported before
	// setting the Stop Event - hServerStopEvent - in
	// ServiceStop().  This avoids a race condition
	// which may result in a 1053 - The Service did not respond...
	// error.
	case SERVICE_CONTROL_STOP:
		ReportStatusToSCMgr( SERVICE_STOP_PENDING, NO_ERROR, 0 );
		ServiceStop();
		return;

	// Update the service status.
    //
	case SERVICE_CONTROL_INTERROGATE:
		break;

	// invalid control code
	//
	default:
		break;
	}

	ReportStatusToSCMgr( ssStatus.dwCurrentState, NO_ERROR, 0 );
}

//
//  FUNCTION: service_main
//
//  PURPOSE: To perform actual initialization of the service
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    This routine performs the service initialization and then calls
//    the user defined ServiceStart() routine to perform majority
//    of the work.
//
void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{
	// register self service control handler:
	sshStatusHandle = RegisterServiceCtrlHandler( TEXT( SZSERVICENAME ), service_ctrl );

	if( !sshStatusHandle )
		goto cleanup;

    // SERVICE_STATUS members that don't change in example
	ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ssStatus.dwServiceSpecificExitCode = 0;

    // report the status to the service control manager.
	if( !ReportStatusToSCMgr(
			SERVICE_START_PENDING, // service state
			NO_ERROR,              // exit code
			10000 ) )               // wait hint
		goto cleanup;
	
	ServiceStart( dwArgc, lpszArgv );

cleanup:
	// try to report the stopped status to the service control manager.
	if( sshStatusHandle )
		ReportStatusToSCMgr(
				SERVICE_STOPPED,
				0,					//GetLastError(),
				5000 );

	return;
}

//
//  FUNCTION: ControlHandler ( DWORD dwCtrlType )
//
//  PURPOSE: Handled console control events
//
//  PARAMETERS:
//    dwCtrlType - type of control event
//
//  RETURN VALUE:
//    True - handled
//    False - unhandled
//
//  COMMENTS:
//
BOOL WINAPI ControlHandler ( DWORD dwCtrlType )
{
	switch( dwCtrlType ) {
	case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
	case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
		_tprintf( TEXT("%s now stopping ."), TEXT(SZSERVICEDISPLAYNAME ) );
		ServiceStop();
		return TRUE;
		break;
    }

	return FALSE;
}

///////////////////////////////////////////////////////////////////
//
//  The following code handles service error message and event log.
//
///////////////////////////////////////////////////////////////////

//
//  FUNCTION: AddToMessageLog(LPTSTR lpszMsg)
//
//  PURPOSE: Allows any thread to log an error message
//
//  PARAMETERS:
//    lpszMsg - text for message
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID AddToMessageLog ( LPTSTR lpszMsg )
{
	TCHAR   szMsg[256];
	HANDLE  hEventSource;
	LPTSTR  lpszStrings[2];
	DWORD dwErr;

	if( !bDebug ) {
		dwErr = GetLastError();

		// Use event logging to log the error.
		hEventSource = RegisterEventSource( NULL, TEXT(SZSERVICENAME) );

		_stprintf( szMsg, TEXT("%s error: %d"), TEXT(SZSERVICENAME), dwErr );
		lpszStrings[0] = szMsg;
		lpszStrings[1] = lpszMsg;

		if( hEventSource != NULL ) {
			ReportEvent ( hEventSource,		// handle of event source
					EVENTLOG_ERROR_TYPE,	// event type
					0,						// event category
					0,						// event ID
					NULL,					// current user's SID
					2,						// strings in lpszStrings
					0,						// no bytes of raw data
					lpszStrings,			// array of error strings
					NULL);					// no raw data

			DeregisterEventSource( hEventSource );
		}
	}
}

//
//  FUNCTION: GetLastErrorText
//
//  PURPOSE: copies error message text to string
//
//  PARAMETERS:
//    lpszBuf - destination buffer
//    dwSize - size of buffer
//
//  RETURN VALUE:
//    destination buffer
//
//  COMMENTS:
//
LPTSTR GetLastErrorText ( LPTSTR lpszBuf, DWORD dwSize )
{
	DWORD dwRet;
	LPTSTR lpszTemp = NULL;

	dwRet = FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
			NULL,
			GetLastError(),
			LANG_NEUTRAL,
			(LPTSTR)&lpszTemp,
			0,
			NULL );

	// supplied buffer is not long enough
	if( !dwRet || ( (long)dwSize < (long)dwRet+14 ) )
		lpszBuf[0] = TEXT('\0');
	else {
		lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  //remove cr and newline character
		_stprintf( lpszBuf, TEXT("%s (0x%x)"), lpszTemp, GetLastError() );
	}

	if( lpszTemp )
		LocalFree( (HLOCAL)lpszTemp );

	return lpszBuf;
}
