/****************
* ts_dict.h
* 1998.3.5 Xilong Pei
*
*****************************************************************************/

#ifndef __TS_DICT_H_
#define __TS_DICT_H_

#include "cfg_fast.h"

extern PCFG_STRU csuDataDictionary;
extern void *asqlAudit;

int dictDupFile( void );
int dictReopenCfgFile( void );
int dictResetCfgFile( void );

#ifdef __TS_DICT_C_

_declspec(dllexport) long asqlGetDData(unsigned char *szGroup, \
				       unsigned char *szLabel, \
				       unsigned char *szKey,   \
				       void *exbuf);
_declspec(dllexport) long asqlPutDData(unsigned char *szGroup, \
				       unsigned char *szLabel, \
				       unsigned char *szKey,   \
				       unsigned char *keyCont, \
				       void *exbuf);
_declspec(dllexport) long asqlEnumDData(unsigned char *szGroup, \
				       unsigned char *szLabel, \
				       unsigned char *szKey,   \
				       void *exbuf);
#else
_declspec(dllimport) long asqlGetDData(unsigned char *szGroup, \
				       unsigned char *szLabel, \
				       unsigned char *szKey,   \
				       void *exbuf);
_declspec(dllimport) long asqlPutDData(unsigned char *szGroup, \
				       unsigned char *szLabel, \
				       unsigned char *szKey,   \
				       unsigned char *keyCont, \
				       void *exbuf);
_declspec(dllimport) long asqlEnumDData(unsigned char *szGroup, \
				       unsigned char *szLabel, \
				       unsigned char *szKey,   \
				       void *exbuf);
#endif

#endif
