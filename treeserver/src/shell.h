/******************************
*	Module Name:
*		SHELL.H
*
*	Abstract:
*		Tree Server service shell function declaration. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1998  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1998.03
********************************************************************************/

#ifndef _INC_SHELL_
#define _INC_SHELL_

#pragma pack(push,1)

#ifdef  __cplusplus
extern "C" {
#endif

DWORD ServiceShell ( 
	LPVOID lpvParam );

DWORD ASQLShell ( 
	LPSTR lpBuffer,
	LPVOID lpvParam );

DWORD FTPSrvShell (
	LPSTR lpBuffer,
	LPVOID lpvParam );

DWORD TUserShell ( 
	LPSTR lpBuffer,
	LPREQUEST lpRequest );

DWORD ServiceThread (
	LPSTR lpBuffer,
	LPVOID lpvParam );

VOID DefaultShell ( 
	LPSTR lpBuffer, 
	LPREQUEST lpRequest );

VOID DefaultProcess (
	LPSTR lpBuffer,
	LPEXCHNG_BUF_INFO lpExchBufInfo );

DWORD SProcShell ( 
	LPSTR lpBuffer, 
	LPREQUEST lpRequest );

DWORD TPrivateEngShell( LPSTR lpBuffer, LPREQUEST lpRequest );

#ifdef  __cplusplus
}
#endif

#pragma pack(pop)

#endif // _INC_SHELL_