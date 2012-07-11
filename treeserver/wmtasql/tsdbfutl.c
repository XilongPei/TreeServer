/****************
* ^-_-^
*                               tsdbfutl.c
*    Copyright (c) Xilong Pei, 1998
*
*
*  NOTICE:
*        in other place, users use put_fld() or PutField() to change rec_buf
*    but this module is changed by userself, so this is needed:
*    	 absSyncDfBh( (dFILE *)df );
*
*****************************************************************************/


#include <windows.h>  
#include <limits.h>   
#include <io.h>
#include <process.h>

#define  __TSDBFUTL_C_    "MAIN"

#include "dir.h"
#include "dio.h"
#include "mistring.h"
#include "strutl.h"
#include "tsdbfutl.h"
#include "ts_const.h"
#include "mistools.h"
#include "diodbt.h"
#include "filetool.h"
#include "taccount.h"
#include "ts_dict.h"
#include "cfg_fast.h"
#include "asqlana.h"
#include "dbtree.h"


TS_USER_MAN tsumDefault;


/* asqlGetDbfDes()
 * error: return: tscc
 *************************************************************************/
_declspec(dllexport) TS_CLIENT_CURSOR *asqlGetDbfDes(TS_CLIENT_CURSOR *tscc, LPSTR szDbName, LPSTR szTbName, \
					EXCHNG_BUF_INFO *exbuf)
{
    char 	buf[MAXPATH];
    dFILE 	*df;
    TS_CLIENT_CURSOR *tscc1;
    TS_COM_PROPS tscp;
    char  	lpszResponse[4096];

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    if( szDbName == NULL )
	szDbName = "";

    if( szDbName[0] != '\0' ) {
	char *sz;
	sz = GetCfgKey(csuDataDictionary, "DATABASE", szDbName, "PATH");
	if( sz != NULL ) {
	    strZcpy(buf, sz, MAXPATH);
	    beSurePath(buf);
	} else buf[0] = '\0';
    } else {
	strcpy(buf, asqlConfigEnv.szAsqlFromPath);
    }

    if( szTbName[1] == ':' || szTbName[0] == '\\' )
	strcpy(buf, szTbName);
    else
	strncat(buf, szTbName, MAXPATH-strlen(buf));

    dsetbuf(0);
    if( szDbName[0] != '\0' ) {
	strcpy(lpszResponse, szDbName);
	strcat(lpszResponse, "*");
	strcat(lpszResponse, buf);
    } else {
	strcpy(lpszResponse, buf);
    }

    df = dAwake(lpszResponse, DOPENPARA);
    if( df == NULL ) {
	tscp.packetType = 'R';
	tscp.msgType = 'E';
	tscp.len = 0;
	tscp.lp = -1;
	memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));

	SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));
	return  tscc;
    }

    //get the right string
    //check

    wmtDbfLock(df);

    tscp.packetType = 'R';
    tscp.msgType = 'D';
    tscp.len = (short)(min(df->headlen,4096-sizeof(TS_COM_PROPS)));
    tscp.lp = (long)df;
    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));

    lseek(df->fp, 0, SEEK_SET);
    read(df->fp, lpszResponse+sizeof(TS_COM_PROPS), tscp.len);

    //set recnum to 0
    *(long *)&lpszResponse[sizeof(TS_COM_PROPS)+4] = 0;
    //clear firstDelRec
    *(long *)&lpszResponse[sizeof(TS_COM_PROPS)+12] = 0;

    wmtDbfUnLock(df);
    tscc1 = malloc(sizeof(TS_CLIENT_CURSOR));
    if( tscc1 == NULL ) {
	dSleep(df);
	tscp.msgType = 'E';
	SrvWriteExchngBuf(exbuf, lpszResponse, 4096);
	return  tscc;
    }
    tscc1->pNext = tscc;
    tscc1->p = df;
    tscc1->pType = 1;

    SrvWriteExchngBuf(exbuf, lpszResponse, 4096);

    return  tscc1;

} //end of asqlGetDbfDes()


/* asqlGetDbfRec()
 *
 *************************************************************************/
_declspec(dllexport) long asqlGetDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					long  recNo, \
					EXCHNG_BUF_INFO *exbuf )
{
    RECPROPS recprops;
    TS_COM_PROPS tscp;
    char  lpszResponse[4096];

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
	    goto  agdr_dfTrue;
	tscc = tscc->pNext;
    }
    df = NULL;
