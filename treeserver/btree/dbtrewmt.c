/*****************
*   BTREEwmt.H   1.0
*
*  Copyright: CRSC 1997
*  Author:    Xilong Pei, 1997.12.14
*			  1998.5.8
****************************************************************************/

#define BtreeProgramBody      2.01

#include <stdio.h>
#include <string.h>
#include <limits.h>

#define __DBTREE_C_      2.01

#include "wst2mt.h"
#include "dbtree.h"

#ifdef WSToMT
CRITICAL_SECTION  btreeIndexCE;
#endif

extern long int IndexStrEqSkip(bHEAD *b_head, char *buf, short count);;
extern bTREE * loadBTreeNode( bHEAD *b_head, long nodeNo );
extern long int  IndexLocateAndGetRecNo( bHEAD *b_head, char *keyContent );
static short _BtreeOpenNum_ = 0;
static short _BtreeOpenHandleNum_ = _BTREEFILENUM_;
static BTREE_OPEN_MAN _BtreeOpenFile_[_BTREEFILENUM_];
/*
-------------------------------------------------------------------------
!!                      indexAwake()
--------------------------------------------------------------------------*/
_declspec(dllexport) bHEAD * dbTreeAwake(char *ndxName)
{
    int    i;
    bHEAD *b_head;
    char temp_s[FILENAME_MAX]; 
//    char *filename;

    strcpy(temp_s, ndxName);
    /*if( ( filename = strchr(temp_s, '.') ) == NULL ) {
	if( strlen(temp_s) > NAMELEN - EXTLEN ) {
		BtreeErr = 2006;                // name is too long
		return  NULL;
	}
    } else {
	*filename = '\0';
    }
    strcat(temp_s, bIndexExtention);
    */

#ifdef WSToMT
    EnterCriticalSection( &btreeIndexCE );
#endif
    for(i = 0;  i < _BtreeOpenNum_;  i++ ) {
	if( stricmp(_BtreeOpenFile_[i].szIndexName, temp_s) == 0 ) {
	    _BtreeOpenFile_[i].count++;
	    if( (b_head = (bHEAD *)malloc( sizeof(bHEAD) )) == NULL ) {
		BtreeErr = 2002;
		return  NULL;
	    }
	    memcpy(b_head, _BtreeOpenFile_[i].bh, sizeof(bHEAD));
	    b_head->pbh = _BtreeOpenFile_[i].bh;
	    b_head->CurNodePtr = (bTREE *)(_BtreeOpenFile_[i].bh->CurNodePtr->nodeNo);

#ifdef WSToMT
	    LeaveCriticalSection( &btreeIndexCE );
#endif
	    return  b_head;

	}
    }

    if( i < _BtreeOpenHandleNum_ ) {
	b_head = dbTreeOpen(temp_s);
	if( b_head == NULL ) {
	    BtreeErr = 2002;
#ifdef WSToMT
	    LeaveCriticalSection( &btreeIndexCE );
#endif
	    return  NULL;
	}
	strcpy(_BtreeOpenFile_[i].szIndexName, temp_s);

	b_head->timeStamp = 0;
	_BtreeOpenFile_[i].bh = b_head;

	_BtreeOpenFile_[i].count = 1;
	if( (b_head = (bHEAD *)malloc( sizeof(bHEAD) )) == NULL ) {
	    dbTreeClose(_BtreeOpenFile_[i].bh);
	    BtreeErr = 2002;
#ifdef WSToMT
	    LeaveCriticalSection( &btreeIndexCE );
#endif
	    return  NULL;
	}
	memcpy(b_head, _BtreeOpenFile_[i].bh, sizeof(bHEAD));
	b_head->pbh = _BtreeOpenFile_[i].bh;
	b_head->CurNodePtr = (bTREE *)(_BtreeOpenFile_[i].bh->CurNodePtr->nodeNo);
	_BtreeOpenNum_++;

#ifdef WSToMT
	LeaveCriticalSection( &btreeIndexCE );
#endif
		return  b_head;
    }

    BtreeErr = 2002;
#ifdef WSToMT
    LeaveCriticalSection( &btreeIndexCE );
#endif
    return( NULL );

} /* end of function indexAwake() */



