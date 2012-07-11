/******************************
*	Module Name:
*		TENGINE.H
*
*	Abstract:
*		Contains defines of the transaction engine management data 
*	structures and function declarations which are used by the kernel.
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1997  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1997.10
********************************************************************************/

#ifndef _INC_TENGINE_
#define _INC_TENGINE_

#include <windows.h>
#include <tchar.h>

#include "tlimits.h"
#include "exchbuff.h"

#pragma pack(push,1)

#ifdef  __cplusplus
extern "C" {
#endif

#define TED_SCHEDULE			1
#define TED_EXCLUSIVE			2
#define TED_MULTI_THREAD		4

#define TED_USER_BASE_REQUEST	1025

typedef struct {
	TCHAR szAlias[MAX_TED_ALIAS+1];
	TCHAR szCommand[MAX_TED_COMMAND+1];
	DWORD dwRequest;
	DWORD dwCapability;
	FARPROC lpfnEntryProc;
	HANDLE hLibrary;
	BOOL fEnableTimer; 
	BOOL fEnableIdle; 
	DWORD dwTimeOut;
	BOOL fEnable;	
} TE_INFO, *PTE_INFO, *LPTE_INFO;

typedef struct {
	TCHAR szAlias[MAX_TED_ALIAS+1];
	TCHAR szEngine[MAX_PATH+1];
	TCHAR szCommand[MAX_TED_COMMAND+1];
	DWORD dwCapability;
	TCHAR szEntryProc[MAX_TED_FUNCTION+1];
	BOOL fEnableTimer; 
	BOOL fEnableIdle; 
	DWORD dwTimeOut;
	BOOL fEnable;	
} TED_INFO, *PTED_INFO, *LPTED_INFO;

//////////////////////////////////////////////////////////////////////////////

DWORD GetAvailableTED( 
	LPTSTR lpszTEDBuffer,
	LPDWORD lpcchBuffer );

DWORD GetTEDInfo( 
	LPCTSTR lpszAlias, 
	LPTED_INFO lpTEDInfo );

LPTE_INFO TELoader (
	VOID );

DWORD TEFree (
	VOID );

DWORD GetTERequest (
	LPSTR lpCommand );

FARPROC GetTEEntryAddress (
	DWORD dwRequest );

#ifdef  __cplusplus
}
#endif

#pragma pack(pop)

#endif // _INC_TENGINE_