/******************************
*	Module Name:
*		TCOMMON.H
*
*	Abstract:
*		This module contains the definetion for common routines to
*	get the information of the Tree Server. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1997  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1997.11
********************************************************************************/

#ifndef _INC_TSCOMMON_
#define _INC_TSCOMMON_

#include <windows.h>

#pragma pack(push,1)

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef __TSCOMMON_DLL

__declspec( dllexport )
DWORD GetTreeServerRootA ( 
	LPSTR lpBuffer, 
	LPDWORD lpccbBuffer );

__declspec( dllexport )
DWORD GetTreeServerRootW ( 
	LPWSTR lpBuffer, 
	LPDWORD lpccbBuffer );

__declspec( dllexport )
DWORD GetTSProfilePathA ( 
	LPSTR lpBuffer,
	LPDWORD lpccbBuffer );

__declspec( dllexport )
DWORD GetTSProfilePathW ( 
	LPWSTR lpBuffer,
	LPDWORD lpccbBuffer );

#else // #ifdef __TSCOMMON_DLL

DWORD GetTreeServerRootA ( 
	LPSTR lpBuffer, 
	LPDWORD lpccbBuffer );

DWORD GetTreeServerRootW ( 
	LPWSTR lpBuffer, 
	LPDWORD lpccbBuffer );

DWORD GetTSProfilePathA ( 
	LPSTR lpBuffer,
	LPDWORD lpccbBuffer );

DWORD GetTSProfilePathW ( 
	LPWSTR lpBuffer,
	LPDWORD lpccbBuffer );

#ifdef UNICODE
#define GetTreeServerRoot GetTreeServerRootW
#else
#define GetTreeServerRoot GetTreeServerRootA
#endif // !UNICODE

#ifdef UNICODE
#define GetTSProfilePath GetTSProfilePathW
#else
#define GetTSProfilePath GetTSProfilePathA
#endif // !UNICODE

#endif // #ifdef __TSCOMMON_DLL

#ifdef  __cplusplus
}
#endif

#pragma pack(pop)

#endif // _INC_TSCOMMON_