agdr_dfTrue:

    if( df == NULL || recNo <= 0  ) {
	tscp.packetType = 'R';
	tscp.msgType = 'E';
	tscp.len = 0;
	tscp.lp = -1;
	memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));

	SrvWriteExchngBuf(exbuf, lpszResponse, 4096);
	return  -1;
    }

    //to consider the view  2000.6.24
    //((dFILE *)df)->rec_p = recNo;
    dseek((dFILE *)df, recNo-1, dSEEK_SET);
 
    if( get1rec((dFILE *)df) == NULL ) {
	tscp.packetType = 'R';
	tscp.msgType = 'E';
	tscp.len = 0;
	tscp.lp = -2;
	memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, lpszResponse, 4096);

	return  -2;
    }

    tscp.packetType = 'R';
    tscp.msgType = 'D';
    tscp.len = sizeof(RECPROPS)+((dFILE *)df)->rec_len;
    tscp.lp = 0;

    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
    recprops.iPhyRecNum = ((dFILE *)df)->rec_p;
    recprops.bDeleteFlag = ((dFILE *)df)->rec_buf[0] - ' ';
    memcpy(&lpszResponse[sizeof(TS_COM_PROPS)], &recprops, sizeof(RECPROPS));

    memcpy(&lpszResponse[sizeof(TS_COM_PROPS)+sizeof(RECPROPS)], ((dFILE *)df)->rec_buf, ((dFILE *)df)->rec_len);
    SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS)+tscp.len);

    return  0;

} //end of asqlGetDbfRec()

/* asqlPutDbfRec()
 *
 *************************************************************************/
_declspec(dllexport) long asqlPutDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					long  recNo, \
					LPSTR recBuf, \
					EXCHNG_BUF_INFO *exbuf )
{
    TS_COM_PROPS tscp;
    char  lpszResponse[4096];

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
	    goto  agdr_dfTrueR;
	tscc = tscc->pNext;
    }
    df = NULL;
agdr_dfTrueR:

    tscp.packetType = 'R';
    tscp.msgType = 'E';
    tscp.len = 0;

    if( df == NULL || recNo <= 0 ) {
	tscp.lp = -1;
	memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, lpszResponse, 4096);

	return  -1;
    }

    memcpy(((dFILE *)df)->rec_buf, recBuf, ((dFILE *)df)->rec_len);

    //((dFILE *)df)->rec_p = recNo;
    dseek((dFILE *)df, recNo-1, dSEEK_SET);

    //1999.12.21
    absSyncDfBh( (dFILE *)df );

    if( put1rec( (dFILE *)df ) == NULL ) {
	//error
	tscp.lp = ((dFILE *)df)->error;
    } else {
	tscp.msgType = 'D';
	tscp.lp = 0;
    }
    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
    SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

    return  0;

} //end of asqlPutDbfRec()



/* asqlFreeDbfDes()
 *
 *************************************************************************/
_declspec(dllexport)  TS_CLIENT_CURSOR *asqlFreeDbfDes(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf )
{
    TS_COM_PROPS tscp;
    TS_CLIENT_CURSOR *tscpLast, *tscpHead;
    char  lpszResponse[4096];

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    tscpLast = NULL;
    tscpHead = tscc;
    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
		    goto  agdr_dfTrueD;
	tscpLast = tscc;
	tscc = tscc->pNext;
    }
    df = NULL;
agdr_dfTrueD:

    if( df == NULL ) {
	tscp.packetType = 'R';
	tscp.msgType = 'E';
	tscp.len = 0;
	tscp.lp = -1;
	memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

	return  tscpHead;
    }

    if( wmtDbfIsLock((dFILE *)df) == 1 )
    { //if it is locked, unlock it.
	wmtDbfUnLock((dFILE *)df);
    }

    dSleep((dFILE *)df);
    if( tscpLast != NULL ) {
	tscpLast->pNext = tscc->pNext;
    } else {
	tscpHead = tscc->pNext;
    }
    free( tscc );

    tscp.packetType = 'R';
    tscp.msgType = 'D';
    tscp.len = 0;
    tscp.lp = 0;
    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
    SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

    return  tscpHead;

} //end of asqlFreeDbfDes()



/* asqlDelDbfRec()
 *
 *************************************************************************/
