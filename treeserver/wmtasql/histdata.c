/****************
* ^-_-^
* histdata.c
* copyright (c) EastUnion Computer Service Co. Ltd. 1995
*               CRSC 1998
*
* author: Xilong Pei
*****************************************************************************/

#define __HISTDATA_C__

#include <stdlib.h>
#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "dio.h"
#include "mistring.h"
#include "asqlana.h"
#include "wmtasql.h"
#include "histdata.h"
#include "ts_const.h"
#include "strutl.h"
#include "cfg_fast.h"
#include "ts_dict.h"
#include "xexp.h"
#include "filetool.h"

char *getasqlAskVal(char *asqlAsk, char *buf);
extern int asqlTaskThreadNum;	//in wmtasql.c

WSToMT dFILE *dfData = NULL;
WSToMT char oldTableName[256];
WSToMT Asql_ENV    asqlEnvSelf;

/****************
* hyPath is type as: c:\tg\history\
*****************************************************************************/
_declspec(dllexport) long hyDataCollect(LPCSTR lpszUser, char *hyPath, char *szRem, \
		   void *VariableRegister, short VariableNum, \
		   EXCHNG_BUF_INFO *exbuf)
{
    dFILE 	*df, *df1;
    char	path[MAXPATH];
    char	condCode[256];
    char	idBuf[256];
    char	asqlAsk[256];
    char	typeBuf[256];
    int         j;
    long	l;
    TS_COM_PROPS tscp;
    char  	buf[4096];
    short rstId, df1CC, df1SI, df1RT, df1RS, dfCC, dfAA;

    dfData = NULL;
    oldTableName[0] = '\0';

    if( lpszUser == NULL ) {
	asqlEnvSelf.szUser[0] = '\0';
    } else {
	strcpy(asqlEnvSelf.szUser, lpszUser);
    }

    if( hyPath == NULL || *hyPath == '\0' ) {
	char *s;

	s = GetCfgKey(csuDataDictionary, "SYSTEM", "DBROOT", "PATH");
	if( s != NULL ) {
	    strcpy(asqlEnvSelf.szAsqlResultPath, s);
	    strcpy(asqlEnvSelf.szAsqlFromPath, s);
	} else {
	    return  0;
	}
    } else if( hyPath[1] != ':' && hyPath[0] != '\\' ) {
	char *s;

	s = GetCfgKey(csuDataDictionary, "DATABASE", hyPath, "PATH");
	if( s != NULL ) {
	    strcpy(asqlEnvSelf.szAsqlResultPath, s);
	    strcpy(asqlEnvSelf.szAsqlFromPath, s);
	} else {
	    return  0;
	}
    } else {
	strcpy(asqlEnvSelf.szAsqlResultPath, hyPath);
	strcpy(asqlEnvSelf.szAsqlFromPath, hyPath);
	beSurePath(asqlEnvSelf.szAsqlResultPath);
	beSurePath(asqlEnvSelf.szAsqlFromPath);
    }

    { // fill the HISTSREM.DBF
	char	dateBuf[16];
	char    timeBuf[16];
	SYSTEMTIME SystemTime;
	/*
	typedef struct _SYSTEMTIME {  // st
	    WORD wYear;
	    WORD wMonth;
	    WORD wDayOfWeek;
	    WORD wDay;
	    WORD wHour; 
	    WORD wMinute; 
	    WORD wSecond; 
	    WORD wMilliseconds; 
	} SYSTEMTIME; 
	*/

	GetLocalTime(&SystemTime);

	sprintf(dateBuf, "%4d年%02d月%02d日", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay);
	sprintf(timeBuf, "%2d时%02d分%02d秒",SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);

	if( szRem != NULL ) {
	    strZcpy(buf, szRem, 255);
	} else {
	    strcpy(buf, dateBuf);
	    strcat(buf, timeBuf);
	}

	makeTrimFilename(path, asqlEnvSelf.szAsqlFromPath, "HISTSREM");
	df = dAwake(path, DOPENPARA);
	if( df == NULL ) {
		return  -1;
	}
	dseek(df, 0, dSEEK_END);
	NewRec(df);
	put_fld(df, GetFldid(df, "EXPLAIN"), buf);

	sprintf(dateBuf, "%4d%02d%02d", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay);
	sprintf(timeBuf, "%2d%02d%02d",SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
	put_fld(df, GetFldid(df, "DATE"), dateBuf);
	put_fld(df, GetFldid(df, "TIME"), timeBuf);

	l = df->rec_p;
	itoa(df->rec_p, idBuf, 10);
	put_fld(df, GetFldid(df, "STAT_ID"), idBuf);
	put1rec(df);
	dSleep(df);
    }

    //calculate the analysis result according to condition dbf
    makeTrimFilename(path, asqlEnvSelf.szAsqlFromPath, "HISTCOND");
    df = dAwake(path, DOPENPARA);
    if( df == NULL ) {
	return  -1;
    }
    makeTrimFilename(path, asqlEnvSelf.szAsqlFromPath, "HISTRSUT");
    df1 = dAwake(path, DOPENPARA);
    if( df1 == NULL ) {
	dSleep(df);
	return  -1;
    }

    dseek(df, 0, dSEEK_SET);
    dseek(df1, 0, dSEEK_END);
    rstId = GetFldid(df, "RESULTTYPE");
    df1CC = GetFldid(df1, "COND_CODE");
    df1SI = GetFldid(df1, "STAT_ID");
    df1RT = GetFldid(df1, "RESULTTYPE");
    df1RS = GetFldid(df1, "RESULT");
    dfCC = GetFldid(df, "COND_CODE");
    dfAA = GetFldid(df, "ASQL_ASK");

//    dsetbuf(0);
    while( !deof(df) ) {

	getrec(df);

	get_fld(df, dfCC, condCode);
	get_fld(df, dfAA,  asqlAsk);
	//macro( buf, 128, asqlAsk, NULL );
	//strZcpy(asqlAsk, buf, 128);

	if( stricmp(condCode, "ASQL") == 0 ) {

		makefilename(asqlAsk, asqlEnvSelf.szAsqlFromPath, asqlAsk);

		/*extern char insideTask;		// define in asqlana.c

		s = getCtlInfomation(&ctlFile, eucfg, NULL, datapath);
		assert( s );
		strcpy(asqlEnv.szAsqlResultPath, hyPath);
		strcpy(asqlEnv.szAsqlFromPath, s);
		insideTask = 1;
		queryTaskMan(asqlAsk, asqlAsk, ASQL_TASK_USE_ENV);
		insideTask = 0;
		*/
		j = wmtAskQS(LONG_MIN+SHRT_MAX, asqlAsk, \
					AsqlExprInFile|Asql_USEENV, \
					&asqlEnvSelf, \
					(SysVarOFunType **)&VariableRegister,
					&VariableNum,
					exbuf);
		if( j >= asqlTaskThreadNum ) {
		    tscp.packetType = 'R';
		    tscp.msgType = 'E';
		    tscp.len = 4096-sizeof(TS_COM_PROPS);
		    tscp.lp = 3;
		    memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
		    strcpy(&buf[sizeof(TS_COM_PROPS)], "ERR:3 INF:No Thread service");
		    SrvWriteExchngBuf(exbuf, buf, 4096);
		    l = -2;
		    break;
		}

		{ //wait the end of this thread
		    MSG msg1;
		    GetMessage(&msg1, NULL, ASQL_ENDTASK_MSG, ASQL_ENDTASK_MSG);
		}

		continue;

	}
	get_fld(df, rstId, typeBuf);

	getasqlAskVal(asqlAsk, buf);
	put_fld(df1, df1CC, condCode);
	put_fld(df1, df1SI, idBuf);
	put_fld(df1, df1RT, typeBuf);
	put_fld(df1, df1RS, buf);
	putrec(df1);
    }

    dSleep(df);
    dSleep(df1);

    if( dfData != NULL )
	dSleep(dfData);

    return  l;

} // end of hyDataCollect()

/*===============
* getasqlAskVal()
*===========================================================================*/
char *getasqlAskVal(char *asqlAsk, char *buf)
{
    char	tableName[256];
    int		x;
    int		y;

    *buf = '\0';
    sscanf(asqlAsk, "%[^(](%d,%d", tableName, &y, &x);
    if( dfData == NULL ) {

	makefilename(tableName, asqlEnvSelf.szAsqlFromPath, tableName);
	dfData = dAwake(tableName, DOPENPARA);
	if( dfData == NULL )	return  NULL;
    } else {
	if( stricmp(tableName, oldTableName) != 0 ) {
	    dSleep( dfData );
	    strcpy(oldTableName, tableName);

	    makefilename(tableName, asqlEnvSelf.szAsqlFromPath, tableName);
	    dfData = dAwake(tableName, DOPENPARA);
	    if( dfData == NULL )	return  NULL;
	}
    }

    dseek(dfData, y-1, dSEEK_SET);
    get1rec(dfData);
    get_fld(dfData, (short)(x-1), buf);

    return buf;

} // end of getasqlAskVal()
