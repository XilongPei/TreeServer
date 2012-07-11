/****************
* wmtasql.c
* Windows NT MultiThread ASQL
* 1997.10.20 Xilong Pei
*
* Notice:
*     this prgram need the program set the called function(s) set their public
* or static variable to:
*     _declspec(thread) type var = initVal;
*
*
*   1.main thread create sub thread, and every thread point to a asqlId
*   2.every thread will hangup after they finished working, 
*   3.main thread should assigned every task a unique integer: it
*   
*   a asql thread will be assigned task when the intOfTask(it) is QMTASQL_IDLE_IT
*
* 1999.5.15 add config from Tengine.ini:
*         DEF_BtreeNodeBufferSize
*         MAX_BtreeNodeBufferSize
*****************************************************************************/
//#define _TEST_
//#define ASQLCLT

#define __WMTASQL_C_

#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <limits.h>
#include <io.h>

#include "asqlana.h"
#include "asqlerr.h"
#include "dio.h"
#include "xexp.h"
#include "wmtasql.h"
#include "cfg_fast.h"
#include "strutl.h"
#include "treepara.h"
#include "strutl.h"
#include "filetool.h"
#include "treevent.h"
#include "mistring.h"
#include "hzth.h"
#include "tsdbfutl.h"
#include "ts_const.h"
#include "ts_dict.h"
#include "odbc_dio.h"
//#include "exchbuff.h"

#include "dbtree.h"
#include "histdata.h"
#include "rjio.h"

#include "sql_dio.h"
#ifndef ASQLCLT
#include <lm.h>

#include "taccount.h"
#include "tscommon.h"
#endif

#define QMTASQL_STOP_IT    -2
#define QMTASQL_IDLE_IT	   -3
#define WAIT_FOR_CALLTHREAD	3600000

ASKQSDATA  askQS_data[ASQL_MAX_TASKTHREAD];
HANDLE	   hAsqlResSem = NULL;		// Resource count of the ASQL engine

static HANDLE asqlTaskThread[ASQL_MAX_TASKTHREAD];
static DWORD  asqlDwIDThread[ASQL_MAX_TASKTHREAD];
//static CRITICAL_SECTION g_CriticalSection[ASQL_MAX_TASKTHREAD];
static CRITICAL_SECTION g_CS_ThreadMan;

int asqlTaskThreadNum = ASQL_MAX_TASKTHREAD;
DWORD WINAPI askQS_tm(LPASKQSDATA lpAskQS_data);

_int64 FileTimeToQuadWord(PFILETIME pFileTime);
PFILETIME QuadWordToFileTime(_int64 qw, PFILETIME pFileTime);
_int64 threadRuningTime(int it);

_int64 i64KernelTimeStart[ASQL_MAX_TASKTHREAD];
_int64 i64UserTimeStart[ASQL_MAX_TASKTHREAD];
DWORD  dwThreadTimeStart[ASQL_MAX_TASKTHREAD];

char   pubBuf[1024];

//PCFG_STRU csu;                              //in cfg_fast.c
//PCFG_STRU csuDataDictionary;		      //in ts_dict.c



/*
 *
 *******************initWmtAskQS()****************************************/