_declspec(dllexport) long asqlDelDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					long  recNo, \
					EXCHNG_BUF_INFO *exbuf )
{
    TS_COM_PROPS tscp;
    char  lpszResponse[4096];

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
	    goto  agdr_dfTrueR;
	tscc = tscc->pNext;
    }
    df = NULL;
agdr_dfTrueR:

    tscp.packetType = 'R';
    tscp.msgType = 'E';
    tscp.len = 0;

    if( df == NULL || recNo <= 0 ) {
	tscp.lp = -1;
	memcpy( lpszResponse, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

	return  -1;
    }

    ((dFILE *)df)->rec_p = recNo;
    if( PutDelChar(((dFILE *)df), '*') != 0 ) {
	//error
	tscp.lp = -4;
    } else {
	tscp.msgType = 'D';
	tscp.lp = 0;
    }
    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
    SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

    return  0;

} //end of asqlDelDbfRec()



/* asqlAppDbfRec()
 * append record
 *************************************************************************/
_declspec(dllexport) long asqlAppDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					LPSTR sRecBuf, \
					EXCHNG_BUF_INFO *exbuf )
{
    TS_COM_PROPS tscp;
    char  lpszResponse[4096];

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
            goto  agdr_dfTrueR;
	tscc = tscc->pNext;
    }
    df = NULL;
agdr_dfTrueR:

    tscp.packetType = 'R';
    tscp.msgType = 'E';
    tscp.len = 0;

    if( df == NULL ) {
	tscp.lp = -1;
	memcpy( lpszResponse, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

	return  -1;
    }

    memcpy(((dFILE *)df)->rec_buf, sRecBuf, ((dFILE *)df)->rec_len);

    //lock it to avoid other thread move to the end for appending
    wmtDbfLock((dFILE *)df);
    dseek((dFILE *)df, 0, dSEEK_END);

    //1999.12.21
    absSyncDfBh( (dFILE *)df );

    if( put1rec( (dFILE *)df ) == NULL ) {
	//error
	tscp.lp = -4;
    } else {
	tscp.msgType = 'D';
	tscp.lp = 0;
    }
    wmtDbfUnLock((dFILE *)df);

    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
    SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

    return  0;

} //end of asqlAppDbfRec()


/* asqlAddDbfRec()
 * append record
 *************************************************************************/
_declspec(dllexport) long asqlAddDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					LPSTR sRecBuf, \
					EXCHNG_BUF_INFO *exbuf )
{
    TS_COM_PROPS tscp;
    long         l;
    char  	 lpszResponse[4096];

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
	    goto  agdr_dfTrueR;
	tscc = tscc->pNext;
    }
	df = NULL;
agdr_dfTrueR:

    tscp.packetType = 'R';
    tscp.msgType = 'E';
    tscp.len = 0;

    if( df == NULL ) {
	tscp.lp = -1;
	memcpy( lpszResponse, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

	return  -1;
    }

    //memcpy(((dFILE *)df)->rec_buf, &lpszResponse[sizeof(TS_COM_PROPS)+sizeof(long)+sizeof(RECPROPS)], ((dFILE *)df)->rec_len);
    memcpy(((dFILE *)df)->rec_buf, sRecBuf, ((dFILE *)df)->rec_len);

    //lock it to avoid other thread move to the end for appending
    wmtDbfLock((dFILE *)df);
    if( (l=daddrec((dFILE *)df)) < 0 ) {
	//error
	tscp.lp = -4;
    } else {
	tscp.msgType = 'D';
	tscp.lp = l;
    }
    wmtDbfUnLock((dFILE *)df);

    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
    SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

    return  0;

} //end of asqlAddDbfRec()


/* asqlDbfDseek()
 * append record
 *************************************************************************/
_declspec(dllexport) long asqlDbfDseek(TS_CLIENT_CURSOR *tscc, void *df, \
					long  recNo, \
					char  cFromWhere, \
					EXCHNG_BUF_INFO *exbuf )
{
    TS_COM_PROPS tscp;
    char  lpszResponse[4096];

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
	    goto  agdr_dfTrueR;
	tscc = tscc->pNext;
    }
    df = NULL;
