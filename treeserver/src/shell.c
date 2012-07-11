/******************************
*	Module Name:
*		SHELL.C
*
*	Abstract:
*		Tree Server service shell module. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1998  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1998.03
*		Add User Manager Interface by NiuJingyu, 1998.06 
*		Add Stored Procedure Interface by NiuJingyu, 1998.07 
*		Add stored Procedure Implements by Motor, August 28, 1998
********************************************************************************/

#define STRICT
#include <windows.h>
#include <limits.h>

#include "treesvr.h"
#include "shell.h"
#include "tuser.h"
#include "wmtasql.h"
#include "ts_const.h"
#include "tsdbfutl.h"
#include "tsftp.h"
#include "s_proc.h"
#include "tlimits.h"
#include "histdata.h"
#include "ts_dict.h"

typedef struct {
	short  type;
	char   VarOFunName[32];
	unsigned char values[ 32 ];
		/* (long)values()
		 * when the variable is ARRAY type the values stored as
		 * ArrayType
		 */
	short length;
} SysVarOFunType, *PSysVarOFunType, *LPSysVarOFunType;

typedef struct tagESERVICEPACKET {
    LPSTR asqlScript;
    short varNum;
    SysVarOFunType *vars;
} ESERVICEPACKET, *PESERVICEPACKET, *LPESERVICEPACKET;

static long lTaskId = 0;
__declspec( thread )TS_CLIENT_CURSOR *lpTSCliCursor = NULL;


extern long singleUserMode;
extern DWORD singleUserThread;

DWORD CALLBACK UserEnumProc ( LPSTR szUserName, LPEXCHNG_BUF_INFO lpExchangeBuf,
							 BOOL bEndOfEnum );
DWORD CALLBACK SprocEnumProc ( LPSTR szSprocName, LPEXCHNG_BUF_INFO lpExchangeBuf,
							 BOOL bEndOfEnum );

VOID TBackupNotifyShell ( LPSTR lpBuffer, LPREQUEST lpRequest );

/**********
* Only used for tsDbfFree()
***********************************/
__declspec( dllimport ) short dSleep( void *df );

// Function: GetUserWorkDir
//
// NOTE:
//		Add by Jingyu Niu, 1999.08
//		For solve one user overlaped logon.
//		When this happen, return a temp path instead of the ture home path.
//
DWORD GetUserWorkDir ( LPCSTR lpUserName, DWORD dwReqId, LPSTR lpHome, LPDWORD lpdwcbHome )
{
	DWORD dwRetCode;
	char szTempPath[MAX_PATH];
	DWORD dwcbPath;
	char szTempDir[MAX_PATH];

	dwcbPath = MAX_PATH;
	dwRetCode = TUserGetInfo( lpUserName, INFO_HOME_DIR, szTempDir, &dwcbPath );
	if( dwRetCode != 0 )
		return dwRetCode;

	wsprintf( szTempPath, "%s\\D%07X", szTempDir, dwReqId );   
	if( *lpdwcbHome <= (DWORD)lstrlen( szTempPath ) )
		return ERROR_MORE_DATA;

	lstrcpy( lpHome, szTempPath );

	return 0;
}

VOID tsDbfFree ( TS_CLIENT_CURSOR *lpTSCliCursor )
{
	TS_CLIENT_CURSOR *p;

	if( !lpTSCliCursor )
		return;

	while( lpTSCliCursor ) {
		p = lpTSCliCursor->pNext;

		switch( lpTSCliCursor->pType ) {
		case 1:
			dSleep( lpTSCliCursor->p );
			asqlTsFreeMem( lpTSCliCursor );
			break;
		default:
			free( lpTSCliCursor );
		}

		lpTSCliCursor = p;
	}

	lpTSCliCursor = NULL;
	return;
}

VOID DefaultProcess ( LPSTR lpBuffer, LPEXCHNG_BUF_INFO lpExchBufInfo )
{
	LPTS_COM_PROPS lptsComProps;
	CHAR szBuffer[PIPE_BUFFER_SIZE];
	DWORD dwRetCode;

	lptsComProps = (LPTS_COM_PROPS)lpBuffer;

	while( lptsComProps->endPacket != '\x0' ) {
		lptsComProps = (LPTS_COM_PROPS)szBuffer;
		
		dwRetCode = SrvReadExchngBuf( lpExchBufInfo, szBuffer, 4096 );
		if( dwRetCode != TERR_SUCCESS )
			return;
	}

	lptsComProps = (LPTS_COM_PROPS)szBuffer;
	ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
	lptsComProps->packetType = pkTS_ERROR;
	SrvWriteExchngBuf( lpExchBufInfo, szBuffer, 4096 );
}

DWORD ServiceShell ( LPVOID lpvParam )
{
	REQUEST			Request;
	DWORD			dwRetCode;
	LPTS_COM_PROPS	lptsComProps;
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	TS_CLIENT_CURSOR *lpTSCliCursor = NULL;

	GetQueueStatus( QS_SENDMESSAGE ); 

	CopyMemory( (LPVOID)&Request, (CONST VOID *)lpvParam, sizeof(REQUEST) );
	FreeSpace( (LPREQUEST)lpvParam );

	// Add by NiJingyu, 1998.03
	PostThreadMessage( (Request.lpExchangeBuf)->dwCliThreadId, TS_SERVICE_START, 0, 0 );
	Sleep( 200 );
	lprintfs( "Service thread starts.\n" );

	while( 1 ) {
//		lprintfs( "########################## Service read exchange buffer.\n" );
		dwRetCode = SrvReadExchngBuf( Request.lpExchangeBuf, szBuffer, 4096 );
//		lprintfs( "########################## End service read exchange buffer.\n" );
		if( dwRetCode != TERR_SUCCESS )
			break;
	
		lptsComProps = (LPTS_COM_PROPS)szBuffer;

		switch( lptsComProps->packetType ) {
		case pkTS_SYNC_PIPE:
			if( lptsComProps->msgType == msgTS_CLIENT_LOGOFF ) {
				goto _EXIT;
			}
			break;

		case pkTS_ASQL:
			ASQLShell( szBuffer, &Request );
			break;

		case pkTS_USER:
			TUserShell( szBuffer, &Request );
			break;

		case pkTS_PIPE_FTP:
			FTPSrvShell( szBuffer, &Request );
			break;
		
		case pkTS_SPROC:
			SProcShell( szBuffer, &Request );
			break;

		case pkTS_PRIVATE_ENG:
			TPrivateEngShell( szBuffer, &Request );
			break;

		case pkTS_BACKUP:
			TBackupNotifyShell( szBuffer, &Request );
			break;

		default:
			DefaultShell( szBuffer, &Request );
		}
	}

_EXIT:
	FreeExchngBuf( Request.lpExchangeBuf );
	TSUnregisterUser( (HANDLE)Request.dwRequestId );
//	CloseHandle( (HANDLE)Request.dwRequestId );

	lprintfs( "Service thread ends.\n\n" );

	tsDbfFree ( lpTSCliCursor );	
	ExitThread( TERR_SUCCESS );

	return  0;
}

VOID TBackupNotifyShell ( LPSTR lpBuffer, LPREQUEST lpRequest )
{
	LPTS_COM_PROPS lptsComProps;
	DWORD dwRetCode;

	lptsComProps = (LPTS_COM_PROPS)lpBuffer;

	switch( lptsComProps->msgType ) {
	case msgTS_BACKUP_NOTIFY: 
	case msgTS_RESTORE_NOTIFY:
		switch( lptsComProps->lp ) {
		case cmTS_WAIT_READY:
		case cmTS_FORCE_READY:
#ifndef TREESVR_STANDALONE
			InterlockedExchange( &singleUserMode, 1 );
			singleUserThread = lpRequest->dwRequestId;
#endif
			break;

		case cmTS_BACKUP_READY:
		case cmTS_RESTORE_READY:
#ifndef TREESVR_STANDALONE
			InterlockedExchange( &singleUserMode, 0 );
			singleUserThread = 0;
#endif
			break;

		default:
			DefaultProcess( lpBuffer, lpRequest->lpExchangeBuf);
		}
		break;

	default:
		DefaultProcess( lpBuffer, lpRequest->lpExchangeBuf);
	}

	ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
	lptsComProps->packetType = pkTS_BACKUP;
	lptsComProps->msgType = msgTS_BACKUP_REPLAY;
	SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
}

VOID DefaultShell ( LPSTR lpBuffer, LPREQUEST lpRequest )
{
	LPTS_COM_PROPS lptsComProps;
	DWORD dwRetCode;

	lptsComProps = (LPTS_COM_PROPS)lpBuffer;

	while( lptsComProps->endPacket != '\x0' ) {
		lptsComProps = (LPTS_COM_PROPS)lpBuffer;
		
		dwRetCode = SrvReadExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
		if( dwRetCode != TERR_SUCCESS )
			return;
	}

	lptsComProps = (LPTS_COM_PROPS)lpBuffer;
	ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
	lptsComProps->packetType = pkTS_ERROR;
	SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
}

