/******************************
*	Module Name:
*		EXCHBUFF.C
*
*	Abstract:
*		Tree Server exchange buffer mamagement module. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1998  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1998.02
********************************************************************************/

#define STRICT
#include <windows.h>
#include <tchar.h>

#include "tlimits.h"
#include "terror.h"
#include "exchbuff.h"
#include "ts_com.h"

LPEXCHNG_BUF_INFO g_lpExchBufInfo = NULL;

VOID InitExchngBufInfo ( LPEXCHNG_BUF_INFO lpBufInfo, LPVOID lpBuffer, HANDLE hEvent )
{
	if( lpBufInfo == NULL )
		return;

	lpBufInfo->lpExchBuf = lpBuffer;
	lpBufInfo->hEmpEvent = hEvent;

	return;
}

LPEXCHNG_BUF_INFO InitExchngBuf ( DWORD dwSize, LPDWORD lpdwRetCode )
{
	UINT uTotalSize;
	HLOCAL hExchBufInfo;
	LPEXCHNG_BUF_INFO lpExchBufInfo;
	DWORD dwRetCode;

	DWORD i;
	BOOL fSuccess = TRUE;

	*lpdwRetCode = TERR_SUCCESS;

	// Total size of the control header and the exchnange buffer.
	uTotalSize = (sizeof(EXCHNG_BUF_INFO) + EXCHNG_BUF_SIZE) * dwSize + 
				sizeof(EXCHNG_BUF_INFO);
	
	// Alloc the memory, initializes memory contents to zero.
	hExchBufInfo = LocalAlloc( LPTR, uTotalSize );
	if( hExchBufInfo == NULL ) {
		*lpdwRetCode = GetLastError();
		return NULL;
	}

	lpExchBufInfo = (LPEXCHNG_BUF_INFO)LocalLock( hExchBufInfo );
	if( lpExchBufInfo == NULL ) {
		*lpdwRetCode = GetLastError();
		LocalFree( hExchBufInfo );
		return NULL;
	}

	for( i = 0; i < dwSize; i++ ) {
		CHAR szEventName[MAX_PATH];

		wsprintf( szEventName, _TEXT( "$TS_ExchngEmp%X$" ), i );
		lpExchBufInfo[i].hEmpEvent = CreateEvent( NULL, FALSE, TRUE, szEventName ); 
		dwRetCode = GetLastError();
		if( lpExchBufInfo[i].hEmpEvent == NULL || dwRetCode == ERROR_ALREADY_EXISTS ) {
			fSuccess = FALSE;
			break;
		}

		lpExchBufInfo[i].lpExchBuf = (LPVOID)( (LPSTR)(lpExchBufInfo) +
						sizeof(EXCHNG_BUF_INFO) * (dwSize+1) +
						EXCHNG_BUF_SIZE * i );
	}

	if( !fSuccess ) {
		ReleaseExchngBuf();
		*lpdwRetCode = dwRetCode;
		return NULL;
	}

	return lpExchBufInfo;
}

DWORD ReleaseExchngBuf ( VOID )
{
	HLOCAL hExchBufInfo;
	LPEXCHNG_BUF_INFO lpPtr;

	if( g_lpExchBufInfo == NULL )
		return ERROR_INVALID_PARAMETER;

	hExchBufInfo = LocalHandle( (LPVOID)g_lpExchBufInfo );
	if( hExchBufInfo == NULL )
		return GetLastError();

	for( lpPtr = g_lpExchBufInfo; lpPtr->lpExchBuf != NULL; lpPtr++ ) {
		if( lpPtr->hEmpEvent != NULL )
			CloseHandle( lpPtr->hEmpEvent );
	}

	LocalUnlock( hExchBufInfo );
	LocalFree( hExchBufInfo );
	
	return  0;

}