agdr_dfTrueR:

    tscp.packetType = 'R';
    tscp.msgType = 'E';
    tscp.len = 0;

    if( df == NULL ) {
	tscp.lp = -1;
	memcpy( lpszResponse, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

	return  -1;
    }

    switch( cFromWhere ) {
	case 'E':
		dseek((dFILE *)df, recNo, dSEEK_END);
		break;
	case 'C':
		dseek((dFILE *)df, recNo, dSEEK_CUR);
		break;
	//case 'S':
	default:
		dseek((dFILE *)df, recNo, dSEEK_SET);
		break;
    }
    tscp.msgType = 'D';
    tscp.lp = 0;

    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
    SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

    return  0;

} //end of asqlDbfDseek()


/* asqlBlobFetch()
 *
 *************************************************************************/
_declspec(dllexport) long asqlBlobFetch(TS_CLIENT_CURSOR *tscc, \
					LPCSTR lpszUser, \
					LPCSTR lpszUserDir,\
					void *df, \
					long recNo, \
					LPSTR fieldName, \
					EXCHNG_BUF_INFO *exbuf )
{
    TS_COM_PROPS tscp;
    unsigned short iw;
    char  	   buf[4096];
    char  	   szCn[128];
//    int   	   i;

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
	    goto  agdr_dfTrueR;
	tscc = tscc->pNext;
    }
	df = NULL;
agdr_dfTrueR:

    tscp.packetType = 'R';
    tscp.msgType = 'E';
    tscp.len = 0;

    if( df == NULL || recNo <= 0 ) {
	tscp.lp = -1;
	memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, buf, sizeof(TS_COM_PROPS));

	return  -1;
    }

    iw = GetFldid((dFILE *)df, fieldName);
    if( iw == 0xFFFF ) {
	tscp.lp = -4;
	memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, buf, sizeof(TS_COM_PROPS));
	return  -4;
    }

    if( lServerAsRunning ) {
	/*i = 32;
	GetComputerName(szCn, &i);
	i = MAXPATH;
	if( GetUserHomeDir(szCn, lpszUser, buf, &i) != 0 )
	{
	    tscp.packetType = 'R';
	    tscp.msgType = 'E';
	    tscp.lp = 0;
	    memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	    strcpy(&buf[sizeof(TS_COM_PROPS)], "ERR:4 INF:No Thread service");
	    tscp.len = strlen(&buf[sizeof(TS_COM_PROPS)]);
	    SrvWriteExchngBuf(exbuf, buf, tscp.len+sizeof(TS_COM_PROPS));
	    return  4;
	}*/
	strZcpy(buf, lpszUserDir, MAXPATH);
	beSurePath(buf);
	sprintf(szCn, "%s.DTM", fieldName);
	makefilename(buf, buf, szCn);
    } else {
	sprintf(buf, "%s.DTM", fieldName);
    }

    wmtDbfLock((dFILE *)df);

    //((dFILE *)df)->rec_p = recNo;
    dseek((dFILE *)df, recNo-1, dSEEK_SET);

    if( get1rec((dFILE *)df) == NULL ) {
	wmtDbfUnLock((dFILE *)df);
	tscp.lp = -4;
	memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, buf, sizeof(TS_COM_PROPS));
	return  -4;
    }

    if( (tscp.lp=dbtToFile((dFILE *)df, iw, buf)) >= 0 ) {
	tscp.msgType = 'D';
    }
    wmtDbfUnLock((dFILE *)df);

    memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
    SrvWriteExchngBuf(exbuf, buf, sizeof(TS_COM_PROPS));

    return  0;

} //end of asqlBlobFetch()


/* asqlBlobPut()
 *
 *************************************************************************/
_declspec(dllexport) long asqlBlobPut(TS_CLIENT_CURSOR *tscc,
					LPCSTR lpszUser, \
					LPCSTR lpszUserDir,\
					void *df, \
					long recNo, \
					LPSTR fieldName, \
					EXCHNG_BUF_INFO *exbuf )
{
    TS_COM_PROPS tscp;
    unsigned short iw;
    char  buf[4096];
    char  szCn[128];
//    int   i;

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
	    goto  agdr_dfTrueR;
	tscc = tscc->pNext;
    }
    df = NULL;