DWORD ASQLShell( LPSTR lpBuffer, LPREQUEST lpRequest )
{
	DWORD	dwRetCode;
	LPSTR	lpszParam;
	long	l_taskId;
	MSG		Msg;
	LPTS_COM_PROPS lptsComProps;

	__try {
		lptsComProps = (LPTS_COM_PROPS)lpBuffer;

		switch( lptsComProps->msgType ) {
		case msgFAST_ASQL: 
			lpszParam = (LPSTR)lptsComProps+sizeof(TS_COM_PROPS);

			switch( lptsComProps->lp ) {
			case cmTS_OPEN_DBF: 
				{
					char *sz;
					char szTrueFileName[MAX_PATH];
					DWORD dwcbPath = MAX_PATH;
	
					//add a tail '\0' to avoid error info
					lpszParam[MAX_PKG_MSG_LEN-1] = '\0';
					sz = strchr(lpszParam, '`');
	
					if( sz != NULL ) {
						*sz = '\0';
					}
					
					///////////////////////////////////////////////////////////////////
					//
					//lpTSCliCursor = asqlGetDbfDes(lpTSCliCursor, ++sz, lpszParam, \
					//		lpRequest->lpExchangeBuf);
					//
					// Update by Jingyu Niu, 1999.08.30
					// For support translate: ^Table -> %UserWorkPath%\Table
					//
					if( *lpszParam != '^' )
						lstrcpy( szTrueFileName, lpszParam );
					else {
						GetUserWorkDir( lpRequest->szUser, lpRequest->dwRequestId, szTrueFileName, &dwcbPath );
						*lpszParam = '\\';
						lstrcat( szTrueFileName, lpszParam );
					}
					lpTSCliCursor = asqlGetDbfDes(lpTSCliCursor, ++sz, szTrueFileName, \
							lpRequest->lpExchangeBuf);
					//////////////////////////////////////////////////////////////////////
				}
				break;
	    
			case cmTS_REC_FETCH:
				asqlGetDbfRec(lpTSCliCursor, (void *)*(long *)lpszParam, \
						*(long *)&lpszParam[sizeof(long)], \
						lpRequest->lpExchangeBuf);
				break;
	
			case cmTS_REC_PUT:
				asqlPutDbfRec(lpTSCliCursor, (void *)*(long *)lpszParam, \
						*(long *)&lpszParam[sizeof(long)], \
						&lpszParam[sizeof(long)+sizeof(long)], \
						lpRequest->lpExchangeBuf);
				break;
	    
			case cmTS_REC_ADD:
				asqlAddDbfRec(lpTSCliCursor, (void *)*(long *)lpszParam, \
						&lpszParam[sizeof(long)], \
						lpRequest->lpExchangeBuf);
				break;

		    case cmTS_REC_APP:
				asqlAppDbfRec(lpTSCliCursor, (void *)*(long *)lpszParam, \
						&lpszParam[sizeof(long)], \
						lpRequest->lpExchangeBuf);
				break;
	
			case cmTS_REC_DEL:
				asqlDelDbfRec(lpTSCliCursor, (void *)*(long *)lpszParam, \
						*(long *)&lpszParam[sizeof(long)], \
						lpRequest->lpExchangeBuf);
				break;

			case cmTS_BLOB_FETCH:
				{
					char	szPath[MAX_PATH];
					DWORD	dwcbPath;

					dwcbPath = MAX_PATH;
					ZeroMemory( szPath, MAX_PATH );
					//TUserGetInfo( lpRequest->szUser, INFO_HOME_DIR, szPath, &dwcbPath );
					GetUserWorkDir( lpRequest->szUser, lpRequest->dwRequestId, szPath, &dwcbPath );
					asqlBlobFetch(lpTSCliCursor, lpRequest->szUser, \
							szPath, \
							(void *)*(long *)lpszParam,\
							*(long *)&lpszParam[sizeof(long)], \
							(char *)&lpszParam[sizeof(long)*2], \
							lpRequest->lpExchangeBuf);
				}
				break;
	
			case cmTS_BLOB_PUT:
				{
					char	szPath[MAX_PATH];
					DWORD	dwcbPath;

					dwcbPath = MAX_PATH;
					ZeroMemory( szPath, MAX_PATH );
					//TUserGetInfo( lpRequest->szUser, INFO_HOME_DIR, szPath, &dwcbPath );
					GetUserWorkDir( lpRequest->szUser, lpRequest->dwRequestId, szPath, &dwcbPath );
					asqlBlobPut(lpTSCliCursor, lpRequest->szUser, \
							szPath, \
							(void *)*(long *)lpszParam, \
							*(long *)&lpszParam[sizeof(long)], \
							(char *)&lpszParam[sizeof(long)*2], \
							lpRequest->lpExchangeBuf);
				}
				break;
	
			case cmTS_CLOSE_DBF:
				lpTSCliCursor = asqlFreeDbfDes(lpTSCliCursor, (void *)*(long *)lpszParam, \
						lpRequest->lpExchangeBuf);
				break;

			case cmTS_DBF_RECNUM:
				asqlGetDbfRecNum(lpTSCliCursor, (void *)*(long *)lpszParam, \
						lpRequest->lpExchangeBuf);
				break;
					
			case cmTS_LOCK_DBF:
				asqlFreeDbfLock( lpTSCliCursor, (void *)*(long *)lpszParam, \
						lpRequest->lpExchangeBuf);
				break;

			case cmTS_UNLOCK_DBF:
				asqlFreeDbfUnLock( lpTSCliCursor, (void *)*(long *)lpszParam, \
						lpRequest->lpExchangeBuf);
				break;
			
			case cmTS_FIELD_INFO:
				asqlGetDbfFldInfo(  lpTSCliCursor, (void *)*(long *)lpszParam, \
						lpRequest->lpExchangeBuf);
				break;

			case cmTS_BLOB_MEM_FETCH:
				{
					char	szPath[MAX_PATH];
					DWORD	dwcbPath;

					dwcbPath = MAX_PATH;
					ZeroMemory( szPath, MAX_PATH );
					//TUserGetInfo( lpRequest->szUser, INFO_HOME_DIR, szPath, &dwcbPath );
					GetUserWorkDir( lpRequest->szUser, lpRequest->dwRequestId, szPath, &dwcbPath );
					asqlBlobMemFetch(lpTSCliCursor, lpRequest->szUser, \
							szPath, \
							(void *)*(long *)lpszParam,\
							*(long *)&lpszParam[sizeof(long)], \
							(char *)&lpszParam[sizeof(long)*2], \
							lpRequest->lpExchangeBuf);
				}
				break;
	
			case cmTS_BLOB_MEM_PUT:
				{
					char	szPath[MAX_PATH];
					DWORD	dwcbPath;

					dwcbPath = MAX_PATH;
					ZeroMemory( szPath, MAX_PATH );
					//TUserGetInfo( lpRequest->szUser, INFO_HOME_DIR, szPath, &dwcbPath );
					GetUserWorkDir( lpRequest->szUser, lpRequest->dwRequestId, szPath, &dwcbPath );
					asqlBlobMemPut(lpTSCliCursor, lpRequest->szUser, \
							szPath, \
							(void *)*(long *)lpszParam, \
							*(long *)&lpszParam[sizeof(long)], \
							(char *)&lpszParam[sizeof(long)*2], \
							lpRequest->lpExchangeBuf);
				}
				break;

			case cmTS_GET_DICT_INFO:
				{
					char *szGroup, *szLabel, *szKey;

					//add a tail '\0' to avoid error info
					lpszParam[MAX_PKG_MSG_LEN-1] = '\0';
					
					szGroup = lpszParam;
					szLabel = strchr(lpszParam, '`');

					if( szLabel != NULL ) {
						*szLabel++ = '\0';

						szKey = strchr(szLabel, '`');

						if( szKey != NULL ) {
							*szKey++ = '\0';
						}
					} else {
						szKey = NULL;
					}
					
					asqlGetDData(szGroup, szLabel, szKey, lpRequest->lpExchangeBuf);

				}
				break;

			case cmTS_PUT_DICT_INFO:
				{
					char *szGroup, *szLabel, *szKey, *szCont = NULL;

					//add a tail '\0' to avoid error info
					lpszParam[MAX_PKG_MSG_LEN-1] = '\0';
					
					szGroup = lpszParam;
					szLabel = strchr(lpszParam, '`');

					if( szLabel != NULL ) {
						*szLabel++ = '\0';

						szKey = strchr(szLabel, '`');

						if( szKey != NULL ) {
							*szKey++ = '\0';
							szCont = strchr(szKey, '`');
							if( szCont != NULL )
								szCont++;
						}
					} else {
						szKey = NULL;
					}
					
					asqlPutDData(szGroup, szLabel, szKey, szCont, lpRequest->lpExchangeBuf);

				}
				break;

			default:
				DefaultProcess( lpBuffer, lpRequest->lpExchangeBuf);
			}
			break;
		
		case msgASQL_TYPE_A:
		case msgASQL_TYPE_B:
		case msgASQL_TYPE_C:
			{
				HANDLE	hTmpBuf = NULL;
				LPSTR	lpTmpBuf;
				DWORD	dwcbTmpBuf;
				DWORD	dwcbUsed;
				CHAR	szPath[MAX_PATH];
				DWORD	dwcbPath;

				l_taskId = abs(InterlockedIncrement( &lTaskId ));
	
				if( lptsComProps->endPacket == '\x0' &&
						lptsComProps->leftPacket == '\x0' ) {
					lpTmpBuf = (LPSTR)(lpBuffer+sizeof(TS_COM_PROPS));
				}
				else {
					dwcbTmpBuf = lptsComProps->lp + 32;
					
					hTmpBuf = LocalAlloc( LPTR, dwcbTmpBuf );
					if( hTmpBuf == NULL ) {
						dwRetCode = GetLastError();
						goto _ASQL_ERROR;
					}
				
					lpTmpBuf = LocalLock( hTmpBuf );
					if( lpTmpBuf == NULL ) {
						dwRetCode = GetLastError();
						LocalFree( hTmpBuf );
						hTmpBuf = NULL;
						goto _ASQL_ERROR;
					}

					CopyMemory( (LPVOID)lpTmpBuf, (CONST VOID *)(lpBuffer+sizeof(TS_COM_PROPS)), 
							(DWORD)lptsComProps->len );
					dwcbUsed = lptsComProps->len;

					while( lptsComProps->endPacket != '\x0' &&
							lptsComProps->leftPacket != '\x0' ) {
						dwRetCode = SrvReadExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
						if( dwRetCode != TERR_SUCCESS )
							goto _ASQL_ERROR;
	
						if( lptsComProps->len + dwcbUsed >= dwcbTmpBuf ) {
							dwRetCode = TERR_SCRIPTS_TOO_LONG;
							goto _ASQL_ERROR;
						}

						CopyMemory( (LPVOID)(lpTmpBuf+dwcbUsed), (CONST VOID *)(lpBuffer+sizeof(TS_COM_PROPS)), 
								(DWORD)lptsComProps->len );
						dwcbUsed += lptsComProps->len;
					}
				}
				
				dwcbPath = MAX_PATH;
				//TUserGetInfo( lpRequest->szUser, INFO_HOME_DIR, szPath, &dwcbPath );
				GetUserWorkDir( lpRequest->szUser, lpRequest->dwRequestId, szPath, &dwcbPath );

				dwRetCode = asqlwmt( 
						l_taskId, 
						lpRequest->szUser, 
						szPath,
						lpTmpBuf,
						lptsComProps->msgType,
						lpRequest->lpExchangeBuf );

				if( dwRetCode < 10000 ) 
				{ //error
					if( hTmpBuf ) {
						LocalUnlock( hTmpBuf );
						LocalFree( hTmpBuf );
					}
					break;
				}
		
				GetMessage( &Msg, NULL, ASQL_ENDTASK_MSG, ASQL_ENDTASK_MSG );
			
				if( hTmpBuf ) {
					LocalUnlock( hTmpBuf );
					LocalFree( hTmpBuf );
				}
				break;

_ASQL_ERROR:
				if( hTmpBuf ) {
					LocalUnlock( hTmpBuf );
					LocalFree( hTmpBuf );
				}

				ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
				lptsComProps->packetType = pkTS_ASQL;
				lptsComProps->msgType = 'E';
				lptsComProps->lp = dwRetCode;
				lptsComProps->len = wsprintf( lpBuffer + sizeof(TS_COM_PROPS),
						"Error Code: %ld.", dwRetCode ); 
				SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				break;
			}

		case msgASQL_TYPE_E:
			{
				ESERVICEPACKET	Esp;
				HANDLE			hTmpBuf = NULL;
				LPSTR			lpTmpBuf;
				HANDLE			hSymTab = NULL;
				LPSysVarOFunType lpSymTab;
				DWORD			dwcbTmpBuf;
				DWORD			dwcbUsed;
				CHAR			szPath[MAX_PATH];
				DWORD			dwcbPath;
				int             i;
				char            *varBpMem;

				l_taskId = abs(InterlockedIncrement( &lTaskId ));

				dwcbTmpBuf = lptsComProps->lp + 32;
				
				hTmpBuf = LocalAlloc( LPTR, dwcbTmpBuf );
				if( hTmpBuf == NULL ) {
					dwRetCode = GetLastError();
					goto _EASQL_ERROR;
				}
				
				lpTmpBuf = LocalLock( hTmpBuf );
				if( lpTmpBuf == NULL ) {
					dwRetCode = GetLastError();
					LocalFree( hTmpBuf );
					hTmpBuf = NULL;
					goto _EASQL_ERROR;
				}

				CopyMemory( (LPVOID)lpTmpBuf, (CONST VOID *)(lpBuffer+sizeof(TS_COM_PROPS)), 
						(DWORD)lptsComProps->len );
				dwcbUsed = lptsComProps->len;

				//read the script from client
				while( lptsComProps->leftPacket != '\x0' ) {
					dwRetCode = SrvReadExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
					if( dwRetCode != TERR_SUCCESS )
						goto _EASQL_ERROR;
	
					if( lptsComProps->len + dwcbUsed >= dwcbTmpBuf ) {
						dwRetCode = TERR_SCRIPTS_TOO_LONG;
						goto _EASQL_ERROR;
					}
	
					CopyMemory( (LPVOID)(lpTmpBuf+dwcbUsed), (CONST VOID *)(lpBuffer+sizeof(TS_COM_PROPS)), 
							(DWORD)lptsComProps->len );
					dwcbUsed += lptsComProps->len;
				}

				//read from client the first VarPacket
				dwRetCode = SrvReadExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				if( dwRetCode != TERR_SUCCESS )
					goto _EASQL_ERROR;

				Esp.varNum = *((unsigned char *)&(lptsComProps->lp) + 3);
				dwcbTmpBuf = lptsComProps->lp & 0xFFFFFF;
				//dwcbUsed = 0;

				//prepare to receive the Var Table
				if( Esp.varNum == 0 ) {
					lpSymTab = NULL;
				}
				else if( lptsComProps->endPacket == '\x0' && 
						lptsComProps->leftPacket == '\x0' ) {
					lpSymTab = (LPSysVarOFunType)(lpBuffer+sizeof(TS_COM_PROPS));
				}
				else {
					//dwcbTmpBuf = lptsComProps->lp * (sizeof( SysVarOFunType ) + 1);
				
					hSymTab = LocalAlloc( LPTR, dwcbTmpBuf );
					if( hSymTab == NULL ) {
						dwRetCode = GetLastError();
						goto _EASQL_ERROR;
					}
				
					lpSymTab = LocalLock( hSymTab );
					if( lpSymTab == NULL ) {
						dwRetCode = GetLastError();
						LocalFree( hSymTab );
						hSymTab = NULL;
						goto _EASQL_ERROR;
					}

					CopyMemory( (LPVOID)lpSymTab, (CONST VOID *)(lpBuffer+sizeof(TS_COM_PROPS)), 
							(DWORD)lptsComProps->len );
					dwcbUsed = lptsComProps->len;

					while( lptsComProps->endPacket != '\x0' && 
							lptsComProps->leftPacket != '\x0' ) {
						dwRetCode = SrvReadExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
						if( dwRetCode != TERR_SUCCESS )
							goto _EASQL_ERROR;
	
						if( lptsComProps->len + dwcbUsed >= dwcbTmpBuf ) {
							dwRetCode = TERR_SCRIPTS_TOO_LONG;
							goto _EASQL_ERROR;
						}

						CopyMemory( (LPVOID)((LPSTR)lpSymTab+dwcbUsed), (CONST VOID *)(lpBuffer+sizeof(TS_COM_PROPS)), 
								(DWORD)lptsComProps->len );
						dwcbUsed += lptsComProps->len;
					}
				}

				Esp.asqlScript = lpTmpBuf;
				Esp.vars = lpSymTab;

				//realloc the memory of Var Table for value length is > 32 bytes
				varBpMem = (char *)lpSymTab + Esp.varNum * sizeof(SysVarOFunType);
				for( i = 0;   i < Esp.varNum;   i++ ) {
					if( lpSymTab[i].length >= 32 ) {
						*(long *)(lpSymTab[i].values) = (long)varBpMem;
						varBpMem += lpSymTab[i].length;
					}
				}
				
/*				{
					int i;

					printf( "Symbol table has %d entries.\n\n", Esp.varNum );
					
					if( Esp.vars != NULL ) {
						printf( "Symbol table:\n" );
						for( i = 0; i < Esp.varNum; i++ ) {
							printf( "Entry %4d-> Type = %d, Variable = %s, Value = %s Length = %d\n",
								i, Esp.vars[i].type, Esp.vars[i].VarOFunName, Esp.vars[i].values, Esp.vars[i].length );  
						}
						//printf( "Press any key to contiume..." );
						//getch();
					}

					if( Esp.asqlScript != NULL ) {
						printf( "\n\nASQL Scripts:\n" );
						printf( "%s\n", Esp.asqlScript );
					}
				}
*/

				dwcbPath = MAX_PATH;
				//TUserGetInfo( lpRequest->szUser, INFO_HOME_DIR, szPath, &dwcbPath );
				GetUserWorkDir( lpRequest->szUser, lpRequest->dwRequestId, szPath, &dwcbPath );

				dwRetCode = asqlwmt( 
						l_taskId, 
						lpRequest->szUser, 
						szPath,
						(LPSTR)&Esp,
						'E',
						lpRequest->lpExchangeBuf );

				if( dwRetCode < 10000 ) 
				{ //error
					if( hTmpBuf ) {
						LocalUnlock( hTmpBuf );
						LocalFree( hTmpBuf );
					}
					if( hSymTab ) {
						LocalUnlock( hSymTab );
						LocalFree( hSymTab );
					}
					break;
				}
		
				//wait the ASQL thread  running
				GetMessage( &Msg, NULL, ASQL_ENDTASK_MSG, ASQL_ENDTASK_MSG );
			
				if( hTmpBuf ) {
					LocalUnlock( hTmpBuf );
					LocalFree( hTmpBuf );
				}
				if( hSymTab ) {
					LocalUnlock( hSymTab );
					LocalFree( hSymTab );
				}
				break;

_EASQL_ERROR:
				if( hTmpBuf ) {
					LocalUnlock( hTmpBuf );
					LocalFree( hTmpBuf );
				}
				if( hSymTab ) {
					LocalUnlock( hSymTab );
					LocalFree( hSymTab );
				}

				ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
				lptsComProps->packetType = pkTS_ASQL;
				lptsComProps->msgType = 'E';
				lptsComProps->lp = dwRetCode;
				lptsComProps->len = wsprintf( lpBuffer + sizeof(TS_COM_PROPS),
						"Error Code: %ld.", dwRetCode ); 
				SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				break;
			}

		case msgASQL_TYPE_D:
			{
				ESERVICEPACKET	Esp;
				HANDLE			hTmpBuf = NULL;
				LPSTR			lpTmpBuf;
				HANDLE			hSymTab = NULL;
				LPSysVarOFunType lpSymTab;
				DWORD			dwcbTmpBuf;
				DWORD			dwcbUsed;
				CHAR			szPath[MAX_PATH];
				DWORD			dwcbPath;
				char			*sz;

				l_taskId = abs(InterlockedIncrement( &lTaskId ));

				dwcbTmpBuf = lptsComProps->lp + 32;
				
				hTmpBuf = LocalAlloc( LPTR, dwcbTmpBuf );
				if( hTmpBuf == NULL ) {
					dwRetCode = GetLastError();
					goto _DASQL_ERROR;
				}
				
				lpTmpBuf = LocalLock( hTmpBuf );
				if( lpTmpBuf == NULL ) {
					dwRetCode = GetLastError();
					LocalFree( hTmpBuf );
					hTmpBuf = NULL;
					goto _DASQL_ERROR;
				}

				CopyMemory( (LPVOID)lpTmpBuf, (CONST VOID *)(lpBuffer+sizeof(TS_COM_PROPS)), 
						(DWORD)lptsComProps->len );
				dwcbUsed = lptsComProps->len;

				while( lptsComProps->leftPacket != '\x0' ) {
					dwRetCode = SrvReadExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
					if( dwRetCode != TERR_SUCCESS )
						goto _DASQL_ERROR;
	
					if( lptsComProps->len + dwcbUsed >= dwcbTmpBuf ) {
						dwRetCode = TERR_SCRIPTS_TOO_LONG;
						goto _DASQL_ERROR;
					}
	
					CopyMemory( (LPVOID)(lpTmpBuf+dwcbUsed), (CONST VOID *)(lpBuffer+sizeof(TS_COM_PROPS)), 
							(DWORD)lptsComProps->len );
					dwcbUsed += lptsComProps->len;
				}

				dwRetCode = SrvReadExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				if( dwRetCode != TERR_SUCCESS )
					goto _DASQL_ERROR;

				Esp.varNum = (short)lptsComProps->lp;
				//dwcbUsed = 0;

				if( Esp.varNum == 0 ) {
					lpSymTab = NULL;
				}
				else if( lptsComProps->endPacket == '\x0' && 
						lptsComProps->leftPacket == '\x0' ) {
					lpSymTab = (LPSysVarOFunType)(lpBuffer+sizeof(TS_COM_PROPS));
				}
				else {
					dwcbTmpBuf = lptsComProps->lp * (sizeof( SysVarOFunType ) + 1);
				
					hSymTab = LocalAlloc( LPTR, dwcbTmpBuf );
					if( hSymTab == NULL ) {
						dwRetCode = GetLastError();
						goto _DASQL_ERROR;
					}
				
					lpSymTab = LocalLock( hSymTab );
					if( lpSymTab == NULL ) {
						dwRetCode = GetLastError();
						LocalFree( hSymTab );
						hSymTab = NULL;
						goto _DASQL_ERROR;
					}

					CopyMemory( (LPVOID)lpSymTab, (CONST VOID *)(lpBuffer+sizeof(TS_COM_PROPS)), 
							(DWORD)lptsComProps->len );
					dwcbUsed = lptsComProps->len;

					while( lptsComProps->endPacket != '\x0' && 
							lptsComProps->leftPacket != '\x0' ) {
						dwRetCode = SrvReadExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
						if( dwRetCode != TERR_SUCCESS )
							goto _DASQL_ERROR;
	
						if( lptsComProps->len + dwcbUsed >= dwcbTmpBuf ) {
							dwRetCode = TERR_SCRIPTS_TOO_LONG;
							goto _DASQL_ERROR;
						}

						CopyMemory( (LPVOID)((LPSTR)lpSymTab+dwcbUsed), (CONST VOID *)(lpBuffer+sizeof(TS_COM_PROPS)), 
								(DWORD)lptsComProps->len );
						dwcbUsed += lptsComProps->len;
					}
				}

				Esp.asqlScript = lpTmpBuf;
				Esp.vars = lpSymTab;
				
/*				{
					int i;

					printf( "Symbol table has %d entries.\n\n", Esp.varNum );
					
					if( Esp.vars != NULL ) {
						printf( "Symbol table:\n" );
						for( i = 0; i < Esp.varNum; i++ ) {
							printf( "Entry %4d-> Type = %d, Variable = %s, Value = %s Length = %d\n",
								i, Esp.vars[i].type, Esp.vars[i].VarOFunName, Esp.vars[i].values, Esp.vars[i].length );  
						}
						//printf( "Press any key to contiume..." );
						//getch();
					}

					if( Esp.asqlScript != NULL ) {
						printf( "\n\nASQL Scripts:\n" );
						printf( "%s\n", Esp.asqlScript );
					}
				}
*/
				dwcbPath = MAX_PATH;
				//TUserGetInfo( lpRequest->szUser, INFO_HOME_DIR, szPath, &dwcbPath );
				GetUserWorkDir( lpRequest->szUser, lpRequest->dwRequestId, szPath, &dwcbPath );

				sz = strchr(Esp.asqlScript, '*');
				if( sz != NULL ) {
					*sz++ = '\0';						
				}

				dwRetCode = hyDataCollect(lpRequest->szUser, Esp.asqlScript, sz, \
																	Esp.vars, Esp.varNum, \
																	lpRequest->lpExchangeBuf);

				if( dwRetCode < 0 ) {
					if( hTmpBuf ) {
						LocalUnlock( hTmpBuf );
						LocalFree( hTmpBuf );
					}
					if( hSymTab ) {
						LocalUnlock( hSymTab );
						LocalFree( hSymTab );
					}
					break;
				}
		
				GetMessage( &Msg, NULL, ASQL_ENDTASK_MSG, ASQL_ENDTASK_MSG );
			
				if( hTmpBuf ) {
					LocalUnlock( hTmpBuf );
					LocalFree( hTmpBuf );
				}
				if( hSymTab ) {
					LocalUnlock( hSymTab );
					LocalFree( hSymTab );
				}
				break;

_DASQL_ERROR:
				if( hTmpBuf ) {
					LocalUnlock( hTmpBuf );
					LocalFree( hTmpBuf );
				}
				if( hSymTab ) {
					LocalUnlock( hSymTab );
					LocalFree( hSymTab );
				}

				ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
				lptsComProps->packetType = pkTS_ASQL;
				lptsComProps->msgType = 'E';
				lptsComProps->lp = dwRetCode;
				lptsComProps->len = wsprintf( lpBuffer + sizeof(TS_COM_PROPS),
						"Error Code: %ld.", dwRetCode ); 
				SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				break;
			}

		case msgASQL_TEST: 
			{
				char _szBuffer[256];
				int iRespCount;
				int cbLen;
				int i;

				cbLen = ( lptsComProps->len < 256 ) ? lptsComProps->len : 256;
				iRespCount = lptsComProps->lp;
				lstrcpyn( _szBuffer, lpBuffer + sizeof(TS_COM_PROPS), cbLen );
				
				ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
				lptsComProps->packetType = pkTS_ASQL;
				lptsComProps->msgType = msgASQL_TEST;
				lptsComProps->leftPacket = '\x1';
				lptsComProps->endPacket = '\x1';
				for( i = 0; i < iRespCount; i++ ) {
					lptsComProps->len = wsprintf( lpBuffer + sizeof(TS_COM_PROPS),
						"Service response %d: [%s]", i, _szBuffer ); 
//					lprintfs( "########################## Service write exchange buffer.\n" );
					dwRetCode = SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
//					lprintfs( "########################## End service write exchange buffer.\n" );
					if( dwRetCode != TERR_SUCCESS )
						break;
				}

				ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
				lptsComProps->packetType = pkTS_ASQL;
				lptsComProps->msgType = msgASQL_TEST;
				lptsComProps->len = wsprintf( lpBuffer + sizeof(TS_COM_PROPS),
						"Service response end." ); 
//				lprintfs( "########################## Service write exchange buffer.\n" );
				SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
//				lprintfs( "########################## End service write exchange buffer.\n" );
			}
			break;

		default:
			DefaultProcess( lpBuffer, lpRequest->lpExchangeBuf);
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER ) {
		MessageBox( NULL, "Error Access memory.", "Error", MB_OK | MB_ICONSTOP );
	}

	return TERR_SUCCESS;
}

// Add stored Procedure Implements by Motor, August 28, 1998
DWORD SProcShell ( LPSTR lpBuffer, LPREQUEST lpRequest )
{
	LPSTR			lpParam;
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwRetCode;

	char			szProcName[MAX_PROC_NAME+1];
	DWORD			dwUserId;
	DWORD			dwUserIdSize;
	char			cPrivilege;

	__try 
	{
		lptsComProps = (LPTS_COM_PROPS)lpBuffer;

		switch( lptsComProps->msgType ) 
		{
		case msgTS_SPROC : 
			lpParam = (LPSTR)lptsComProps+sizeof(TS_COM_PROPS);
	
			switch( lptsComProps->lp ) 
			{
			case cmTS_NEW_PROC :
			case cmTS_UPDATE_PROC :
			{
				HANDLE	hTmpBuf = NULL;
				LPSTR	lpTmpBuf;
				DWORD	dwcbTmpBuf = 0;
				DWORD	dwcbUsed = 0;
				long	lMethod;
				
				lMethod = lptsComProps->lp;
				
				dwcbTmpBuf = *( LPDWORD )lpParam + 32;
					
				lstrcpyn( szProcName, lpParam + sizeof( DWORD ), MAX_PROC_NAME );
				dwUserIdSize = sizeof( DWORD );
				TUserGetInfo( lpRequest->szUser, INFO_USER_ID, &dwUserId, &dwUserIdSize );
				
				if( lptsComProps->endPacket == '\x0' &&
						lptsComProps->leftPacket == '\x0' ) 
				{
					lpTmpBuf = (LPSTR)(lpBuffer+sizeof(TS_COM_PROPS));
				}
				else 
				{
					hTmpBuf = LocalAlloc( LPTR, dwcbTmpBuf );
					if( hTmpBuf == NULL ) 
					{
						dwRetCode = GetLastError();
						goto _NU_PROC_EXIT;
					}
				
					lpTmpBuf = LocalLock( hTmpBuf );
					if( lpTmpBuf == NULL ) 
					{
						dwRetCode = GetLastError();
						LocalFree( hTmpBuf );
						hTmpBuf = NULL;
						goto _NU_PROC_EXIT;
					}

					while( lptsComProps->endPacket != '\x0' &&
							lptsComProps->leftPacket != '\x0' ) 
					{
						dwRetCode = SrvReadExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
						if( dwRetCode != TERR_SUCCESS )
							goto _NU_PROC_EXIT;
	
						if( lptsComProps->len + dwcbUsed >= dwcbTmpBuf ) 
						{
							dwRetCode = TERR_SCRIPTS_TOO_LONG;
							goto _NU_PROC_EXIT;
						}

						CopyMemory( (LPVOID)(lpTmpBuf+dwcbUsed), (CONST VOID *)(lpBuffer+sizeof(TS_COM_PROPS)), 
								(DWORD)lptsComProps->len );
						dwcbUsed += lptsComProps->len;
					}
				}
				
				if( lMethod == cmTS_NEW_PROC )
					dwRetCode = NewStoredProc( szProcName, dwUserId, lpTmpBuf, dwcbUsed );
				else
					dwRetCode = UpdateStoredProc( szProcName, dwUserId, lpTmpBuf, dwcbUsed );


_NU_PROC_EXIT :
				if( hTmpBuf )
				{
					LocalUnlock( hTmpBuf );
					LocalFree( hTmpBuf );
				}

				ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
				lptsComProps->packetType = pkTS_SPROC;
				lptsComProps->msgType = 'D';
				lptsComProps->lp = dwRetCode;
			
				if( dwRetCode != 0 ) 
				{
					GetErrorMessage( dwRetCode, lpParam, 1024 );
					lptsComProps->len = lstrlen( lpParam );
				}	

				SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				break;
			}

			case cmTS_DELETE_PROC:
				lstrcpyn( szProcName, lpParam, MAX_PROC_NAME );
				dwUserIdSize = sizeof( DWORD );
				TUserGetInfo( lpRequest->szUser, INFO_USER_ID, &dwUserId, &dwUserIdSize );
				
				dwRetCode = DeleteStoredProc( szProcName, dwUserId );
			
				ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
				lptsComProps->packetType = pkTS_SPROC;
				lptsComProps->msgType = 'D';
				lptsComProps->lp = dwRetCode;
			
				if( dwRetCode != 0 ) 
				{
					GetErrorMessage( dwRetCode, lpParam, 1024 );
					lptsComProps->len = lstrlen( lpParam );
				}	

				SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				
				break;

			case cmTS_GET_PROC :
			{
				HANDLE	hTmpBuf = NULL;
				LPSTR	lpTmpBuf;
				DWORD	dwProcLen, i;

				lstrcpyn( szProcName, lpParam, MAX_PROC_NAME );
				dwUserIdSize = sizeof( DWORD );
				TUserGetInfo( lpRequest->szUser, INFO_USER_ID, &dwUserId, &dwUserIdSize );
				
//				lprintfs( "Processing cmTS_GET_PROC...\n" );
//				lprintfs( "ProcName = %s\n", szProcName );

				dwProcLen = GetStoredProcSize( szProcName, dwUserId );
				if( dwProcLen == ULONG_MAX )
				{
					dwRetCode = GetLastError();
					goto _GET_PROC_EXIT;
				}

				hTmpBuf = LocalAlloc( LPTR, dwProcLen );
				if( hTmpBuf == NULL )
				{
					dwRetCode = GetLastError();
					goto _GET_PROC_EXIT;
				}
				
				lpTmpBuf = LocalLock( hTmpBuf );
				if( lpTmpBuf == NULL ) 
				{
					dwRetCode = GetLastError();
					LocalFree( hTmpBuf );
					hTmpBuf = NULL;
					goto _GET_PROC_EXIT;
				}

				dwRetCode = ReadStoredProc( szProcName, dwUserId, lpTmpBuf, &dwProcLen );
				if( dwRetCode != 0 )
					goto _GET_PROC_EXIT;

				for( i = 0; i < dwProcLen; i += EXCHANGE_CONTENT_SIZE )
				{
					ZeroMemory( lptsComProps, sizeof( TS_COM_PROPS));

					lptsComProps->packetType = pkTS_SPROC;
					lptsComProps->msgType = 'D';
	
					if( ( i + EXCHANGE_CONTENT_SIZE ) < dwProcLen )
					{
						lptsComProps->len = EXCHANGE_CONTENT_SIZE;
						lptsComProps->leftPacket = '\1';
						lptsComProps->endPacket = '\1';
					}
					else
					{
						lptsComProps->len = ( short )( dwProcLen - i );
						lptsComProps->leftPacket = '\0';
						lptsComProps->endPacket = '\0';
					}
		
					lptsComProps->lp = 0;

					memcpy( lpBuffer + sizeof( TS_COM_PROPS ), lpTmpBuf + i, 
							lptsComProps->len );

					SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				}
				
				break;

_GET_PROC_EXIT :
				if( hTmpBuf ) 
				{
					LocalUnlock( hTmpBuf );
					LocalFree( hTmpBuf );
				}

				ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
				lptsComProps->packetType = pkTS_SPROC;
				lptsComProps->msgType = 'D';
				lptsComProps->lp = dwRetCode;
			
				if( dwRetCode != 0 ) 
				{
					GetErrorMessage( dwRetCode, lpParam, 1024 );
					lptsComProps->len = lstrlen( lpParam );
				}

				SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				break;
			}

			case cmTS_EXECUTE_PROC :
			{
				HANDLE	hTmpBuf = NULL;
				LPSTR	lpTmpBuf;
				DWORD	dwProcLen;
				long	l_taskId;
				MSG		Msg;
				CHAR	szPath[MAX_PATH];
				DWORD	dwcbPath;
				
				ESERVICEPACKET Esp;
				HANDLE	hSymTab = NULL;
				LPSysVarOFunType lpSymTab;
				DWORD	dwcbTmpBuf = 0;
				DWORD	dwcbUsed = 0;

				Esp.varNum = ( short )( *( LPDWORD )lpParam );

				lstrcpyn( szProcName, lpParam + sizeof( DWORD ), MAX_PROC_NAME );
				dwUserIdSize = sizeof( DWORD );
				TUserGetInfo( lpRequest->szUser, INFO_USER_ID, &dwUserId, &dwUserIdSize );
				
				l_taskId = abs(InterlockedIncrement( &lTaskId ));

				dwProcLen = GetStoredProcSize( szProcName, dwUserId );
				hTmpBuf = LocalAlloc( LPTR, dwProcLen+16 );
				if( hTmpBuf == NULL )
				{
					dwRetCode = GetLastError();
					goto _EXECUTE_PROC_ERROR;
				}
				
				lpTmpBuf = LocalLock( hTmpBuf );
				if( lpTmpBuf == NULL ) 
				{
					dwRetCode = GetLastError();
					LocalFree( hTmpBuf );
					hTmpBuf = NULL;
					goto _EXECUTE_PROC_ERROR;
				}

				dwRetCode = ReadStoredProc( szProcName, dwUserId, lpTmpBuf, &dwProcLen );
				if( dwRetCode != 0 )
					goto _EXECUTE_PROC_ERROR;


				dwcbPath = MAX_PATH;
				//TUserGetInfo( lpRequest->szUser, INFO_HOME_DIR, szPath, &dwcbPath );
				GetUserWorkDir( lpRequest->szUser, lpRequest->dwRequestId, szPath, &dwcbPath );

				{ //give client an information 
					ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
					lptsComProps->packetType = pkTS_SPROC;
					lptsComProps->msgType = 'D';
					lptsComProps->lp = 0;
					lptsComProps->len = 0;
					SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				}

				//read from client and execute
				dwRetCode = SrvReadExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				if( dwRetCode != TERR_SUCCESS )
					goto _EXECUTE_PROC_ERROR;

				dwcbUsed = 0;

				if( Esp.varNum == 0 ) {
					lpSymTab = NULL;
				}
				else if( lptsComProps->endPacket == '\x0' && 
												lptsComProps->leftPacket == '\x0' ) {
					lpSymTab = (LPSysVarOFunType)(lpBuffer+sizeof(TS_COM_PROPS));
				}
				else {
					dwcbTmpBuf = Esp.varNum * (sizeof( SysVarOFunType ) + 1);
				
					hSymTab = LocalAlloc( LPTR, dwcbTmpBuf );
					if( hSymTab == NULL ) {
						dwRetCode = GetLastError();
						goto _EXECUTE_PROC_ERROR;
					}
				
					lpSymTab = LocalLock( hSymTab );
					if( lpSymTab == NULL ) {
						dwRetCode = GetLastError();
						LocalFree( hSymTab );
						hSymTab = NULL;
						goto _EXECUTE_PROC_ERROR;
					}

					CopyMemory( (LPVOID)lpSymTab, (CONST VOID *)(lpBuffer+sizeof(TS_COM_PROPS)), 
							(DWORD)lptsComProps->len );
					dwcbUsed += lptsComProps->len;

					while( lptsComProps->endPacket != '\x0' && 
												lptsComProps->leftPacket != '\x0' ) {
						dwRetCode = SrvReadExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
						if( dwRetCode != TERR_SUCCESS )
							goto _EXECUTE_PROC_ERROR;
	
						if( lptsComProps->len + dwcbUsed >= dwcbTmpBuf ) {
														dwRetCode = TERR_SCRIPTS_TOO_LONG;
							goto _EXECUTE_PROC_ERROR;
						}

						CopyMemory( (LPVOID)((LPSTR)lpSymTab+dwcbUsed), (CONST VOID *)(lpBuffer+sizeof(TS_COM_PROPS)), 
								(DWORD)lptsComProps->len );
						dwcbUsed += lptsComProps->len;
					}
				}

				Esp.asqlScript = lpTmpBuf;
				Esp.vars = lpSymTab;
				
				dwRetCode = asqlwmt( 
						l_taskId, 
						lpRequest->szUser, 
						szPath,
						(LPSTR)&Esp,
						'E',
						lpRequest->lpExchangeBuf );

				GetMessage( &Msg, NULL, ASQL_ENDTASK_MSG, ASQL_ENDTASK_MSG );

				if( hSymTab ) {
					LocalUnlock( hSymTab );
					LocalFree( hSymTab );
				}

				if( dwRetCode < 10000 ) 
				{
					if( hTmpBuf ) 
					{
						LocalUnlock( hTmpBuf );
						LocalFree( hTmpBuf );
					}
					break;
				}
		
				if( hTmpBuf ) 
				{
					LocalUnlock( hTmpBuf );
					LocalFree( hTmpBuf );
				}
				break;

_EXECUTE_PROC_ERROR :
				if( hTmpBuf ) 
				{
					LocalUnlock( hTmpBuf );
					LocalFree( hTmpBuf );
				}

				ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
				lptsComProps->packetType = pkTS_SPROC;
				lptsComProps->msgType = 'E';
				lptsComProps->lp = dwRetCode;
				lptsComProps->len = wsprintf( lpBuffer + sizeof(TS_COM_PROPS),
						"Error Code: %ld.", dwRetCode ); 
				SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );

				break;
			}

			case cmTS_PROC_GRANT :
				lstrcpyn( szProcName, lpParam, MAX_PROC_NAME );
				cPrivilege = *( lpParam + lstrlen( szProcName ) + 1 );

				dwUserIdSize = sizeof( DWORD );
				TUserGetInfo( lpRequest->szUser, INFO_USER_ID, &dwUserId, &dwUserIdSize );
				
				if( dwUserId == 1 ) {
				
					dwUserIdSize = sizeof( DWORD );
					dwRetCode = TUserGetInfo( lpParam + lstrlen( szProcName ) + 2, INFO_USER_ID, &dwUserId, &dwUserIdSize );

					if( dwRetCode == 0 ) {
						dwRetCode = spGrantUserPrivilege( szProcName, dwUserId, cPrivilege );
					}
				} else 
					dwRetCode = TERR_USER_NO_PRIVILEGE;


				ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
				lptsComProps->packetType = pkTS_SPROC;
				lptsComProps->msgType = 'D';
				lptsComProps->lp = dwRetCode;
			
				if( dwRetCode != 0 )
				{
					GetErrorMessage( dwRetCode, lpParam, 1024 );
					lptsComProps->len = lstrlen( lpParam );
				}	

				SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				break;
			
			case cmTS_PROC_REVOKE :
				lstrcpyn( szProcName, lpParam, MAX_PROC_NAME );
				cPrivilege = *( lpParam + lstrlen( szProcName ) + 1 );

				dwUserIdSize = sizeof( DWORD );
				TUserGetInfo( lpRequest->szUser, INFO_USER_ID, &dwUserId, &dwUserIdSize );

				if( dwUserId == 1 ) {
				
					dwUserIdSize = sizeof( DWORD );
					dwRetCode = TUserGetInfo( lpParam + lstrlen( szProcName ) + 2, INFO_USER_ID, &dwUserId, &dwUserIdSize );

					if( dwRetCode == 0 ) {
						dwRetCode  = spRevokeUserPrivilege( szProcName, dwUserId, cPrivilege );
					}
				} else 
					dwRetCode = TERR_USER_NO_PRIVILEGE;

				ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
				lptsComProps->packetType = pkTS_SPROC;
				lptsComProps->msgType = 'D';
				lptsComProps->lp = dwRetCode;
			
				if( dwRetCode != 0 ) 
				{
					GetErrorMessage( dwRetCode, lpParam, 1024 );
					lptsComProps->len = lstrlen( lpParam );
				}	

				SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				break;

			case cmTS_PROC_ENUM:
				TSprocEnum( SprocEnumProc, lpRequest->lpExchangeBuf );
				break;

			default:
				;
			}

		default:
			;
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER ) {
		MessageBox( NULL, "Error Access memory.", "Error", MB_OK | MB_ICONSTOP );
	}

	return TERR_SUCCESS;
}

DWORD CALLBACK UserEnumProc ( LPSTR szUserName, LPEXCHNG_BUF_INFO lpExchangeBuf,
							 BOOL bEndOfEnum )
{
	CHAR szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS lptsComProps;
	DWORD dwRetCode;

	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	ZeroMemory( szBuffer, PIPE_BUFFER_SIZE );
	
	dwRetCode = TUserGetAllInfo( szUserName, 
			(LPUSER_INFO)(szBuffer+sizeof(TS_COM_PROPS)) );
	
	lptsComProps->packetType = pkTS_USER;
	lptsComProps->msgType = msgTS_USER_REPLAY;
	lptsComProps->len = sizeof(USER_INFO);
	lptsComProps->lp = dwRetCode;
	if( !bEndOfEnum ) {
		lptsComProps->leftPacket = '\x1';
		lptsComProps->endPacket = '\x1';
	}

	dwRetCode = SrvWriteExchngBuf( lpExchangeBuf, szBuffer, 4096 );
	return dwRetCode;
}

DWORD CALLBACK UserListProc ( LPSTR szUserName, LPEXCHNG_BUF_INFO lpExchangeBuf,
							 BOOL bEndOfEnum )
{
	static LPSTR lpBuffer = NULL;
	LPTS_COM_PROPS lptsComProps;
	DWORD dwRetCode;
	static int i = 0;

	if( NULL == lpBuffer ) {
		lpBuffer =  HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, PIPE_BUFFER_SIZE );
		if( NULL == lpBuffer )
			return GetLastError();
	}

	if( i < PIPE_BUFFER_SIZE - sizeof(TS_COM_PROPS) - 2 * MAX_USER ) {
		lstrcpy( lpBuffer+sizeof(TS_COM_PROPS)+i, szUserName );
		i += lstrlen( szUserName );
		if( !bEndOfEnum ) {
			*(lpBuffer+sizeof(TS_COM_PROPS)+i) = ';';
			i++;
		}
	}

	dwRetCode = 0;

	if( (i > (PIPE_BUFFER_SIZE - sizeof(TS_COM_PROPS) - 2 * MAX_USER)) || bEndOfEnum ) {
		lptsComProps = (LPTS_COM_PROPS)lpBuffer;
		lptsComProps->packetType = pkTS_USER;
		lptsComProps->msgType = msgTS_USER_REPLAY;
		lptsComProps->len = i;
		lptsComProps->lp = 0;

		if( bEndOfEnum ) {
			lptsComProps->leftPacket = '\x0';
			lptsComProps->endPacket = '\x0';
		}
		else {
			lptsComProps->leftPacket = '\x1';
			lptsComProps->endPacket = '\x1';
		}

		dwRetCode = SrvWriteExchngBuf( lpExchangeBuf, lpBuffer, 4096 );

		i = 0;
	}
	
	if( bEndOfEnum ) {
		HeapFree( GetProcessHeap(), 0, lpBuffer );
		lpBuffer = NULL;
	}

	return dwRetCode;
}

