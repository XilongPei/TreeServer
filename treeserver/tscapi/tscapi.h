/*------------------------------------------------------------------------------*\
 |	Module Name:																|
 |		TSCApi.h																|
 |	Abstract :																	|
 |		 Type definitions and function prototypes for TSCApi.c					|
 |	Author :																	|
 |		Motor																	|
 |	Date :																		|
 |		06/15/1998																|
 |																				|	
 |  Copyright (c). China Railway Software Corporation. 1998.					|
\*------------------------------------------------------------------------------*/
#ifndef __INC_TSCAPI
#define __INC_TSCAPI

#include <windows.h>
#include <lmcons.h>

#ifdef  _MSC_VER
/*
 * Currently, all MS C compilers for TreeServer applications 
 * default to 1 byte alignment.
 */
#pragma pack(push,1)
#endif  /* _MSC_VER */

#ifdef  __cplusplus
namespace TSCAPI {

extern "C" {
#endif

/*
typedef struct tagDioField {
	unsigned char	field[32];
	unsigned char	fieldtype;
	unsigned char	fieldlen;
	unsigned char	fielddec;
	long			fieldstart;
} DIO_FIELD, *PDIO_FIELD, *LPDIO_FIELD;
*/
// Changed by Jingyu Niu, 1999.08
// the new struct accord with dFIELD struct
typedef struct tagDioField {
	unsigned char field[32];
	unsigned char fieldtype;
	long 	      fieldlen;
	unsigned char fielddec;
	long	      sdword;		//use for ODBC data, 1998.7.1
	void          *px;
	long		fieldstart;
} DIO_FIELD, *PDIO_FIELD, *LPDIO_FIELD;

typedef struct tagTSFIELD {
	unsigned char field[11];			//1..11
	unsigned char fieldtype;			//12
	char          szRes1[4];			//13,14,15,16
	unsigned char fieldlen;				//17
	unsigned char fielddec;				//18
	char          szRes2[14];			//19-32
} TSFIELD;

typedef struct tagTableInfo {
    unsigned char tab_flag;				//1
    char          last_modi_time[3];    //2,3,4
    long		  rec_num;				//4,5,6,7
    short		  headlen;				//8,9
    short		  rec_len;				//10,11
    short         field_num;			//this should be calculated,headlen/32
    char          szRes[20];
    TSFIELD   	  fields[1];             //according to field_num
} TABLE_INFO, *PTABLE_INFO, *LPTABLE_INFO;

typedef struct tagSysVarOFunType {
	short			type;
	char			VarOFunName[32];
	unsigned char	values[32];
	short			length;
} SysVarOFunType, *PSysVarOFunType, *LPSysVarOFunType;

typedef struct tagRECProps {
    long  iSeqNum;	   //: Longint;          { When Seq# supported only }
    long  iPhyRecNum;      //: Longint;          { When Phy Rec#s supported only }
    short iRecStatus;      //: Word;             { Delayed Updates Record Status }
    short bSeqNumChanged;  //: WordBool;         { Not used }
    short bDeleteFlag;     //: WordBool;         { When soft delete supported only }
} RECPROPS, *LPRECPROPS;

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

#ifdef __LIBRARY__

/*------------------------------------------------------------------------------*\
 | User Security Functions														|
\*------------------------------------------------------------------------------*/
_declspec( dllexport )
DWORD	tsLogon ( LPCSTR szServer, LPCSTR szUser, LPCSTR szPasswd, 
				  LPHANDLE lphConnect );

_declspec( dllexport )
BOOL	tsLogoff ( HANDLE hConnect );

_declspec( dllexport )
DWORD	tsUserChangePwd ( HANDLE hConnect, 
						  LPCSTR szOldPasswd, LPCSTR szNewPasswd );

/*------------------------------------------------------------------------------*\
 | Database Operation Functions													|	
\*------------------------------------------------------------------------------*/
_declspec( dllexport )
DWORD	tsTableOpen ( HANDLE hConnect, LPCSTR szDatabase, 
					  LPCSTR szTable, LPTABLE_INFO *lppTableInfo );

_declspec( dllexport )
BOOL	tsTableClose( HANDLE hConnect, DWORD dwTableId );

_declspec( dllexport )
BOOL	tsTableLock( HANDLE hConncet, DWORD dwTableId );

_declspec( dllexport )
BOOL	tsTableUnlock( HANDLE hConncet, DWORD dwTableId );

_declspec( dllexport )
DWORD	tsTableGetRec( HANDLE hConnect, DWORD dwTableId, DWORD dwRecNo, 
					   LPSTR szRecBuf, DWORD dwBufLen, LPRECPROPS lpRecProps );

_declspec( dllexport )
BOOL	tsTableDelRec( HANDLE hConnect, DWORD dwTableId, DWORD dwRecNo );

_declspec( dllexport )
BOOL	tsTablePutRec( HANDLE hConnect, DWORD dwTableId, DWORD dwRecNo, 
					   LPCSTR szRecBuf, DWORD dwBufLen );

_declspec( dllexport )
BOOL	tsTableAppendRec( HANDLE hConnect, DWORD dwTableId, LPCSTR szRecBuf, 
						  DWORD dwBufLen );

_declspec( dllexport )
BOOL	tsTableInsertRec( HANDLE hConnect, DWORD dwTableId, LPCSTR szRecBuf, 
						  DWORD dwBufLen );

_declspec( dllexport )
BOOL	tsTableBlobGet( HANDLE hConnect, DWORD dwTableId, LPCSTR szFieldName, 
						DWORD dwRecNo, LPCSTR szFileName );

_declspec( dllexport )
BOOL	tsTableBlobPut( HANDLE hConnect, DWORD dwTableId, LPCSTR szFieldName, 
						DWORD dwRecNo, LPCSTR szFileName );

_declspec( dllexport )
LONG	tsTableRecNum( HANDLE hConncet, DWORD dwTableId );

_declspec( dllexport )
DWORD	tsTableGetFieldInfo( HANDLE hConnect, DWORD dwTableId, 
							 LPDIO_FIELD lpFieldInfo, DWORD dwFieldLen );

_declspec( dllexport )
LPSTR	tsTableGetFieldByName( LPDIO_FIELD lpFieldInfo, LPCSTR szRecBuf,
							   LPCSTR szFieldName,LPSTR szResult, 
							   DWORD dwResultSize);
_declspec( dllexport )
LPSTR	tsTableGetfieldById( LPDIO_FIELD lpdioField, LPCSTR szRecBuf, 
							 DWORD dwFieldId );

/*------------------------------------------------------------------------------*\
 | Transaction language ASQL Functions											|	
\*------------------------------------------------------------------------------*/
_declspec( dllexport )
DWORD	tsCallAsqlSvr( HANDLE hConnect, LPCSTR szAsqlScriptFileName, 
					   LPCSTR szResultFileName, LPSTR szInstResult, 
					   DWORD dwInstResultSize );

_declspec( dllexport )
DWORD tsCallAsqlSvrM( HANDLE hConnect, LPCSTR szAsqlScript, LPVOID lpRsutMem, 
					  LPDWORD lpdwLen, LPSTR szInstResult, DWORD dwInstResultSize );

_declspec( dllexport )
DWORD tsCallAsqlSvrMVar( HANDLE hConnect, LPCSTR szAsqlScript, 
						 LPSysVarOFunType lpXexpVar, DWORD dwXexpVarNum, 
						 LPSTR szInstResult, DWORD dwInstResultSize, 
						 LPVOID lpRsutMem, LPDWORD lpdwLen );

_declspec( dllexport )
DWORD tsHistDataCollect( HANDLE hConnect, LPCSTR szHistcondPath, LPCSTR szRem,
						 LPSysVarOFunType lpXexpVar, DWORD dwXexpVarNum, 
						 LPSTR szInstResult, DWORD dwInstResultSize, 
						 LPVOID lpRsutMem, LPDWORD lpdwLen );

_declspec( dllexport )
DWORD tsGetUserNetPath ( HANDLE hConnect, LPSTR lpNetPath, DWORD dwBuffer );

_declspec( dllexport )
BOOL tsGetRegisteredUser( HANDLE hConnect, LPCLIENT_RES_INFO lpClientResInfo, 
						  LPDWORD lpdwMaxClientNum );

_declspec( dllexport )
DWORD tsGetIdenticalUserNum( HANDLE hConnect, LPCSTR szUser );

_declspec( dllexport )
DWORD tsCreateStoredProc( HANDLE hConnect,	LPCSTR lpProcName, LPSTR lpBuffer, 
						  DWORD	dwBufLen );

_declspec( dllexport )
DWORD tsDeleteStoredProc( HANDLE hConnect, LPCSTR lpProcName );

_declspec( dllexport )
DWORD tsGetStoredProc( HANDLE hConnect, LPCSTR lpProcName, LPSTR lpBuffer,
					   LPDWORD lpdwBufLen );

_declspec( dllexport )
DWORD tsUpdateStoredProc( HANDLE hConnect, LPCSTR lpProcName, LPSTR	lpBuffer,
						  DWORD dwBufLen );

_declspec( dllexport )
DWORD tsGrantUserPrivilege( HANDLE hConnect, LPCSTR	lpProcName, LPCSTR szUserName, char cPrivilege );

_declspec( dllexport )
DWORD tsRevokeUserPrivilege( HANDLE	hConnect, LPCSTR lpProcName, LPCSTR	szUserName, char cPrivilege );

_declspec( dllexport )
DWORD tsExecuteStoredProc( HANDLE hConnect, LPCSTR lpProcName,
						 LPSysVarOFunType lpXexpVar, DWORD dwXexpVarNum, 
						 LPSTR szInstResult, DWORD dwInstResultSize, 
						 LPVOID lpRsutMem, LPDWORD lpdwLen );

__declspec( dllexport )
DWORD tsGetSprocList ( HANDLE hConnect, LPSTR szListBuffer, DWORD dwBuffer );

__declspec( dllexport )
DWORD tsGetDictInfo( HANDLE hConnect, LPCSTR szGroup, 
				   LPCSTR szLabel, LPCSTR szKey, LPSTR *szCont, int dwContSize );

__declspec( dllexport )
DWORD tsPutDictInfo( HANDLE hConnect, LPCSTR szGroup, 
							LPCSTR szLabel, LPCSTR szKey, LPCSTR *szCont );

__declspec( dllexport )
DWORD tsGeneralFunCall ( HANDLE hConnect, char packetType, 
										  char msgType,
										  long lp,
										  LPSTR szListBuffer, DWORD dwBuffer );
__declspec( dllexport )
int tsSetTcpipPort( char *szPortOrService );

__declspec( dllexport )
DWORD tsFtpGet( HANDLE hConnect,
						LPSTR lpRemoteFile, LPSTR lpLocalFile );

__declspec( dllexport )
DWORD tsFtpPut( HANDLE hConnect, LPSTR lpLocalFile, LPSTR lpRemoteFile );

#else // __LIBRARY__

/*------------------------------------------------------------------------------*\
 | User Security Functions														|
\*------------------------------------------------------------------------------*/
_declspec( dllimport )
DWORD	tsLogon ( LPCSTR szServer, LPCSTR szUser, LPCSTR szPasswd, 
				  LPHANDLE lphConnect );

_declspec( dllimport )
BOOL	tsLogoff ( HANDLE hConnect );

_declspec( dllimport )
DWORD	tsUserChangePwd ( HANDLE hConnect, 
						  LPCSTR szOldPasswd, LPCSTR szNewPasswd );

/*------------------------------------------------------------------------------*\
 | Database Operation Functions													|	
\*------------------------------------------------------------------------------*/
_declspec( dllimport )
DWORD	tsTableOpen ( HANDLE hConnect, LPCSTR szDatabase, 
					  LPCSTR szTable, LPTABLE_INFO *lppTableInfo );

_declspec( dllimport )
BOOL	tsTableClose( HANDLE hConnect, DWORD dwTableId );

_declspec( dllimport )
BOOL	tsTableLock( HANDLE hConncet, DWORD dwTableId );

_declspec( dllimport )
BOOL	tsTableUnlock( HANDLE hConncet, DWORD dwTableId );

_declspec( dllimport )
DWORD	tsTableGetRec( HANDLE hConnect, DWORD dwTableId, DWORD dwRecNo, 
					   LPSTR szRecBuf, DWORD dwBufLen, LPRECPROPS lpRecProps );

_declspec( dllimport )
BOOL	tsTableDelRec( HANDLE hConnect, DWORD dwTableId, DWORD dwRecNo );

_declspec( dllimport )
BOOL	tsTablePutRec( HANDLE hConnect, DWORD dwTableId, DWORD dwRecNo, 
					   LPCSTR szRecBuf, DWORD dwBufLen );

_declspec( dllimport )
BOOL	tsTableAppendRec( HANDLE hConnect, DWORD dwTableId, LPCSTR szRecBuf, 
						  DWORD dwBufLen );

_declspec( dllimport )
BOOL	tsTableInsertRec( HANDLE hConnect, DWORD dwTableId, LPCSTR szRecBuf, 
						  DWORD dwBufLen );

_declspec( dllimport )
BOOL	tsTableBlobGet( HANDLE hConnect, DWORD dwTableId, LPCSTR szFieldName, 
						DWORD dwRecNo, LPCSTR szFileName );

_declspec( dllimport )
BOOL	tsTableBlobPut( HANDLE hConnect, DWORD dwTableId, LPCSTR szFieldName, 
						DWORD dwRecNo, LPCSTR szFileName );

_declspec( dllimport )
LONG	tsTableRecNum( HANDLE hConncet, DWORD dwTableId );

_declspec( dllimport )
DWORD	tsTableGetFieldInfo( HANDLE hConnect, DWORD dwTableId, 
							 LPDIO_FIELD lpFieldInfo, DWORD dwFieldLen );

_declspec( dllimport )
LPSTR	tsTableGetFieldByName( LPDIO_FIELD lpFieldInfo, LPCSTR szRecBuf,
							   LPCSTR szFieldName,LPSTR szResult, 
							   DWORD dwResultSize);
_declspec( dllimport )
LPSTR	tsTableGetfieldById( LPDIO_FIELD lpdioField, LPCSTR szRecBuf, 
							 DWORD dwFieldId );

/*------------------------------------------------------------------------------*\
 | Transaction language ASQL Functions											|	
\*------------------------------------------------------------------------------*/
_declspec( dllimport )
DWORD	tsCallAsqlSvr( HANDLE hConnect, LPCSTR szAsqlScriptFileName, 
					   LPCSTR szResultFileName, LPSTR szInstResult, 
					   DWORD dwInstResultSize );

_declspec( dllimport )
DWORD tsCallAsqlSvrM( HANDLE hConnect, LPCSTR szAsqlScript, LPVOID lpRsutMem, 
					  LPDWORD lpdwLen, LPSTR szInstResult, DWORD dwInstResultSize );

_declspec( dllimport )
DWORD tsCallAsqlSvrMVar( HANDLE hConnect, LPCSTR szAsqlScript, 
						 LPSysVarOFunType lpXexpVar, DWORD dwXexpVarNum, 
						 LPSTR szInstResult, DWORD dwInstResultSize, 
						 LPVOID lpRsutMem, LPDWORD lpdwLen );

_declspec( dllimport )
DWORD tsHistDataCollect( HANDLE hConnect, LPCSTR szHistcondPath, LPCSTR szRem,
						 LPSysVarOFunType lpXexpVar, DWORD dwXexpVarNum, 
						 LPSTR szInstResult, DWORD dwInstResultSize, 
						 LPVOID lpRsutMem, LPDWORD lpdwLen );

_declspec( dllimport )
DWORD tsGetUserNetPath ( HANDLE hConnect, LPSTR lpNetPath, DWORD dwBuffer );

_declspec( dllimport )
BOOL tsGetRegisteredUser( HANDLE hConnect, LPCLIENT_RES_INFO lpClientResInfo, 
						  LPDWORD lpdwMaxClientNum );

_declspec( dllimport )
DWORD tsGetIdenticalUserNum( HANDLE hConnect, LPCSTR szUser );

_declspec( dllimport )
DWORD tsCreateStoredProc( HANDLE hConnect,	LPCSTR lpProcName, LPSTR lpBuffer, 
						  DWORD	dwBufLen );

_declspec( dllimport )
DWORD tsDeleteStoredProc( HANDLE hConnect, LPCSTR lpProcName );

_declspec( dllimport )
DWORD tsGetStoredProc( HANDLE hConnect, LPCSTR lpProcName, LPSTR lpBuffer,
					   LPDWORD lpdwBufLen );

_declspec( dllimport )
DWORD tsUpdateStoredProc( HANDLE hConnect, LPCSTR lpProcName, LPSTR	lpBuffer,
						  DWORD dwBufLen );

_declspec( dllimport )
DWORD tsGrantUserPrivilege( HANDLE hConnect, LPCSTR	lpProcName, LPCSTR szUserName, char cPrivilege );

_declspec( dllimport )
DWORD tsRevokeUserPrivilege( HANDLE	hConnect, LPCSTR lpProcName, LPCSTR	szUserName, char cPrivilege );

_declspec( dllimport )
DWORD tsExecuteStoredProc( HANDLE hConnect, LPCSTR lpProcName,
						 LPSysVarOFunType lpXexpVar, DWORD dwXexpVarNum, 
						 LPSTR szInstResult, DWORD dwInstResultSize, 
						 LPVOID lpRsutMem, LPDWORD lpdwLen );

__declspec( dllimport )
DWORD tsGetSprocList ( HANDLE hConnect, LPSTR szListBuffer, DWORD dwBuffer );

__declspec( dllimport )
DWORD tsGetDictInfo( HANDLE hConnect, LPCSTR szGroup, 
				   LPCSTR szLabel, LPCSTR szKey, LPSTR *szCont, int dwContSize );

__declspec( dllimport )
DWORD tsPutDictInfo( HANDLE hConnect, LPCSTR szGroup, 
							LPCSTR szLabel, LPCSTR szKey, LPCSTR *szCont );

__declspec( dllimport )
DWORD tsGeneralFunCall ( HANDLE hConnect, char packetType, 
										  char msgType,
										  long lp,
										  LPSTR szListBuffer, DWORD dwBuffer );
__declspec( dllimport )
int tsSetTcpipPort( char *szPortOrService );

__declspec( dllimport )
DWORD tsFtpGet( HANDLE hConnect,
						LPSTR lpRemoteFile, LPSTR lpLocalFile );

__declspec( dllimport )
DWORD tsFtpPut( HANDLE hConnect, LPSTR lpLocalFile, LPSTR lpRemoteFile );

#endif // __LIBRARY__

#ifdef  __cplusplus
}

}
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif // __INC_TSCAPI