LPEXCHNG_BUF_INFO AllocExchngBuf ( VOID )
{
	LPEXCHNG_BUF_INFO lpPtr;

	if( g_lpExchBufInfo == NULL )
		return NULL;

	for( lpPtr = g_lpExchBufInfo; lpPtr->lpExchBuf != NULL; lpPtr++ )
		if( lpPtr->cFlag == '\x0' )
			break;

	if( lpPtr->lpExchBuf == NULL )
		return NULL;

	lpPtr->cFlag = '\x1';
	lpPtr->cErrorStat = ERROR_OK;

	return lpPtr;
}

VOID FreeExchngBuf ( LPEXCHNG_BUF_INFO lpExchBufInfo )
{
	if( lpExchBufInfo == NULL )
		return;

	SetEvent( lpExchBufInfo->hEmpEvent );

	lpExchBufInfo->cFlag = '\x0';
}

DWORD CliWriteExchngBuf ( LPEXCHNG_BUF_INFO lpExchBufInfo, LPVOID lpBuffer, DWORD dwcbBuffer )
{
	if( lpExchBufInfo == NULL || lpBuffer == NULL || 
			dwcbBuffer <= 0 || dwcbBuffer > EXCHNG_BUF_SIZE )
		return ERROR_INVALID_PARAMETER;

	// lprintfs( "->CliWriteExchangeBuffer->\n" );

	// Add by Niu Jingyu, 1998.03.13
	if( lpExchBufInfo->cErrorStat != ERROR_OK ) {
		if( lpExchBufInfo->cErrorStat == ERROR_CLI_CLOSE )
			return TERR_CLIENT_CLOSE;
		if( lpExchBufInfo->cErrorStat == ERROR_SRV_CLOSE )
			return TERR_SERVICE_CLOSE;
	}

	_try {
		WaitForSingleObject( lpExchBufInfo->hEmpEvent, INFINITE );
		// lprintfs( "->CliWriteExchangeBuffer Wait for Sync OK.\n" );

		CopyMemory( lpExchBufInfo->lpExchBuf, (CONST VOID *)lpBuffer, dwcbBuffer );

		PostThreadMessage( lpExchBufInfo->dwSrvThreadId, TS_EXCHBUF_WRRD, 0, 0 );
		// lprintfs( "->CliWriteExchangeBuffer Post Service Msg OK.\n" );
	}
	_except ( EXCEPTION_EXECUTE_HANDLER ) {
		char szBuffer[256];

		wsprintf( szBuffer, "CliWriteExchngBuf() cause an exception.\n" );
		MessageBox( NULL, szBuffer, "Inspector", MB_OK | MB_ICONSTOP );
	}

	// Add by Niu Jingyu, 1998.03.13
	if( lpExchBufInfo->cErrorStat != ERROR_OK ) {
		if( lpExchBufInfo->cErrorStat == ERROR_CLI_CLOSE )
			return TERR_CLIENT_CLOSE;
		if( lpExchBufInfo->cErrorStat == ERROR_SRV_CLOSE )
			return TERR_SERVICE_CLOSE;
	}
	
	return TERR_SUCCESS;
}

DWORD CliReadExchngBuf ( LPEXCHNG_BUF_INFO lpExchBufInfo, LPVOID lpBuffer, DWORD dwcbBuffer )
{
	MSG Msg;

	if( lpExchBufInfo == NULL || lpBuffer == NULL || 
			dwcbBuffer <= 0 || dwcbBuffer > EXCHNG_BUF_SIZE )
		return ERROR_INVALID_PARAMETER;

	// lprintfs( "->CliReadExchangeBuffer->\n" );

	// Add by Niu Jingyu, 1998.03.13
	if( lpExchBufInfo->cErrorStat != ERROR_OK ) {
		if( lpExchBufInfo->cErrorStat == ERROR_CLI_CLOSE )
			return TERR_CLIENT_CLOSE;
		if( lpExchBufInfo->cErrorStat == ERROR_SRV_CLOSE )
			return TERR_SERVICE_CLOSE;
	}

	_try {
		GetMessage( &Msg, NULL, TS_EXCHBUF_WRRD, TS_EXCHBUF_WRRD );
		// lprintfs( "->CliReadExchangeBuffer Get Service Msg OK.\n" );

		CopyMemory( lpBuffer, (CONST VOID *)lpExchBufInfo->lpExchBuf, dwcbBuffer );

		SetEvent( lpExchBufInfo->hEmpEvent );
		// lprintfs( "->CliReadExchangeBuffer SetEvent OK.\n" );
	}
	_except ( EXCEPTION_EXECUTE_HANDLER ) {
		char szBuffer[256];

		wsprintf( szBuffer, "CliReadExchngBuf() cause an exception.\n" );
		MessageBox( NULL, szBuffer, "Inspector", MB_OK | MB_ICONSTOP );
	}

	// Add by Niu Jingyu, 1998.03.13
	if( lpExchBufInfo->cErrorStat != ERROR_OK ) {
		if( lpExchBufInfo->cErrorStat == ERROR_CLI_CLOSE )
			return TERR_CLIENT_CLOSE;
		if( lpExchBufInfo->cErrorStat == ERROR_SRV_CLOSE )
			return TERR_SERVICE_CLOSE;
	}

	return TERR_SUCCESS;
}