/*
-------------------------------------------------------------------------
			indexSleep()
PURPOSE: indexSleep will never close file.
-------------------------------------------------------------------------*/
_declspec(dllexport) int dbTreeSleep( bHEAD *bh )
{
    bHEAD *obh;
    int   i;

#ifdef WSToMT
    EnterCriticalSection( &btreeIndexCE );
    if( bh->pbh != NULL ) {
	obh = bh;
        bh = bh->pbh;
	free(obh);
    }
#endif
    for(i = 0;  i < _BtreeOpenNum_ && _BtreeOpenFile_[i].bh != bh;  i++ );

    if( i >= _BtreeOpenNum_ ) {
#ifdef WSToMT
        LeaveCriticalSection( &btreeIndexCE );
#endif
		return  -1;
    }
    _BtreeOpenFile_[i].count--;
    if( _BtreeOpenFile_[i].count < 1 ) {
        dbTreeClose(_BtreeOpenFile_[i].bh);

	_BtreeOpenNum_--;
	memmove(&_BtreeOpenFile_[i], &_BtreeOpenFile_[i+1], \
					(_BtreeOpenNum_-i)*sizeof(BTREE_OPEN_MAN));
#ifdef WSToMT
	LeaveCriticalSection( &btreeIndexCE );
#endif
	return  0;
    }
#ifdef WSToMT
    LeaveCriticalSection( &btreeIndexCE );
#endif
    return  _BtreeOpenFile_[i].count;

} //end of indexSleep()


/*
-------------------------------------------------------------------------
			saveBtreeEnv()
-------------------------------------------------------------------------*/
void saveBtreeEnv( bHEAD *bh )
{
	bHEAD *obh = bh->pbh;
	char  *s;

	if( obh == NULL )
	return;

	bh->keyCount = obh->keyCount;
	bh->timeStamp = ++(obh->timeStamp);
					// record number
	bh->CurNodePtr = (bTREE *)(obh->CurNodePtr->nodeNo);
					// current node pointer
	bh->nodeCurPos = obh->nodeCurPos;        // position in current node
	s = &(obh->CurNodePtr->keyBuf[bh->nodeCurPos*(bh->keyStoreLen)]);
	memcpy(bh->szKeyBuf, s, bh->key4Len );
	bh->recNo = *(long *)&s[bh->keyLen];

} //end of saveBtreeEnv()


/*
-------------------------------------------------------------------------
			restoreBtreeEnv()
-------------------------------------------------------------------------*/
int restoreBtreeEnv(bHEAD *bh)
{
	bHEAD *obh = bh->pbh;
	int   nodeFail;

	if( obh == NULL )
	return  -1;

	if( bh->timeStamp == obh->timeStamp )
	return  0;			//need not

    obh->keyCount = bh->keyCount;
					// record number
    obh->CurNodePtr = loadBTreeNode(bh, (long)(bh->CurNodePtr));
					// current node pointer
    nodeFail = 0;
    if( obh->CurNodePtr == NULL )
    { //the node has been coporated
	nodeFail = 1;
    } else {
	if( bh->nodeCurPos >= obh->CurNodePtr->keyNum ) {
	    nodeFail = 1;
	} else {
#ifdef euObjectBtree
	    if( memcmp(bh->szKeyBuf, &(obh->CurNodePtr->keyBuf[ \
			bh->nodeCurPos*(bh->keyStoreLen)]), bh->key4Len) != 0 )
#else
	    if( bh->recNo != *(long *)&(obh->CurNodePtr->keyBuf[ \
			bh->nodeCurPos*(bh->keyStoreLen)+bh->keyLen]) )
#endif
	    {
		nodeFail = 1;
	    }
	}
    }
    if( nodeFail ) {
	long l;

	if( (l = IndexLocateAndGetRecNo(bh, bh->szKeyBuf)) == LONG_MIN ) {
	    return  1; 	//key is missing
	}

	if( bh->recNo == l ) {
	    return  0;
	}

	while( 1 ) {
	    if( (l = IndexStrEqSkip(bh, bh->szKeyBuf, 1)) == LONG_MIN )
		{ //key exist, but recno of df is missing
		return  1;
	    }
	    if( bh->recNo == l ) {
		return  0;
	    }
	}
	return  1;
    }

    obh->nodeCurPos = bh->nodeCurPos;   // position in current node
    return  0;

} //end of restoreBtreeEnv()


/*
-------------------------------------------------------------------------
			IndexRelease()
-------------------------------------------------------------------------*/
_declspec(dllexport) int dbTreeRelease( bHEAD *bh )
{
	int   i;

#ifdef WSToMT
	EnterCriticalSection( &btreeIndexCE );
#endif
    for(i = 0;  i < _BtreeOpenNum_;  i++ ) {
          dbTreeClose(_BtreeOpenFile_[i].bh );
    }
    _BtreeOpenNum_ = 0;

#ifdef WSToMT
    LeaveCriticalSection( &btreeIndexCE );
#endif
    return  0;

} //end of IndexRelease()



/*********************<end of btreewmt.c>***********************************/