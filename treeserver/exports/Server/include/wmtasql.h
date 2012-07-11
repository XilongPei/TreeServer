/****************
* wmtasql.h
* Windows NT MultiThread ASQL
* 1997.10.20 Xilong
*****************************************************************************/

#ifndef __WMTASQL_H_
#define __WMTASQL_H_

#include "exchbuff.h"

#ifdef __WMTASQL_C_
#include "dir.h"
#include "dio.h"
#include "xexp.h"

#define ASQL_MAX_RUNTIME	12345678
#define ASQL_SWAP_CHECK_GAP	60000
#define ASQL_MAX_TASKTHREAD	100

typedef struct tagASKQSDATA {
    int   it;			//integer of task
    
    DWORD  callThreadId;
    EXCHNG_BUF_INFO *wmtExbuf;

    char *FileName;		//task name, if it is a finename, it must 
						//contains the path to recognize users
						//else if must has a mark
    short ConditionType;                        //Xilong:no use now.
    dFILE *FromTable;
    dFILE *ToTable;
    SysVarOFunType **VariableRegister;
    short *VariableNum;

    char  szAsqlResultPath[MAXPATH];
    char  szAsqlFromPath[MAXPATH];
    char  szUser[32];

    char  szRet[256];
    unsigned short qsError;

} ASKQSDATA;
typedef ASKQSDATA *LPASKQSDATA;


extern _int64 i64KernelTimeStart[ASQL_MAX_TASKTHREAD];
extern _int64 i64UserTimeStart[ASQL_MAX_TASKTHREAD];

extern DWORD  dwThreadTimeStart[ASQL_MAX_TASKTHREAD];

_declspec(dllexport) DWORD ProcessAttach( void );
_declspec(dllexport) DWORD ProcessDetach( void );
_declspec(dllexport) DWORD justRunASQL(LPSTR lpszParam, LPSTR lpszResponse, DWORD cbResponse);
_declspec(dllexport) DWORD asqlwmt(long id, LPCSTR lpszUser, LPCSTR lpszUserDir,\
				   LPSTR lpszParam, \
				   char  taskType, \
				   EXCHNG_BUF_INFO *exbuf);

int wmtAskQS(long it, char *FileName, short ConditionType, Asql_ENV *asqlEnvSelf, \
	     SysVarOFunType **VariableRegister, short *VariableNum, \
	     EXCHNG_BUF_INFO *exbuf);

_declspec(dllexport) DWORD  threadElapseTime(int it);
_declspec(dllexport) char *callAsqlSvr(char *szFileName, char *szFromPath, char *szToPath);
_declspec(dllexport) char *callAsqlSvrM(char *szAsqlScript, char *szFromPath, char *szToPath);
_declspec(dllexport) char *callAsqlSvrMVar(char *szAsqlScript, \
					   char *szFromPath, \
					   char *szToPath, \
					   SysVarOFunType *var,
					   int  varnum );
#else

_declspec(dllimport) DWORD ProcessAttach( void );
_declspec(dllimport) DWORD ProcessDetach( void );
_declspec(dllimport) DWORD justRunASQL(LPSTR lpszParam, LPSTR lpszResponse, DWORD cbResponse);
_declspec(dllimport) DWORD asqlwmt(long id, LPCSTR lpszUser, LPCSTR lpszUserDir,\
				   LPSTR lpszParam, \
				   char  taskType, \
				   EXCHNG_BUF_INFO *exbuf);
extern int wmtAskQS(long it, char *FileName, short ConditionType, Asql_ENV *asqlEnvSelf, \
	     SysVarOFunType **VariableRegister, short *VariableNum, \
	     EXCHNG_BUF_INFO *exbuf);

_declspec(dllimport) DWORD  threadElapseTime(int it);
_declspec(dllimport) char *callAsqlSvr(char *szFileName, char *szFromPath, char *szToPath);
_declspec(dllimport) char *callAsqlSvrM(char *szAsqlScript, char *szFromPath, char *szToPath);
_declspec(dllimport) char *callAsqlSvrMVar(char *szAsqlScript, \
					   char *szFromPath, \
					   char *szToPath, \
					   SysVarOFunType *var,
					   int  varnum );

#endif

#endif