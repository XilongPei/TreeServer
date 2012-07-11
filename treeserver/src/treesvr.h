/* By Niu Jingyu */
#ifndef _INC_TREESERVER_
#define _INC_TREESERVER_

#include <lmcons.h>

#include "ts_com.h"
#include "terror.h"
#include "tlimits.h"
#include "tqueue.h"
#include "conv.h"
#include "exchbuff.h"
#include "tengine.h"
#include "tscommon.h"
#include "taccount.h"
#include "schedule.h"
#include "s_proc.h"

#pragma pack(push,1)

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct tagClinetResInfo {
	CHAR szUser[LM20_UNLEN+1];
	CHAR szComputer[CNLEN+1];
	HANDLE hToken;
	SYSTEMTIME tLogonTime;
	//HANDLE hAgentThread;
	DWORD dwAgentThreadId;
	//HANDLE hServiceThread;
	DWORD dwServiceThreadId;
} CLIENT_RES_INFO, *PCLIENT_RES_INFO, *LPCLIENT_RES_INFO;

// Added by Motor, 8 August, 1998
// This variable is used for accessing SYSTEM_RES_INFO::lpClientResInfo
extern CRITICAL_SECTION CriticalSection_User;

typedef struct tagSystemResInfo {
	DWORD dwUsers;
	DWORD dwCurrentUsers;
	HANDLE hConvThread;
	DWORD dwConvThreadId;
	HANDLE hConvThreadTcpip;
	DWORD dwConvThreadIdTcpip;
	HANDLE hSchedulerThread;
	DWORD dwSchedulerhreadId;
	LPQUEUE lpSharedQueue;
	LPEXCHNG_BUF_INFO lpExchBufInfo;
	LPTE_INFO lpTEngineInfo;
	LPCLIENT_RES_INFO lpClientResInfo;
} SYSTEM_RES_INFO, *PSYSTEM_RES_INFO, *LPSYSTEM_RES_INFO;

extern SYSTEM_RES_INFO SystemResInfo;

int lprintfs(char *format, ...);
DWORD ServiceStart ( DWORD dwArgc, LPTSTR *lpszArgv );
VOID ServiceStop( void );
void CmdInstallService( void );
void CmdRemoveService ( void );
void CmdDebugService ( int argc, char ** argv );
void CmdStartService ( int argc, char ** argv );
void CmdQueryService ( void );
void CmdStopService ( void );
BOOL ReportStatusToSCMgr ( DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
VOID WINAPI service_ctrl ( DWORD dwCtrlCode );
void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);
BOOL WINAPI ControlHandler ( DWORD dwCtrlType );
VOID AddToMessageLog ( LPTSTR lpszMsg );
LPTSTR GetLastErrorText ( LPTSTR lpszBuf, DWORD dwSize );

#ifdef TREESVR_STANDALONE
DWORD RunStandalone( int argc, char ** argv );
#endif 


#ifdef  __cplusplus
}
#endif

#pragma pack(pop)

#endif // _INC_TREESERVER_