_declspec(dllexport) DWORD ProcessAttach( void )
{
    int       i;
    char      buf[MAXPATH];
    char      buff[MAXPATH];
    char      szIni[MAXPATH];
    char      *s;
    MEMORYSTATUS memstatus;
extern RJFILE   *rjFile;

//define in Btree.c
////////////////////////////////////////////////////////////////////////////
extern short MAX_BtreeNodeBufferSize;
extern short DEF_BtreeNodeBufferSize;
////////////////////////////////////////////////////////////////////////////


#ifndef ASQLCLT
    i = MAXPATH;
    i = GetTSProfilePath(buf, (DWORD *)&i);
    if( buf[0] != '\0' && i == 0 )
	strcat(buf, "\\");
    else {
	char *sz;

	_fullpath(buf, GetCommandLine(), MAXPATH);
	sz = strrchr(buf, '\\');
	if( sz != NULL )
	    *(sz+1) = '\0';
	strcat(buf, "Config\\");
    }

    strcpy(buff, buf);
    strcpy(szIni, buf);
    strcat(buf, "ASQLWMT.CFG" );
#else
    buff[0] = '\0';
    szIni[0] = '\0';
    strcpy(buf, "ASQLWMT.CFG" );
#endif

    strcat(szIni, "TENGINE.INI");

#ifndef ASQLCLT
    asqlTaskThreadNum = GetPrivateProfileInt("ASQL", "ThreadNum", 6, szIni);
    if( asqlTaskThreadNum > ASQL_MAX_TASKTHREAD )
	asqlTaskThreadNum = ASQL_MAX_TASKTHREAD;
    else if( asqlTaskThreadNum < 0 )
	asqlTaskThreadNum = 6;


    memstatus.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus( &memstatus );

    DEF_BtreeNodeBufferSize = (short)GetPrivateProfileInt("ASQL", "DEF_BtreeNodeBufferSize", 32, szIni);
    if( (unsigned long)DEF_BtreeNodeBufferSize * BTREE_BUFMAXLEN >= memstatus.dwTotalPhys )
	DEF_BtreeNodeBufferSize = 32;
    else if( DEF_BtreeNodeBufferSize < 32 )
	DEF_BtreeNodeBufferSize = 32;

    MAX_BtreeNodeBufferSize = (short)GetPrivateProfileInt("ASQL", "MAX_BtreeNodeBufferSize", 320, szIni);
    if( (unsigned long)MAX_BtreeNodeBufferSize * BTREE_BUFMAXLEN >= memstatus.dwTotalPhys )
	MAX_BtreeNodeBufferSize = 320;
    else if( MAX_BtreeNodeBufferSize < 320 )
	MAX_BtreeNodeBufferSize = 320;


    // Initialize the semaphore which indicate how many resources are available.
    hAsqlResSem = CreateSemaphore( NULL, asqlTaskThreadNum, asqlTaskThreadNum, "$AsqlResSem$" );
    if( hAsqlResSem == NULL )
    {
	return 1;
    }

    i = GetPrivateProfileInt("ASQL", "ODBC_CONNECT_NUM", 0, szIni);
    if( i > 0 )
    {
	char szDSN[256];
	char szUser[256];
	char szPassword[256];

	GetPrivateProfileString("ASQL", "ODBC_DSN", "",  szDSN, 256, szIni);
	GetPrivateProfileString("ASQL", "ODBC_USER", "",  szUser, 256, szIni);
	GetPrivateProfileString("ASQL", "ODBC_Password", "",  szPassword, 256, szIni);
	if( InitOdbcAccess(i, szDSN, szUser, szPassword) == FALSE )
	{

		char szTemp[512];

		strcpy(szTemp, "ODBC Connect Error:");
		strcat(szTemp, szOdbcExecResult);

		treesvrInfoReg(NULL,
			   EVENTLOG_WARNING_TYPE,
			   1,
			   12,
			   NULL,
			   0,
			   szTemp,
			   NULL);
	}
    }

    lServerAsRunning = 1;
#else

#ifdef _TEST_
    asqlTaskThreadNum = 10;   //ASQL_MAX_TASKTHREAD;
    hAsqlResSem = CreateSemaphore( NULL, asqlTaskThreadNum, asqlTaskThreadNum, "$AsqlResSem$" );
    if( hAsqlResSem == NULL )
    {
	return 1;
    }
#else
    asqlTaskThreadNum = 1;   //ASQL_MAX_TASKTHREAD;
#endif

    lServerAsRunning = 0;
#endif

    {
	char slib[256];
	GetPrivateProfileString("ASQL", "ADFUNDLL", "",  slib, 256, szIni);
	if( slib[0] != '\0' ) {
	    hLibXexp = LoadLibrary(slib);
	    if( hLibXexp == NULL ) {
		char szTemp[256+80];
		strcpy(szTemp, "无法装入动态连接库: ");
		strcat(szTemp, slib);

		treesvrInfoReg(NULL,
			   EVENTLOG_WARNING_TYPE,
			   1,
			   12,
			   NULL,
			   0,
			   szTemp,
			   NULL);
	    }
	} else	hLibXexp = NULL;
    }

    strcat(buff, "ASQLADIT.GDC");
    asqlAudit = dbTreeOpen(buff);

    //temp porgram, I will seperate the database dictionary and program
    //config later
    csu = OpenCfgFile( buf );
    csuDataDictionary = csu;

    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    memset(askQS_data, 0, asqlTaskThreadNum*sizeof(ASKQSDATA));

    for( i = 0;  i < asqlTaskThreadNum;  i++ ) {
	   //create the threads and let them know which one it is
	   askQS_data[i].it = i;
	   asqlTaskThread[i] = (HANDLE)_beginthreadex(NULL, 0, askQS_tm, \
			      (LPVOID)&askQS_data[i], 0, &asqlDwIDThread[i]);
	    if( asqlTaskThread[i] == NULL ) {
			return  1;
	    }
    }

    //wait for the sub thread finishing runing
    Sleep(1000);
    for( i = 0;  i < asqlTaskThreadNum;  i++ )
    {
	    while( askQS_data[i].it >= 0 )
		    Sleep( 10 );
    }

    //initialize  CriticalSection for DIO
    InitializeCriticalSection( &dofnCE );
    InitializeCriticalSection( &btreeIndexCE );
    InitializeCriticalSection( &g_CS_ThreadMan );

    _BtreeOpenNum_ = 0;
    _DioOpenFileNum_ = 0;

    s = GetCfgKey(csu, "SYSTEM", "DBROOT", "PATH");
    if( s != NULL ) {
	strZcpy(asqlConfigEnv.szAsqlFromPath, s, MAXPATH);
	beSurePath(asqlConfigEnv.szAsqlFromPath);
    } else {
	asqlConfigEnv.szAsqlFromPath[0] = '\0';
    }

    s = GetCfgKey(csu, "SYSTEM", NULL, "CODE");
    if( s != NULL ) {
	strZcpy(buf, s, MAXPATH);
	if( hOpen(buf) != 1 )
	{
	    treesvrInfoReg(NULL,
			   EVENTLOG_WARNING_TYPE,
			   1,
			   12,
			   NULL,
			   0,
			   "配置文件HZTH CODECOD=? 没有设置对或代码文件打开出错",
			   NULL);
	}
    }

    //rjFile = openRJ( "RJ" );
    s = GetCfgKey(csu, "SYSTEM", NULL, "LOGFILE");
    if( s != NULL ) {
	strZcpy(buf, s, MAXPATH);

	if ( access(buf, 6) != 0 ) {
	    rjFile = createRJ(buf);
	} else {
	    rjFile = openRJ(buf);
	}

	if( rjFile == NULL )
	{
	    treesvrInfoReg(NULL,
			   EVENTLOG_WARNING_TYPE,
			   1,
			   12,
			   NULL,
			   0,
			   "日志设置错或文件打开出错",
			   NULL);
	}
    } else {
	rjFile = NULL;
    }

    s = GetCfgKey(csu, "POLICY", "OPENDBF", "NUM");
    if( s != NULL ) {
	i = atoi(s);
	for( ;  i > 0;  i--) {
	     sprintf(buf, "F%d", i);
	     s = GetCfgKey(csu, "POLICY", "OPENDBF", buf);
	     if( s != NULL ) {
		 dFILE *df;
		 char  *sz;
		 char   szDataBase[32];

		 sz = strchr(s, '*');
		 if( sz == NULL ) {
			makefilename(buf, asqlConfigEnv.szAsqlFromPath, s);
		 } else {
			strZcpy(buf, s, MAXPATH);

			sz = '\0';
			strZcpy(szDataBase, s, 32);
			s = GetCfgKey(csuDataDictionary, "DATABASE", szDataBase, "PATH");

			if( s != NULL ) {
			    strZcpy(asqlEnv.szAsqlFromPath, s, MAXPATH);
			    makefilename(buf, asqlEnv.szAsqlFromPath, buf);
			} else {
			    char showBuf[512];

			    sprintf(showBuf, "[POLICY]中设置错: %s", buf);
			    treesvrInfoReg(NULL,
					   EVENTLOG_WARNING_TYPE,
					   1,
					   12,
					   NULL,
					   0,
					   showBuf,
					   NULL);
			    makefilename(buf, asqlConfigEnv.szAsqlFromPath, buf);
			}
		 }

		 df = dAwake(buf, DOPENPARA);
		 if( df == NULL ) {

		     char showBuf[512];

		     sprintf(showBuf, "无法快速打开数据表: %s", buf);
		     treesvrInfoReg(NULL,
			   EVENTLOG_WARNING_TYPE,
			   1,
			   12,
			   NULL,
			   0,
			   showBuf,
			   NULL);
                 }

             }
	}
    }

    /*s = GetCfgKey(csu, "POLICY", "OPENNDX", "NUM");
    if( s != NULL ) {
        char ndxPath[MAXPATH];
        char dbfName[MAXPATH];

        strcpy(ndxPath, asqlConfigEnv.szAsqlFromPath);
        //strcat(ndxPath, "NDX\\");

        i = atoi(s);
        for( ;  i > 0;  i--) {
             sprintf(buf, "NDX%d", i);
	     s = GetCfgKey(csu, "FASTOPEN", "NDX", buf);
             if( s != NULL ) {
                 bHEAD *bh;

                 makefilename(buf, ndxPath, s);

		 sprintf(dbfName, "DBF%d", i);
		 s = GetCfgKey(csu, "FASTOPEN", "NDX", dbfName);
		 if( s == NULL ) {

		     char showBuf[512];

		     sprintf(showBuf, "配置文件[FASTOPEN] #NDX DBF*= 没有与NDX×＝配对: %s", dbfName);
		     treesvrInfoReg(NULL,
			   EVENTLOG_WARNING_TYPE,
			   1,
			   12,
			   NULL,
			   0,
			   showBuf,
			   NULL);
		     continue;
		 }
		 strZcpy(dbfName, s, MAXPATH);
		 s = GetCfgKey(csu, "DBROOT", TrimFileName(dbfName), "MNDXKEY");
		 makefilename(dbfName, asqlConfigEnv.szAsqlFromPath, dbfName);

		 bh = IndexAwake(dbfName, buf, BTREE_FOR_CLOSEDBF);
		 if( bh == NULL ) {

		     char showBuf[512];

		     if( s != NULL )
			 bh = IndexBAwake(dbfName, s, buf, BTREE_FOR_CLOSEDBF);
		     if( bh == NULL ) {
			 sprintf(showBuf, "索引文件打开出错: %s", buf);
			 treesvrInfoReg(NULL,
					     EVENTLOG_WARNING_TYPE,
					     1,
					     12,
					     NULL,
					     0,
					     showBuf,
					     NULL);
		     } //if( bh == NULL )
		 } //if( bh == NULL )
	     } //if( s != NULL )
	} //for()
    } //if( s != NULL )
    */

    treesvrInfoReg(NULL, EVENTLOG_WARNING_TYPE,
			   1,
			   12,
			   NULL,
			   0,
			   "ASQL事务服务引挚正常起动",
			   NULL);

    s = GetCfgKey(csu, "SYSTEM", NULL, "TEMP_PATH");
    if( s != NULL ) {
	extern char  tmpPath[MAXPATH];

	strZcpy(tmpPath, s, MAXPATH);
	beSurePath(tmpPath);
    } else {
	//defined in asqlana.c
	extern char  tmpPath[MAXPATH];
	DWORD  dwRetCode;

	dwRetCode = GetEnvironmentVariable(
		"TMP",
		tmpPath,
		MAXPATH );

	if( dwRetCode != 0 ) {
	    beSurePath(tmpPath);
	}
    }


    return  0;

} //end of ProcessAttach()


/*
 *
 *******************closeWmtAskQS()**************************************/
_declspec(dllexport) DWORD ProcessDetach( void )
{
    int i;

    if( asqlTaskThreadNum == 0 )
    {
	return  1;
    }

    for( i = 0;  i < asqlTaskThreadNum;  i++ ) {
		askQS_data[i].it = QMTASQL_STOP_IT;
		askQS_data[i].FileName = NULL;
		PostThreadMessage(asqlDwIDThread[i], ASQL_STARTTASK_MSG, 0, 0);

		Sleep( 0 );
		while( askQS_data[i].it != QMTASQL_IDLE_IT )
		    Sleep( 0 );
		/*1998.3.8 Xilong
		while( ResumeThread( asqlTaskThread[i] ) < 1 ) {
			Sleep(0);
		}*/
    }

    if( hLibXexp != NULL )
	FreeLibrary( hLibXexp );
    hClose();
    dRelease(NULL);
    IndexRelease(NULL);

    CloseCfgFile( csuDataDictionary );

    if( hAsqlResSem != NULL )
    {
	CloseHandle( hAsqlResSem );
	hAsqlResSem = NULL;
    }
    CloseOdbcAccess();

    return  0;

} //end of ProcessDetach()


/*
 *
 *******************wmtAskQS()***********************************************/
