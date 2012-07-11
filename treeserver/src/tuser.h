/******************************
*	Module Name:
*		TUSER.H
*
*	Abstract:
*		Tree Server user information definition and const values. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1998  China Railway Software Corporation
*
*	Revision History:
*		Write by NiuJingyu, 1998.05
********************************************************************************/

#ifndef __INC_TUSER
#define __INC_TUSER

#include "dbtree.h"
#include "tlimits.h"
#include "exchbuff.h"

#ifdef  _MSC_VER
/*
 * Currently, all MS C compilers for TreeServer applications
 * default to 1 byte alignment.
 */
#pragma pack(push,1)
#endif  /* _MSC_VER */

#ifdef  __cplusplus
extern "C" {
#endif

#define INFO_USER_ID		1
#define INFO_PASSWD			2
#define INFO_HOME_DIR		3
#define INFO_LOGON_COMPUTER	4
#define INFO_LOGON_HOURS	5
#define INFO_ACCESS_MASK	6
#define INFO_DESCRIPTION	7
#define INFO_LOGON_FAIL		8
#define INFO_LOCKED			9

#define PERR(api, dwErrorCode) printf( "\n%s: Error %d from %s on line %d\n",  \
			__FILE__, dwErrorCode, api, __LINE__ );
#define PMSG(msg) printf( "\n%s line %d: %s",  \
			__FILE__, __LINE__, msg );

typedef struct tagTsTime {
	unsigned short year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
	unsigned char week;
} TS_TIME, *PTS_TIME, *LPTS_TIME;

typedef struct tagUserInfo {
	CHAR szUser[MAX_USER_NAME];
	DWORD dwUserId;
	CHAR szPasswd[MAX_PASSWD];
	CHAR szHomeDir[MAX_PATH];
	CHAR szLogonComputer[MAX_LOGON_COMPUTER];
	BYTE bLogonHours[MAX_LOGON_HOURS];
	CHAR szAccessMask[MAX_ACCESS_MASK];
	CHAR szDescription[MAX_DESCRIPTION];
	CHAR cLogonFail;
	CHAR cLocked;
	TS_TIME tLastLogon;
	TS_TIME tLastLogonFail;
	CHAR szReserved[158];
} USER_INFO, *PUSER_INFO, *LPUSER_INFO;

typedef bHEAD *PBHEAD;
typedef DWORD (CALLBACK ENUMCALLBACK)(LPSTR szUserName,LPEXCHNG_BUF_INFO lpExchangeBuf,BOOL bEndOfEnum );
typedef ENUMCALLBACK FAR *LPENUMCALLBACK;

BOOL TOpenUserDB ( 
	LPCSTR lpUserDBName );

BOOL TCloseUserDB ( 
	VOID );

BOOL TCreateUserDB ( 
	LPCSTR lpUserDBName );

DWORD TUserAdd ( 
	PUSER_INFO lpUserInfo );

DWORD TUserDelete ( 
	LPCSTR lpUserName );

DWORD TUserGetInfo ( 
	LPCSTR lpUserName, 
	int iIndex, 
	LPVOID lpValueBuf, 
	LPDWORD lpcbBuf );

DWORD TUserSetInfo ( 
	LPCSTR lpUserName, 
	int iIndex, 
	LPVOID lpValueBuf, 
	DWORD cbBuf );

DWORD TUserIncLogonFailCount ( 
	LPCSTR lpUserName );

DWORD TUserClearLogonFailCount ( 
	LPCSTR lpUserName );

BOOL TUserLock ( 
	LPCSTR lpUserName );

BOOL TUserUnlock ( 
	LPCSTR lpUserName );

BOOL TUserLogon ( 
	LPCSTR lpUserName, 
	LPCSTR lpPasswd, 
	LPCSTR lpComputer, 
	LPDWORD lpdwUserId );

DWORD TUserEnum ( 
	LPENUMCALLBACK lpEnumProc,
	LPEXCHNG_BUF_INFO lpExchangeBuf );

DWORD TUserReadTemplate ( 
	PUSER_INFO lpUserInfo );

DWORD TUserWriteTemplate ( 
	PUSER_INFO lpUserInfo );

DWORD TUserRename ( 
	LPCSTR lpOldUserName, 
	LPCSTR lpNewUserName );

DWORD TUserSetAllInfo ( 
	LPCSTR lpUserName, 
	PUSER_INFO lpUserInfo );

DWORD TUserGetAllInfo ( 
	LPCSTR lpUserName, 
	PUSER_INFO lpUserInfo );


#ifdef  __cplusplus
}
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif // __INC_TUSER
