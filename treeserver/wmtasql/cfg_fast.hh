/****************
*  CFG_FAST.H
*  author: Richard
*
*  copyright (c) Shanghai Tiedao University MIS Research, 1996
***************************************************************************/

#ifndef __CFG_FAST_H

#define __CFG_FAST_H
#include <stdio.h>
#include <windows.h>

typedef struct tagCFG_STRU {
	FILE *fp;
	void *df;			//dFILE *
	unsigned short memSize;
	unsigned char *cacheMem;
	unsigned short cacheMemTail;
	unsigned short cacheTableHead;
	unsigned short cacheTableTail;
	unsigned short curGroupOffset;
	unsigned short curLabelOffset;
	unsigned short curKeyOffset;

	CRITICAL_SECTION 	dCriticalSection;

	char      szFileName[256];

} CFG_STRU;

typedef CFG_STRU * PCFG_STRU;

extern PCFG_STRU csu;

PCFG_STRU OpenCfgFile( char *szFileName );

void CloseCfgFile( PCFG_STRU csu );

char *GetCfgKey(CFG_STRU *csu, unsigned char *szGroup,                  \
				  unsigned char *szLabel,               \
				  unsigned char *szKey);

short WriteCfgKey(CFG_STRU *csu, unsigned char *szGroup,                \
				    unsigned char *szLabel,             \
				    unsigned char *szKey,               \
				    unsigned char *keyCont);

long enumItemInCache( PCFG_STRU csu, unsigned char *szGroup,              \
				     unsigned char *szLabel,              \
				     unsigned char *szKey,   		  \
				     char          *buf,                  \
				     int	   *iBufSize );

#endif