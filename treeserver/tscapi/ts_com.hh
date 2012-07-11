/******************************
*	Module Name:
*		TS_COM.H
*
*	Abstract:
*		Contains defines of the TreeServer package information and
*	command const value.
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1997  China Railway Software Corporation
*
*	Revision History:
*		Write by NiuJingyu, 1997.10
********************************************************************************/

#ifndef _INC_TS_COM_
#define _INC_TS_COM_

#ifdef  _MSC_VER
/*
 * Currently, all MS C compilers for application 
 * default to 1 byte alignment.
 */
#pragma pack(push,1)
#endif  /* _MSC_VER */

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct tagTS_COM_PROPS {
    char  packetType;	  // '$'
    char  msgType;
    short len;		  //this packet length
    long  lp;
	char leftPacket;
	char endPacket;
} TS_COM_PROPS, *PTS_COM_PROPS, *LPTS_COM_PROPS;

//###################################################
#define pkTS_SYNC_PIPE					'$'
	#define msgTS_CLIENT_LOGON			'L'
	#define msgTS_CLIENT_LOGOFF			'E'
	#define msgTS_LOGON_OK				'O'
	#define msgTS_LOGON_ERROR			'X'
	#define msgTS_PIPE_CLOSE			'C'
	#define msgTS_NO_DATA				'N'
	
//###################################################
#define pkTS_ASQL						'Q'
	#define msgFAST_ASQL				'Q'
		#define cmTS_OPEN_DBF			0x2000
		#define cmTS_CLOSE_DBF          0x2001
		#define cmTS_REC_FETCH          0x2002
		#define cmTS_REC_PUT            0x2003
		#define cmTS_REC_ADD			0x2004
		#define cmTS_REC_APP            0x2005
		#define cmTS_REC_DEL            0x2006
		#define cmTS_BLOB_FETCH         0x2007
		#define cmTS_BLOB_PUT           0x2008
		#define cmTS_DBF_RECNUM         0x2009
		#define cmTS_LOCK_DBF			0x2010
		#define cmTS_UNLOCK_DBF			0x2011
		#define cmTS_FIELD_INFO			0x2012
		#define cmTS_BLOB_MEM_FETCH		0x2013
		#define cmTS_BLOB_MEM_PUT		0x2014
	#define msgASQL_TYPE_A				'A'
	#define msgASQL_TYPE_B				'B'
	#define msgASQL_TYPE_C				'C'
	#define msgASQL_TYPE_E				'E'  // Require the user defined symbol table 
	#define msgASQL_TEST				'T' 

//###################################################
#define pkTS_PIPE_FTP					'F'
	#define msgTS_FTP_FILE				'F'
		#define cmFTP_FILE_OK			0
		#define cmFTP_FILE_ERROR		0xFFFFFFFF
		#define cmFTP_FILE_PUT			0x3001
		#define cmFTP_FILE_GET			0x3002
		#define cmFTP_FILE_DATA			0x3003
		#define cmFTP_FILE_END			0x3004

//###################################################
#define pkTS_USER						'U'
	#define msgTS_USER_REPLAY			'R'
	#define msgTS_USER					'U'
		#define cmTS_USER_CHNG_PWD		0x4000
		#define cmTS_USER_NET_PATH		0x4013
	#define msgTS_USER_MANAGE			'M'
		#define cmTS_USER_ADD			0x4001
		#define cmTS_USER_DEL			0x4002
		#define cmTS_USER_INFO			0x4003
		#define cmTS_USER_MODI			0x4004
		#define cmTS_USER_ENUM			0x4005
		#define cmTS_USER_LOCK			0x4006
		#define cmTS_USER_UNLOCK		0x4007
		#define cmTS_USER_CLEAR_LF		0x4008
		#define cmTS_USER_RENAME		0x4009
		#define cmTS_USER_RD_TEMPLATE	0x4010
		#define cmTS_USER_WR_TEMPLATE	0x4011
		#define cmTS_USER_DEFAULT_HOME  0x4012
	#define msgTS_REGISTERED_USER		'Q'
	#define msgTS_IDENTICAL_USER_NUM	'N'
#ifndef __INC_TUSER

#define INFO_USER_ID		1
#define INFO_PASSWD			2
#define INFO_HOME_DIR		3
#define INFO_LOGON_COMPUTER	4
#define INFO_LOGON_HOURS	5
#define INFO_ACCESS_MASK	6
#define INFO_DESCRIPTION	7
#define INFO_LOGON_FAIL		8
#define INFO_LOCKED			9

#endif // __INC_TUSER

#define pkTS_SPROC						'P'
	#define msgTS_SPROC					'P'
		#define cmTS_NEW_PROC			0x5000
		#define cmTS_DELETE_PROC		0x5001
		#define cmTS_GET_PROC			0x5002
		#define cmTS_UPDATE_PROC		0x5003
		#define cmTS_EXECUTE_PROC		0x5004
		#define cmTS_PROC_GRANT			0x5005
		#define cmTS_PROC_REVOKE		0x5006

//###################################################
#define pkTS_TEST						'T'

//###################################################
#define pkTS_MONITOR					'M'

//###################################################
#define pkTS_INFO						'I'

//###################################################
#define pkTS_ERROR						'E'
	#define msgTS_ERROR_MSG				'E'

#ifdef  __cplusplus
}
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */
 
#endif // _INC_TS_COM_