int wmtAskQS(long it, char *FileName, short ConditionType, Asql_ENV *asqlEnvSelf, \
			    SysVarOFunType **VariableRegister, short *VariableNum,\
			    EXCHNG_BUF_INFO *exbuf)
{
    int   i;
    //static int lastThread = -1;

    WaitForSingleObject( hAsqlResSem, INFINITE );

    //search for idle thread
    EnterCriticalSection( &g_CS_ThreadMan );
    /*for(i = (lastThread+1)%asqlTaskThreadNum;  i != lastThread && askQS_data[i].it != QMTASQL_IDLE_IT;  i++)
    {
	if( i >= asqlTaskThreadNum - 1 )
	    i = 0;
    }*/
    for(i = 0;  askQS_data[i].it != QMTASQL_IDLE_IT && \
						i < asqlTaskThreadNum;  i++)
    {
	;       //no statement
    }
    if( i >= asqlTaskThreadNum )
    { //no idle thread
      //this should not appeares 1998.8.22
      //////////////////
	   ReleaseSemaphore( hAsqlResSem, 1, NULL );

	   LeaveCriticalSection( &g_CS_ThreadMan );
	   return  i;
    }
    //lastThread = i;
    askQS_data[i].it = it;
    LeaveCriticalSection( &g_CS_ThreadMan );

    askQS_data[i].callThreadId = GetCurrentThreadId();
    askQS_data[i].wmtExbuf = exbuf;
    /*if( it == 2 ) {
	char buf[800];

	sprintf(buf, "extbuf:%ld event%ld cliid:%ld srvid:%ld flag:%c",
	exbuf->lpExchBuf,
	exbuf->hEmpEvent,
	exbuf->dwCliThreadId,
	exbuf->dwSrvThreadId,
	exbuf->cFlag);

	MessageBox(NULL, buf, "ok", MB_OK);
    }*/

    askQS_data[i].ConditionType = ConditionType;
    askQS_data[i].VariableRegister = VariableRegister;
    askQS_data[i].VariableNum = VariableNum;
    /* no use, remed
    askQS_data[i].FromTable = NULL;
    askQS_data[i].ToTable = NULL;
    */

    //get the asql runing config set
    strZcpy(askQS_data[i].szAsqlFromPath, asqlEnvSelf->szAsqlFromPath, MAXPATH);
    strZcpy(askQS_data[i].szAsqlResultPath, asqlEnvSelf->szAsqlResultPath, MAXPATH);
    strZcpy(askQS_data[i].szUser, asqlEnvSelf->szUser, 32);

    askQS_data[i].FileName = FileName;

    PostThreadMessage(asqlDwIDThread[i], ASQL_STARTTASK_MSG, 0, 0);
    /*1998.3.8 Xilong
    while( ResumeThread( asqlTaskThread[i] ) < 1 ) {
	Sleep(0);
    }
    */
    dwThreadTimeStart[i] = GetTickCount();

    return  i;

} //end of wmtAskQS()


/*use lpAskQS_data to fetch data all the time, for we will get data from
 *the same address all the time
 ===================askQS_tm()==============================================*/
DWORD WINAPI askQS_tm(LPASKQSDATA lpAskQS_data)
{
//    FILETIME ftKernelTimeStart;
//    FILETIME ftUserTimeStart;
//    FILETIME ftDummy;
    HANDLE   hl;
    char     buf[1024];
extern WSToMT FromToStru fFrTo;

    if( lpAskQS_data->it >= 0 )
    { //find and store my thread id
	intOfThread = lpAskQS_data->it;
	DuplicateHandle(GetCurrentProcess(),
			GetCurrentThread(),
			GetCurrentProcess(),
			&(hl),
			DUPLICATE_SAME_ACCESS,
			TRUE,
			DUPLICATE_SAME_ACCESS);
	lpAskQS_data->it = QMTASQL_IDLE_IT;
    }

    while ( 1 ) {

	if( lpAskQS_data->it == QMTASQL_STOP_IT ) {
		lpAskQS_data->it = QMTASQL_IDLE_IT;
		CloseHandle(hl);
		_endthreadex(0);
	}

	if( lpAskQS_data->FileName != NULL )
	{
	    _try {
		//get start time
		//PostThreadMessage(lpAskQS_data->callThreadId, ASQL_STARTTASK_MSG, 1, 1998);

		/*reserved!!!!!
		GetThreadTimes(asqlTaskThread[intOfThread], &ftDummy, &ftDummy, \
					    &ftKernelTimeStart, &ftUserTimeStart);
		i64KernelTimeStart[intOfThread] = FileTimeToQuadWord(&ftKernelTimeStart);
		i64UserTimeStart[intOfThread] = FileTimeToQuadWord(&ftUserTimeStart);
		*/

		strZcpy(asqlEnv.szAsqlFromPath, lpAskQS_data->szAsqlFromPath, MAXPATH);
		strZcpy(asqlEnv.szAsqlResultPath, lpAskQS_data->szAsqlResultPath, MAXPATH);
		strZcpy(asqlEnv.szUser, lpAskQS_data->szUser, 32);


		strZcpy(SysVar[SysVar_ASQLUSER].values, lpAskQS_data->szUser, 32);

		//1998.11.24
		//support _ASQLTO in xexp.c
		*(long *)SysVar[SysVar_ASQLTO].values = (long)fFrTo.cTargetFileName;
		//indirect get data, so MAXPATH(256B) long
		SysVar[SysVar_ASQLTO].length = MAXPATH;

		//1999.5.16
		//support _XERROR _QSERROR in xexp.c
		*(long *)SysVar[SysVar_XERROR].values = (long)&(ErrorSet.xERROR);
		*(long *)SysVar[SysVar_QSERROR].values = (long)&qsError;


		treeSvrTaskId = lpAskQS_data->it;
		callThreadId = lpAskQS_data->callThreadId;
		wmtExbuf = (unsigned long)lpAskQS_data->wmtExbuf;

		/*{
		    char buf[256];
		    sprintf(buf, "thread %d\n%s", intOfThread, lpAskQS_data->FileName);
		    MessageBox(NULL, buf, "tell", MB_OK);
		}*/
		/*{
			int i;
			for( i = 0;  i < 12345;  i++);
		}*/

		AskQS(lpAskQS_data->FileName, lpAskQS_data->ConditionType, \
						  //AsqlExprInFile|Asql_USEENV,
					NULL, //lpAskQS_data->FromTable,
					NULL, //lpAskQS_data->ToTable,
					lpAskQS_data->VariableRegister,
					lpAskQS_data->VariableNum);

		if( qsError == 0 ) {
		    lpAskQS_data->qsError=qsError;
		    strcpy(buf, "ERR:0");
		} else {
		    sprintf(buf, "ERR:%d INF:%s\n%s\nXEXP:%s", \
					       lpAskQS_data->qsError=qsError, \
					       AsqlErrorMes(), szAsqlErrBuf, \
					       GetErrorMes( GeterrorNo() ) );
		}

		strZcpy(lpAskQS_data->szRet, buf, 256);
	    }
	    _except( EXCEPTION_EXECUTE_HANDLER ) {
		lpAskQS_data->qsError = (unsigned short)0xFFFF;
		strcpy(lpAskQS_data->szRet, "ASQL_SERVER_ERROR");
		//Sleep( 0 );
	    }

	    /*if( qsError >= 9001 )
	    {
		int i;
		char buf[1024];

		itoa(qsError, buf, 10);
		for(i = 0;  i < _DioOpenFileNum_;  i++)
		{
		      strcat(buf, _DioOpenFile_[i].df->name);
		      strcat(buf, "\n");
		}
		MessageBox(NULL, buf, "error", MB_OK);
	    }*/

	    //_finally {
		lpAskQS_data->FileName = NULL;
		//}

		/*
		 * send back the variable table to client
		 */
		if( lpAskQS_data->VariableRegister != NULL && \
					   *(lpAskQS_data->VariableNum) > 0 )
		{
		int 	        ii;
		//int 	        step;
		int             sendMems;
		//char 	        szBuffer[4096];
		TS_COM_PROPS    tsComProps;
		SysVarOFunType *vars = *(lpAskQS_data->VariableRegister);

		sendMems = sizeof(SysVarOFunType) * (*(lpAskQS_data->VariableNum));
		for( ii = 0;   ii < *(lpAskQS_data->VariableNum);  ii++)
		{
			if( vars[ii].length >= 32 ) {
			sendMems += vars[ii].length;
			}
		}

		//lptsComProps = (TS_COM_PROPS *)szBuffer;

		//step = (4096-sizeof(TS_COM_PROPS)) / sizeof(SysVarOFunType);
		//ZeroMemory(szBuffer, 4096);

		tsComProps.packetType = 'R';
		tsComProps.msgType = 'V';
		tsComProps.lp = (*(lpAskQS_data->VariableNum) << 24) | sendMems;
		tsComProps.leftPacket = '\1';
		tsComProps.endPacket = '\1';

		SrvWriteExchngBufEx(lpAskQS_data->wmtExbuf,
					&tsComProps,
					vars,
					sendMems,
					FALSE);

		/*for( ii = 0;  ii+step < lptsComProps->lp;  ii += step )
		{
			lptsComProps->len = sizeof(SysVarOFunType) * step;

			memcpy(&szBuffer[sizeof(TS_COM_PROPS)], \
				   &((*(lpAskQS_data->VariableRegister))[ii]), \
				   lptsComProps->len);

			SrvWriteExchngBuf(lpAskQS_data->wmtExbuf, szBuffer, \
					  lptsComProps->len+sizeof(TS_COM_PROPS));
		}

		lptsComProps->leftPacket = '\0';
		lptsComProps->len = sizeof(SysVarOFunType) * \
					  (*(lpAskQS_data->VariableNum)-ii);
		memcpy(&szBuffer[sizeof(TS_COM_PROPS)], \
				   &((*(lpAskQS_data->VariableRegister))[ii]), \
				   lptsComProps->len);

		SrvWriteExchngBuf(lpAskQS_data->wmtExbuf, szBuffer, \
				      lptsComProps->len+sizeof(TS_COM_PROPS));
		*/
	    } /* pay attention to the address of the variable
	       */


	    if( lpAskQS_data->it > 0 ) {
		TS_COM_PROPS tscp;
		char  lpsz[4096];

		memset(&tscp, 0, sizeof(TS_COM_PROPS));
		tscp.packetType = 'R';
		tscp.msgType = 'D';
		tscp.len = min(4096-sizeof(TS_COM_PROPS), strlen(buf)+1);
		tscp.lp = 0;
		memcpy(lpsz, &tscp, sizeof(TS_COM_PROPS));
		strZcpy(&lpsz[sizeof(TS_COM_PROPS)], buf, tscp.len);
		SrvWriteExchngBuf(lpAskQS_data->wmtExbuf, lpsz, tscp.len+sizeof(TS_COM_PROPS));
	    }

	    PostThreadMessage(lpAskQS_data->callThreadId, ASQL_ENDTASK_MSG, lpAskQS_data->qsError, 0);

	    lpAskQS_data->it = QMTASQL_IDLE_IT;

	    //release engine
	    ReleaseSemaphore( hAsqlResSem, 1, NULL );
	}

	{
	   MSG msg1;
	   GetMessage(&msg1, NULL, ASQL_STARTTASK_MSG, ASQL_STARTTASK_MSG);
	}
	//1998.3.8 Xilong
	//SuspendThread(hl);
    }

    CloseHandle(hl);
    return  0;

} //end of askQS_tm()

