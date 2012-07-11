/******************************
*	Module Name:
*		TUSER.C
*
*	Abstract:
*		Tree Server user manage module, include all user manage
*	API entrys. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1998  China Railway Software Corporation
*
*	Revision History:
*		Write by NiuJingyu, 1998.05
*
*
* 1999/7/26 Xilong Pei
* in my version of Windows:
*   lstrcmp("abc", "abcd") will return a value as 0
* so I have to use strcmp() function
*
********************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <io.h>
#include <tchar.h>
#include <lm.h>
#include <direct.h>

#include "treesvr.h"
#include "dbtree.h"
#include "tuser.h"
#include "tlimits.h"
#include "terror.h"
#include "exchbuff.h"
#include "password.h"

static int mkdirs(const char *path);


PBHEAD UserDB = NULL;
HANDLE hUserDBMutex;

static int mkdirs(const char *path)
{
    char   *s;
    char   dir[256];

    strcpy(dir, path);
    s = dir;
    while( (s = strchr(s, '\\')) != NULL ) {
		*s = '\0';
		mkdir(dir);
//here we cannot stop mkdir, for the next lever subdir
//	if( stat )	return  stat;
		*s = '\\';
		s++;
		if( *s == '\0' )	return  0;
    }

    return  mkdir(dir);

} // end of mkdirs()

BOOL TOpenUserDB ( LPCSTR lpUserDBName )
{
	DWORD dwRetCode;

	UserDB = dbTreeOpen( (LPSTR)lpUserDBName );
	if( UserDB == NULL ) {
		SetLastError( TERR_OPEN_USER_DB ); 
		return FALSE;
	}

	hUserDBMutex = CreateMutex( NULL, FALSE, _TEXT("TS_UserDBMutex") );
	dwRetCode = GetLastError();
	if( dwRetCode != 0 ) {
		if( hUserDBMutex ) 
			CloseHandle( hUserDBMutex );
		
		dbTreeClose( UserDB );
		SetLastError( TERR_USER_DB_IN_USE );
		return FALSE;
	}

	return TRUE;
}

BOOL TCloseUserDB ( VOID )
{
	if( UserDB ) {
		CloseHandle( hUserDBMutex );

		if( dbTreeClose( UserDB ) != 0 ) {
			SetLastError( TERR_CLOSE_USER_DB ); 
			return FALSE;
		}
		else
			return TRUE;
	}

	return TRUE;
}

BOOL TCreateUserDB ( LPCSTR lpUserDBName )
{
	bHEAD *bh;
	USER_INFO UserInfo;

	if( lpUserDBName == NULL ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if( dbTreeBuild( (LPSTR)lpUserDBName, MAX_USER_NAME, NODE_SIZE ) != 0 ) {
		SetLastError( TERR_CREATE_USER_DB ); 
		return FALSE;
	}

	bh = dbTreeOpen( (LPSTR)lpUserDBName );
	if( bh == NULL ) {
		SetLastError( TERR_OPEN_USER_DB ); 
		return FALSE;
	}

	ZeroMemory( (PVOID)&UserInfo, sizeof(USER_INFO) );
	lstrcpy( UserInfo.szUser, "#T_USER" );
	UserInfo.dwUserId = 1;
	lstrcpy( UserInfo.szHomeDir, "%USER_HOME%\\%USER_NAME%"  );
	FillMemory( UserInfo.bLogonHours, MAX_LOGON_HOURS, 0xFF );
	FillMemory( UserInfo.szAccessMask, MAX_ACCESS_MASK, 0xFF );

	if( writeBtreeData( bh, UserInfo.szUser, (LPSTR)&UserInfo,  sizeof( USER_INFO ) ) != 0 ) {
		SetLastError( TERR_WRITE_USER_DB ); 
		return FALSE;
	}

	if( dbTreeClose( bh ) != 0 ) {
		SetLastError( TERR_CLOSE_USER_DB ); 
		return FALSE;
	}

	return TRUE;
}

DWORD TUserReadTemplate ( PUSER_INFO lpUserInfo )
{
	CHAR szUserName[MAX_USER_NAME];
	LPSTR ptr = NULL;
	DWORD dwRetCode = 0;

	if( lpUserInfo == NULL )
		return ERROR_INVALID_PARAMETER;

	if( UserDB == NULL )
		return TERR_NO_USER_DB;

	ZeroMemory( szUserName, MAX_USER_NAME );
	lstrcpy( szUserName, "#T_USER" );

	__try {
		WaitForSingleObject( hUserDBMutex, INFINITE );

		ptr = readBtreeData( UserDB, szUserName, (LPSTR)lpUserInfo, sizeof( USER_INFO ) );
		if( !ptr ) {
			dwRetCode = TERR_TEMPLATE_NOT_EXIST;
			__leave;
		}

		lpUserInfo->szUser[0] = '\x0';
		lpUserInfo->dwUserId = 0;
		lpUserInfo->szPasswd[0] = '\x0';
		lpUserInfo->szLogonComputer[0] = '\x0';
		lpUserInfo->szDescription[0] = '\x0';
	}
	__finally {
		ReleaseMutex( hUserDBMutex );
	}
	
	return dwRetCode;
}

DWORD TUserWriteTemplate ( PUSER_INFO lpUserInfo )
{
	CHAR szUserName[MAX_USER_NAME];
	LPUSER_INFO ptr = NULL;
	DWORD dwRetCode = 0;

	if( lpUserInfo == NULL )
		return ERROR_INVALID_PARAMETER;

	if( UserDB == NULL )
		return TERR_NO_USER_DB;

	__try {
		WaitForSingleObject( hUserDBMutex, INFINITE );

		ptr = (LPUSER_INFO)readBtreeData( UserDB, szUserName, NULL, sizeof( USER_INFO ) );
		if( !ptr ) {
			dwRetCode = TERR_TEMPLATE_NOT_EXIST;
			__leave;
		}

		ZeroMemory( lpUserInfo->szUser, MAX_USER_NAME );
		lstrcpy( lpUserInfo->szUser, "#T_USER" );
		lpUserInfo->dwUserId = ptr->dwUserId;
		lpUserInfo->szPasswd[0] = '\x0';
		lpUserInfo->szLogonComputer[0] = '\x0';
		lpUserInfo->szDescription[0] = '\x0';

		if( writeBtreeData( UserDB, lpUserInfo->szUser, (LPSTR)lpUserInfo,  sizeof( USER_INFO ) ) != 0 ) {
			dwRetCode = TERR_WRITE_USER_DB; 
			__leave;
		}

		dbTreeFlush( UserDB );
	}
	__finally {
		if( ptr )
			freeBtreeMem( (LPSTR)ptr );

		ReleaseMutex( hUserDBMutex );
	}
	
	return dwRetCode;
}

DWORD TUserAdd ( PUSER_INFO lpUserInfo )
{
	CHAR szUserName[MAX_USER_NAME];
	PUSER_INFO ptr = NULL;
	DWORD dwRetCode = 0;

	if( lpUserInfo == NULL )
		return ERROR_INVALID_PARAMETER;

	if( *lpUserInfo->szUser == '\x0' )
		return ERROR_INVALID_PARAMETER;

	if( UserDB == NULL )
		return TERR_NO_USER_DB;

	lstrcpyn( szUserName, lpUserInfo->szUser, MAX_USER_NAME );
	_strupr( szUserName );
	ZeroMemory( lpUserInfo->szUser, MAX_USER_NAME );
	lstrcpyn( lpUserInfo->szUser, szUserName, MAX_USER_NAME );

	__try {
		WaitForSingleObject( hUserDBMutex, INFINITE );

		ptr = (PUSER_INFO)readBtreeData( UserDB, lpUserInfo->szUser, NULL, sizeof( USER_INFO ) );
		if( ptr ) {
			//
			//freeBtreeMem( (LPSTR)ptr );
			dwRetCode = TERR_USER_ALREADY_EXIST;
			__leave;
		}
	
		ZeroMemory( szUserName, MAX_USER_NAME );
		lstrcpy( szUserName, "#T_USER" );

		ptr = (PUSER_INFO)readBtreeData( UserDB, szUserName, NULL, sizeof( USER_INFO ) );
		if( !ptr ) {
			dwRetCode = TERR_USER_DB_INTERNAL_ERROR;
			__leave;
		}

		lpUserInfo->dwUserId = (ptr->dwUserId)++;
	
		if( writeBtreeData( UserDB, lpUserInfo->szUser, (LPSTR)lpUserInfo,  sizeof( USER_INFO ) ) != 0 ) {
			dwRetCode = TERR_WRITE_USER_DB;
			__leave;
		}

		if( writeBtreeData( UserDB, szUserName, (LPSTR)ptr,  sizeof( USER_INFO ) ) != 0 ) {
			dwRetCode = TERR_WRITE_USER_DB;
			__leave;
		}

		dbTreeFlush( UserDB );
	}
	__finally {
		mkdirs( lpUserInfo->szHomeDir );
		if( ptr )
			freeBtreeMem( (LPSTR)ptr );

		ReleaseMutex( hUserDBMutex );
	}

	return dwRetCode;
}

DWORD TUserDelete ( LPCSTR lpUserName )
{
	CHAR szUserName[MAX_USER_NAME];
	PUSER_INFO ptr;
	DWORD dwRetCode = 0;

	if( lpUserName == NULL )
		return ERROR_INVALID_PARAMETER;

	if( *lpUserName == '\x0' )
		return ERROR_INVALID_PARAMETER;

	if( UserDB == NULL )
		return TERR_NO_USER_DB;

	ZeroMemory( szUserName, MAX_USER_NAME );
	lstrcpyn( szUserName, lpUserName, MAX_USER_NAME );
	_strupr( szUserName );

	__try {
		WaitForSingleObject( hUserDBMutex, INFINITE );

		ptr = (PUSER_INFO)readBtreeData( UserDB, szUserName, NULL, sizeof( USER_INFO ) );
		if( !ptr ) {
			dwRetCode = TERR_USER_NOT_EXIST;
			__leave;
		}

		freeBtreeMem( (LPSTR)ptr );

		if( freeBtreeData( UserDB, szUserName ) < 0 ) {
			dwRetCode = TERR_WRITE_USER_DB;
			__leave;
		}

		dbTreeFlush( UserDB );
	}
	__finally {
		ReleaseMutex( hUserDBMutex );
	}

	return dwRetCode;
}

DWORD TUserRename ( LPCSTR lpOldUserName, LPCSTR lpNewUserName )
{
	CHAR szUserName[MAX_USER_NAME];
	LPUSER_INFO ptr = NULL;
	DWORD dwRetCode = 0;

	if( lpOldUserName == NULL || lpNewUserName == NULL ||
			*lpOldUserName == '\x0' || *lpNewUserName == '\x0' )
		return ERROR_INVALID_PARAMETER;

	if( UserDB == NULL )
		return TERR_NO_USER_DB;

	if( lstrcmpi( lpOldUserName, lpNewUserName ) == 0 )
		return 0;

	ZeroMemory( szUserName, MAX_USER_NAME );
	lstrcpyn( szUserName, lpOldUserName, MAX_USER_NAME );
	_strupr( szUserName );

	__try {
		WaitForSingleObject( hUserDBMutex, INFINITE );

		ptr = (LPUSER_INFO)readBtreeData( UserDB, szUserName, NULL, sizeof( USER_INFO ) );
		if( !ptr ) {
			dwRetCode = TERR_USER_NOT_EXIST;
			__leave;
		}
		
		ZeroMemory( ptr->szUser, MAX_USER_NAME );
		lstrcpyn( ptr->szUser, lpNewUserName, MAX_USER_NAME );
		_strupr( ptr->szUser );

		if( writeBtreeData( UserDB, ptr->szUser, (LPSTR)ptr,  sizeof( USER_INFO ) ) != 0 ) {
			dwRetCode = TERR_WRITE_USER_DB;
			__leave;
		}

		if( freeBtreeData( UserDB, szUserName ) < 0 ) {
			dwRetCode = TERR_WRITE_USER_DB;
			__leave;
		}
	}
	__finally {
		if( ptr )
			freeBtreeMem( (LPSTR)ptr );

		ReleaseMutex( hUserDBMutex );
	}
	
	return dwRetCode;
}

DWORD TUserGetInfo ( LPCSTR lpUserName, int iIndex, LPVOID lpValueBuf, LPDWORD lpcbBuf )
{
	CHAR	szUserName[MAX_USER_NAME];
	PUSER_INFO ptr = NULL;
	DWORD	dwRetCode = 0;
	int		len;

	if( lpUserName == NULL )
		return ERROR_INVALID_PARAMETER;

	if( *lpUserName == '\x0' )
		return ERROR_INVALID_PARAMETER;

	if( UserDB == NULL )
		return TERR_NO_USER_DB;

	ZeroMemory( szUserName, MAX_USER_NAME );
	lstrcpyn( szUserName, lpUserName, MAX_USER_NAME );
	_strupr( szUserName );

	__try {
		WaitForSingleObject( hUserDBMutex, INFINITE );

		ptr = (PUSER_INFO)readBtreeData( UserDB, szUserName, NULL, sizeof( USER_INFO ) );
		if( !ptr ) {
			dwRetCode = TERR_USER_NOT_EXIST;
			__leave;
		}

		switch( iIndex ) {
		case INFO_USER_ID:
			if( *lpcbBuf < sizeof( ptr->dwUserId ) ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}
		
			*((LPDWORD)lpValueBuf) = ptr->dwUserId;
			*lpcbBuf = sizeof( ptr->dwUserId );
			break;

		case INFO_PASSWD:
			
			len = lstrlen( ptr->szPasswd );
			if( *lpcbBuf <= (DWORD)len ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			lstrcpyn( (LPSTR)lpValueBuf, ptr->szPasswd, MAX_PASSWD );
			*lpcbBuf = len;
			break;

		case INFO_HOME_DIR:
//#ifdef TREESVR_STANDALONE
			if( *(ptr->szHomeDir) == '\0' )
			{
				char szProfile[MAX_PATH];
				char szHome[MAX_PATH];
				DWORD dwcbPath;

				dwcbPath = MAX_PATH;
				ZeroMemory( szProfile, MAX_PATH );
				dwRetCode = GetTreeServerRoot( szProfile, &dwcbPath );
				if( dwRetCode == 0 ) {
					lstrcat( szProfile, TEXT( "\\Config\\TreeSvr.ini" ) );
					GetPrivateProfileString( "Directory", "UserHome", "\x0",
							szHome, MAX_PATH, szProfile );

					len = lstrlen( szHome );
					if( *lpcbBuf <= (DWORD)len ) {
						dwRetCode = ERROR_MORE_DATA;
						break;
					}

					lstrcpyn( (LPSTR)lpValueBuf, szHome, MAX_PATH );
				}
			} else {
//#else
				len = lstrlen( ptr->szHomeDir );
				if( *lpcbBuf <= (DWORD)len ) {
					dwRetCode = ERROR_MORE_DATA;
					break;
				}

				lstrcpyn( (LPSTR)lpValueBuf, ptr->szHomeDir, MAX_PATH );
				*lpcbBuf = len;
//#endif
			}
			break;

		case INFO_LOGON_COMPUTER:
			if( *lpcbBuf < MAX_LOGON_COMPUTER ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			lstrcpyn( (LPSTR)lpValueBuf, ptr->szLogonComputer, MAX_LOGON_COMPUTER );
			*lpcbBuf = lstrlen( ptr->szLogonComputer );
			break;

		case INFO_LOGON_HOURS:
			if( *lpcbBuf < MAX_LOGON_HOURS ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			CopyMemory( (PVOID)lpValueBuf, (CONST VOID *)ptr->bLogonHours, MAX_LOGON_HOURS );
			*lpcbBuf = MAX_LOGON_HOURS;
			break;

		case INFO_ACCESS_MASK:
			if( *lpcbBuf < MAX_ACCESS_MASK ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			CopyMemory( (PVOID)lpValueBuf, (CONST VOID *)ptr->szAccessMask, MAX_ACCESS_MASK );
			*lpcbBuf = MAX_ACCESS_MASK;
			break;

		case INFO_DESCRIPTION:
			if( *lpcbBuf < MAX_DESCRIPTION ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			lstrcpyn( (LPSTR)lpValueBuf, ptr->szDescription, MAX_DESCRIPTION );
			*lpcbBuf = lstrlen( ptr->szDescription );
			break;

		case INFO_LOGON_FAIL:
			if( *lpcbBuf < sizeof( int ) ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			*((int *)lpValueBuf) = (int)ptr->cLogonFail;
			*lpcbBuf = sizeof( int );
			break;

		case INFO_LOCKED:
			if( *lpcbBuf < 1 ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			*((char *)lpValueBuf) = ptr->cLocked;
			*lpcbBuf = 1;
			break;

		default:
			dwRetCode = TERR_CAN_NOT_ACCESS;
		}

	}
	__finally {
		if( ptr )
			freeBtreeMem( (LPSTR)ptr );

		ReleaseMutex( hUserDBMutex );
	}
	
	return dwRetCode;
}

DWORD TUserGetAllInfo ( LPCSTR lpUserName, PUSER_INFO lpUserInfo )
{
	CHAR	szUserName[MAX_USER_NAME];
	LPSTR	ptr = NULL;
	DWORD	dwRetCode = 0;

	if( lpUserName == NULL || lpUserInfo == NULL )
		return ERROR_INVALID_PARAMETER;

	if( *lpUserName == '\x0' )
		return ERROR_INVALID_PARAMETER;

	if( UserDB == NULL )
		return TERR_NO_USER_DB;

	ZeroMemory( szUserName, MAX_USER_NAME );
	lstrcpyn( szUserName, lpUserName, MAX_USER_NAME );
	_strupr( szUserName );

	__try {
		WaitForSingleObject( hUserDBMutex, INFINITE );

		ptr = readBtreeData( UserDB, szUserName, (LPSTR)lpUserInfo, sizeof( USER_INFO ) );
		if( !ptr ) {
			dwRetCode = TERR_USER_NOT_EXIST;
			__leave;
		}

	}
	__finally {
		ReleaseMutex( hUserDBMutex );
	}
	
	return dwRetCode;
}

DWORD TUserSetInfo ( LPCSTR lpUserName, int iIndex, LPVOID lpValueBuf, DWORD cbBuf )
{
	CHAR		szUserName[MAX_USER_NAME];
	PUSER_INFO	ptr = NULL;
	DWORD		dwRetCode = 0;

	if( lpUserName == NULL )
		return ERROR_INVALID_PARAMETER;

	if( *lpUserName == '\x0' )
		return ERROR_INVALID_PARAMETER;

	if( UserDB == NULL )
		return TERR_NO_USER_DB;

	ZeroMemory( szUserName, MAX_USER_NAME );
	lstrcpyn( szUserName, lpUserName, MAX_USER_NAME );
	_strupr( szUserName );

	__try {
		WaitForSingleObject( hUserDBMutex, INFINITE );

		ptr = (PUSER_INFO)readBtreeData( UserDB, szUserName, NULL, sizeof( USER_INFO ) );
		if( !ptr ) {
			dwRetCode = TERR_USER_NOT_EXIST;
			__leave;
		}

		switch( iIndex ) {
		case INFO_USER_ID:
			/*
			if( cbBuf > sizeof( ptr->dwUserId ) ) {
				dwRetCode = ERROR_MORE_DATA;
			break;
			}
		
			CopyMemory( (LPVOID)&ptr->dwUserId, (CONST VOID *)lpValueBuf, sizeof( ptr->dwUserId ) );
			*/
			dwRetCode = TERR_CAN_NOT_ACCESS;
			break;

		case INFO_PASSWD:			
			if( cbBuf >= MAX_PASSWD ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			lstrcpyn( ptr->szPasswd, (LPSTR)lpValueBuf, cbBuf+1 );
			break;

		case INFO_HOME_DIR:
			if( cbBuf >= MAX_PATH ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			lstrcpyn( ptr->szHomeDir, (LPSTR)lpValueBuf, cbBuf+1 );
			mkdirs( ptr->szHomeDir );
			break;

		case INFO_LOGON_COMPUTER:
			if( cbBuf >= MAX_LOGON_COMPUTER ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			lstrcpyn( ptr->szLogonComputer, (LPSTR)lpValueBuf, MAX_LOGON_COMPUTER );
			break;

		case INFO_LOGON_HOURS:
			if( cbBuf > MAX_LOGON_HOURS ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			CopyMemory( (PVOID)ptr->bLogonHours, (CONST VOID *)lpValueBuf, MAX_LOGON_HOURS );
			break;

		case INFO_ACCESS_MASK:
			if( cbBuf > MAX_ACCESS_MASK ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			CopyMemory( (PVOID)ptr->szAccessMask, (CONST VOID *)lpValueBuf, MAX_ACCESS_MASK );
			break;

		case INFO_DESCRIPTION:
			if( cbBuf < MAX_DESCRIPTION ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			lstrcpyn( ptr->szDescription, (LPSTR)lpValueBuf, MAX_DESCRIPTION );
			break;

		case INFO_LOGON_FAIL:
			if( cbBuf != sizeof( int ) ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			ptr->cLogonFail = (char)(*((int *)lpValueBuf));
			break;

		case INFO_LOCKED:
			if( cbBuf != 1 ) {
				dwRetCode = ERROR_MORE_DATA;
				break;
			}

			ptr->cLocked = *((char *)lpValueBuf);
			break;

		default:
			dwRetCode = TERR_CAN_NOT_ACCESS;
		}

		if( dwRetCode == 0 ) {
			if( writeBtreeData( UserDB, szUserName, (LPSTR)ptr,  sizeof( USER_INFO ) ) != 0 ) {
				dwRetCode = TERR_WRITE_USER_DB;
				__leave;
			}
			dbTreeFlush( UserDB );
		}
	}
	__finally {
		if( ptr )
			freeBtreeMem( (LPSTR)ptr );

		ReleaseMutex( hUserDBMutex );
	}
	
	return dwRetCode;
}

DWORD TUserSetAllInfo ( LPCSTR lpUserName, PUSER_INFO lpUserInfo )
{
	CHAR	szUserName[MAX_USER_NAME];
	LPSTR	ptr = NULL;
	DWORD	dwRetCode = 0;

	
	if( lpUserName == NULL || lpUserInfo == NULL )
		return ERROR_INVALID_PARAMETER;

	if( *lpUserName == '\x0' )
		return ERROR_INVALID_PARAMETER;

	if( UserDB == NULL )
		return TERR_NO_USER_DB;

	ZeroMemory( szUserName, MAX_USER_NAME );
	lstrcpyn( szUserName, lpUserName, MAX_USER_NAME );
	_strupr( szUserName );

	CopyMemory( lpUserInfo->szUser, szUserName, MAX_USER_NAME );

	__try {
		WaitForSingleObject( hUserDBMutex, INFINITE );

		ptr = readBtreeData( UserDB, szUserName, NULL, sizeof( USER_INFO ) );
		if( !ptr )
		{
			dwRetCode = TERR_USER_NOT_EXIST;
			__leave;
		}			

		lpUserInfo->dwUserId = ((PUSER_INFO)ptr)->dwUserId;
		freeBtreeMem( ptr );

		if( writeBtreeData( UserDB, szUserName, (LPSTR)lpUserInfo,  sizeof( USER_INFO ) ) != 0 ) {
			dwRetCode = TERR_WRITE_USER_DB;
			__leave;
		}
		dbTreeFlush( UserDB );
	}
	__finally {
		ReleaseMutex( hUserDBMutex );
	}
	
	return dwRetCode;
}

DWORD TUserIncLogonFailCount ( LPCSTR lpUserName )
{
	CHAR szUserName[MAX_USER_NAME];
	PUSER_INFO ptr = NULL;
	DWORD dwRetCode = 0;
	CHAR MaxLogonFailCount;
	CHAR EnableLock;

	if( lpUserName == NULL )
		return ERROR_INVALID_PARAMETER;

	if( *lpUserName == '\x0' )
		return ERROR_INVALID_PARAMETER;

	if( UserDB == NULL )
		return TERR_NO_USER_DB;

	__try {
		WaitForSingleObject( hUserDBMutex, INFINITE );

		ZeroMemory( szUserName, MAX_USER_NAME );
		lstrcpy( szUserName, "#T_USER" );

		ptr = (PUSER_INFO)readBtreeData( UserDB, szUserName, NULL, sizeof( USER_INFO ) );
		if( !ptr ) {
			dwRetCode = TERR_USER_DB_INTERNAL_ERROR;
			__leave;
		}

		MaxLogonFailCount = (int)ptr->cLogonFail;
		EnableLock = ptr->cLocked;
		freeBtreeMem( (LPSTR)ptr );
		
		ZeroMemory( szUserName, MAX_USER_NAME );
		lstrcpyn( szUserName, lpUserName, MAX_USER_NAME );
		_strupr( szUserName );

		ptr = (PUSER_INFO)readBtreeData( UserDB, szUserName, NULL, sizeof( USER_INFO ) );
		if( !ptr ) {
			dwRetCode = TERR_USER_NOT_EXIST;
			__leave;
		}

		ptr->cLogonFail++;

		if( ptr->cLogonFail >= MaxLogonFailCount && EnableLock == '\x1' )
			ptr->cLocked = '\x1';
		
		if( writeBtreeData( UserDB, szUserName, (LPSTR)ptr,  sizeof( USER_INFO ) ) != 0 ) {
			dwRetCode = TERR_WRITE_USER_DB;
			__leave;
		}
		dbTreeFlush( UserDB );
	}
	__finally {
		if( ptr )
			freeBtreeMem( (LPSTR)ptr );

		ReleaseMutex( hUserDBMutex );
	}
	
	return dwRetCode;
}

DWORD TUserClearLogonFailCount ( LPCSTR lpUserName )
{
	CHAR szUserName[MAX_USER_NAME];
	PUSER_INFO ptr = NULL;
	DWORD dwRetCode = 0;

	if( lpUserName == NULL )
		return ERROR_INVALID_PARAMETER;

	if( *lpUserName == '\x0' )
		return ERROR_INVALID_PARAMETER;

	if( UserDB == NULL )
		return TERR_NO_USER_DB;

	ZeroMemory( szUserName, MAX_USER_NAME );
	lstrcpyn( szUserName, lpUserName, MAX_USER_NAME );
	_strupr( szUserName );

	__try {
		WaitForSingleObject( hUserDBMutex, INFINITE );

		ptr = (PUSER_INFO)readBtreeData( UserDB, szUserName, NULL, sizeof( USER_INFO ) );
		if( !ptr ) {
			dwRetCode = TERR_USER_NOT_EXIST;
			__leave;
		}

		if( ptr->cLogonFail == '\x0' ) {
			dwRetCode = 0;
			__leave;
		}
		
		ptr->cLogonFail = '\x0';

		if( writeBtreeData( UserDB, szUserName, (LPSTR)ptr,  sizeof( USER_INFO ) ) != 0 ) {
			dwRetCode = TERR_WRITE_USER_DB;
			__leave;
		}
		dbTreeFlush( UserDB );
	}
	__finally {
		if( ptr )
			freeBtreeMem( (LPSTR)ptr );

		ReleaseMutex( hUserDBMutex );
	}
	
	return dwRetCode;
}

BOOL TUserLock ( LPCSTR lpUserName )
{
	CHAR szUserName[MAX_USER_NAME];
	PUSER_INFO ptr = NULL;
	DWORD dwRetCode = 0;

	if( lpUserName == NULL ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if( *lpUserName == '\x0' ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if( UserDB == NULL ) {
		SetLastError( TERR_NO_USER_DB );
		return FALSE;
	}

	ZeroMemory( szUserName, MAX_USER_NAME );
	lstrcpyn( szUserName, lpUserName, MAX_USER_NAME );
	_strupr( szUserName );

	__try {
		WaitForSingleObject( hUserDBMutex, INFINITE );

		ptr = (PUSER_INFO)readBtreeData( UserDB, szUserName, NULL, sizeof( USER_INFO ) );
		if( !ptr ) {
			dwRetCode = TERR_USER_NOT_EXIST;
			__leave;
		}

		if( ptr->cLocked == '\x1' ) {
			dwRetCode = 0;
			__leave;
		}

		ptr->cLocked = '\x1';
		if( writeBtreeData( UserDB, szUserName, (LPSTR)ptr,  sizeof( USER_INFO ) ) != 0 ) {
			dwRetCode = TERR_WRITE_USER_DB;
			__leave;
		}
		dbTreeFlush( UserDB );
	}
	__finally {
		if( ptr )
			freeBtreeMem( (LPSTR)ptr );

		ReleaseMutex( hUserDBMutex );
	}
	
	if( dwRetCode != 0 ) {
		SetLastError( dwRetCode );
		return FALSE;
	}
	else
		return TRUE;
}

BOOL TUserUnlock ( LPCSTR lpUserName )
{
	CHAR szUserName[MAX_USER_NAME];
	PUSER_INFO ptr = NULL;
	DWORD dwRetCode = 0;

	if( lpUserName == NULL ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if( *lpUserName == '\x0' ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if( UserDB == NULL ) {
		SetLastError( TERR_NO_USER_DB );
		return FALSE;
	}

	ZeroMemory( szUserName, MAX_USER_NAME );
	lstrcpyn( szUserName, lpUserName, MAX_USER_NAME );
	_strupr( szUserName );

	__try {
		WaitForSingleObject( hUserDBMutex, INFINITE );

		ptr = (PUSER_INFO)readBtreeData( UserDB, szUserName, NULL, sizeof( USER_INFO ) );
		if( !ptr ) {
			dwRetCode = TERR_USER_NOT_EXIST;
			__leave;
		}

		if( ptr->cLocked == '\x0' ) {
			dwRetCode = 0;
			__leave;
		}

		ptr->cLocked = '\x0';
		if( writeBtreeData( UserDB, szUserName, (LPSTR)ptr,  sizeof( USER_INFO ) ) != 0 ) {
			dwRetCode = TERR_WRITE_USER_DB;
			__leave;
		}
		dbTreeFlush( UserDB );
	}
	__finally {
		if( ptr )
			freeBtreeMem( (LPSTR)ptr );

		ReleaseMutex( hUserDBMutex );
	}
	
	if( dwRetCode != 0 ) {
		SetLastError( dwRetCode );
		return FALSE;
	}
	else
		return TRUE;
}

DWORD TUserEnum ( LPENUMCALLBACK lpEnumProc, LPEXCHNG_BUF_INFO lpExchangeBuf )
{
	LPSTR lpKey;
	DWORD dwRetCode;

	if( lpEnumProc == NULL )
		return ERROR_INVALID_PARAMETER;

	if( UserDB == NULL )
		return TERR_NO_USER_DB;

	WaitForSingleObject( hUserDBMutex, INFINITE );

	dbTreeGoTop( UserDB );
	while( !dbTreeEof( UserDB ) ) {
		lpKey = dbTreeGetKeyContent( UserDB );
		if( strcmpi( lpKey, "#T_USER" ) == 0 ) {
			dbTreeSkip( UserDB, 1 );
			continue;
		}

		dwRetCode = (*lpEnumProc)( lpKey, lpExchangeBuf, FALSE );
		if( dwRetCode != 0 ) {
			ReleaseMutex( hUserDBMutex );
			return dwRetCode;
		}

		dbTreeSkip( UserDB, 1 );
	}
	lpKey = dbTreeGetKeyContent( UserDB );
	dwRetCode = (*lpEnumProc)( lpKey, lpExchangeBuf, TRUE );
	
	ReleaseMutex( hUserDBMutex );
	
	if( dwRetCode != 0 )
		return dwRetCode;

	return 0;
}

BOOL TUserLogon ( LPCSTR lpUserName, LPCSTR lpPasswd, LPCSTR lpComputer, 
				 LPDWORD lpdwUserId )
{
	USER_INFO	UserInfo;
	LPSTR		ptr;
    BOOL		bEnable = FALSE;
	DWORD		dwRetCode;
	SYSTEMTIME	sysTime;
	int         iStrCmp;

	if( lpUserName == NULL || lpPasswd == NULL ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	dwRetCode = TUserGetAllInfo( lpUserName, &UserInfo );
	if( dwRetCode != 0 ) {
		SetLastError( dwRetCode );
		return FALSE;
	}

	GetLocalTime( &sysTime );

	if( *lpPasswd == '\t' ) {
		
		char buf[128];
		lstrcpy(buf, lpUserName);
		
		iStrCmp = strcmp(EncryptPasswd(buf), lpPasswd+1);
	} else {
		iStrCmp = strcmp( lpPasswd, UserInfo.szPasswd );
	}

	if( iStrCmp != 0 ) {
		SetLastError( TERR_INVALID_PASSWORD );

		// Inc the user logon fail count.
		UserInfo.cLogonFail += 1;

		UserInfo.tLastLogonFail.year = (unsigned short)sysTime.wYear;
		UserInfo.tLastLogonFail.month = (unsigned char)sysTime.wMonth;
		UserInfo.tLastLogonFail.day = (unsigned char)sysTime.wDay;
		UserInfo.tLastLogonFail.hour = (unsigned char)sysTime.wHour;
		UserInfo.tLastLogonFail.min = (unsigned char)sysTime.wMinute;
		UserInfo.tLastLogonFail.sec = (unsigned char)sysTime.wSecond;
		UserInfo.tLastLogonFail.week = (unsigned char)sysTime.wDayOfWeek;
	
		TUserSetAllInfo ( lpUserName, &UserInfo );

		return FALSE;
	}

	if( UserInfo.cLocked == '\x1' ) {
		SetLastError( TERR_USER_LOCKED );

		UserInfo.tLastLogonFail.year = (unsigned short)sysTime.wYear;
		UserInfo.tLastLogonFail.month = (unsigned char)sysTime.wMonth;
		UserInfo.tLastLogonFail.day = (unsigned char)sysTime.wDay;
		UserInfo.tLastLogonFail.hour = (unsigned char)sysTime.wHour;
		UserInfo.tLastLogonFail.min = (unsigned char)sysTime.wMinute;
		UserInfo.tLastLogonFail.sec = (unsigned char)sysTime.wSecond;
		UserInfo.tLastLogonFail.week = (unsigned char)sysTime.wDayOfWeek;
	
		TUserSetAllInfo ( lpUserName, &UserInfo );

		return FALSE;
	}

	if( UserInfo.szLogonComputer[0] != '\x0' ) {
		for( ptr = UserInfo.szLogonComputer; *ptr != '\x0' 
				&& ptr - UserInfo.szLogonComputer < MAX_LOGON_COMPUTER; 
				ptr += (lstrlen( ptr ) + 1) ) {
			if( strcmpi( lpComputer, ptr ) == 0 ) {
				bEnable = TRUE;
				break;
			}
		}
	}
	else
		bEnable = TRUE;

	if( !bEnable ) {
		SetLastError( TERR_INVALID_LOGON_COMPUTER );

		// Inc the user logon fail count.
		(UserInfo.cLogonFail)++;
	
		UserInfo.tLastLogonFail.year = (unsigned short)sysTime.wYear;
		UserInfo.tLastLogonFail.month = (unsigned char)sysTime.wMonth;
		UserInfo.tLastLogonFail.day = (unsigned char)sysTime.wDay;
		UserInfo.tLastLogonFail.hour = (unsigned char)sysTime.wHour;
		UserInfo.tLastLogonFail.min = (unsigned char)sysTime.wMinute;
		UserInfo.tLastLogonFail.sec = (unsigned char)sysTime.wSecond;
		UserInfo.tLastLogonFail.week = (unsigned char)sysTime.wDayOfWeek;
	
		TUserSetAllInfo ( lpUserName, &UserInfo );
		
		return FALSE;
	}

	*lpdwUserId = UserInfo.dwUserId;

	UserInfo.tLastLogon.year = (unsigned short)sysTime.wYear;
	UserInfo.tLastLogon.month = (unsigned char)sysTime.wMonth;
	UserInfo.tLastLogon.day = (unsigned char)sysTime.wDay;
	UserInfo.tLastLogon.hour = (unsigned char)sysTime.wHour;
	UserInfo.tLastLogon.min = (unsigned char)sysTime.wMinute;
	UserInfo.tLastLogon.sec = (unsigned char)sysTime.wSecond;
	UserInfo.tLastLogon.week = (unsigned char)sysTime.wDayOfWeek;
	
	// Clear the user logon fail count.
	UserInfo.cLogonFail = '\x0';
	
	TUserSetAllInfo ( lpUserName, &UserInfo );

	return TRUE;
}

/*
void CALLBACK DisplayUserName( LPSTR szUserName )
{
	printf( "\t%s\n", szUserName );
}

void main( void )
{
	CHAR szUserDB[MAX_PATH] = "D:\\User.DB";
	USER_INFO UserInfo;
	DWORD dwRetCode;
	char *ptr;
	char szCmdLine[255];
	char szErrorMsg[255];

	if( _access( szUserDB, 00 ) != 0 ) {
		if( !TCreateUserDB ( szUserDB ) ) {
			dwRetCode = GetLastError();
			GetErrorMessage( dwRetCode, szErrorMsg, 255 );
			printf( "Error Code: %ld. Error Message: %s\n", dwRetCode, szErrorMsg );
			return;
		}
		printf( "Create a new user database.\n" );
	}

	if( !TOpenUserDB ( szUserDB ) ) {
			dwRetCode = GetLastError();
			GetErrorMessage( dwRetCode, szErrorMsg, 255 );
			printf( "Error Code: %ld. Error Message: %s\n", dwRetCode, szErrorMsg );
	}

	while( 1 ) {
		ZeroMemory( (PVOID)&UserInfo, sizeof( USER_INFO ) );
		printf( "# " );
		gets( szCmdLine );
		
		for( ptr = szCmdLine; *ptr != '\x0'; ptr ++ ) {
			if( *ptr == ' ' )
				*ptr = '\x0';
		}
		*(++ptr) = '\x0';

		ptr = szCmdLine;

		if( lstrcmpi( ptr, "ADD" ) == 0 ) {
			char buffer[36];
			int len, pos = 0;

			ptr += ( lstrlen( ptr ) + 1 );
			if( *ptr == '\x0' ) {
				printf( "ADD [UserName]\n" );
				continue;
			}

			if( lstrlen( ptr ) >= MAX_USER_NAME ) {
				printf( "User name can not exceed %d character.\n", MAX_USER_NAME );
				continue;
			}
			lstrcpyn( UserInfo.szUser, ptr, MAX_USER_NAME );
			
			printf( "Description: " );
			gets( UserInfo.szDescription );
			printf( "Password: " );
			gets( UserInfo.szPasswd );
			printf( "Home Directory: " );
			gets( UserInfo.szHomeDir );
			printf( "Logon Computers: \n" );
			while( ( len = lstrlen( gets( buffer ) ) ) != 0 ) {
				lstrcpy( &(UserInfo.szLogonComputer[pos]), buffer );
				pos += (len+1);
			}
			UserInfo.szLogonComputer[pos] = '\x0';

			dwRetCode = TUserAdd( &UserInfo );
			if( dwRetCode != 0 ) {
				GetErrorMessage( dwRetCode, szErrorMsg, 255 );
				printf( "Error Code: %ld. Error Message: %s\n", dwRetCode, szErrorMsg );
			}

			printf( "The command completed successfully.\n" );
		}
		else if( lstrcmpi( ptr, "DEL" ) == 0 ) {
			ptr += ( lstrlen( ptr ) + 1 );
			if( *ptr == '\x0' ) {
				printf( "DEL [UserName]\n" );
				continue;
			}

			dwRetCode = TUserDelete( ptr );
			if( dwRetCode != 0 ) {
				GetErrorMessage( dwRetCode, szErrorMsg, 255 );
				printf( "Error Code: %ld. Error Message: %s\n", dwRetCode, szErrorMsg );
			}			
			printf( "The command completed successfully.\n" );
		}
		else if( lstrcmpi( ptr, "MODIFY" ) == 0 ) {
			ptr += ( lstrlen( ptr ) + 1 );
			if( *ptr == '\x0' ) {
				printf( "DEL [UserName]\n" );
				continue;
			}

			dwRetCode = TUserGetAllInfo( ptr, &UserInfo );
			if( dwRetCode != 0 ) {
				GetErrorMessage( dwRetCode, szErrorMsg, 255 );
				printf( "Error Code: %ld. Error Message: %s\n", dwRetCode, szErrorMsg );
				continue;
			}
		}
		else if( lstrcmpi( ptr, "FINGER" ) == 0 ) {
			char tmp[8];
			char *p;

			ptr += ( lstrlen( ptr ) + 1 );
			if( *ptr == '\x0' ) {
				printf( "FINGER [UserName]\n" );
				continue;
			}

			dwRetCode = TUserGetAllInfo( ptr, &UserInfo );
			if( dwRetCode != 0 ) {
				GetErrorMessage( dwRetCode, szErrorMsg, 255 );
				printf( "Error Code: %ld. Error Message: %s\n", dwRetCode, szErrorMsg );
				continue;
			}

			if( UserInfo.cLocked == '\x0' )
				lstrcpy( tmp, "No" );
			else if( UserInfo.cLocked == '\x1' )
				lstrcpy( tmp, "Yes" );
			else
				lstrcpy( tmp, "Unkown" );

			printf( "User Name: %s\n", UserInfo.szUser );
			printf( "User ID: %ld\n", UserInfo.dwUserId );
			printf( "Description: %s\n", UserInfo.szDescription );
			printf( "Password: %s\n", UserInfo.szPasswd );
			printf( "Home Directory: %s\n", UserInfo.szHomeDir );
			printf( "Account Locked: %s\n", tmp );
			printf( "Logon Failure Count: %d\n", (int)UserInfo.cLogonFail );
			printf( "Logon Computers: \n" );
			for( p = UserInfo.szLogonComputer; *p != '\x0'; p += (lstrlen( p )+1) ) {
				printf( "\t%s\n", p );
			}
			printf( "The command completed successfully.\n" );
		}
		else if( lstrcmpi( ptr, "LOGON" ) == 0 ) {
			char passwd[36];
			char computer[36];
			DWORD dwUserId;

			ptr += ( lstrlen( ptr ) + 1 );
			if( *ptr == '\x0' ) {
				printf( "FINGER [UserName]\n" );
				continue;
			}
			
			printf( "Password: " );
			gets( passwd );
			printf( "Computer: " );
			gets( computer );

			if( !TUserLogon( ptr, passwd, computer, &dwUserId ) ) {
				dwRetCode = GetLastError();
				GetErrorMessage( dwRetCode, szErrorMsg, 255 );
				printf( "Error Code: %ld. Error Message: %s\n", dwRetCode, szErrorMsg );
				continue;
			}
			printf( "User ID: %ld\n", dwUserId );
			printf( "The command completed successfully.\n" );
		}
		else if( lstrcmpi( ptr, "LOCK" ) == 0 ) {
			ptr += ( lstrlen( ptr ) + 1 );
			if( *ptr == '\x0' ) {
				printf( "LOCK [UserName]\n" );
				continue;
			}

			if( !TUserLock( ptr ) ) {
				dwRetCode = GetLastError();
				GetErrorMessage( dwRetCode, szErrorMsg, 255 );
				printf( "Error Code: %ld. Error Message: %s\n", dwRetCode, szErrorMsg );
				continue;
			}
			printf( "The command completed successfully.\n" );
		}
		else if( lstrcmpi( ptr, "UNLOCK" ) == 0 ) {
			ptr += ( lstrlen( ptr ) + 1 );
			if( *ptr == '\x0' ) {
				printf( "UNLOCK [UserName]\n" );
				continue;
			}

			if( !TUserUnlock( ptr ) ) {
				dwRetCode = GetLastError();
				GetErrorMessage( dwRetCode, szErrorMsg, 255 );
				printf( "Error Code: %ld. Error Message: %s\n", dwRetCode, szErrorMsg );
				continue;
			}
			printf( "The command completed successfully.\n" );
		}
		else if( lstrcmpi( ptr, "LIST" ) == 0 ) {
			printf( "-----------------------------------------------\n" );

			dwRetCode = TUserEnum( (FARPROC)DisplayUserName );
			if( dwRetCode != 0 ) {
				GetErrorMessage( dwRetCode, szErrorMsg, 255 );
				printf( "Error Code: %ld. Error Message: %s\n", dwRetCode, szErrorMsg );
				continue;
			}
			printf( "The command completed successfully.\n" );
		}
		else if( lstrcmpi( ptr, "EXIT" ) == 0 ) {
			break;
		}
		else {
			if( *ptr != '\x0' )
				printf( "Invalid command \"%s\".\n", ptr );
		}
	}

	TCloseUserDB();
}
*/