DWORD TUserShell ( LPSTR lpBuffer, LPREQUEST lpRequest )
{
	DWORD		dwRetCode;
	LPTS_COM_PROPS lptsComProps;
	LPSTR		lpParam;
	LPUSER_INFO lpUserInfo;
	int			iInfoIndex;
	CHAR		cDirFlag;
	DWORD		dwBuffer;
	BOOL		bSuccess;
	
	lptsComProps = (LPTS_COM_PROPS)lpBuffer;
	lpParam = lpBuffer + sizeof(TS_COM_PROPS);

	switch( lptsComProps->msgType ) {
	case msgTS_USER_MANAGE: 
		switch( lptsComProps->lp ) {
		case cmTS_USER_ADD:
			cDirFlag = *lpParam;
			lpUserInfo = (LPUSER_INFO)(lpParam + 1);
			
			// if( CheckPathName( lpUserInfo->szHomeDIr ) )
				
			// if( cDirFlag == '\x1' ) {

			
			dwRetCode = TUserAdd( lpUserInfo );
			
			ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
			lptsComProps->packetType = pkTS_USER;
			lptsComProps->msgType = msgTS_USER_REPLAY;
			lptsComProps->lp = dwRetCode;
			
			if( dwRetCode != 0 ) {
				GetErrorMessage( dwRetCode, lpParam, 1024 );
				lptsComProps->len = lstrlen( lpParam );
			}

			SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
			break;
	    
		case cmTS_USER_DEL:
			cDirFlag = *lpParam;
			lpParam[lptsComProps->len] = '\x0';

			dwRetCode = TUserDelete( (LPCSTR)(lpParam+1) );
			
			ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
			lptsComProps->packetType = pkTS_USER;
			lptsComProps->msgType = msgTS_USER_REPLAY;
			lptsComProps->lp = dwRetCode;
			
			if( dwRetCode != 0 ) {
				GetErrorMessage( dwRetCode, lpParam, 1024 );
				lptsComProps->len = lstrlen( lpParam );
			}

			SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
			break;

		case cmTS_USER_INFO:
			lpParam[lptsComProps->len] = '\x0';
			iInfoIndex = (int)(*lpParam);

			if( iInfoIndex == 0 ) {
				dwBuffer = sizeof( USER_INFO );
				dwRetCode = TUserGetAllInfo( (LPCSTR)(lpParam+sizeof(int)), (LPUSER_INFO)lpParam );
			}
			else {
				dwBuffer = PIPE_BUFFER_SIZE - sizeof( TS_COM_PROPS ) - 1;
				dwRetCode = TUserGetInfo( lpParam+sizeof(int), iInfoIndex, (LPVOID)lpParam, &dwBuffer );
			}
			
			///
			lpUserInfo = (LPUSER_INFO)lpParam;
			///
			ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
			lptsComProps->packetType = pkTS_USER;
			lptsComProps->msgType = msgTS_USER_REPLAY;
			lptsComProps->lp = dwRetCode;
			
			if( dwRetCode != 0 ) {
				GetErrorMessage( dwRetCode, lpParam, 1024 );
				lptsComProps->len = lstrlen( lpParam );
			}
			else 
				lptsComProps->len = (int)dwBuffer;

			SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
			break;

		case cmTS_USER_MODI:
			lpParam[lptsComProps->len] = '\x0';
			iInfoIndex = (int)(*lpParam);
			lpUserInfo = (LPUSER_INFO)(lpParam+sizeof(int)+strlen(lpParam+sizeof(int))+1 );

			if( iInfoIndex == 0 ) {
				dwRetCode = TUserSetAllInfo( (LPCSTR)(lpParam+sizeof(int)), lpUserInfo );
			}
			else {
				dwBuffer = lptsComProps->len - sizeof(int) - lstrlen( lpParam+sizeof(int) ) - 1;
				dwRetCode = TUserSetInfo( lpParam+sizeof(int), iInfoIndex, (LPVOID)lpUserInfo, dwBuffer );
			}
			
			ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
			lptsComProps->packetType = pkTS_USER;
			lptsComProps->msgType = msgTS_USER_REPLAY;
			lptsComProps->lp = dwRetCode;
			
			if( dwRetCode != 0 ) {
				GetErrorMessage( dwRetCode, lpParam, 1024 );
				lptsComProps->len = lstrlen( lpParam );
			}

			SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
			break;

		case cmTS_USER_ENUM:
			TUserEnum( UserEnumProc, lpRequest->lpExchangeBuf );
			break;

		case cmTS_USER_LOCK:
			lpParam[lptsComProps->len] = '\x0';
			bSuccess = TUserLock( lpParam );

			ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
			lptsComProps->packetType = pkTS_USER;
			lptsComProps->msgType = msgTS_USER_REPLAY;
			
			if( bSuccess )
				lptsComProps->lp = 0;
			else {
				dwRetCode = GetLastError();
				lptsComProps->lp = dwRetCode;
				GetErrorMessage( dwRetCode, lpParam, 1024 );
				lptsComProps->len = lstrlen( lpParam );
			}

			SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
			break;
			
		case cmTS_USER_UNLOCK: 
			lpParam[lptsComProps->len] = '\x0';
			bSuccess = TUserLock( lpParam );

			ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
			lptsComProps->packetType = pkTS_USER;
			lptsComProps->msgType = msgTS_USER_REPLAY;
			
			if( bSuccess )
				lptsComProps->lp = 0;
			else {
				dwRetCode = GetLastError();
				lptsComProps->lp = dwRetCode;
				GetErrorMessage( dwRetCode, lpParam, 1024 );
				lptsComProps->len = lstrlen( lpParam );
			}

			SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
			break;

		case cmTS_USER_CLEAR_LF:
			lpParam[lptsComProps->len] = '\x0';
			dwRetCode = TUserClearLogonFailCount( lpParam );
			
			ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
			lptsComProps->packetType = pkTS_USER;
			lptsComProps->msgType = msgTS_USER_REPLAY;
			lptsComProps->lp = dwRetCode;
			
			if( dwRetCode != 0 ) {
				GetErrorMessage( dwRetCode, lpParam, 1024 );
				lptsComProps->len = lstrlen( lpParam );
			}

			SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
			break;

		case cmTS_USER_RENAME:
			lpParam[lptsComProps->len] = '\x0';
			dwRetCode = TUserRename( lpParam, (LPCSTR)(lpParam+lstrlen(lpParam)+1) );
			
			ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
			lptsComProps->packetType = pkTS_USER;
			lptsComProps->msgType = msgTS_USER_REPLAY;
			lptsComProps->lp = dwRetCode;
			
			if( dwRetCode != 0 ) {
				GetErrorMessage( dwRetCode, lpParam, 1024 );
				lptsComProps->len = lstrlen( lpParam );
			}

			SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
			break;

		case cmTS_USER_RD_TEMPLATE:
			dwRetCode = TUserReadTemplate( (LPUSER_INFO)lpParam );
			
			ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
			lptsComProps->packetType = pkTS_USER;
			lptsComProps->msgType = msgTS_USER_REPLAY;
			lptsComProps->lp = dwRetCode;
			
			if( dwRetCode != 0 ) {
				GetErrorMessage( dwRetCode, lpParam, 1024 );
				lptsComProps->len = lstrlen( lpParam );
			}
			else 
				lptsComProps->len = sizeof( USER_INFO );

			SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
			break;

		case cmTS_USER_WR_TEMPLATE:
			lpUserInfo = (LPUSER_INFO)lpParam;

			dwRetCode = TUserWriteTemplate( lpUserInfo );
			
			ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
			lptsComProps->packetType = pkTS_USER;
			lptsComProps->msgType = msgTS_USER_REPLAY;
			lptsComProps->lp = dwRetCode;
			
			if( dwRetCode != 0 ) {
				GetErrorMessage( dwRetCode, lpParam, 1024 );
				lptsComProps->len = lstrlen( lpParam );
			}

			SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
			break;

		case cmTS_USER_DEFAULT_HOME:
			{
				char szProfile[MAX_PATH];
				DWORD dwcbPath;

				dwcbPath = MAX_PATH;
				ZeroMemory( szProfile, MAX_PATH );
				ZeroMemory( lpBuffer, PIPE_BUFFER_SIZE );
				dwRetCode = GetTreeServerRoot( szProfile, &dwcbPath );
				if( dwRetCode == 0 ) {
					lstrcat( szProfile, TEXT( "\\Config\\TreeSvr.ini" ) );
					GetPrivateProfileString( "Directory", "UserHome", "\x0", 
							lpParam, MAX_PATH, szProfile );
				}

				lptsComProps->packetType = pkTS_USER;
				lptsComProps->msgType = msgTS_USER_REPLAY;
				lptsComProps->lp = dwRetCode;
			
				if( dwRetCode != 0 ) {
					GetErrorMessage( dwRetCode, lpParam, 1024 );
				}

				lptsComProps->len = lstrlen( lpParam ) + 1;

				SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
			}
			break;

		case cmTS_USER_LIST:
			TUserEnum( UserListProc, lpRequest->lpExchangeBuf );
			break;

		default:
			DefaultProcess( lpBuffer, lpRequest->lpExchangeBuf);
		}
		break;
		
	case msgTS_USER:
		switch( lptsComProps->lp ) {
		case cmTS_USER_CHNG_PWD:
			{
				CHAR szBuffer[80];
				LPSTR lpOldPasswd, lpNewPasswd;

				bSuccess = FALSE;
				lpOldPasswd = lpParam;
				lpNewPasswd = lpParam + lstrlen( lpParam ) + 1;

				dwBuffer = 80;
				dwRetCode = TUserGetInfo( lpRequest->szUser, INFO_PASSWD, (LPVOID)szBuffer, &dwBuffer );
				
				if( dwRetCode == 0 ) {
					if( ! strcmp( lpOldPasswd, szBuffer ) ) {
						dwRetCode = TUserSetInfo( lpRequest->szUser, INFO_PASSWD, (LPVOID)lpNewPasswd, lstrlen( lpNewPasswd ) );
						if( dwRetCode == 0 )
							bSuccess = TRUE;
					}
					else
						dwRetCode = TERR_INVALID_PASSWORD;
				}

				ZeroMemory( (PVOID)lptsComProps, sizeof( TS_COM_PROPS ) ); 
				lptsComProps->packetType = pkTS_USER;
				lptsComProps->msgType = msgTS_USER_REPLAY;
			
				if( bSuccess )
					lptsComProps->lp = 0;
				else {
					lptsComProps->lp = dwRetCode;
					GetErrorMessage( dwRetCode, lpParam, 1024 );
					lptsComProps->len = lstrlen( lpParam );
				}

				SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
			}
			break;

		case cmTS_USER_NET_PATH:
			{
				char szProfile[MAX_PATH];
				char szHome[MAX_PATH];
				char szUserHome[MAX_PATH];
				char szShare[MAX_PATH];
				CHAR szComputer[36];
				DWORD dwcbPath;

#ifdef TREESVR_STANDALONE
				dwcbPath = MAX_PATH;
				//dwRetCode = TUserGetInfo( lpRequest->szUser, INFO_HOME_DIR, (LPVOID)szUserHome, &dwcbPath );
				dwRetCode = GetUserWorkDir( lpRequest->szUser, lpRequest->dwRequestId, (LPVOID)szUserHome, &dwcbPath );
				if( dwRetCode == 0 )
					lstrcpy( lpParam, szUserHome );
#else
				dwcbPath = MAX_PATH;
				ZeroMemory( szProfile, MAX_PATH );
				ZeroMemory( lpBuffer, PIPE_BUFFER_SIZE );
				dwRetCode = GetTreeServerRoot( szProfile, &dwcbPath );
				if( dwRetCode == 0 ) {
					lstrcat( szProfile, TEXT( "\\Config\\TreeSvr.ini" ) );
					GetPrivateProfileString( "Directory", "UserHome", "\x0", 
							szHome, MAX_PATH, szProfile );
					GetPrivateProfileString( "Directory", "HomeShareName", "\x0", 
							szShare, MAX_PATH, szProfile );

					dwcbPath = 36;
					GetComputerName( szComputer, &dwcbPath ); 
					dwcbPath = MAX_PATH;
					//dwRetCode = TUserGetInfo( lpRequest->szUser, INFO_HOME_DIR, (LPVOID)szUserHome, &dwcbPath );
					dwRetCode = GetUserWorkDir( lpRequest->szUser, lpRequest->dwRequestId, (LPVOID)szUserHome, &dwcbPath );
					if( dwRetCode == 0 ) {
						if( _strnicmp( szUserHome, szHome, lstrlen( szHome ) ) == 0 ) {
							if( szUserHome[lstrlen(szHome)] == '\\' ) {
								wsprintf( lpParam, "\\\\%s\\%s%s", szComputer, szShare, &szUserHome[lstrlen(szHome)] );
							}
							else
								dwRetCode = -1;
						}
						else
							dwRetCode = -1;
					}
				}
#endif

				lptsComProps->packetType = pkTS_USER;
				lptsComProps->msgType = msgTS_USER_REPLAY;
				lptsComProps->lp = dwRetCode;

				if( dwRetCode != 0 ) {
					GetErrorMessage( dwRetCode, lpParam, 1024 );
				}

				lptsComProps->len = lstrlen( lpParam ) + 1;

				SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
			}
			break;

		default:
			DefaultProcess( lpBuffer, lpRequest->lpExchangeBuf);
		}
		break;

	// Added by Motor, August 8, 1998
	case msgTS_REGISTERED_USER :
	{
		DWORD  i, j;

		ZeroMemory( lpBuffer, PIPE_BUFFER_SIZE );

		lptsComProps->packetType = pkTS_USER;
		lptsComProps->msgType = msgTS_REGISTERED_USER;

		EnterCriticalSection( &CriticalSection_User );
		
		j = sizeof( TS_COM_PROPS );
		for( i = 0;  i < SystemResInfo.dwUsers && 
					 j + sizeof( CLIENT_RES_INFO ) < PIPE_BUFFER_SIZE - sizeof( TS_COM_PROPS );  
			 i++ ) 
		{
			if( SystemResInfo.lpClientResInfo[i].szUser[0] != '\0' )
			{
				CopyMemory( ( PVOID )( lpBuffer + j ), 
						( CONST VOID * )( &(SystemResInfo.lpClientResInfo[i]) ), 
						sizeof( CLIENT_RES_INFO ) );
				j += sizeof( CLIENT_RES_INFO );
			}
		}

		lptsComProps->len = ( short )j - sizeof( TS_COM_PROPS );
		
		LeaveCriticalSection( &CriticalSection_User );

		SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
		break;
	}

	case msgTS_IDENTICAL_USER_NUM :
	{
		DWORD	i, iUserNum = 0;
		LPSTR	lpUserName;

		lpUserName = lpBuffer + sizeof( TS_COM_PROPS );

		EnterCriticalSection( &CriticalSection_User );

		for( i = 0;  i < SystemResInfo.dwUsers; i++ ) 
			if( ! strcmpi( SystemResInfo.lpClientResInfo[i].szUser, lpUserName ) )
				iUserNum++;
		
		LeaveCriticalSection( &CriticalSection_User );

		ZeroMemory( lpBuffer, PIPE_BUFFER_SIZE );

		lptsComProps->packetType = pkTS_USER;
		lptsComProps->msgType = msgTS_IDENTICAL_USER_NUM;
		lptsComProps->lp = iUserNum;

		SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, sizeof( TS_COM_PROPS ) );
		break;
	}


	default:
		DefaultProcess( lpBuffer, lpRequest->lpExchangeBuf);
	}

	return TERR_SUCCESS;
}