/*short  mAskQS(char *FileName, short ConditionType, dFILE *FromTable, \
		    dFILE *ToTable, SysVarOFunType **VariableRegister, \
							short *VariableNum )
{
    printf("Runing:--> %s\n", FileName);
	dioTest(FileName);
	return  0;
}*/



/*
 *
 ===================FileTimeToQuadWord()================================*/
_int64 FileTimeToQuadWord(PFILETIME pFileTime)
{
    _int64 qw;

    qw = pFileTime->dwHighDateTime;
    qw <<= 32;
    qw |= pFileTime->dwLowDateTime;

    return  qw;

} //end of FileTimeToQuadWord()


/*
 *
 ===================QuadWordToFileTime()================================*/
PFILETIME QuadWordToFileTime(_int64 qw, PFILETIME pFileTime)
{
    pFileTime->dwHighDateTime = (DWORD)(qw>>32);
    pFileTime->dwLowDateTime = (DWORD)(qw&0xFFFFFFFF);

    return pFileTime;

}//end of QuadWordToFileTime()


/*
 *
 ===================threadRuningTime()==================================*/
_int64 threadRuningTime(int it)
{
    FILETIME ftKernelTimeEnd;
    FILETIME ftUserTimeEnd;
    FILETIME ftDummy;
    _int64 qwKernelTimeElapsed, qwUserTimeElapsed, qwTotalTimeElapsed;

    //get end time
    GetThreadTimes(asqlTaskThread[it], &ftDummy, &ftDummy, \
										&ftKernelTimeEnd, &ftUserTimeEnd);

    //get the elapsed kernel and user time by convverting the start and end
    //times from FILETIMEs to quad words, and then subtract the start time
    //from the end times
    qwKernelTimeElapsed = FileTimeToQuadWord(&ftKernelTimeEnd) - \
						  i64KernelTimeStart[it];
    qwUserTimeElapsed = FileTimeToQuadWord(&ftUserTimeEnd) - \
						  i64UserTimeStart[it];
                          

    qwTotalTimeElapsed = qwKernelTimeElapsed + qwUserTimeElapsed;

    return  qwTotalTimeElapsed;

} //end of threadRuningTime()



/*
 *
 ===================threadElapsedTime()==================================*/
DWORD threadElapseTime(int it)
{
    return  GetTickCount() - dwThreadTimeStart[it];
} //end of threadElapsedTime()



#ifdef _TEST_
char *buf[10];

