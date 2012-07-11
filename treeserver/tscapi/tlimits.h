/******************************
*	Module Name:
*		TLIMITS.H
*
*	Abstract:
*		Contains defines for a number of implementation dependent values
*       which are commonly used in Tree Server.
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1997  China Railway Software Corporation
*
*	Revision History:
*		Write by NiuJingyu, 1997.10
*		Update by NiuJingyu, 1998.03
********************************************************************************/

#ifndef _INC_TLIMITS_
#define _INC_TLIMITS_

#include <lmcons.h>

#include "ts_com.h"

#pragma pack(push,1)

#ifdef  __cplusplus
extern "C" {
#endif

#define MAX_SERVICE_THREAD	10

#define MAX_TED_CAPABILITY	3
#define MAX_TED_ALIAS		16
#define MAX_TED_COMMAND		16
#define MAX_TED_FUNCTION	64

#define NODE_SIZE			1024
#define MAX_USER_NAME		16
#define MAX_USER			16
#define MAX_PASSWD			16
#define MAX_COMPUTER		15
#define MAX_ACCESS_MASK		16
#define MAX_LOGON_HOURS		21
#define MAX_LOGON_COMPUTER	(MAX_COMPUTER+1)*20
#define MAX_DESCRIPTION		128

#define MAX_COMMAND			MAX_TED_COMMAND
#define MAX_REQUEST_PARAM	512
#define MAX_RESPONSE_STRING	512

#define MIN_QUEUE	10
#define MAX_QUEUE	50

#define MAX_QUEUE_WAIT	1000

#define EXCHNG_BUF_SIZE			4096
#define PIPE_BUFFER_SIZE		EXCHNG_BUF_SIZE
#define EXCHANGE_CONTENT_SIZE	(EXCHNG_BUF_SIZE-sizeof(TS_COM_PROPS))
#define PIPE_TIMEOUT		5000

#define MAX_PROC_NAME		32
#define MAX_PRIVILEGE_NAME	MAX_PROC_NAME+4
#define PROC_NODE_SIZE		1024

#ifdef  __cplusplus
}
#endif

#pragma pack(pop)

#endif // _INC_TLIMITS_