/****************
* ^-_-^
*                               TS_CONSTS.c
*    Copyright (c) Xilong Pei, 1998
*****************************************************************************/
#ifndef __TS_CONSTS_H_
#define __TS_CONSTS_H_

#include "ts_com.h"

#define WAIT_FOR_CALLTHREAD	3600000
//#define MAX_PKG_MSG_LEN		(4096-sizeof(TS_COM_PROPS))
#define MAX_PKG_MSG_LEN		(4096-10)

#define ASQL_STARTTASK_MSG	0x1001
#define ASQL_ENDTASK_MSG	0x1002
#define ASQL_TRAN_MSG		0x1003


/*


typedef struct tagTS_COM_PROPS {
    char  packetType;	  //'Q' 'R'
    char  msgType;
    short len;		  //this packet length
    long  lp;
    char  leftPacket;     //0, not; others not end;
    char  endPacket;      //rights given up: 0:give up
} TS_COM_PROPS;

#define cmTS_OPEN_DBF		0x2000
#define cmTS_CLOSE_DBF          0x2001
#define cmTS_REC_FETCH          0x2002
#define cmTS_REC_PUT            0x2003
#define cmTS_REC_ADD		0x2004
#define cmTS_REC_APP            0x2005
#define cmTS_REC_DEL            0x2006
#define cmTS_BLOB_FETCH         0x2007
#define cmTS_BLOB_PUT           0x2008
#define cmTS_DBF_RECNUM         0x2009
*/

#endif