int main(int argc, char *argv[] )
{
    int i = 0;
	int j, k = 1;
	dFILE *df, *df1, *df2;
        bHEAD *bh;
	dFIELD field[2];
	char   buff[40960];
	long   l, ll;
	char   *s;
	_int64 i64;

	HANDLE hsem;
	TS_COM_PROPS tscp1;
extern RJFILE   *rjFile;

char *locateKeywordInBuf(char *sz, char *szkey, char *szFollowKey, int ibufsize);
int FileCompressToSend(LPSTR lpFileName, void *lpExchBufInfo);

	DWORD First, Last, Time;

	char *spp = "    thi is a test         ";
	char *s2;



	MEMORYSTATUS lpm;
	GlobalMemoryStatus(&lpm);

	i64 = 0x8000000000000000; //
/*
	bh = IndexOpen("", "lz0_1.ndx", BTREE_FOR_ITSELF);
	IndexGoTop(bh);
	while( !IndexEof(bh) ) {
		s = IndexGetKeyContent(bh);
		printf("%s\n", s);
		IndexSkip(bh, 1);
	}
	s = IndexGetKeyContent(bh);
		printf("%s\n", s);
	IndexClose(bh);

	DES("online1", "online1", buff);
	i = strcmp(buff, "\x90\x91\x93\x96\x91\x9a");
*/

//	dbftotal("g:\\lwf\\dbf\\1.dbf", "g:\\lwf\\dbf\\2.dbf");

        //FileCompressToSend("e:\\wmtasql\\cal.txt",NULL);


//	s = locateKeywordInBuf("shang hai tie dao", "dao", buff, 4096);

	//s2 = lrtrim(spp);
	//printf("%s\n", s2);

//	goto DIO_TEST;
//	goto BUFF_TEST;
	
	//InitOdbcAccess(1, "ORACLE_DB", "P04", "P04");
/*	First = GetTickCount();
	printf( "%ld\n", First );

	for( i = 0; i < 1000; i++ )
	{
	    //char buf[128];

	    //printf( "----- %d -----\n", i );
	    //sprintf( buf, "insert into cbg.cbg1 values('Test %d')", i );
	    odbcExecute( "insert into cbg.cbg1 values('Test')" );
	}

	Last = GetTickCount();
	printf( "%ld\n", Last );

	Time = ( Last - First ) / 1000;

	printf( "%ld\n", Time );

	CloseOdbcAccess();

	return  0;



	l = 0;
	df = dopen("odbc:oracle_db,P04,P04,SELECT * FROM P_YC", DOPENPARA);
	while( deof(df) && l < 10 )
	{
		l++;
		if( getrec(df) == NULL )
			break;
		printf("%s\n", &(df->rec_buf[1]));
	}
	dclose(df);
	return  0;
*/

	/*
	tscp1.packetType = 'Q';
	tscp1.msgType = 'Q';
	tscp1.len = strlen("lz0.dbf");
	tscp1.lp = cmTS_OPEN_DBF;

	memcpy(buff, &tscp1, sizeof(TS_COM_PROPS));
	memcpy(buff+sizeof(TS_COM_PROPS), "lz0.dbf", 8);
	*/

/////////////
	/*
	for(i = 0;  i < 1; i++) {
	    printf("begin %d\n", i);
	strcpy(buff, "tgask");
	justRunASQL(buff, buff, 4096);
	printf("finish %d\n", i);
	}
        return  0;
	*/
/////////////

	/*HWND	hh;

	hh = FindWindowEx( NULL,NULL, "MS-DOS 方式", NULL);
	printf("MS_DOS:%ld\n", hh);
	*/

/*	printf("begin\n");
	bh=dbTreeOpen("e:\\btree\\ndxname");
	dbTreeClose(bh);


	hLibXexp = LoadLibrary("e:\\wmtasql\\xexpdll_\\xexpdll.dll");
	if( hLibXexp == NULL ) {
		printf("DLL error %ld\n", GetLastError());
	}
*/
	//rjFile = openRJ( "RJ" );
	//rjFile = NULL;
//	odbcConnect("oracle_db", "p04", "p04");
	/*s = WordAnalyse("1=2");
	CalExpr(s);
	i = GetCurrentResultType();
	printf("%d\n", i);
	FreeCode(s);
*/
/*================================*/
	/*
	ProcessAttach();
	hyDataCollect("pei", "e:\\tg\\fx\\", "Song Hui 1998 09 04", "pei", NULL, NULL, NULL);
	ProcessDetach();
	return  0;
	*/
/**/
	csuDataDictionary = OpenCfgFile( "e:\\wmtasql\\asqlwmt.cfg" );
	//s = GetCfgKey(csuDataDictionary, "SYSTEM", "DBROOT", "PATHabc");
	//printf(s);
	
	i = 4096;
	//enumItemInCache( csuDataDictionary,               \
	//			     "$",  "1",            \
	//			     NULL,   		  \
	//			     buff,                  \
	//			     &i);
	//printf(buff);

	//exit(0);
//	i = hOpen("e:\\wmtasql\\code.cod");
/*
	{
	    CODE_STRUCT CodeStruct;
	    CodeStruct.CodeNo = 0xFFFF;

	    hGetCodeRecById( 12, 0, "", &CodeStruct, 'M');
	    printf("%s\n", CodeStruct.Chinese);
	    hGetCodeRecById( 12, 1, "", &CodeStruct, 'M');
	    printf("%s\n", CodeStruct.Chinese);
	}*/
	//memset(buff, ' ', 40960);
	//buff[19959] = '\0';
	//i = 1;
	

/*-=-=*/
	s = "to cx,a 10 G, b 10 0 C\r"
	    "cond \r"
	    "begin\r"
	    "end";

				     

	if( argc < 2 )
	{
	    printf("\nUsage:\n");
	    printf("    wmtasql <asqlScriptFileName>\n\n");
	    printf("use default script file: tgask\n");
	    s=callAsqlSvr("tgask", NULL, NULL);
	} else {
	    s=callAsqlSvr(argv[1], NULL, NULL);
	}

	//closeRJ( rjFile );
	//rjFile = NULL;

	//hClose();
	CloseCfgFile( csuDataDictionary );
	printf("%s\n", SysVar[0].values);

	printf("%s\n", s);
	return  0;
/*-=-=*/
	
	InitializeCriticalSection( &dofnCE );
	df = dAwake("perchet.dbf", DOPENPARA);
	dfcopy(df, buff);
	return  0;

	df1 = dAwake("sub00.vew", DOPENPARA);
	df2 = dAwake("tgview.dbf", DOPENPARA);
	
	s=callAsqlSvr("tgask", NULL, NULL);
	printf("%s\n", s);

	//dseek(df1,0,dSEEK_SET);
	//memset(df1->rec_buf, ' ', df1->rec_len);
	//put1rec(df1);

	dSleep(df);
	dSleep(df1);
	dSleep(df2);
	return  0;

DIO_TEST:
	//df = dopen("odbc:oracle_db,p04,p04,select * from z_ycitem", DOPENPARA);
	InitializeCriticalSection( &dofnCE );
	dsetbuf(40000);
	
	test();
	exit(0);

	df = dAwake("e:\\wmtasql\\test.dbf", DOPENPARA);
	dseek(df,0,dSEEK_SET);
	for(i=0; i < 100;  i++) {
	    sprintf(&(df->rec_buf[1]), "%d", i);
	    putrec(df);
	}
	dSleep(df);
	exit(0);


	csuDataDictionary = OpenCfgFile( "asqlwmt.cfg" );
	InitializeCriticalSection( &dofnCE );
	df = dAwake("e:\\wmtasql\\p_yc.dbf", DOPENPARA);
	if( df ==  NULL )
	    return  1;

	putrec(df);

	return  0;

	do {
	    printf( "%ld -> ", df->rec_p );
	    if( getrec(df) == NULL )
		break;

	    df->rec_buf[0] = ' ';
	    printf("%s\n", substr(df->rec_buf, 1, 60));
	} while( 1 );
	dclose(df);
	CloseCfgFile( csuDataDictionary );
	return  0;



	strncpy(buff, "abcdefg", 10);
	strncpy(buff, "abcdefg", 3);

	ProcessAttach();
//	field[0] = "BH";
//	field[1] = NULL;
	bh = IndexBuild("lz0.dbf", field, "lz0", BTREE_FOR_CLOSEDBF);
	IndexClose(bh);
	ProcessDetach();
	return  0;

//dcreate test =============================
	InitializeCriticalSection( &dofnCE );
	strcpy(field[0].field, "test");
	field[0].fieldtype = 'C';
	field[0].fieldlen = 10;
	field[0].fielddec = 0;
	field[1].field[0] = '\0';
	df = dcreate("test", field);

	i = dIsAwake("test");

	dSetAwake(df, &df);

	i = dIsAwake("test");
	
	strcpy(field[0].field, "tt");
	df1 = dcreate("test", field);
	dSetAwake(df1, &df1);

	dSleep(df);
	dSleep(df1);

	return  0;

//dcreate test =============================-----------------

//memo field test====================================
BUFF_TEST:
	ProcessAttach();

	df = dAwake("e:\\wmtasql\\memotest.dbf", DOPENPARA);
	/*get1rec(df);
	dbtFromFile(df, 1, "e:\\wmtasql\\a1");
	put_fld(df, 0, "1");
	putrec(df);
*/
	_try {
	dseek(df, 0, dSEEK_SET);
	get1rec(df);
	strcpy(buff, "Shanghai Tiedao University");
	for( i = 0 ;  i < 1000;  i++) {
	    strcat(buff, " Shanghai Tiedao University");
	}

	blobFromMem(df, 1, buff, 40960);
	{
	    long memSize;
	    char *sp;
	    sp = blobToMem(df, 1, &memSize);
	    printf("%s %d\n", sp, memSize);
	    freeBlobMem(sp);
	}
	dSleep(df);
	}

_except( EXCEPTION_EXECUTE_HANDLER ) {
		strcpy(pubBuf, "ASQL_SERVER_ERROR");
    }
	    
	ProcessDetach();

	return  0;

//memo field test====================================
#ifdef XXX
/*	dbtFromFile(df, 1, "e:\\wmtasql\\debug\\a2");
	put_fld(df, 0, "2");
	putrec(df);

	get1rec(df);
	dbtFromFile(df, 1, "e:\\wmtasql\\debug\\a3");
	put_fld(df, 0, "3");
	putrec(df);

	dseek(df, 0, dSEEK_SET);
	get1rec(df);
	dbtFromFile(df, 1, "e:\\wmtasql\\debug\\aa.c");
	put_fld(df, 0, "2");
	put1rec(df);
	dbtToFile(df, 1, "e:\\wmtasql\\debug\\bb.c");
	dclose(df);
	return  2;

	strcpy(field[0].field,"ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	field[0].fieldtype = 'C';
	field[0].fieldlen = 10;
	field[0].fielddec = 0;
	field[1].field[0] = '\0';

	hBuildCodeLib( "d:\\code\\code.dbf" );
	//return 0;

	ProcessAttach();
	df = dopen("e:\\wmtasql\\ls.dbf", DOPENPARA);
	dseek(df, 10, dSEEK_SET);
	RecDelete(df);
	dseek(df, 18, dSEEK_SET);
	RecDelete(df);
	dpack(df);
	dclose(df);
	ProcessDetach();
	return  0;



	ProcessAttach();
	tscp1.packetType = 'Q';
	tscp1.msgType = 'Q';
	tscp1.len = strlen("lz0.dbf");
	tscp1.lp = cmTS_OPEN_DBF;

	memcpy(buff, &tscp1, sizeof(TS_COM_PROPS));
	memcpy(buff+sizeof(TS_COM_PROPS), "lz0.dbf", 8);

	justRunASQL(buff, buff, 4096);
	ll = l = ((TS_COM_PROPS *)buff)->lp;
	
	{
	    int i;
	    dFILE *df = (dFILE *)ll;

	    i = open("DBF.DBF", O_RDWR|O_CREAT|O_TRUNC|O_BINARY|SH_DENYWR, S_IWRITE);
	    if( i > 0 ) {
		write(i, buff+sizeof(TS_COM_PROPS), df->headlen);
		close(i);
	    }
	}


	tscp1.packetType = 'Q';
	tscp1.msgType = 'Q';
	tscp1.len = 8;
	tscp1.lp = cmTS_REC_FETCH;

	for(l = 1;  l < 2;  l++) {
		int i;
		dFILE *df = (dFILE *)ll;

		memcpy(buff, &tscp1, sizeof(TS_COM_PROPS));
		memcpy(buff+sizeof(TS_COM_PROPS), &ll, 4);
		memcpy(buff+sizeof(TS_COM_PROPS)+4, &l, 4);
		justRunASQL(buff, buff, 4096);

		i = open("DBF.DBF", O_RDWR|O_BINARY|SH_DENYWR, S_IWRITE);
		if( i > 0 ) {
			lseek(i, 0, SEEK_END);
			write(i, buff+sizeof(TS_COM_PROPS)+sizeof(RECPROPS), df->rec_len);
			close(i);
		}
	}
	
	tscp1.packetType = 'Q';
	tscp1.msgType = 'Q';
	tscp1.len = 8;
	tscp1.lp = cmTS_REC_DEL;
	memcpy(buff, &tscp1, sizeof(TS_COM_PROPS));
	memcpy(buff+sizeof(TS_COM_PROPS), &ll, 4);
	l = 1;
	memcpy(buff+sizeof(TS_COM_PROPS)+4, &l, 4);
	justRunASQL(buff, buff, 4096);

	tscp1.packetType = 'Q';
	tscp1.msgType = 'Q';
	tscp1.len = 8;
	tscp1.lp = cmTS_BLOB_PUT;
	memcpy(buff, &tscp1, sizeof(TS_COM_PROPS));
	memcpy(buff+sizeof(TS_COM_PROPS), &ll, 4);
	l = 1;
	memcpy(buff+sizeof(TS_COM_PROPS)+4, &l, 4);
	memcpy(buff+sizeof(TS_COM_PROPS)+8, "memo", 5);
	justRunASQL(buff, buff, 4096);


	
	tscp1.packetType = 'Q';
	tscp1.msgType = 'Q';
	tscp1.len = 8;
	tscp1.lp = cmTS_REC_PUT;

	memcpy(buff, &tscp1, sizeof(TS_COM_PROPS));
	memcpy(buff+sizeof(TS_COM_PROPS), &ll, 4);
	l = 1;
	memcpy(buff+sizeof(TS_COM_PROPS)+4, &l, 4);
	memcpy(buff+sizeof(TS_COM_PROPS)+8, "people's republic of china", 27);
	justRunASQL(buff, buff, 4096);
	
	tscp1.packetType = 'Q';
	tscp1.msgType = 'Q';
	tscp1.len = 8;
	tscp1.lp = cmTS_BLOB_FETCH;
	memcpy(buff, &tscp1, sizeof(TS_COM_PROPS));
	memcpy(buff+sizeof(TS_COM_PROPS), &ll, 4);
	l = 1;
	memcpy(buff+sizeof(TS_COM_PROPS)+4, &l, 4);
	memcpy(buff+sizeof(TS_COM_PROPS)+8, "memo", 5);
	justRunASQL(buff, buff, 4096);


	tscp1.packetType = 'Q';
	tscp1.msgType = 'Q';
	tscp1.len = 8;
	tscp1.lp = cmTS_CLOSE_DBF;
	memcpy(buff, &tscp1, sizeof(TS_COM_PROPS));
	memcpy(buff+sizeof(TS_COM_PROPS), &ll, 4);
	justRunASQL(buff, buff, 4096);

	return  0;
	buf[0] = "thread0 ";
	buf[1] = "thread1 ";
	buf[2] = "thread2 ";
	buf[3] = "thread3 ";
	buf[4] = "thread4 ";
	buf[5] = "thread5 ";
	buf[6] = "thread6 ";


	ProcessAttach();
	printf("init success\n");

	hsem = CreateSemaphore(NULL, 1, 1, NULL);
	j = wmtAskQS(1234, "Tgask", AsqlExprInFile|Asql_USEENV, &asqlEnv, NULL, NULL, NULL);
	if( j >= asqlTaskThreadNum )
		printf("no thread to service\n");
	else
		printf("thread %d is servicing\n", j);


    {
	MSG msg1;

	while( 1 ) {
	    GetMessage(&msg1, NULL, 0, 65535);
	    printf("message received: %d\n", msg1.message);
	    if( msg1.message == ASQL_ENDTASK_MSG ) {
		break;
	    }
	}
	ReleaseSemaphore(hsem, 1, NULL);
    }
    CloseHandle(hsem);

	printf("asql finish. return value %s", buff);
	
	ProcessDetach();
	return  0;
*/
#endif
}