DWORD FTPSrvShell ( LPSTR lpBuffer, LPREQUEST lpRequest )
{
	DWORD			dwRetCode;
	LPTS_COM_PROPS	lptsComProps;
	CHAR			szHomeDir[MAX_PATH];
	CHAR			szTreeRoot[MAX_PATH];
	CHAR			szFileName[MAX_PATH];
	DWORD			dwBuffer, dwcbPath = MAX_PATH;
	
	lptsComProps = (LPTS_COM_PROPS)lpBuffer;

	dwBuffer = MAX_PATH;
	TUserGetInfo( lpRequest->szUser, INFO_HOME_DIR, szHomeDir, &dwBuffer );
	GetTreeServerRoot( szTreeRoot, &dwcbPath );

	switch( lptsComProps->msgType ) {
	case msgTS_FTP_FILE:
		//if( strnicmp( (LPSTR)(lpBuffer+sizeof(TS_COM_PROPS)), "@\\CODE\\", 7 ) == 0 )
		// By Jingyu Niu, 2000.08.25
		*(lpBuffer+sizeof(TS_COM_PROPS)+lptsComProps->len) = '\x0';
		if( *(LPSTR)(lpBuffer+sizeof(TS_COM_PROPS)) = '@' )
			wsprintf( szFileName, "%s\\%s", szTreeRoot, (LPSTR)(lpBuffer+sizeof(TS_COM_PROPS)+2) );
		else
			wsprintf( szFileName, "%s\\%s", szHomeDir, (LPSTR)(lpBuffer+sizeof(TS_COM_PROPS)) );

		switch( lptsComProps->lp ) {
		case cmFTP_FILE_PUT:
			dwRetCode = RespFtpPut( szFileName, lpRequest->lpExchangeBuf );
			if( dwRetCode != TERR_SUCCESS ) {
				ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
				lptsComProps->packetType = pkTS_PIPE_FTP;
				lptsComProps->msgType = msgTS_FTP_FILE;
				lptsComProps->lp = -1;
					
				dwRetCode = SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				if( dwRetCode != TERR_SUCCESS )
						break;
			}
			break;

		case cmFTP_FILE_GET:
			dwRetCode = RespFtpGet( szFileName, lpRequest->lpExchangeBuf );
			if( dwRetCode != TERR_SUCCESS ) {
				ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
				lptsComProps->packetType = pkTS_PIPE_FTP;
				lptsComProps->msgType = msgTS_FTP_FILE;
				lptsComProps->lp = -1;
					
				dwRetCode = SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
				if( dwRetCode != TERR_SUCCESS )
					break;
			}
			break;

		default:
			dwRetCode = 0xFFFFFFFF;
			DefaultProcess( lpBuffer, lpRequest->lpExchangeBuf);
		}
		break;

	default:
		DefaultProcess( lpBuffer, lpRequest->lpExchangeBuf);
	}

	return dwRetCode;
}