DWORD SrvWriteExchngBuf ( LPEXCHNG_BUF_INFO lpExchBufInfo, LPVOID lpBuffer, DWORD dwcbBuffer )
{
	if( lpExchBufInfo == NULL || lpBuffer == NULL || 
			dwcbBuffer <= 0 || dwcbBuffer > EXCHNG_BUF_SIZE )
		return ERROR_INVALID_PARAMETER;

	// lprintfs( "->SrvWriteExchangeBuffer->\n" );

	// Add by Niu Jingyu, 1998.03.13
	if( lpExchBufInfo->cErrorStat != ERROR_OK ) {
		if( lpExchBufInfo->cErrorStat == ERROR_CLI_CLOSE )
			return TERR_CLIENT_CLOSE;
		if( lpExchBufInfo->cErrorStat == ERROR_SRV_CLOSE )
			return TERR_SERVICE_CLOSE;
	}

	_try {
		WaitForSingleObject( lpExchBufInfo->hEmpEvent, INFINITE );
		// lprintfs( "->SrvWriteExchangeBuffer Wait for sync OK.\n" );

		CopyMemory( lpExchBufInfo->lpExchBuf, (CONST VOID *)lpBuffer, dwcbBuffer );

		PostThreadMessage( lpExchBufInfo->dwCliThreadId, TS_EXCHBUF_WRRD, 0, 0 );
		// lprintfs( "->CliWriteExchangeBuffer Post Client agent Msg OK.\n" );
	}
	_except ( EXCEPTION_EXECUTE_HANDLER ) {
		char szBuffer[256];

		wsprintf( szBuffer, "SvrWriteExchngBuf() cause an exception.\n" );
		MessageBox( NULL, szBuffer, "Inspector", MB_OK | MB_ICONSTOP );
	}

	// Add by Niu Jingyu, 1998.03.13
	if( lpExchBufInfo->cErrorStat != ERROR_OK ) {
		if( lpExchBufInfo->cErrorStat == ERROR_CLI_CLOSE )
			return TERR_CLIENT_CLOSE;
		if( lpExchBufInfo->cErrorStat == ERROR_SRV_CLOSE )
			return TERR_SERVICE_CLOSE;
	}

	return TERR_SUCCESS;
}

