/******************************
*	Module Name:
*		TACCOUNT.H
*
*	Abstract:
*		Contains the user's information access routine's declarations.
*
*		Build this module need the library: NETAPI32.LIB
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) 1997  China Railway Software Corporation
*
*	Revision History:
*		Write by NiuJingyu, 1997.11
********************************************************************************/

#ifndef _INC_TACCOUNT_
#define _INC_TACCOUNT_

#include <windows.h>

#ifdef __TACCOUNT_MAIN

__declspec( dllexport )
DWORD MakeComputerNameW ( 
	LPCWSTR lpwComputer,
	LPWSTR lpwComputerName,
	LPDWORD lpcbwComputerName );

__declspec( dllexport )
DWORD GetUserHomeDirW (
	LPCWSTR lpwServer,
	LPCWSTR lpwUserName,
	LPWSTR lpwHomeDir,
	LPDWORD lpcbHomeDir );

__declspec( dllexport )
DWORD GetUserHomeDirA (
	LPCSTR lpServer,
	LPCSTR lpUserName,
	LPSTR lpHomeDir,
	LPDWORD lpcbHomeDir );

#else // #ifdef __TACCOUNT_MAIN

DWORD MakeComputerNameW ( 
	LPCWSTR lpwComputer,
	LPWSTR lpwComputerName,
	LPDWORD lpcbwComputerName );

DWORD GetUserHomeDirW (
	LPCWSTR lpwServer,
	LPCWSTR lpwUserName,
	LPWSTR lpwHomeDir,
	LPDWORD lpcbHomeDir );

DWORD GetUserHomeDirA (
	LPCSTR lpServer,
	LPCSTR lpUserName,
	LPSTR lpHomeDir,
	LPDWORD lpcbHomeDir );

#ifdef UNICODE
#define GetUserHomeDir GetUserHomeDirW
#else
#define GetUserHomeDir GetUserHomeDirA
#endif // !UNICODE

#endif // #ifdef __TACCOUNT_MAIN

#endif // _INC_TACCOUNT_