DWORD TPrivateEngShell( LPSTR lpBuffer, LPREQUEST lpRequest )
{
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwRetCode = 0, dwRequest, i;
	FARPROC			lpfnProc;
	HANDLE			hTmpBuf = NULL;
	LPSTR			lpTmpBuf;
	DWORD			dwcbTmpBuf = 0;
	DWORD			dwcbUsed = 0;
	long			l_taskId;

	lptsComProps = (LPTS_COM_PROPS)lpBuffer;

	dwRequest = GetTERequest( lpBuffer + sizeof( TS_COM_PROPS ) );
	if( dwRequest == 0 ) 
	{
		dwRetCode = GetLastError();
		goto _PRIVATE_ENG_SHELL_ERR;
	}

	lpfnProc = GetTEEntryAddress( dwRequest );
	if( !lpfnProc ) 
	{
		dwRetCode = TERR_UNKNOWN_ERROR;
		goto  _PRIVATE_ENG_SHELL_ERR;
	}
	
	dwcbTmpBuf =  ( DWORD )lptsComProps->lp;
				
	hTmpBuf = LocalAlloc( LPTR, dwcbTmpBuf + 32 );
	if( hTmpBuf == NULL ) 
	{
		dwRetCode = GetLastError();
		goto _PRIVATE_ENG_SHELL_ERR;
	}
				
	lpTmpBuf = LocalLock( hTmpBuf );
	if( lpTmpBuf == NULL ) 
	{
		dwRetCode = GetLastError();
		LocalFree( hTmpBuf );
		hTmpBuf = NULL;
		goto _PRIVATE_ENG_SHELL_ERR;
	}

	//give an engine ready information to client
	ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
	lptsComProps->packetType = pkTS_PRIVATE_ENG;
	lptsComProps->msgType = 'D';
	lptsComProps->lp = 0;
	lptsComProps->len = 0;
	SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );


	dwcbUsed = 0;
	while( dwcbUsed < dwcbTmpBuf )
	{
		dwRetCode = SrvReadExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
		if( dwRetCode != TERR_SUCCESS )
			goto _PRIVATE_ENG_SHELL_ERR;
	
		if( lptsComProps->len + dwcbUsed > dwcbTmpBuf ) 
		{
			dwRetCode = TERR_SCRIPTS_TOO_LONG;
			goto _PRIVATE_ENG_SHELL_ERR;
		}

		CopyMemory( (LPVOID)(lpTmpBuf+dwcbUsed), (CONST VOID *)(lpBuffer+sizeof(TS_COM_PROPS)), 
					(DWORD)lptsComProps->len );
		dwcbUsed += lptsComProps->len;
	}

	l_taskId = abs(InterlockedIncrement( &lTaskId ));

	(*lpfnProc)( l_taskId, lpTmpBuf, &dwcbUsed, lpRequest );

	for( i = 0; i < dwcbUsed; i += EXCHANGE_CONTENT_SIZE )
	{
		ZeroMemory( lptsComProps, sizeof( TS_COM_PROPS));

		lptsComProps->packetType = pkTS_PRIVATE_ENG;
		lptsComProps->msgType = 'D';
		lptsComProps->lp = 0;

		if( ( i + EXCHANGE_CONTENT_SIZE ) < dwcbUsed )
		{
			lptsComProps->len = EXCHANGE_CONTENT_SIZE;
			lptsComProps->leftPacket = '\1';
			lptsComProps->endPacket = '\1';
		}
		else
		{
			lptsComProps->len = ( short )( dwcbUsed - i );
			lptsComProps->leftPacket = '\0';
			lptsComProps->endPacket = '\0';
		}
		
		memcpy( lpBuffer + sizeof( TS_COM_PROPS ), lpTmpBuf + i, 
				lptsComProps->len );

		SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );
	}

	if( hTmpBuf ) 
	{
		LocalUnlock( hTmpBuf );
		LocalFree( hTmpBuf );
	}

	return dwRetCode;

