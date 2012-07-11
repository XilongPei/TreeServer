/******************************
*	Module Name:
*		S_PROC.H
*
*	Abstract:
*		TreeServer stored procedure function declaration. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1998  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1998.07
********************************************************************************/

#ifndef __INC_S_PROC
#define __INC_S_PROC

#include <windows.h>
#include "dbtree.h"
#include "tlimits.h"
#include "exchbuff.h"

#pragma pack(push,1)

#ifdef  __cplusplus
extern "C" {
#endif

typedef bHEAD *SP_FILE;

#ifndef ENUMCALLBACK
	typedef DWORD (CALLBACK ENUMCALLBACK)(LPSTR szUserName,LPEXCHNG_BUF_INFO lpExchangeBuf,BOOL bEndOfEnum );
	typedef ENUMCALLBACK FAR *LPENUMCALLBACK;
#endif

typedef struct tagSP_Privilege {
	DWORD dwUserId;
	CHAR cPrivilege;
} SP_PRIVILEGE, *PSP_PRIVILEGE, *LPSP_PRIVILEGE;
 
#define PRIVILEGE_EXECUTE	1
#define PRIVILEGE_READ		2
#define PRIVILEGE_WRITE		4
#define PRIVILEGE_ALL		PRIVILEGE_EXECUTE|PRIVILEGE_READ|PRIVILEGE_WRITE
#define PRIVILEGE_RDWR		PRIVILEGE_READ|PRIVILEGE_WRITE

BOOL spCreateFile ( 
	LPCSTR lpFileName );

BOOL spDeleteFile ( 
	LPCSTR lpFileName );

BOOL OpenStoredProcFile ( 
	LPCSTR lpFileName );

BOOL CloseStoredProcFile ( 
	VOID );

BOOL spAuditUserPrivilege ( 
	LPCSTR lpProcName,
	DWORD dwUserId,
	char cPrivilege ); 

DWORD spGrantUserPrivilege (
	LPCSTR lpProcName, 
	DWORD dwUserId,
	char cPrivilege );  

DWORD spRevokeUserPrivilege ( 
	LPCSTR lpProcName,
	DWORD dwUserId,
	char cPrivilege );  

DWORD MakeStoredProcName ( 
	LPCSTR lpUserName,
	LPCSTR lpProcName,
	LPSTR lpCompleteProcName );

DWORD NewStoredProc (
	LPCSTR lpProcName,
	DWORD dwUserId,
	LPSTR lpBuffer,
	DWORD dwcbBuffer );

DWORD DeleteStoredProc (
	LPCSTR lpProcName,
	DWORD dwUserId );

DWORD GetStoredProcSize (
	LPCSTR lpProcName,
	DWORD dwUserId );

DWORD ReadStoredProc ( 
	LPCSTR lpProcName,
	DWORD dwUserId,
	LPSTR lpBuffer,
	LPDWORD lpdwcbBuffer );

DWORD UpdateStoredProc (
	LPCSTR lpProcName,
	DWORD dwUserId,
	LPSTR lpBuffer,
	DWORD dwcbBuffer );

DWORD TSprocEnum ( 
	LPENUMCALLBACK lpEnumProc, 
	LPEXCHNG_BUF_INFO lpExchangeBuf );

#ifdef  __cplusplus
}
#endif

#pragma pack(pop)

#endif // __INC_S_PROC