/******************************
*	Module Name:
*		SCHEDULE.H
*
*	Abstract:
*		Tree Server transation schedule function declaretions. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1997  China Railway Software Corporation.
*
*	Revision History:
*		Write by NiuJingyu, 1997.10
*		Update by NiuJingyu, 1998.03
********************************************************************************/

#ifndef _INC_SCHEDULE_
#define _INC_SCHEDULE_

#include <windows.h>

#pragma pack(push,1)

#ifdef  __cplusplus
extern "C" {
#endif

DWORD Schedule ( 
	LPVOID lpvParam );

DWORD InitSchedule (
	VOID );

INT GetAvailableSpace ( 
	VOID );

DWORD FreeSpace ( 
	LPREQUEST lpRequest );

DWORD TSRegisterUser (
	LPREQUEST lpRequest );

DWORD TSUnregisterUser (
	HANDLE hUserToken );

#ifdef  __cplusplus
}
#endif

#pragma pack(pop)

#endif // _INC_SCHEDULE_

