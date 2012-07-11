/*********
 * t_lock_r.h
 * 1999.1.17
 * ^-_-^
 ****************************************************************************/


#ifndef __T_LOCK_R_H__
#define __T_LOCK_R_H__

#include "dio.h"

int toLockRec(dFILE *df, long recno);
int freeRecLock(dFILE *df);


#endif