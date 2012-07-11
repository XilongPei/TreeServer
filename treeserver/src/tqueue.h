/******************************
*	Module Name:
*		TQUEUE.H
*
*	Abstract:
*		Contains defines of the queue management data structures
*	and function declarations which are commonly used in Tree 
*	Server kernel.
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1997  China Railway Software Corporation
*
*	Revision History:
*		Write by NiuJingyu, 1997.10
*		Rewrite by NiuJingyu, 1998.01
********************************************************************************/

#ifndef _INC_TQUEUE_
#define _INC_TQUEUE_

#include <windows.h>

#include "tlimits.h"
#include "exchbuff.h"

#pragma pack(push,1)

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct {
	CHAR szUser[MAX_USER+1];				// [IN] User's logon name.
	CHAR szPasswd[MAX_PASSWD+1];			// [IN] User's logon password.
	CHAR szComputer[MAX_COMPUTER+1];		// [IN] Client computer name.
	DWORD dwRequestId;						// [IN][RESERVED] Indentfy the current request, use the threadID.
											// [OUT] IF Succrss, return the user's token handle.
	DWORD dwRequest;						// [IN][RESERVED].	
											// [OUT] Error code.
	LPEXCHNG_BUF_INFO lpExchangeBuf;		// [OUT] If success	it point to the exchange buffer information, else is NULL.
	SYSTEMTIME Time;						// [RESERVED] Current system time.
} REQUEST, *PREQUEST, *LPREQUEST;

typedef struct {
	HANDLE hQueueMutex;						// Snchronize the read/write of the queue.
	HANDLE hResSemaphore;					// Resource count of the queue.
	HANDLE hUserSemaphore;					// User count of the queue.
	LPHANDLE lphEvent;						// Snchronization object(auto-reset event) handle array.
	DWORD dwTotal;							// Total resource of the queue.
	DWORD dwPointerIn;						// Queue tail(Write pointer).
	DWORD dwPointerOut;						// Queue header(read pointer).
	LPREQUEST lpQueue;						// Queue pointer.
} TQUEUE, *PTQUEUE, *LPQUEUE;

//////////////////////////////////////////////////////////////////////////////

LPQUEUE InitQueue ( 
	DWORD dwSize,
	LPDWORD lpdwRetCode );

DWORD ReleaseQueue ( VOID ); 

DWORD SendRequest ( 
	LPREQUEST lpRequest );

DWORD ProcessRequest (
	LPREQUEST lpRequest );

#ifdef  __cplusplus
}
#endif

#pragma pack(pop)

#endif // _INC_TQUEUE_