DWORD SrvReadExchngBuf ( LPEXCHNG_BUF_INFO lpExchBufInfo, LPVOID lpBuffer, DWORD dwcbBuffer )
{
	MSG Msg;

	if( lpExchBufInfo == NULL || lpBuffer == NULL || 
			dwcbBuffer <= 0 || dwcbBuffer > EXCHNG_BUF_SIZE )
		return ERROR_INVALID_PARAMETER;

	// lprintfs( "->SrvReadExchangeBuffer->\n" );

	// Add by Niu Jingyu, 1998.03.13
	if( lpExchBufInfo->cErrorStat != ERROR_OK ) {
		if( lpExchBufInfo->cErrorStat == ERROR_CLI_CLOSE )
			return TERR_CLIENT_CLOSE;
		if( lpExchBufInfo->cErrorStat == ERROR_SRV_CLOSE )
			return TERR_SERVICE_CLOSE;
	}

	_try {
		GetMessage( &Msg, NULL, TS_EXCHBUF_WRRD, TS_EXCHBUF_WRRD );
		// lprintfs( "->SrvReadExchangeBuffer Get Client agent Msg OK.\n" );

		CopyMemory( lpBuffer, (CONST VOID *)lpExchBufInfo->lpExchBuf, dwcbBuffer );

		SetEvent( lpExchBufInfo->hEmpEvent );
		// lprintfs( "->SrvReadExchangeBuffer SetEvent OK.\n" );
	}
	_except ( EXCEPTION_EXECUTE_HANDLER ) {
		char szBuffer[256];

		wsprintf( szBuffer, "SvrReadExchngBuf() cause an exception.\n" );
		MessageBox( NULL, szBuffer, "Inspector", MB_OK | MB_ICONSTOP );
	}

	// Add by Niu Jingyu, 1998.03.13
	if( lpExchBufInfo->cErrorStat != ERROR_OK ) {
		if( lpExchBufInfo->cErrorStat == ERROR_CLI_CLOSE )
			return TERR_CLIENT_CLOSE;
		if( lpExchBufInfo->cErrorStat == ERROR_SRV_CLOSE )
			return TERR_SERVICE_CLOSE;
	}

	return TERR_SUCCESS;
}

// Add by Niu Jingyu, 1998.07.07
DWORD SrvWriteExchngBufEx ( LPEXCHNG_BUF_INFO lpExchBufInfo, 
						  LPTS_COM_PROPS lptsComProps, 
						  LPVOID lpBuffer, 
						  DWORD dwcbBuffer,
						  BOOL bIfEnd )
{
	CHAR szTmpBuffer[EXCHNG_BUF_SIZE];
	LPTS_COM_PROPS lptscp;
	LPSTR lpPtr;
	DWORD dwOffset;
	DWORD dwRetCode = 0;
	int step;

	if( lpExchBufInfo == NULL || lpBuffer == NULL || 
			dwcbBuffer <= 0 || lptsComProps == NULL )
		return ERROR_INVALID_PARAMETER;

	step = EXCHNG_BUF_SIZE - sizeof( TS_COM_PROPS );
	lptscp = (LPTS_COM_PROPS)szTmpBuffer;	
	lpPtr = szTmpBuffer + sizeof( TS_COM_PROPS );

	CopyMemory( (PVOID)szTmpBuffer, (CONST VOID *)lptsComProps, sizeof(  TS_COM_PROPS ) );
	lptscp->leftPacket = '\x1';
	lptscp->endPacket = '\x1';
	lptscp->lp = dwcbBuffer;
	lptscp->len = step;

	for( dwOffset = 0; dwOffset+step < dwcbBuffer; dwOffset += step ) {
		CopyMemory( (PVOID)lpPtr, (CONST VOID *)((LPSTR)lpBuffer+dwOffset), step );
		dwRetCode = SrvWriteExchngBuf( lpExchBufInfo, szTmpBuffer, EXCHNG_BUF_SIZE );
		if( dwRetCode != 0 )
			break;
	}

	if( dwRetCode == 0 ) {
		lptscp->leftPacket = '\x0';
		if( bIfEnd )
			lptscp->endPacket = '\x0';

		lptscp->len = (int)(dwcbBuffer - dwOffset);

		CopyMemory( (PVOID)lpPtr, (CONST VOID *)((LPSTR)lpBuffer+dwOffset), lptscp->len );
		dwRetCode = SrvWriteExchngBuf( lpExchBufInfo, szTmpBuffer, EXCHNG_BUF_SIZE );
	}

	return dwRetCode;
}

