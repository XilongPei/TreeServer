/******************************
*	Module Name:
*		TSFTP.C
*
*	Abstract:
*		Tree Server internal FTP server module. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1998  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1998.03
********************************************************************************/

#define STRICT
#include <windows.h>

#include "taccount.h"
#include "treesvr.h"

DWORD RespFtpPut ( LPSTR lpFileName, LPEXCHNG_BUF_INFO lpExchBuf )
{
	HANDLE hFile;
	CHAR szBuffer[PIPE_BUFFER_SIZE];
	DWORD dwcbBuffer;
	DWORD dwcbRDWR;
	DWORD dwRetCode;
	LPTS_COM_PROPS lptsComProps;
	BOOL fSuccess;
	BOOL fEof = FALSE;
	BOOL fRDWR_OK = TRUE;

	if( lpFileName == NULL || lpExchBuf == NULL ) {
		return ERROR_INVALID_PARAMETER;
	}

	lptsComProps = (LPTS_COM_PROPS)szBuffer; 

	hFile = CreateFile( 
			lpFileName,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL );

	ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
	if( hFile == INVALID_HANDLE_VALUE ) {
		lptsComProps->packetType = pkTS_PIPE_FTP;
		lptsComProps->msgType = msgTS_FTP_FILE;
		lptsComProps->lp = GetLastError();
					
		dwRetCode = SrvWriteExchngBuf( lpExchBuf, szBuffer, 4096 );

		return TERR_SUCCESS;
	}

	lptsComProps->packetType = pkTS_PIPE_FTP;
	lptsComProps->msgType = msgTS_FTP_FILE;
	lptsComProps->lp = cmFTP_FILE_OK;
					
	dwRetCode = SrvWriteExchngBuf( lpExchBuf, szBuffer, 4096 );
	if( dwRetCode != TERR_SUCCESS ) {
		CloseHandle( hFile );
		DeleteFile( lpFileName );
	}

	while( !fEof ) {
		dwRetCode = SrvReadExchngBuf( lpExchBuf, szBuffer, 4096 );
		if( dwRetCode != TERR_SUCCESS ) {
			fRDWR_OK = FALSE;
			break;
		}

		if( lptsComProps->lp == cmFTP_FILE_END ) {
			fEof = TRUE; 
			break;
		}
		
		if( lptsComProps->lp == cmFTP_FILE_ERROR ) {
			fRDWR_OK = FALSE;
			break;
		}

		dwcbBuffer = lptsComProps->len;
		fSuccess = WriteFile ( hFile,
				(LPVOID)(szBuffer+sizeof(TS_COM_PROPS)),
				dwcbBuffer,
				&dwcbRDWR,
				NULL );
		if ( !fSuccess || dwcbRDWR != dwcbBuffer ) {
			fRDWR_OK = FALSE;
		}
	}

    FlushFileBuffers( hFile );
	CloseHandle( hFile );

	ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
	lptsComProps->packetType = pkTS_PIPE_FTP;
	lptsComProps->msgType = msgTS_FTP_FILE;

	if( !fRDWR_OK ) {
		DeleteFile( lpFileName );
		lptsComProps->lp = cmFTP_FILE_ERROR;
	}
	else {
		lptsComProps->lp = cmFTP_FILE_OK;
	}

	SrvWriteExchngBuf( lpExchBuf, szBuffer, 4096 );

	return TERR_SUCCESS;
}

DWORD RespFtpGet ( LPSTR lpFileName, LPEXCHNG_BUF_INFO lpExchBuf )
{
	HANDLE hFile;
	CHAR szBuffer[PIPE_BUFFER_SIZE];
	DWORD dwcbBuffer;
	DWORD dwcbRDWR;
	DWORD dwRetCode;
	LPTS_COM_PROPS lptsComProps;
	BOOL fSuccess;
	BOOL fEof = FALSE;
	BOOL fRDWR_OK = TRUE;

	lptsComProps = (LPTS_COM_PROPS)szBuffer; 

	if( lpFileName == NULL || lpExchBuf == NULL ) {
		return ERROR_INVALID_PARAMETER;
	}

	hFile = CreateFile( 
			lpFileName,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL );

	ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );

	if( hFile == INVALID_HANDLE_VALUE ) {
		lptsComProps->packetType = pkTS_PIPE_FTP;
		lptsComProps->msgType = msgTS_FTP_FILE;
		lptsComProps->lp = GetLastError();
					
		dwRetCode = SrvWriteExchngBuf( lpExchBuf, szBuffer, 4096 );
		return TERR_SUCCESS;
	}

	lptsComProps->packetType = pkTS_PIPE_FTP;
	lptsComProps->msgType = msgTS_FTP_FILE;
	lptsComProps->lp = cmFTP_FILE_OK;
	lptsComProps->leftPacket = '\x1';
	lptsComProps->endPacket = '\x1';
					
	dwRetCode  = SrvWriteExchngBuf( lpExchBuf, szBuffer, 4096 );
	if( dwRetCode != TERR_SUCCESS ) {
		CloseHandle( hFile );
		return TERR_SUCCESS;
	}

	SetFilePointer( hFile, 0, NULL, FILE_BEGIN );
	lptsComProps->packetType = pkTS_PIPE_FTP;
	lptsComProps->msgType = msgTS_FTP_FILE;
	lptsComProps->lp = cmFTP_FILE_DATA;
	lptsComProps->leftPacket = '\x1';
	lptsComProps->endPacket = '\x1';

	while( !fEof ) {
		dwcbBuffer = PIPE_BUFFER_SIZE - sizeof(TS_COM_PROPS);
		fSuccess = ReadFile ( hFile,
				(LPVOID)(szBuffer+sizeof(TS_COM_PROPS)),
				dwcbBuffer,
				&dwcbRDWR,
				NULL );
		if ( !fSuccess || dwcbRDWR != dwcbBuffer ) {
			dwRetCode = GetLastError();
			if( dwRetCode == ERROR_HANDLE_EOF || dwRetCode == 0 ) {
				if( dwcbRDWR == 0 )
					break;
				else
					fEof = TRUE;
			}
			else {
				fRDWR_OK = FALSE;
				break;
			}
		}

		lptsComProps->len = (short)dwcbRDWR;
		dwRetCode = SrvWriteExchngBuf( lpExchBuf, szBuffer, 4096 );
		if( dwRetCode != TERR_SUCCESS ) {
			fRDWR_OK = FALSE;
			break;
		}
	}

	CloseHandle( hFile );

	ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
	lptsComProps->packetType = pkTS_PIPE_FTP;
	lptsComProps->msgType = msgTS_FTP_FILE;

	if( fRDWR_OK )
		lptsComProps->lp = cmFTP_FILE_END;
	else {
		lptsComProps->lp = cmFTP_FILE_ERROR;
		CopyMemory( (LPVOID)(szBuffer+sizeof(TS_COM_PROPS)), (LPVOID)&dwRetCode, sizeof(DWORD) );
	}

	SrvWriteExchngBuf( lpExchBuf, szBuffer, 4096 );

	return TERR_SUCCESS;
}
