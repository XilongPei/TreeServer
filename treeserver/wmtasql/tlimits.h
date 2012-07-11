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
*	Copyright (c) 1997  China Railway Software Corporation
*
*	Revision History:
*		Write by NiuJingyu, 1997.10
********************************************************************************/

#ifndef _INC_TLIMITS_
#define _INC_TLIMITS_

#define MAX_SERVICE_THREAD	10

#define MAX_TED_CAPABILITY	3
#define MAX_TED_ALIAS		16
#define MAX_TED_COMMAND		16
#define MAX_TED_FUNCTION	64

#define MAX_USER			16
#define MAX_PASSWD			16
#define MAX_COMPUTER		16
#define MAX_COMMAND			MAX_TED_COMMAND
#define MAX_REQUEST_PARAM	512
#define MAX_RESPONSE_STRING	512

#define MIN_QUEUE	10
#define MAX_QUEUE	50

#define MAX_QUEUE_WAIT	1000

#define EXCHNG_BUF_SIZE		4096
#define PIPE_BUFFER_SIZE	EXCHNG_BUF_SIZE
#define PIPE_TIMEOUT		5000

#endif // _INC_TLIMITS_