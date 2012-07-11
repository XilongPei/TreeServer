/****************
 * db_s_ndx.h
 * ^-_-^  1999.1.13
 ***************************************************************************/

#ifndef __DB_S_NDX__
#define __DB_S_NDX__

#include "dio.h"
#include "btree.h"
#include "asqlana.h"

int dbfOrderSyncToNdx(dFILE *df, bHEAD *bh, short wSkipAdjust);
int orderbyExec(FromToStru *fFrTo);


#endif