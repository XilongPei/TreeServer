/****************
* ^-_-^
*                               tsdbfutl.h
*    Copyright (c) Xilong Pei, 1998
*****************************************************************************/

#ifndef __TSDBFUTL_H_
#define __TSDBFUTL_H_

#include <windows.h>

#include "exchbuff.h"

//this define come from c++builder\doc\bde.int
typedef struct tagRECProps {
    long  iSeqNum;	   //: Longint;          { When Seq# supported only }
    long  iPhyRecNum;      //: Longint;          { When Phy Rec#s supported only }
    short iRecStatus;      //: Word;             { Delayed Updates Record Status }
    short bSeqNumChanged;  //: WordBool;         { Not used }
    short bDeleteFlag;     //: WordBool;         { When soft delete supported only }
} RECPROPS;

typedef struct tagTS_CLIENT_CURSOR {
    void *p;
    short pType;
    struct tagTS_CLIENT_CURSOR *pNext;
} TS_CLIENT_CURSOR;

typedef struct tagTS_USER_MAN {
    TS_CLIENT_CURSOR *tscc;
    char computerUsr[32];
} TS_USER_MAN;

#ifdef __TSDBFUTL_C_
_declspec(dllexport) TS_CLIENT_CURSOR *asqlGetDbfDes(TS_CLIENT_CURSOR *tscc,\
					LPSTR szDbName, LPSTR szTbName, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllexport) long asqlGetDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					long  recNo, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllexport) long asqlPutDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					long  recNo, \
					LPSTR recBuf, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllexport)  TS_CLIENT_CURSOR *asqlFreeDbfDes(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllexport) long asqlDelDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					long  recNo, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllexport) long asqlAppDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					LPSTR sRecBuf, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllexport) long asqlAddDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					LPSTR sRecBuf, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllexport) long asqlDbfDseek(TS_CLIENT_CURSOR *tscc, void *df, \
					long  recNo, \
					char  cFromWhere, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllexport) long asqlBlobFetch(TS_CLIENT_CURSOR *tscc, \
					LPCSTR lpszUser, \
					LPCSTR lpszUserDir,\
					void *df, \
					long recNo, \
					LPSTR fieldName, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllexport) long asqlBlobPut(TS_CLIENT_CURSOR *tscc, \
					LPCSTR lpszUser, \
					LPCSTR lpszUserDir,\
					void *df, \
					long recNo, \
					LPSTR fieldName, \
					EXCHNG_BUF_INFO *exbuf );
_declspec(dllexport) long asqlGetDbfRecNum(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf );
_declspec(dllexport) long asqlGetDbfFldInfo(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf );
_declspec(dllexport) TS_CLIENT_CURSOR *asqlFreeDbfLock(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf );
_declspec(dllexport) TS_CLIENT_CURSOR *asqlFreeDbfUnLock(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf );
_declspec(dllexport) long asqlBlobMemFetch(TS_CLIENT_CURSOR *tscc, \
					LPCSTR lpszUser, \
					LPCSTR lpszUserDir,\
					void *df, \
					long recNo, \
					LPSTR fieldName, \
					EXCHNG_BUF_INFO *exbuf );
_declspec(dllexport) long asqlBlobMemPut(TS_CLIENT_CURSOR *tscc, \
					LPCSTR lpszUser, \
					LPCSTR lpszUserDir,\
					void *df, \
					long recNo, \
					LPSTR fieldName, \
					EXCHNG_BUF_INFO *exbuf );
_declspec(dllexport) void asqlTsFreeMem( void *p );
#else
extern TS_USER_MAN tsumDefault;
_declspec(dllimport) TS_CLIENT_CURSOR *asqlGetDbfDes(TS_CLIENT_CURSOR *tscc,\
					LPSTR szDbName, LPSTR szTbName, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllimport) long asqlGetDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					long  recNo, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllimport) long asqlPutDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					long  recNo, \
					LPSTR recBuf, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllimport)  TS_CLIENT_CURSOR *asqlFreeDbfDes(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllimport) long asqlDelDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					long  recNo, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllimport) long asqlAppDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					LPSTR sRecBuf, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllimport) long asqlAddDbfRec(TS_CLIENT_CURSOR *tscc, void *df, \
					LPSTR sRecBuf, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllimport) long asqlDbfDseek(TS_CLIENT_CURSOR *tscc, void *df, \
					long  recNo, \
					char  cFromWhere, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllimport) long asqlBlobFetch(TS_CLIENT_CURSOR *tscc, \
					LPCSTR lpszUser, \
					LPCSTR lpszUserDir,\
					void *df, \
					long recNo, \
					LPSTR fieldName, \
					EXCHNG_BUF_INFO *exbuf );

_declspec(dllimport) long asqlBlobPut(TS_CLIENT_CURSOR *tscc, \
					LPCSTR lpszUser, \
					LPCSTR lpszUserDir,\
					void *df, \
					long recNo, \
					LPSTR fieldName, \
					EXCHNG_BUF_INFO *exbuf );
_declspec(dllimport) long asqlGetDbfRecNum(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf );
_declspec(dllimport) long asqlGetDbfFldInfo(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf );
_declspec(dllimport) TS_CLIENT_CURSOR *asqlFreeDbfLock(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf );
_declspec(dllimport) TS_CLIENT_CURSOR *asqlFreeDbfUnLock(TS_CLIENT_CURSOR *tscc, \
					void *df, \
					EXCHNG_BUF_INFO *exbuf );
_declspec(dllimport) long asqlBlobMemFetch(TS_CLIENT_CURSOR *tscc, \
					LPCSTR lpszUser, \
					LPCSTR lpszUserDir,\
					void *df, \
					long recNo, \
					LPSTR fieldName, \
					EXCHNG_BUF_INFO *exbuf );
_declspec(dllimport) long asqlBlobMemPut(TS_CLIENT_CURSOR *tscc, \
					LPCSTR lpszUser, \
					LPCSTR lpszUserDir,\
					void *df, \
					long recNo, \
					LPSTR fieldName, \
					EXCHNG_BUF_INFO *exbuf );
_declspec(dllimport) void asqlTsFreeMem( void *p );
#endif
#endif