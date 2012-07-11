/****************
* ^-_-^
* histdata.c
* copyright (c) EastUnion Computer Service Co. Ltd. 1995
*               CRSC 1998
*
* author: Xilong Pei
*****************************************************************************/

#ifndef __HISDATA_H_
#define __HISDATA_H_

#ifdef __HISTDATA_C__
_declspec(dllexport) long hyDataCollect(LPCSTR lpszUser, char *hyPath, char *szRem, \
		   void *VariableRegister, short VariableNum, \
		   EXCHNG_BUF_INFO *exbuf);
#else
_declspec(dllimport) long hyDataCollect(LPCSTR lpszUser, char *hyPath, char *szRem, \
		   void *VariableRegister, short VariableNum, \
		   EXCHNG_BUF_INFO *exbuf);
#endif

#endif