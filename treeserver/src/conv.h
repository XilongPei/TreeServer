/******************************
*	Module Name:
*		CONV.H
*
*	Abstract:
*		Tree Server User Conversation Interface function declaration. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1997  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1997.10
*		Update by NiuJingyu, 1998.02
********************************************************************************/

#ifndef _INC_CONVERSATION_
#define _INC_CONVERSATION_

#include <windows.h>
#include "tqueue.h"

#pragma pack(push,1)

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct 
{
	HANDLE	hReadPipe;
	HANDLE	hWritePipe;
} PIPE_STRU;

DWORD InitConvInterface (
	VOID );

DWORD WINAPI PipeSelectConnectThread ( 
	LPVOID lpvParam );

DWORD WINAPI TcpipSelectConnectThread (
	LPVOID lpvParam );

DWORD WINAPI ClientAgentThread ( 
	LPVOID lpvParam );

DWORD WINAPI TcpipClientAgentThread (
	LPVOID lpvParam );

DWORD MsgToRequest ( 
	LPCSTR lpMessage,
	LPREQUEST lpRequest );

#ifdef  __cplusplus
}
#endif

#pragma pack(pop)

#endif // _INC_CONVERSATION_