_PRIVATE_ENG_SHELL_ERR :
	if( hTmpBuf ) 
	{
		LocalUnlock( hTmpBuf );
		LocalFree( hTmpBuf );
	}

	ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
	lptsComProps->packetType = pkTS_PRIVATE_ENG;
	lptsComProps->msgType = 'E';
	lptsComProps->lp = dwRetCode;
	lptsComProps->len = wsprintf( lpBuffer + sizeof(TS_COM_PROPS),
						"Error Code: %ld.", dwRetCode ); 
	SrvWriteExchngBuf( lpRequest->lpExchangeBuf, lpBuffer, 4096 );

	return dwRetCode;
}


DWORD CALLBACK SprocEnumProc ( LPSTR szSprocName, LPEXCHNG_BUF_INFO lpExchangeBuf,
							 BOOL bEndOfEnum )
{
	CHAR			szBuffer[PIPE_BUFFER_SIZE];
	LPTS_COM_PROPS	lptsComProps;
	DWORD			dwRetCode;
	
	lptsComProps = (LPTS_COM_PROPS)szBuffer;

	ZeroMemory( szBuffer, sizeof( TS_COM_PROPS) );

	lstrcpy(szBuffer+sizeof( TS_COM_PROPS), szSprocName);
	
	lptsComProps->packetType = pkTS_SPROC;
	lptsComProps->msgType = msgTS_SPROC_REPLAY;
	
	lptsComProps->len = lstrlen(szSprocName)+1;

	lptsComProps->lp = 0;
	if( !bEndOfEnum ) {
		lptsComProps->leftPacket = '\x1';
		lptsComProps->endPacket = '\x1';
	}

	dwRetCode = SrvWriteExchngBuf( lpExchangeBuf, szBuffer, 4096 );
	return dwRetCode;
}