agdr_dfTrueR:

    tscp.packetType = 'R';
    tscp.msgType = 'E';
    tscp.len = 0;

    if( df == NULL || recNo <= 0 ) {
	tscp.lp = -1;
	memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, buf, sizeof(TS_COM_PROPS));

	return  -1;
    }

    iw = GetFldid((dFILE *)df, fieldName);
    if( iw == 0xFFFF ) {
	tscp.lp = -4;
	memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, buf, sizeof(TS_COM_PROPS));
	return  -4;
    }

    if( lServerAsRunning ) {
	/*i = 32;
	GetComputerName(szCn, &i);
	i = MAXPATH;
	if( GetUserHomeDir(szCn, lpszUser, buf, &i) != 0 )
	{
	    tscp.packetType = 'R';
	    tscp.msgType = 'E';
	    tscp.len = 4096-sizeof(TS_COM_PROPS);
	    tscp.lp = 0;
	    memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	    strcpy(&buf[sizeof(TS_COM_PROPS)], "ERR:4 INF:No Thread service");
	    SrvWriteExchngBuf(exbuf, buf, 4096);
	    return  4;
	}*/
	strZcpy(buf, lpszUserDir, MAXPATH);
	beSurePath(buf);
	sprintf(szCn, "%s.DTM", fieldName);
	makefilename(buf, buf, szCn);
    } else {
	sprintf(buf, "%s.DTM", fieldName);
    }

    wmtDbfLock((dFILE *)df);

    //((dFILE *)df)->rec_p = recNo;
    dseek((dFILE *)df, recNo-1, dSEEK_SET);

    if( get1rec((dFILE *)df) == NULL ) {
	wmtDbfUnLock((dFILE *)df);
	tscp.lp = -4;
	memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, buf, 4096);
	return  -4;
    }

    if( (tscp.lp=dbtFromFile((dFILE *)df, iw, buf)) >= 0 ) {
	tscp.msgType = 'D';
    }
    wmtDbfUnLock((dFILE *)df);

    memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
    SrvWriteExchngBuf(exbuf, buf, tscp.len+sizeof(TS_COM_PROPS));

    return  0;

} //end of asqlBlobPut()


/* asqlGetDbfRecNum()
 *
 *************************************************************************/
_declspec(dllexport) long asqlGetDbfRecNum(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf )
{
    TS_COM_PROPS tscp;
    long         l;
    char  lpszResponse[4096];

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
		    goto  agdr_dfTrueD;
	tscc = tscc->pNext;
    }
    df = NULL;
agdr_dfTrueD:

    if( df == NULL ) {
	tscp.packetType = 'R';
	tscp.msgType = 'E';
	tscp.len = 0;
	tscp.lp = -1;
	memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

	return  -1;
    }

    l = getRecNum((dFILE *)df);
    tscp.packetType = 'R';
    tscp.msgType = 'D';
    tscp.len = 0;
    tscp.lp = l;
    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
    SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

    return  l;

} //end of asqlGetDbfRecNum()


/* asqlGetDbfFldInfo()
 *
 *************************************************************************/
_declspec(dllexport) long asqlGetDbfFldInfo(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf )
{
    TS_COM_PROPS tscp;
    dFIELD 	*field;
    char        *sz;
    int          i;
    char  lpszResponse[4096];

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
		    goto  agdr_dfTrueD;
	tscc = tscc->pNext;
    }
    df = NULL;
agdr_dfTrueD:

    if( df == NULL ) {
	tscp.packetType = 'R';
	tscp.msgType = 'E';
	tscp.len = 0;
	tscp.lp = -1;
	memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

	return  -1;
    }

    field = dfcopy((dFILE *)df, (dFIELD *)(lpszResponse+sizeof(TS_COM_PROPS)));
    if( field == NULL ) {
	tscp.packetType = 'R';
	tscp.msgType = 'E';
	tscp.len = 0;
	tscp.lp = -2;
	memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

	return  -2;
    }

    sz = field[0].fieldstart;
    for( i = 0;   field[i].field[0] != '\0';   i++ ) {
	field[i].fieldstart = field[i].fieldstart - sz + (char *)1;
    }

    tscp.packetType = 'R';
    tscp.msgType = 'D';
    tscp.len = ((dFILE *)df)->field_num*sizeof(dFIELD);
    tscp.lp = ((dFILE *)df)->field_num;
    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));

    SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS)+tscp.len);

    return  0;

} //end of asqlGetDbfFldInfo()



/* asqlFreeDbfLock()
 *
 *************************************************************************/
