/****************
* ts_dict.c
* 1998.3.5 Xilong Pei
*
*****************************************************************************/

#define __TS_DICT_C_

#include "cfg_fast.h"
#include "windows.h"
#include "ts_dict.h"
#include "strutl.h"
#include "mistring.h"
#include "memutl.h"
#include "ts_const.h"
#include "exchbuff.h"
#include "dio.h"

PCFG_STRU csuDataDictionary = NULL;
void      *asqlAudit= NULL;

/* asqlGetDData()
 *
 *************************************************************************/
_declspec(dllexport) long asqlGetDData(unsigned char *szGroup, \
				       unsigned char *szLabel, \
				       unsigned char *szKey,   \
				       void *exbuf)
{
    char *sz;
    TS_COM_PROPS tscp;
    char  lpszResponse[4096];

    memset(&tscp, 0, sizeof(TS_COM_PROPS));
    sz = GetCfgKey(csuDataDictionary, szGroup, szLabel, szKey);
    if( sz == NULL )
	sz = "";

    tscp.packetType = 'R';
    tscp.msgType = 'D';
    tscp.len = min(strlen(sz)+1, 4096-sizeof(TS_COM_PROPS));
    tscp.lp = 0;

    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
    strZcpy(&lpszResponse[sizeof(TS_COM_PROPS)], sz, tscp.len);
    SrvWriteExchngBuf((EXCHNG_BUF_INFO  *)exbuf, lpszResponse, sizeof(TS_COM_PROPS)+tscp.len);

    return  0;

} //end of asqlGetDData()


/* asqlPutDData()
 *
 *************************************************************************/
_declspec(dllexport) long asqlPutDData(unsigned char *szGroup, \
				       unsigned char *szLabel, \
				       unsigned char *szKey,   \
				       unsigned char *keyCont, \
				       void *exbuf)
{
    TS_COM_PROPS tscp;
    char  lpszResponse[4096];

    memset(&tscp, 0, sizeof(TS_COM_PROPS));
    tscp.lp = WriteCfgKey(csuDataDictionary, szGroup,                  \
					     szLabel,                  \
					     szKey,                    \
					     keyCont);
    tscp.packetType = 'R';
    tscp.msgType = 'D';
    tscp.len = 0;

    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
    SrvWriteExchngBuf((EXCHNG_BUF_INFO *)exbuf, lpszResponse, sizeof(TS_COM_PROPS));

    return  0;

} //end of asqlPutDData()



/* asqlEnumDData()
 *
 *************************************************************************/
_declspec(dllexport) long asqlEnumDData(unsigned char *szGroup, \
				       unsigned char *szLabel, \
				       unsigned char *szKey,   \
				       void *exbuf)
{
    TS_COM_PROPS tscp;
    char         lpszResponse[4096];
    int		 i;

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    i = 4096-sizeof(TS_COM_PROPS);
	if( csuDataDictionary == NULL ) {
		i = 0;
	} else {
		enumItemInCache(csuDataDictionary, szGroup, szLabel, szKey,
				     &lpszResponse[sizeof(TS_COM_PROPS)], \
				     &i);
	} 

    tscp.packetType = 'R';
    tscp.msgType = 'D';
    tscp.len = i;
    tscp.lp = 0;

    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
    SrvWriteExchngBuf((EXCHNG_BUF_INFO  *)exbuf, lpszResponse, sizeof(TS_COM_PROPS)+tscp.len);

    return  0;

} //end of asqlEnumDData()


int dictDupFile( void )
{
    char buf1[260], buf2[260];
    char buf3[260], buf4[260];
    char buf5[260], buf6[260];

	if( csuDataDictionary == NULL )
		return  1;

    strcpy(buf1, csuDataDictionary->szFileName);
    strcpy(buf2, csuDataDictionary->szFileName);
    strcat(buf1, "CONFIG\\ASQLWMT.CFG");
    strcat(buf2, "CONFIG\\ASQLWMT.BAK");

    strcpy(buf3, csuDataDictionary->szFileName);
    strcpy(buf4, csuDataDictionary->szFileName);
    strcat(buf3, "CONFIG\\FIELDCTL.DBF");
    strcat(buf4, "CONFIG\\FIELDBAK.DBF");

    strcpy(buf5, csuDataDictionary->szFileName);
    strcpy(buf6, csuDataDictionary->szFileName);
    strcat(buf5, "CONFIG\\FIELDCTL.DBT");
    strcat(buf6, "CONFIG\\FIELDBAK.DBT");

    EnterCriticalSection( &(csuDataDictionary->dCriticalSection) );
    dflush( (dFILE *)(csuDataDictionary->df) );
    CopyFile(buf1, buf2, FALSE);
    CopyFile(buf3, buf4, FALSE);
    CopyFile(buf5, buf6, FALSE);
    LeaveCriticalSection( &(csuDataDictionary->dCriticalSection) );

    return  0;

} //end of dictDupFile()


//
int dictReopenCfgFile( void )
{
    char buf1[260], buf2[260];
    char buf3[260], buf4[260];
    char buf5[260], buf6[260];
    PCFG_STRU csu;

	if( csuDataDictionary == NULL )
		return  1;

    strcpy(buf1, csuDataDictionary->szFileName);
    strcpy(buf2, csuDataDictionary->szFileName);
    strcat(buf1, "CONFIG\\ASQLWMT.CFG");
    strcat(buf2, "CONFIG\\ASQLWMT.BAK");

    strcpy(buf3, csuDataDictionary->szFileName);
    strcpy(buf4, csuDataDictionary->szFileName);
    strcat(buf3, "CONFIG\\FIELDCTL.DBF");
    strcat(buf4, "CONFIG\\FIELDBAK.DBF");

    strcpy(buf5, csuDataDictionary->szFileName);
    strcpy(buf6, csuDataDictionary->szFileName);
    strcat(buf5, "CONFIG\\FIELDCTL.DBT");
    strcat(buf6, "CONFIG\\FIELDBAK.DBT");

    EnterCriticalSection( &(csuDataDictionary->dCriticalSection) );
    csu = csuDataDictionary;
    csuDataDictionary = NULL;
    LeaveCriticalSection( &(csu->dCriticalSection) );
    CloseCfgFile( csu );

    CopyFile(buf2, buf1, FALSE);
    CopyFile(buf4, buf3, FALSE);
    CopyFile(buf6, buf5, FALSE);

    //if now there is someone is waiting for the csuDataDictionary,
    //there will be error

    csuDataDictionary = OpenCfgFile( buf1 );

    return  0;

} //end of dictReopenCfgFile()



//reset the dictionary of ASQL server
int dictResetCfgFile( void )
{
    PCFG_STRU	csu;
	char		buf1[260];
	
	if( csuDataDictionary == NULL )
		return  1;

	strcpy(buf1, csuDataDictionary->szFileName);
    strcat(buf1, "CONFIG\\ASQLWMT.CFG");

    EnterCriticalSection( &(csuDataDictionary->dCriticalSection) );
    csu = csuDataDictionary;
    csuDataDictionary = NULL;
    LeaveCriticalSection( &(csu->dCriticalSection) );
    CloseCfgFile( csu );

    //if now there is someone is waiting for the csuDataDictionary,
    //there will be error

    csuDataDictionary = OpenCfgFile( buf1 );

    return  0;

} //end of dictReopenCfgFile()

