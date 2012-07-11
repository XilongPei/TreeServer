/****************
* tabtools.c
* 1995.11.1
* author: Xilong Pei
* copyright (c) EastUnion Computer Service Co., Ltd.
*****************************************************************************/

#ifndef __TABTOOLS_H__
#define __TABTOOLS_H__

#include "dio.h"
#include "btree.h"

short changeFldName(dFILE *df, short fldId, char *fldName);
short fillTable(bHEAD *bh, char *key, char *dataFld, void *data);

#endif