#endif

/*void dioTest(char *s)
{
	dFILE *df;
	char buffer[80];

	if(( df = dAwake( "d:\\wmtasql\\lz00", DOPENPARA )) == NULL ) {
		printf( "Error open file\n" );
		return;
	}

	dseek( df, 0, dSEEK_SET );
	while( !deof( df )) {
		getrec( df );
		Sleep(200);
		get_fld ( df, 0, buffer );
		if( (strnicmp(s, "thread1", 7) == 0) || \
			(strnicmp(s, "thread3", 7) == 0) || \
			(strnicmp(s, "thread5", 7) == 0) )
			printf ( "%s   Recn:%ld : %s\n", s, df->rec_no, buffer );
	}
	dSleep( df );
}
*/

/* callAsqlSvr()
 * DLL main entry program
 ****************************************************************************/
_declspec(dllexport) char *callAsqlSvr(char *szFileName, char *szFromPath, char *szToPath)
{
extern WSToMT FromToStru fFrTo;
/*/test
typedef struct {
	short  type;
	char   VarOFunName[32];
	unsigned char values[ MAX_OPND_LENGTH ];

	short length;
} XXX;
	XXX x[300];
	int i = 300;
	void *p;


	x[0].type = 1051;
	strcpy(x[0].VarOFunName, "var");
	x[0].length = 30;
	p = &x;
*/


    if( szFromPath != NULL ) {
	strZcpy(asqlEnv.szAsqlFromPath, szFromPath, MAXPATH);
	beSurePath(asqlEnv.szAsqlFromPath);
    } else
	asqlEnv.szAsqlFromPath[0] = '\0';

    if( szToPath != NULL ) {
	strZcpy(asqlEnv.szAsqlResultPath, szToPath, MAXPATH);
	beSurePath(asqlEnv.szAsqlResultPath);
    } else
	asqlEnv.szAsqlResultPath[0] = '\0';

    __try {

	InitializeCriticalSection( &dofnCE );
	InitializeCriticalSection( &btreeIndexCE );


	//1998.11.24
	//support _ASQLTO in xexp.c
	*(long *)SysVar[SysVar_ASQLTO].values = (long)fFrTo.cTargetFileName;
	//indirect get data, so MAXPATH(256B) long
	SysVar[SysVar_ASQLTO].length = MAXPATH;

	AskQS(szFileName, AsqlExprInFile|Asql_USEENV, \
					NULL, //lpAskQS_data->FromTable,
					NULL, //lpAskQS_data->ToTable,
					NULL, //VariableRegister,
					0);   //VariableNum);

	if( qsError == 0 ) {
	    strcpy(pubBuf, "ERR:0");
	} else {
	    sprintf(pubBuf, "ERR:%d INF:%s\n%s\nXEXP:%s", \
					       qsError, \
					       AsqlErrorMes(), szAsqlErrBuf, \
					       GetErrorMes( GeterrorNo() ) );
	}
    }
    _except( EXCEPTION_EXECUTE_HANDLER ) {
		strcpy(pubBuf, "ASQL_SERVER_ERROR");
    }

    return  pubBuf;

} //end of callAsqlSvr()


/* callAsqlSvrM()
 * DLL main entry program
 ****************************************************************************/
_declspec(dllexport) char *callAsqlSvrM(char *szAsqlScript, char *szFromPath, char *szToPath)
{
    if( szFromPath != NULL )
	strZcpy(asqlEnv.szAsqlFromPath, szFromPath, MAXPATH);
    else
	asqlEnv.szAsqlFromPath[0] = '\0';

    if( szToPath != NULL )
	strZcpy(asqlEnv.szAsqlResultPath, szToPath, MAXPATH);
    else
	asqlEnv.szAsqlResultPath[0] = '\0';

    __try {

	InitializeCriticalSection( &dofnCE );
	InitializeCriticalSection( &btreeIndexCE );

	AskQS(szAsqlScript, AsqlExprInMemory|Asql_USEENV, \
					NULL, //lpAskQS_data->FromTable,
					NULL, //lpAskQS_data->ToTable,
					NULL, //VariableRegister,
					0);   //VariableNum);

	if( qsError == 0 ) {
	    strcpy(pubBuf, "ERR:0");
	} else {
	    sprintf(pubBuf, "ERR:%d INF:%s\n%s\nXEXP:%s", \
					       qsError, \
					       AsqlErrorMes(), szAsqlErrBuf, \
					       GetErrorMes( GeterrorNo() ) );
	}
    }
    _except( EXCEPTION_EXECUTE_HANDLER ) {
		strcpy(pubBuf, "ASQL_SERVER_ERROR");
    }

    return  pubBuf;

} //end of callAsqlSvrM()


/* callAsqlSvrMVar()
 * DLL main entry program
 ****************************************************************************/
_declspec(dllexport) char *callAsqlSvrMVar(char *szAsqlScript, \
					   char *szFromPath, \
					   char *szToPath, \
					   SysVarOFunType *var,
					   int  varnum )
{
    if( szFromPath != NULL )
	strZcpy(asqlEnv.szAsqlFromPath, szFromPath, MAXPATH);
    else
	asqlEnv.szAsqlFromPath[0] = '\0';

    if( szToPath != NULL )
	strZcpy(asqlEnv.szAsqlResultPath, szToPath, MAXPATH);
    else
	asqlEnv.szAsqlResultPath[0] = '\0';

    __try {

	InitializeCriticalSection( &dofnCE );
	InitializeCriticalSection( &btreeIndexCE );

	AskQS(szAsqlScript, AsqlExprInMemory|Asql_USEENV, \
					NULL, //lpAskQS_data->FromTable,
					NULL, //lpAskQS_data->ToTable,
					&var, //VariableRegister,
					(short *)&varnum);   //VariableNum);

	if( qsError == 0 ) {
	    strcpy(pubBuf, "ERR:0");
	} else {
	    sprintf(pubBuf, "ERR:%d INF:%s\n%s\nXEXP:%s", \
					       qsError, \
					       AsqlErrorMes(), szAsqlErrBuf, \
					       GetErrorMes( GeterrorNo() ) );
	}
    }
    _except( EXCEPTION_EXECUTE_HANDLER ) {
		strcpy(pubBuf, "ASQL_SERVER_ERROR");
    }

    return  pubBuf;

} //end of callAsqlSvrMVar()


