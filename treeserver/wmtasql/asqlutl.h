/****************
* asqlutl.h
* copyright (c)
* author: Xilong Pei
*****************************************************************************/

#ifndef __ASQLUTL_H__
#define __ASQLUTL_H__

#ifdef __cplusplus
extern "C" {
#endif

//write FromToCondition base program
short askLittleTaskGen(char *fileName, char *szFrom, char *szTo, \
		       char *szCon, char *szAct);
long asqlDbCommit( void );

dFILE *getRealDbfId(short id, short *newid);

dFILE *getRealDbfIdByName(char *fldname, short *newid);


#ifdef __cplusplus
}
#endif
#endif