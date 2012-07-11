/******************************
*	Module Name:
*		EXCHBUFF.H
*
*	Abstract:
*		Tree Server exchange buffer management function declaration. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1997  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1997.10
*		Update by NiuJingyu, 1998.02
*		Update by NiuJingyu, 1998.03
********************************************************************************/

#ifndef _INC_EXCHBUFF_
#define _INC_EXCHBUFF_

#include "ts_com.h"

#pragma pack(push,1)

#ifdef  __cplusplus
extern "C" {
#endif

#define TS_EXCHBUF_WRRD		0x7001
#define TS_SERVICE_START	0x7002

#define ERROR_OK			'\x0'
#define ERROR_SRV_CLOSE		'\x1'
#define ERROR_CLI_CLOSE		'\x2'

typedef struct tagExchngBufBufInfo {
	LPVOID lpExchBuf;
	HANDLE hEmpEvent;
	DWORD dwCliThreadId;
	DWORD dwSrvThreadId;
	CHAR cErrorStat;	// Add by Niu Jingyu, 1998.03.13
	CHAR cFlag;
} EXCHNG_BUF_INFO, *PEXCHNG_BUF_INFO, *LPEXCHNG_BUF_INFO; 

VOID InitExchngBufInfo ( 
	LPEXCHNG_BUF_INFO lpBufInfo, 
	LPVOID lpBuffer, 
	HANDLE hEvent );

LPEXCHNG_BUF_INFO InitExchngBuf ( 
	DWORD dwSize, 
	LPDWORD lpdwRetCode );

DWORD ReleaseExchngBuf (
	VOID );

LPEXCHNG_BUF_INFO AllocExchngBuf (
	VOID );

VOID FreeExchngBuf ( 
	LPEXCHNG_BUF_INFO lpExchBufInfo );

DWORD CliWriteExchngBuf ( 
	LPEXCHNG_BUF_INFO lpExchBufInfo, 
	LPVOID lpBuffer, DWORD dwcbBuffer );

DWORD CliReadExchngBuf ( 
	LPEXCHNG_BUF_INFO lpExchBufInfo, 
	LPVOID lpBuffer, 
	DWORD dwcbBuffer );

DWORD SrvWriteExchngBuf ( 
	LPEXCHNG_BUF_INFO lpExchBufInfo,
	LPVOID lpBuffer,
	DWORD dwcbBuffer );

DWORD SrvReadExchngBuf ( 
	LPEXCHNG_BUF_INFO lpExchBufInfo, 
	LPVOID lpBuffer, 
	DWORD dwcbBuffer );

// Add by Niu Jingyu, 1998.07.07
DWORD SrvReadExchngBufEx ( 
	LPEXCHNG_BUF_INFO lpExchBufInfo, 
	LPSTR *plpBuffer, 
	LPDWORD lpdwcbBuffer,
	BOOL *pbIfEnd );

// Add by Niu Jingyu, 1998.07.07
DWORD SrvWriteExchngBufEx ( 
	LPEXCHNG_BUF_INFO lpExchBufInfo, 
	LPTS_COM_PROPS lptsComProps, 
	LPVOID lpBuffer, 
	DWORD dwcbBuffer,
	BOOL bIfEnd );

// Add by Niu Jingyu, 1998.03.13
VOID SrvSetExchBufError ( 
	LPEXCHNG_BUF_INFO lpExchBufInfo );

// Add by Niu Jingyu, 1998.03.13
VOID CliSetExchBufError ( 
	LPEXCHNG_BUF_INFO lpExchBufInfo );

#ifdef  __cplusplus
}
#endif

#pragma pack(pop)

#endif // _INC_EXCHBUFF_