// Add by Niu Jingyu, 1998.07.07
DWORD SrvReadExchngBufEx ( LPEXCHNG_BUF_INFO lpExchBufInfo, 
						  LPSTR *plpBuffer, 
						  LPDWORD lpdwcbBuffer,
						  BOOL *pbIfEnd )
{
	CHAR szTmpBuffer[EXCHNG_BUF_SIZE];
	HANDLE hDataBuf;
	LPSTR lpDataBuf;
	LPTS_COM_PROPS lptscp;
	LPSTR lpPtr;
	DWORD dwcbData;
	DWORD dwOffset;
	DWORD dwRetCode;

	if( lpExchBufInfo == NULL || plpBuffer == NULL || 
			lpdwcbBuffer == NULL )
		return ERROR_INVALID_PARAMETER;

	*plpBuffer = NULL;
	*lpdwcbBuffer = 0;
	*pbIfEnd = TRUE;

	lptscp = (LPTS_COM_PROPS)szTmpBuffer;	
	lpPtr = szTmpBuffer + sizeof( TS_COM_PROPS );

	dwRetCode = SrvReadExchngBuf( lpExchBufInfo, szTmpBuffer, EXCHNG_BUF_SIZE );
	if( dwRetCode != 0 )
		return dwRetCode;

	if( lptscp->len > lptscp->lp ) {
		SrvSetExchBufError( lpExchBufInfo );
		return TERR_DATA_TOO_LONG;
	}

	dwcbData = lptscp->lp;
	hDataBuf = LocalAlloc( LPTR, lptscp->lp+8 );
	if( hDataBuf == NULL ) {
		SrvSetExchBufError( lpExchBufInfo );
		return GetLastError();
	}

	lpDataBuf = LocalLock( hDataBuf );
	if( lpDataBuf == NULL ) {
		SrvSetExchBufError( lpExchBufInfo );
		LocalFree( hDataBuf );
		return GetLastError();
	}

	CopyMemory( (PVOID)lpDataBuf, (CONST VOID *)lpPtr, lptscp->len );
	dwOffset = lptscp->len;

	while( lptscp->leftPacket != '\x0' ) {
		dwRetCode = SrvReadExchngBuf( lpExchBufInfo, szTmpBuffer, EXCHNG_BUF_SIZE );
		if( dwRetCode != 0 ) {
			LocalUnlock( hDataBuf ); 
			LocalFree( hDataBuf );
			return dwRetCode;
		}

		if( dwOffset + lptscp->len > dwcbData ) {
			SrvSetExchBufError( lpExchBufInfo );
			LocalUnlock( hDataBuf ); 
			LocalFree( hDataBuf );
			return TERR_DATA_TOO_LONG;
		}

		CopyMemory( (PVOID)(lpDataBuf+dwOffset), (CONST VOID *)lpPtr, lptscp->len );
		dwOffset += lptscp->len;
	}

	if( lptscp->endPacket != '\x0' )
		*pbIfEnd = FALSE;

	*plpBuffer = lpDataBuf;
	*lpdwcbBuffer = dwcbData;

	return 0;
}

// Add by Niu Jingyu, 1998.03.13
VOID SrvSetExchBufError ( LPEXCHNG_BUF_INFO lpExchBufInfo )
{
	lpExchBufInfo->cErrorStat = ERROR_SRV_CLOSE;
	SetEvent( lpExchBufInfo->hEmpEvent );
	// lprintfs( "->SrvExchangeBuffer Set Error SetEvent OK.\n" );

	PostThreadMessage( lpExchBufInfo->dwCliThreadId, TS_EXCHBUF_WRRD, 0, 0 );
	// lprintfs( "->SvrExchangeBuffer SetError Post Client agent Msg OK.\n" );
}

// Add by Niu Jingyu, 1998.03.13
VOID CliSetExchBufError ( LPEXCHNG_BUF_INFO lpExchBufInfo )
{
	lpExchBufInfo->cErrorStat = ERROR_CLI_CLOSE;
	SetEvent( lpExchBufInfo->hEmpEvent );
	// lprintfs( "->CliExchangeBuffer Set Error SetEvent OK.\n" );

	PostThreadMessage( lpExchBufInfo->dwSrvThreadId, TS_EXCHBUF_WRRD, 0, 0 );
	// lprintfs( "->CliExchangeBuffer SetError Post Service Msg OK.\n" );
}