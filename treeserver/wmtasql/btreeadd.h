/***************************************************************************
*   BTREEADD.H   1.0
*
*  Copyright: East-Union Computer Service Co., Ltd.
*             1994
*  Author:    Xilong Pei
****************************************************************************/

#ifndef __BTREEADD_H_
#define __BTREEADD_H_

char *initBufSink( bHEAD *bh );
char *getBtreeKey( bHEAD *bh, char *keyCont );
void freeBufSink( void );

short IndexReBuild( char *ndxName, float breakPercent );
    /* build the index btree
    * success: return 1, fail return -1
    */

long IndexPhySyncUpdate( bHEAD *b_head );
#endif