_declspec(dllexport) DWORD justRunASQL(LPSTR lpszParam, LPSTR lpszResponse, DWORD cbResponse)
{
#ifndef ASQL_NT    
    int j,i;
    char buff[10][512];
    Asql_ENV  asqlEnvSelf;
    HANDLE    hEvent;
    MSG       msg1;
    TS_COM_PROPS *tscp1;
    EXCHNG_BUF_INFO exBi;


    tscp1 = (TS_COM_PROPS *)lpszParam;
    //hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    if( tscp1->packetType == 'Q' && tscp1->msgType == 'Q' ) {

	InitExchngBufInfo(&exBi, lpszResponse, hEvent);

	//quick task
	lpszParam += sizeof(TS_COM_PROPS);
	switch( tscp1->lp ) {
	    case cmTS_OPEN_DBF:
		//open dbf
	    {
		char *sz = strchr(lpszParam, '`');

		InitializeCriticalSection( &dofnCE );
		if( sz != NULL ) {
		    *sz = '\0';
		}
		tsumDefault.tscc = asqlGetDbfDes(NULL, sz, lpszParam, \
					&exBi);
	    }
		break;
	    case cmTS_REC_FETCH:
		//fetchRecord();
		asqlGetDbfRec(tsumDefault.tscc, (dFILE *)*(long *)lpszParam, \
					*(long *)&lpszParam[sizeof(long)], \
					&exBi);
		break;
	    case cmTS_REC_PUT:
		//putrecord();
		asqlPutDbfRec(tsumDefault.tscc, (dFILE *)*(long *)lpszParam, \
					*(long *)&lpszParam[sizeof(long)], \
					&lpszParam[sizeof(long)+sizeof(long)], \
					&exBi);
		break;
	    case cmTS_REC_ADD:
		//putrecord();
		//asqlAddDbfRec(tsumDefault.tscc, (dFILE *)*(long *)lpszParam, \
		//			&exBi);
		break;
	    case cmTS_REC_APP:
		//putrecord();
		asqlAppDbfRec(tsumDefault.tscc, (dFILE *)*(long *)lpszParam, \
					&lpszParam[sizeof(long)], \
					&exBi);
		break;
	    case cmTS_REC_DEL:
		//putrecord();
		asqlDelDbfRec(tsumDefault.tscc, (dFILE *)*(long *)lpszParam, \
					*(long *)&lpszParam[sizeof(long)], \
					&exBi);
		break;
	    case cmTS_BLOB_FETCH:
		asqlBlobFetch(tsumDefault.tscc, "", "", (dFILE *)*(long *)lpszParam,\
					*(long *)&lpszParam[sizeof(long)], \
					(char *)&lpszParam[sizeof(long)*2], \
					&exBi);
		break;
	    case cmTS_BLOB_PUT:
		asqlBlobPut(tsumDefault.tscc, "", "", (dFILE *)*(long *)lpszParam, \
					*(long *)&lpszParam[sizeof(long)], \
					(char *)&lpszParam[sizeof(long)*2], \
					&exBi);
		break;
	    case cmTS_CLOSE_DBF:
		//close dbf
		asqlFreeDbfDes(tsumDefault.tscc, (dFILE *)*(long *)lpszParam, \
					&exBi);
		break;
	    case cmTS_DBF_RECNUM:
		asqlGetDbfRecNum(tsumDefault.tscc, (dFILE *)*(long *)lpszParam, \
					&exBi);
		break;
	}
	return  0;
    } else {

	int kk;

	ProcessAttach();
	strZcpy(asqlEnvSelf.szAsqlFromPath, asqlConfigEnv.szAsqlFromPath, MAXPATH);
	asqlEnvSelf.szAsqlResultPath[0] = '\0';
	asqlEnvSelf.szUser[0] = '\0';

	for(kk = 0; kk < 1; kk++) {

printf("\ntask %d\n", kk);

	j = wmtAskQS(1234, lpszParam, AsqlExprInFile|Asql_USEENV, &asqlEnv, NULL, NULL, NULL);
	if( j >= asqlTaskThreadNum ) {
	    printf("error\n");
	    //SetEvent(hEvent);
	    //CloseHandle(hEvent);
	    return  1;
	}

	for(i=0; i< 0;  i++){
	    sprintf(buff[i], "tgask%d", i+1);
	j = wmtAskQS(1235+i, buff[i], AsqlExprInFile|Asql_USEENV, &asqlEnv, NULL, NULL, NULL);
	if( j >= asqlTaskThreadNum ) {
	    printf("error\n");
	    //SetEvent(hEvent);
	    //CloseHandle(hEvent);
	    return  1;
	}
	}

	for(i = 0;  i < 1;  i++)
	while( 1 ) {
printf("wait...  ");
	    GetMessage(&msg1, NULL, ASQL_ENDTASK_MSG, ASQL_ENDTASK_MSG);
	    if( msg1.message == ASQL_ENDTASK_MSG )
		break;
	    //SetEvent(hEvent);
	}

	//SetEvent(hEvent);
    }
    }
    //CloseHandle(hEvent);


    if( lpszResponse != NULL ) {
	strZcpy(lpszResponse, buff[0], cbResponse);
    }

    ProcessDetach();

    return  askQS_data[j].qsError;
#else
    return  0;
#endif
} //end of justRunASQL()




#ifdef ASQL_NT
BOOL WINAPI DllMain(HINSTANCE hinsrDll, DWORD fdwReason, LPVOID lpvReserved) 
{
    BOOL fOK = TRUE;
    switch( fdwReason) {
	case DLL_PROCESS_ATTACH:
	    //DLL is attaching to the address space of current process.
	    //Increment this module's usage count when it is attached to a process

	    //initialize  CriticalSection for DIO
	    InitializeCriticalSection( &dofnCE );
	    InitializeCriticalSection( &btreeIndexCE );
	    InitializeCriticalSection( &g_CS_ThreadMan );

	    _BtreeOpenNum_ = 0;
	    _DioOpenFileNum_ = 0;

	break;
	case DLL_THREAD_ATTACH:
	    //A new thread is being created in the current process
	break;
	case DLL_THREAD_DETACH:
	    //A thread is exiting cleanly.
	break;
	case DLL_PROCESS_DETACH:
	    //The calling process is detaching the DLL from its address space.
	break;
    }
    return  fOK;
} //end of DllMain()


/* asqlwmt()
 * DLL main entry program
 * id is the task id
 *************************************************************************/