_declspec(dllexport) TS_CLIENT_CURSOR *asqlFreeDbfLock(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf )
{
    TS_COM_PROPS tscp;
    TS_CLIENT_CURSOR *tscpLast, *tscpHead;
    char  lpszResponse[4096];

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    tscpLast = NULL;
    tscpHead = tscc;
    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
		    goto  agdr_dfTrueD;
	tscpLast = tscc;
	tscc = tscc->pNext;
    }
    df = NULL;
agdr_dfTrueD:

    if( df == NULL ) {
	tscp.packetType = 'R';
	tscp.msgType = 'E';
	tscp.len = 0;
	tscp.lp = -1;
	memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

	return  tscpHead;
    }

    /*dSleep((dFILE *)df);
    if( tscpLast != NULL ) {
	tscpLast->pNext = tscc->pNext;
    } else {
	tscpHead = tscc->pNext;
    }
    free( tscc );
    */
    tscp.lp = wmtDbfTryLock((dFILE *)df);

    tscp.packetType = 'R';
    tscp.msgType = 'D';
    tscp.len = 0;
    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
    SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

    return  tscpHead;

} //end of asqlFreeDbfLock()


/* asqlFreeDbfUnLock()
 *
 *************************************************************************/
_declspec(dllexport) TS_CLIENT_CURSOR *asqlFreeDbfUnLock(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf )
{
    TS_COM_PROPS tscp;
    TS_CLIENT_CURSOR *tscpLast, *tscpHead;
    char  lpszResponse[4096];

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    tscpLast = NULL;
    tscpHead = tscc;
    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
		    goto  agdr_dfTrueD;
	tscpLast = tscc;
	tscc = tscc->pNext;
    }
    df = NULL;
agdr_dfTrueD:

    if( df == NULL ) {
	tscp.packetType = 'R';
	tscp.msgType = 'E';
	tscp.len = 0;
	tscp.lp = -1;
	memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
	SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

	return  tscpHead;
    }

    /*dSleep((dFILE *)df);
    if( tscpLast != NULL ) {
	tscpLast->pNext = tscc->pNext;
    } else {
	tscpHead = tscc->pNext;
    }
    free( tscc );
    */
    if( wmtDbfIsLock((dFILE *)df) == 1 ) {
	wmtDbfUnLock((dFILE *)df);
	tscp.lp = 0;	//unlock it
    } else {
	tscp.lp = 1;    //not locked
    }

    tscp.packetType = 'R';
    tscp.msgType = 'D';
    tscp.len = 0;
    memcpy(lpszResponse, &tscp, sizeof(TS_COM_PROPS));
    SrvWriteExchngBuf(exbuf, lpszResponse, sizeof(TS_COM_PROPS));

    return  tscpHead;

} //end of asqlFreeDbfUnLock()



/* asqlBlobMemFetch()
 *
 *************************************************************************/
_declspec(dllexport) long asqlBlobMemFetch(TS_CLIENT_CURSOR *tscc, \
					LPCSTR lpszUser, \
					LPCSTR lpszUserDir,\
					void *df, \
					long recNo, \
					LPSTR fieldName, \
					EXCHNG_BUF_INFO *exbuf )
{
    TS_COM_PROPS tscp;
    unsigned short iw;
    char  	   buf[4096];
    char           *sp;
    long           memSize;

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
	    goto  agdr_dfTrueR;
	tscc = tscc->pNext;
    }

    df = NULL;
agdr_dfTrueR:

    tscp.packetType = 'R';
    tscp.msgType = 'E';
    tscp.len = 0;

    if( df == NULL || recNo <= 0 ) {
	tscp.lp = -1;
	SrvWriteExchngBuf(exbuf, &tscp, sizeof(TS_COM_PROPS));

	return  -1;
    }

    iw = GetFldid((dFILE *)df, fieldName);
    if( iw == 0xFFFF ) {
	tscp.lp = -4;
	SrvWriteExchngBuf(exbuf, &tscp, sizeof(TS_COM_PROPS));
	return  -4;
    }

    //((dFILE *)df)->rec_p = recNo;
    dseek((dFILE *)df, recNo-1, dSEEK_SET);

    if( get1rec((dFILE *)df) == NULL ) {
	tscp.lp = -4;
	SrvWriteExchngBuf(exbuf, &tscp, sizeof(TS_COM_PROPS));
	return  -4;
    }

    sp = blobToMem((dFILE *)df, iw, &memSize);
    if( sp != NULL ) {
	TS_COM_PROPS tscp1;
	//
	//tscp.leftPacket = '\1';	//no use: there are any other packet to transmit
	//tscp.endPacket = '\1';  //not end, DONNOT GIVE UP
	tscp1.packetType = 'R';
	tscp1.msgType = 'B';
	//tscp.lp = 0;
	SrvWriteExchngBufEx(exbuf, &tscp1, sp, memSize, 0);

	//come from readBtreeData(), free it with its function
	//free(sp);
	freeBlobMem((dFILE *)df, sp);

	//send a 'D' packet
	tscp.msgType = 'D';
	tscp.lp = 0;
	SrvWriteExchngBuf(exbuf, &tscp, sizeof(TS_COM_PROPS));

	return  0;
    }

    tscp.lp = -5;
    memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
    SrvWriteExchngBuf(exbuf, buf, sizeof(TS_COM_PROPS));
    return  -5;

} //end of asqlBlobMemFetch()


