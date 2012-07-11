/******************************
*	Module Name:
*		TSFTP.H
*
*	Abstract:
*		Tree Server FTP server function declaretions. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1998  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1998.03
********************************************************************************/

#ifndef _INC_TSFTP_
#define _INC_TSFTP_

#pragma pack(push,1)

#ifdef  __cplusplus
extern "C" {
#endif

DWORD RespFtpPut ( 
	LPSTR lpFileName, 
	LPEXCHNG_BUF_INFO lpExchBuf );

DWORD RespFtpGet ( 
	LPSTR lpFileName,
	LPEXCHNG_BUF_INFO lpExchBuf );

#ifdef  __cplusplus
}
#endif

#pragma pack(pop)

#endif // _INC_TSFTP_