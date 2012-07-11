/******************************
*	Module Name:
*		S_PROC.C
*
*	Abstract:
*		TreeServer stored procedure module. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1998  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1998.07
********************************************************************************/

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include "s_proc.h"
#include "dbtree.h"
#include "tlimits.h"
#include "terror.h"

SP_FILE spFile;
HANDLE hSPMutex;

BOOL spCreateFile ( LPCSTR lpFileName )
{
	if( lpFileName == NULL ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if( dbTreeBuild( (LPSTR)lpFileName, MAX_PRIVILEGE_NAME, PROC_NODE_SIZE ) != 0 ) {
		SetLastError( TERR_CREATE_SP_FILE ); 
		return FALSE;
	}

	return TRUE;
}

BOOL spDeleteFile ( LPCSTR lpFileName )
{
	if( lpFileName == NULL ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	return DeleteFile( lpFileName );
}

BOOL OpenStoredProcFile ( LPCSTR lpFileName )
{
	DWORD dwRetCode;

	spFile = dbTreeOpen( (LPSTR)lpFileName );
	if( spFile == NULL ) {
		SetLastError( TERR_OPEN_SP_FILE ); 
		return FALSE;
	}

	hSPMutex = CreateMutex( NULL, FALSE, _TEXT("TS_StoredProcMutex") );
	dwRetCode = GetLastError();
	if( dwRetCode != 0 ) {
		if( hSPMutex ) 
			CloseHandle( hSPMutex );
		
		dbTreeClose( spFile );
		SetLastError( TERR_SP_FILE_IN_USE );
		return FALSE;
	}

	return TRUE;
}

BOOL CloseStoredProcFile ( VOID )
{
	if( spFile ) {
		CloseHandle( hSPMutex );

		if( dbTreeClose( spFile ) != 0 ) {
			SetLastError( TERR_CLOSE_SP_FILE ); 
			return FALSE;
		}
		else
			return TRUE;
	}

	return TRUE;
}

DWORD MakeStoredProcName ( LPCSTR lpUserName, LPCSTR lpProcName, LPSTR lpCompleteProcName )
{
	if( lpUserName == NULL || lpProcName == NULL || 
			lpCompleteProcName == NULL || *lpUserName == '\x0' ||
			*lpProcName == '\x0' )
		return ERROR_INVALID_PARAMETER;

	if( lstrlen( lpUserName ) >= MAX_USER_NAME ||
			lstrlen( lpProcName ) >= MAX_PROC_NAME )
		return TERR_INVALID_PROC_NAME;

	if( strchr( lpProcName, '.' ) ) {
		lstrcpy( lpCompleteProcName, lpProcName );
		return 0;
	}

	if( lstrlen( lpProcName ) + lstrlen( lpUserName ) + 1 >= MAX_PROC_NAME )
		return TERR_INVALID_PROC_NAME;

	wsprintf( lpCompleteProcName, "%s.%s", lpUserName, lpProcName );
	return 0;
}

BOOL spAuditUserPrivilege ( LPCSTR lpProcName, DWORD dwUserId, char cPrivilege )
{
	CHAR szPrivilegeName[MAX_PRIVILEGE_NAME];
	long dataLen;
	LPSP_PRIVILEGE ptr=NULL;
	LPSP_PRIVILEGE p;
	BOOL bFind = FALSE;
	BOOL bRetCode = TRUE;
	
	if( lpProcName == NULL || dwUserId < 0 || 
			lstrlen( lpProcName ) >= MAX_PROC_NAME ||
			cPrivilege <= '\x0' || cPrivilege > '\x7' ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	ZeroMemory( szPrivilegeName, MAX_PRIVILEGE_NAME );
	wsprintf( szPrivilegeName, "SP.%s", lpProcName );
	strupr( szPrivilegeName );

	__try {
		WaitForSingleObject( hSPMutex, INFINITE );

		dataLen = getBtreeDataLen( spFile, szPrivilegeName );
		if( dataLen <= 0 ) {
			SetLastError( TERR_PROC_NOT_EXIST );
			bRetCode = FALSE;
			__leave;
		}

		ptr = (LPSP_PRIVILEGE)malloc( dataLen );
		if( ptr == NULL ) {
			bRetCode = FALSE;
			__leave;
		}

		ptr = (LPSP_PRIVILEGE)readBtreeData( spFile, szPrivilegeName, (LPSTR)ptr, dataLen );
		if( !ptr ) {
			SetLastError( TERR_READ_SP_FILE );
			bRetCode = FALSE;
			__leave;
		}

		for( p = ptr; (LPSTR)p < (LPSTR)ptr + dataLen - sizeof(SP_PRIVILEGE); p++ ) {
			if( p->dwUserId == dwUserId ) {
				bFind = TRUE;
				dataLen -= sizeof(SP_PRIVILEGE);
				break;
			}
		}

		if( (p->cPrivilege & cPrivilege) != cPrivilege )
			bRetCode = FALSE;
	}
	__finally {
		if( ptr )
			free( (LPSTR)ptr );

		ReleaseMutex( hSPMutex );
	}

	return bRetCode;
}

DWORD spGrantUserPrivilege ( LPCSTR lpProcName, DWORD dwUserId, char cPrivilege )
{
	CHAR szPrivilegeName[MAX_PRIVILEGE_NAME];
	long dataLen;
	LPSP_PRIVILEGE ptr;
	BOOL bNew = FALSE;
	DWORD dwRetCode = 0;
	
	if( lpProcName == NULL || dwUserId < 0 || 
			lstrlen( lpProcName ) >= MAX_PROC_NAME ||
			cPrivilege <= '\x0' || cPrivilege > '\x7' )
		return ERROR_INVALID_PARAMETER;

	ZeroMemory( szPrivilegeName, MAX_PRIVILEGE_NAME );
	wsprintf( szPrivilegeName, "SP.%s", lpProcName );
	strupr( szPrivilegeName );
	
	__try {
		WaitForSingleObject( hSPMutex, INFINITE );

		dataLen = getBtreeDataLen( spFile, szPrivilegeName );
		if( dataLen > 0 )
			dataLen += sizeof(SP_PRIVILEGE);
		else {
			dataLen = sizeof(SP_PRIVILEGE);
			bNew = TRUE;
		}

		ptr = (LPSP_PRIVILEGE)malloc( dataLen );
		if( ptr == NULL ) {
			dwRetCode = GetLastError();
			__leave;
		}

		if( bNew ) {
			ptr->dwUserId = dwUserId;
			ptr->cPrivilege = cPrivilege;
		} 
		else {
			LPSP_PRIVILEGE p;

			ptr = (LPSP_PRIVILEGE)readBtreeData( spFile, szPrivilegeName, (LPSTR)ptr, dataLen-sizeof(SP_PRIVILEGE) );
			if( !ptr ) {
				dwRetCode = TERR_READ_SP_FILE;
				__leave;
			}

			for( p = ptr; (LPSTR)p < (LPSTR)ptr + dataLen - sizeof(SP_PRIVILEGE); p++ ) {
				if( p->dwUserId == dwUserId ) {
					dataLen -= sizeof(SP_PRIVILEGE);
					break;
				}
			}

			p->cPrivilege |= cPrivilege;
		}

		if( writeBtreeData(  spFile, szPrivilegeName, (LPSTR)ptr, dataLen ) != 0 ) {
			dwRetCode = TERR_WRITE_SP_FILE;
			__leave;
		}

		dbTreeFlush( spFile );
	}
	__finally {
		if( ptr )
			free( (LPSTR)ptr );

		ReleaseMutex( hSPMutex );
	}

	return dwRetCode;
}

DWORD spRevokeUserPrivilege ( LPCSTR lpProcName, DWORD dwUserId, char cPrivilege )
{
	CHAR szPrivilegeName[MAX_PRIVILEGE_NAME];
	long dataLen;
	LPSP_PRIVILEGE ptr;
	LPSP_PRIVILEGE p;
	BOOL bFind = FALSE;
	DWORD dwRetCode = 0;
	
	if( lpProcName == NULL || dwUserId < 0 || 
			lstrlen( lpProcName ) >= MAX_PROC_NAME ||
			cPrivilege <= '\x0' || cPrivilege > '\x7' )
		return ERROR_INVALID_PARAMETER;

	ZeroMemory( szPrivilegeName, MAX_PRIVILEGE_NAME );
	wsprintf( szPrivilegeName, "SP.%s", lpProcName );
	strupr( szPrivilegeName );
	
	__try {
		WaitForSingleObject( hSPMutex, INFINITE );

		dataLen = getBtreeDataLen( spFile, szPrivilegeName );
		if( dataLen <= 0 ) {
			dwRetCode = TERR_PROC_NOT_EXIST;
			__leave;
		}

		ptr = (LPSP_PRIVILEGE)malloc( dataLen );
		if( ptr == NULL ) {
			dwRetCode = GetLastError();
			__leave;
		}

		ptr = (LPSP_PRIVILEGE)readBtreeData( spFile, szPrivilegeName, (LPSTR)ptr, dataLen-sizeof(SP_PRIVILEGE) );
		if( !ptr ) {
			dwRetCode = TERR_READ_SP_FILE;
			__leave;
		}

		for( p = ptr; (LPSTR)p < (LPSTR)ptr + dataLen - sizeof(SP_PRIVILEGE); p++ ) {
			if( p->dwUserId == dwUserId ) {
				bFind = TRUE;
				break;
			}

			if( !bFind ) 
				__leave;

			p->cPrivilege &= (~cPrivilege)&0x07;
			if( p->cPrivilege == 0 ) {
				int size;

				size = dataLen - (p-ptr+1)*sizeof(SP_PRIVILEGE);
				memmove( (LPSTR)p, (LPSTR)p+sizeof(SP_PRIVILEGE), size );
				dataLen -= sizeof(SP_PRIVILEGE);
			}
		}

		if( writeBtreeData(  spFile, szPrivilegeName, (LPSTR)ptr, dataLen ) != 0 ) {
			dwRetCode = TERR_WRITE_SP_FILE;
			__leave;
		}

		dbTreeFlush( spFile );
	}
	__finally {
		if( ptr )
			free( (LPSTR)ptr );

		ReleaseMutex( hSPMutex );
	}

	return dwRetCode;
}

DWORD NewStoredProc ( LPCSTR lpProcName, DWORD dwUserId, LPSTR lpBuffer, DWORD dwcbBuffer )
{
	CHAR szProcName[MAX_PRIVILEGE_NAME];
	DWORD dwRetCode = 0;
	int dataLen;

	if( lpProcName == NULL || dwUserId <= 0 ||
		lstrlen( lpProcName ) >= MAX_PROC_NAME ||
		lpBuffer == NULL || dwcbBuffer == 0 )
		return ERROR_INVALID_PARAMETER;

	ZeroMemory( szProcName, MAX_PRIVILEGE_NAME );
	lstrcpy( szProcName, lpProcName );
	strupr( szProcName );

	__try {
		WaitForSingleObject( hSPMutex, INFINITE );

		dataLen = getBtreeDataLen( spFile, szProcName );
		if( dataLen > 0 ) {
			dwRetCode = TERR_PROC_ALREADY_EXIST;
			__leave;
		}

		if( writeBtreeData( spFile, szProcName, lpBuffer, dwcbBuffer ) != 0 ) {
			dwRetCode = TERR_WRITE_SP_FILE;
			__leave;
		}
		
		dwRetCode = spGrantUserPrivilege( lpProcName, dwUserId, PRIVILEGE_ALL );
		if( dwRetCode != 0 )
			freeBtreeData( spFile, szProcName );

		dbTreeFlush( spFile );
	}
	__finally {
		ReleaseMutex( hSPMutex );
	}

	return dwRetCode;
}

DWORD DeleteStoredProc ( LPCSTR lpProcName, DWORD dwUserId )
{
	CHAR szProcName[MAX_PRIVILEGE_NAME];
	CHAR szPrivilegeName[MAX_PRIVILEGE_NAME];
	DWORD dwRetCode = 0;
	int dataLen;

	if( lpProcName == NULL || dwUserId <= 0 ||
		lstrlen( lpProcName ) >= MAX_PROC_NAME )
		return ERROR_INVALID_PARAMETER;

	ZeroMemory( szProcName, MAX_PRIVILEGE_NAME );
	lstrcpy( szProcName, lpProcName );
	strupr( szProcName );
	ZeroMemory( szPrivilegeName, MAX_PRIVILEGE_NAME );
	wsprintf( szPrivilegeName, "SP.%s", lpProcName );
	strupr( szPrivilegeName );

	if( !spAuditUserPrivilege( lpProcName, dwUserId, PRIVILEGE_WRITE ) )
		return TERR_USER_NO_PRIVILEGE;

	__try {
		WaitForSingleObject( hSPMutex, INFINITE );

		dataLen = getBtreeDataLen( spFile, szProcName );
		if( dataLen <= 0 ) {
			dwRetCode = TERR_PROC_NOT_EXIST;
			__leave;
		}

		if( freeBtreeData( spFile, szProcName ) < 0 ) {
			dwRetCode = TERR_WRITE_SP_FILE;
			__leave;
		}

		if( freeBtreeData( spFile, szPrivilegeName ) < 0 ) {
			dwRetCode = TERR_WRITE_SP_FILE;
			__leave;
		}
	}
	__finally {
		ReleaseMutex( hSPMutex );
	}

	return dwRetCode;
}

DWORD GetStoredProcSize ( LPCSTR lpProcName, DWORD dwUserId )
{
	CHAR szProcName[MAX_PRIVILEGE_NAME];
	DWORD dwRetCode = 0;
	int dataLen;

	if( lpProcName == NULL || dwUserId <= 0 ||
		lstrlen( lpProcName ) >= MAX_PROC_NAME ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return 0xFFFFFFFF;
	}

	if( !spAuditUserPrivilege( lpProcName, dwUserId, PRIVILEGE_READ ) ) {
		return 0xFFFFFFFF;
	}

	ZeroMemory( szProcName, MAX_PRIVILEGE_NAME );
	lstrcpy( szProcName, lpProcName );
	strupr( szProcName );
	
	__try {
		WaitForSingleObject( hSPMutex, INFINITE );

		dataLen = getBtreeDataLen( spFile, szProcName );
		if( dataLen <= 0 ) {
			SetLastError( TERR_PROC_NOT_EXIST );
			dwRetCode = 0xFFFFFFFF;
			__leave;
		}

		dwRetCode = (DWORD)dataLen;
	}
	__finally {
		ReleaseMutex( hSPMutex );
	}

	return dwRetCode;
}

DWORD ReadStoredProc ( LPCSTR lpProcName, DWORD dwUserId, LPSTR lpBuffer, LPDWORD lpdwcbBuffer ) 
{
	CHAR szProcName[MAX_PRIVILEGE_NAME];
	LPSTR ptr;
	DWORD dwRetCode = 0;
	int dataLen;

	if( lpProcName == NULL || dwUserId <= 0 ||
		lstrlen( lpProcName ) >= MAX_PROC_NAME ||
		lpBuffer == NULL || *lpdwcbBuffer == 0 )
		return ERROR_INVALID_PARAMETER;

	if( !spAuditUserPrivilege( lpProcName, dwUserId, PRIVILEGE_READ ) &&
		!spAuditUserPrivilege( lpProcName, dwUserId, PRIVILEGE_READ ) )
		return TERR_USER_NO_PRIVILEGE;

	ZeroMemory( szProcName, MAX_PRIVILEGE_NAME );
	lstrcpy( szProcName, lpProcName );
	strupr( szProcName );
	
	__try {
		WaitForSingleObject( hSPMutex, INFINITE );

		dataLen = getBtreeDataLen( spFile, szProcName );
		if( dataLen <= 0 ) {
			dwRetCode = TERR_PROC_NOT_EXIST;
			__leave;
		}

		if( *lpdwcbBuffer < (DWORD)dataLen ) {
			*lpdwcbBuffer = dataLen; 
			dwRetCode = ERROR_MORE_DATA;
			__leave;
		}

		*lpdwcbBuffer = dataLen; 
		ptr = readBtreeData( spFile, szProcName, lpBuffer, dataLen );
		if( !ptr ) {
			dwRetCode = TERR_READ_SP_FILE;
			__leave;
		}
	}
	__finally {
		ReleaseMutex( hSPMutex );
	}

	return dwRetCode;
}

DWORD UpdateStoredProc ( LPCSTR lpProcName, DWORD dwUserId, LPSTR lpBuffer, DWORD dwcbBuffer )
{
	CHAR szProcName[MAX_PRIVILEGE_NAME];
	DWORD dwRetCode = 0;
	int dataLen;

	if( lpProcName == NULL || dwUserId <= 0 ||
		lstrlen( lpProcName ) >= MAX_PROC_NAME ||
		lpBuffer == NULL || dwcbBuffer == 0 )
		return ERROR_INVALID_PARAMETER;

	ZeroMemory( szProcName, MAX_PRIVILEGE_NAME );
	lstrcpy( szProcName, lpProcName );
	strupr( szProcName );

	if( !spAuditUserPrivilege( lpProcName, dwUserId, PRIVILEGE_WRITE ) )
		return TERR_USER_NO_PRIVILEGE;

	__try {
		WaitForSingleObject( hSPMutex, INFINITE );

		dataLen = getBtreeDataLen( spFile, szProcName );
		if( dataLen <= 0 ) {
			dwRetCode = TERR_PROC_NOT_EXIST;
			__leave;
		}

		if( writeBtreeData( spFile, szProcName, lpBuffer, dwcbBuffer ) != 0 ) {
			dwRetCode = TERR_WRITE_SP_FILE;
			__leave;
		}

		dbTreeFlush( spFile );
	}
	__finally {
		ReleaseMutex( hSPMutex );
	}

	return dwRetCode;
}


DWORD TSprocEnum ( LPENUMCALLBACK lpEnumProc, LPEXCHNG_BUF_INFO lpExchangeBuf )
{
	LPSTR lpKey;
	DWORD dwRetCode;

	if( lpEnumProc == NULL )
		return ERROR_INVALID_PARAMETER;

	if( spFile == NULL )
		return TERR_NO_USER_DB;

	WaitForSingleObject( hSPMutex, INFINITE );
	dbTreeGoTop( spFile );
	while( !dbTreeEof( spFile ) ) {
		lpKey = dbTreeGetKeyContent( spFile );
		if( _strnicmp( lpKey, "SP.", 3) == 0 ) {
			dbTreeSkip( spFile, 1 );
			continue;
		}

		dwRetCode = (*lpEnumProc)( lpKey, lpExchangeBuf, FALSE );
		if( dwRetCode != 0 ) {
			ReleaseMutex( hSPMutex );
			return dwRetCode;
		}

		dbTreeSkip( spFile, 1 );
	}
	lpKey = dbTreeGetKeyContent( spFile );
	dwRetCode = (*lpEnumProc)( lpKey, lpExchangeBuf, TRUE );

	ReleaseMutex( hSPMutex );

	if( dwRetCode != 0 )
		return dwRetCode;

	return 0;
}