/* asqlBlobMemPut()
 *
 *************************************************************************/
_declspec(dllexport) long asqlBlobMemPut(TS_CLIENT_CURSOR *tscc, \
					LPCSTR lpszUser, \
					LPCSTR lpszUserDir,\
					void *df, \
					long recNo, \
					LPSTR fieldName, \
					EXCHNG_BUF_INFO *exbuf )
{
    TS_COM_PROPS tscp;
    unsigned short iw;
    char           *sp;
    long           memSize;
    BOOL 	   pbIfEnd;

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    while( tscc != NULL ) {
	if( tscc->p == (void *)df )
	    goto  agdr_dfTrueR;
	tscc = tscc->pNext;
    }
	df = NULL;
agdr_dfTrueR:

    if( SrvReadExchngBufEx(exbuf, &sp, &memSize, &pbIfEnd) != 0 ) {
	tscp.lp = -5;
	SrvWriteExchngBuf(exbuf, &tscp, sizeof(TS_COM_PROPS));
	return  -5;
    }

    tscp.packetType = 'R';
    tscp.msgType = 'E';
    tscp.len = 0;

    if( df == NULL || recNo <= 0 ) {
	HANDLE hDataBuf;

	hDataBuf = LocalHandle(sp);
	LocalUnlock( hDataBuf );
	LocalFree( hDataBuf );

	tscp.lp = -1;
	SrvWriteExchngBuf(exbuf, &tscp, sizeof(TS_COM_PROPS));

	return  -1;
    }

    iw = GetFldid((dFILE *)df, fieldName);
    if( iw == 0xFFFF ) {
	HANDLE hDataBuf;

	hDataBuf = LocalHandle(sp);
	LocalUnlock( hDataBuf );
	LocalFree( hDataBuf );

	tscp.lp = -4;
	SrvWriteExchngBuf(exbuf, &tscp, sizeof(TS_COM_PROPS));
	return  -4;
    }

    //((dFILE *)df)->rec_p = recNo;
    dseek((dFILE *)df, recNo-1, dSEEK_SET);

    if( get1rec((dFILE *)df) == NULL ) {
	HANDLE hDataBuf;

	hDataBuf = LocalHandle(sp);
	LocalUnlock( hDataBuf );
	LocalFree( hDataBuf );

	tscp.lp = -4;
	SrvWriteExchngBuf(exbuf, &tscp, sizeof(TS_COM_PROPS));
	return  -4;
    }

    //2000.7.21
    /*
    if( memSize < 0 ) {
	
    } else {
	blobFromMem((dFILE *)df, iw, sp, memSize);
    }*/
  
    if( memSize >= 0 ) {
	blobFromMem((dFILE *)df, iw, sp, memSize);
    }


    { ///////////////////////
	HANDLE hDataBuf;

	hDataBuf = LocalHandle(sp);
	LocalUnlock( hDataBuf );
	LocalFree( hDataBuf );
    }

    tscp.msgType = 'D';
    tscp.lp = 0;
    SrvWriteExchngBuf(exbuf, &tscp, sizeof(TS_COM_PROPS));

    return  0;

} //end of asqlBlobMemPut()


/* asqTslFreeMem()
 *
 *************************************************************************/
_declspec(dllexport) void asqlTsFreeMem( void *p )
{
    free( p );
}


/************************* end of tsdbfutl.c ********************************/