_declspec(dllexport) DWORD asqlwmt(long id, LPCSTR lpszUser, LPCSTR lpszUserDir,\
				   LPSTR lpszParam, \
				   char  taskType, \
				   EXCHNG_BUF_INFO *exbuf)
{
    TREESVR_PARA *tps;
    //char  szCn[32];
    int   i, j, m;
    char  szTgask[MAXPATH];
    Asql_ENV     asqlEnvSelf;
    TS_COM_PROPS tscp;
    char  buf[4096];
typedef struct tagESERVICEPACKET {
    LPSTR asqlScript;
    short varNum;
    SysVarOFunType *vars;
} ESERVICEPACKET;

    memset(&tscp, 0, sizeof(TS_COM_PROPS));

    /*if( lpszParam == NULL ) {
	tscp.packetType = 'R';
	tscp.msgType = 'E';
	tscp.len = 4096-sizeof(TS_COM_PROPS);
	tscp.lp = 1;
	memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	strcpy(&buf[sizeof(TS_COM_PROPS)], "ERR:2 INF:param error");
	SrvWriteExchngBuf(exbuf, buf, 4096);
	return  1;
    }*/
    if( id < 0 ) {
	tscp.packetType = 'R';
	tscp.msgType = 'E';
	tscp.len = 4096-sizeof(TS_COM_PROPS);
	tscp.lp = 2;
	memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	strcpy(&buf[sizeof(TS_COM_PROPS)], "ERR:2 INF:param error");
	SrvWriteExchngBuf(exbuf, buf, 4096);
	return  2;
    }

#ifndef ASQLCLT
    if( lServerAsRunning ) {
	/*i = 32;
	GetComputerName(szCn, &i);
	i = MAXPATH;
	//if( GetUserHomeDir(szCn, lpszUser, buf, &i) != 0 )
	if( GetUserHomeDir(NULL, lpszUser, buf, &i) != 0 )
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

	strZcpy(buf, lpszUserDir, 260);
	beSurePath(buf);
    } else
#endif
    {
	buf[0] = '\0';
    }

    strZcpy(asqlEnvSelf.szUser, lpszUser, 32);
    switch( taskType )
    {
      case 'A':
	strZcpy(asqlEnvSelf.szAsqlFromPath, asqlConfigEnv.szAsqlFromPath, MAXPATH);
	strZcpy(asqlEnvSelf.szAsqlResultPath, buf, MAXPATH);
	j = wmtAskQS(id, lpszParam, AsqlExprInMemory|Asql_USEENV, &asqlEnvSelf, \
		     NULL, NULL, exbuf);
	if( j >= asqlTaskThreadNum ) {
	    tscp.packetType = 'R';
	    tscp.msgType = 'E';
	    tscp.len = 4096-sizeof(TS_COM_PROPS);
	    tscp.lp = 3;
	    memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	    strcpy(&buf[sizeof(TS_COM_PROPS)], "ERR:3 INF:No Thread service");
	    SrvWriteExchngBuf(exbuf, buf, 4096);
	    return  3;
	}
	break;
      case 'B':
	//use default database root path and user's result path
	strZcpy(asqlEnvSelf.szAsqlFromPath, asqlConfigEnv.szAsqlFromPath, MAXPATH);
	strZcpy(asqlEnvSelf.szAsqlResultPath, buf, MAXPATH);
	strZcpy(szTgask, buf, MAXPATH);
	strcat(szTgask, lpszParam);
	j = wmtAskQS(id, szTgask, AsqlExprInFile|Asql_USEENV, &asqlEnvSelf, \
		     NULL, NULL, exbuf);
	if( j >= asqlTaskThreadNum ) {
	    tscp.packetType = 'R';
	    tscp.msgType = 'E';
	    tscp.len = 4096-sizeof(TS_COM_PROPS);
	    tscp.lp = 3;
	    memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	    strcpy(&buf[sizeof(TS_COM_PROPS)], "ERR:3 INF:No Thread service");
	    SrvWriteExchngBuf(exbuf, buf, 4096);
	    return  3;
	}
	break;
      case 'C':
	tps = getTreeSvrPara(strlen(lpszParam), lpszParam);

	/*sz = getUsrWorkDir(pcMark, usr, pwd, usrWorkDir);
	if( sz == NULL )
		return  1;
	*/
	//use default database root path and user's result path
	strZcpy(asqlEnvSelf.szAsqlFromPath, asqlConfigEnv.szAsqlFromPath, MAXPATH);
	strZcpy(asqlEnvSelf.szAsqlResultPath, buf, MAXPATH);

	/*wmtAskQS(long it, char *FileName, short ConditionType, dFILE *FromTable, \
		    dFILE *ToTable, SysVarOFunType **VariableRegister, \
							short *VariableNum )*/
	i = lookTSPara(tps, "CMD");
	m = countTSPara(tps);
	if( i >= 0 ) {
	    strZcpy(szTgask, buf, MAXPATH);
	    strcat(szTgask, tps[i].value);
	    j = wmtAskQS(id, szTgask, AsqlExprInFile|Asql_USEENV, &asqlEnvSelf, \
			 (SysVarOFunType **)tps, (short *)&m, exbuf);
	    if( j >= asqlTaskThreadNum ) {
		free(tps);
		tscp.packetType = 'R';
		tscp.msgType = 'E';
		tscp.len = 4096-sizeof(TS_COM_PROPS);
		tscp.lp = 3;
		memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
		strcpy(&buf[sizeof(TS_COM_PROPS)], "ERR:3 INF:No Thread service");
		SrvWriteExchngBuf(exbuf, buf, 4096);
		return  3;
	    }
	}
	free(tps);
	break;
      case 'D':
      {
	char *sz;
	long  l;

	sz = strchr(lpszParam, '`');
	if( sz != NULL ) {
	    *sz = '\0';
	    sz++;
	} else {
	    sz = NULL;
	}
	if( (l=hyDataCollect(lpszUser, lpszParam, sz, NULL, (short)0, exbuf)) < 0 ) {
	    tscp.packetType = 'R';
	    tscp.msgType = 'E';
	    tscp.len = 4096-sizeof(TS_COM_PROPS);
	    tscp.lp = 3;
	    memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	    strcpy(&buf[sizeof(TS_COM_PROPS)], "ERR:3 INF:No Thread service");
	    SrvWriteExchngBuf(exbuf, buf, 4096);
	    return  8;
	}
	tscp.packetType = 'R';
	tscp.msgType = 'D';
	tscp.len = 4096-sizeof(TS_COM_PROPS);
	tscp.lp = 3;
	memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	sprintf(&buf[sizeof(TS_COM_PROPS)], "ERR:0 INF:STAT_ID=%d ", l);
	SrvWriteExchngBuf(exbuf, buf, 4096);
	return  3;
      }
	break;

      case 'E':
      // call asql with symbol table
      //
	strZcpy(asqlEnvSelf.szAsqlFromPath, asqlConfigEnv.szAsqlFromPath, MAXPATH);
	strZcpy(asqlEnvSelf.szAsqlResultPath, buf, MAXPATH);

	j = wmtAskQS(id, ((ESERVICEPACKET *)lpszParam)->asqlScript, \
			 AsqlExprInMemory|Asql_USEENV, \
			 &asqlEnvSelf, \
			 &(((ESERVICEPACKET *)lpszParam)->vars), \
			 &(((ESERVICEPACKET *)lpszParam)->varNum), \
			 exbuf);

	if( j >= asqlTaskThreadNum ) {
	    tscp.packetType = 'R';
	    tscp.msgType = 'E';
	    tscp.len = 4096-sizeof(TS_COM_PROPS);
	    tscp.lp = 3;
	    memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	    strcpy(&buf[sizeof(TS_COM_PROPS)], "ERR:3 INF:No Thread service");
	    SrvWriteExchngBuf(exbuf, buf, 4096);
	    return  3;
	}
	break;

      default:
	//error
	tscp.packetType = 'R';
	tscp.msgType = 'E';
	tscp.len = 4096-sizeof(TS_COM_PROPS);
	tscp.lp = 5;
	memcpy(buf, &tscp, sizeof(TS_COM_PROPS));
	strcpy(&buf[sizeof(TS_COM_PROPS)], "ERR:5 INF:param error");
	SrvWriteExchngBuf(exbuf, buf, 4096);
	return  5;
    }

    return  j + 10000;

} //end of asqlwmt()

#endif //ASQL_NT






/* asqlidle()
 * DLL main entry program
 *
 *************************************************************************/
_declspec(dllexport) long asqlidle( void )
{
    int   i;
    long  dw;
    static _int64 i64AsqlKernelTimeStart = 0, i64AsqlUserTimeStart = 0;
    HANDLE hl;

    {
    FILETIME ftKernelTimeEnd;
    FILETIME ftUserTimeEnd;
    FILETIME ftDummy;
    _int64 qwKernelTimeElapsed, qwUserTimeElapsed, qwTotalTimeElapsed;

    DuplicateHandle( 	GetCurrentProcess(),
			GetCurrentThread(),
			GetCurrentProcess(),
			&(hl),
			DUPLICATE_SAME_ACCESS,
  			TRUE,
			DUPLICATE_SAME_ACCESS);
	    
    //get end time
    GetThreadTimes(hl, &ftDummy, &ftDummy, &ftKernelTimeEnd, &ftUserTimeEnd);

    //get the elapsed kernel and user time by convverting the start and end
    //times from FILETIMEs to quad words, and then subtract the start time
    //from the end times
    qwKernelTimeElapsed = FileTimeToQuadWord(&ftKernelTimeEnd) - \
						  i64AsqlKernelTimeStart;
    qwUserTimeElapsed = FileTimeToQuadWord(&ftUserTimeEnd) - \
						  i64AsqlUserTimeStart;
                          
    qwTotalTimeElapsed = qwKernelTimeElapsed + qwUserTimeElapsed;

    if( qwTotalTimeElapsed > ASQL_SWAP_CHECK_GAP ) {
	i64AsqlKernelTimeStart = FileTimeToQuadWord(&ftKernelTimeEnd);
	i64AsqlUserTimeStart = FileTimeToQuadWord(&ftUserTimeEnd);
    } else {
    	return  1;
    }
    }

    for(i = 0;  i < _DioOpenFileNum_;   i++) {
	if( dflush(_DioOpenFile_[i].df) )
	{
		//some database is flush error
		dw |= 0x1FF;
	}
    }


    //look for the dead thread, kill them
    for( i = 0;  i < asqlTaskThreadNum;  i++ ) {
	   
	   //create the threads and let them know which one it is
	   if( threadElapseTime(i) > ASQL_MAX_RUNTIME ) 
	   {
			TerminateThread(asqlTaskThread[i], 0);
			askQS_data[i].it = i;
			asqlTaskThread[i] = (HANDLE)_beginthreadex(NULL, 0, askQS_tm, \
			      (LPVOID)&askQS_data[i], 0, &asqlDwIDThread[i]);
			if( asqlTaskThread[i] == NULL )
			{
				//there is(are) thread dead, suggest restart the system
				dw |= 0x1FFF;
				return  dw;
			}

			while( askQS_data[i].it >= 0 )
			    Sleep( 10 );
	   }
    }

    return  dw;

} //end of asqlidle()




/************************** end of wmtasql.c ********************************/
