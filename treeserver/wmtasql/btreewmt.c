/*****************
*   BTREEwmt.H   1.0
*
*  Copyright: CRSC 1997
*  Author:    Xilong Pei, 1997.12.14
****************************************************************************/

#define BtreeProgramBody      2.01

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "btree.h"
#include "wst2mt.h"

#ifdef WSToMT
CRITICAL_SECTION  btreeIndexCE;
#endif

extern long int  IndexLocateAndGetRecNo( bHEAD *b_head, char *keyContent );
extern long int  IndexStrEqSkipInside( bHEAD *b_head, char *buf, int count );
short _BtreeOpenNum_ = 0;
short _BtreeOpenHandleNum_ = _BTREEFILENUM_;
BTREE_OPEN_MAN _BtreeOpenFile_[_BTREEFILENUM_];
/*
-------------------------------------------------------------------------
!!                      indexAwake()
--------------------------------------------------------------------------*/
_declspec(dllexport) bHEAD * IndexAwake(char *dbfName, char *ndxName, \
                                                       InputDbfType DbfType)
{
    int    i;
    bHEAD *b_head, *obh;
    char temp_s[FILENAME_MAX];
    char *filename;
    char *s;

    strcpy(temp_s, ndxName);
    if( ( filename = strchr(temp_s, '.') ) == NULL ) {
	if( strlen(temp_s) > NAMELEN - EXTLEN ) {
		BtreeErr = 2006;                // name is too long
		return  NULL;
	}
    } else {
	*filename = '\0';
    }
    strcat(temp_s, bIndexExtention);

#ifdef WSToMT
    EnterCriticalSection( &btreeIndexCE );
#endif
    for(i = 0;  i < _BtreeOpenNum_;  i++ ) {
	if( stricmp(_BtreeOpenFile_[i].szIndexName, temp_s) == 0 ) {
	    _BtreeOpenFile_[i].count++;

	    obh = _BtreeOpenFile_[i].bh;
	    if( (b_head = (bHEAD *)malloc( sizeof(bHEAD) )) == NULL ) {
		BtreeErr = 2002;
#ifdef WSToMT
		LeaveCriticalSection( &btreeIndexCE );
#endif
		return  NULL;
	    }

	    //lock the index
	    EnterCriticalSection( &(obh->dCriticalSection) );

	    memcpy(b_head, obh, sizeof(bHEAD));
	    b_head->pbh = obh;

	    //saveBtreeEnv
	    b_head->CurNodePtr = (bTREE *)(obh->CurNodePtr->nodeNo);
	    s = &(obh->CurNodePtr->keyBuf[obh->nodeCurPos*(obh->keyStoreLen)]);
	    memcpy(b_head->szKeyBuf, s, b_head->key4Len );
	    b_head->recNo = *(long *)&s[b_head->keyLen];

	    //1999.11.20
	    b_head->type = DbfType;

	    //this index if for you!
	    if( DbfType == BTREE_FOR_OPENDBF )
		b_head->dbfPtr = (dFILE *)dbfName;

	    //unlock the index
	    LeaveCriticalSection( &(obh->dCriticalSection) );

#ifdef WSToMT
	    LeaveCriticalSection( &btreeIndexCE );
#endif
	    return  b_head;

	}
    }

    if( i < _BtreeOpenHandleNum_ ) {
	b_head = IndexOpen(dbfName, temp_s, DbfType);
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
	obh = b_head;

	_BtreeOpenFile_[i].count = 1;
	if( (b_head = (bHEAD *)malloc( sizeof(bHEAD) )) == NULL ) {
	    IndexClose(obh);
	    BtreeErr = 2002;
#ifdef WSToMT
	    LeaveCriticalSection( &btreeIndexCE );
#endif
	    return  NULL;
	}
	memcpy(b_head, obh, sizeof(bHEAD));
	b_head->pbh = obh;

	//saveBtreeEnv
	b_head->CurNodePtr = (bTREE *)(obh->CurNodePtr->nodeNo);
	s = &(obh->CurNodePtr->keyBuf[obh->nodeCurPos*(obh->keyStoreLen)]);
	memcpy(b_head->szKeyBuf, s, b_head->key4Len );
	b_head->recNo = *(long *)&s[b_head->keyLen];

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
_declspec(dllexport) int IndexSleep( bHEAD *bh )
{
    bHEAD *obh;
    int   i;

    if( bh == NULL )
	return  -4;

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
	
	//2000.8.5
	//if the bHEAD isn't managed by the BUFFER, close it directly
	IndexClose(bh);

#ifdef WSToMT
	LeaveCriticalSection( &btreeIndexCE );
#endif
	return  -1;
    }
    _BtreeOpenFile_[i].count--;
    if( _BtreeOpenFile_[i].count < 1 ) {
	IndexClose(_BtreeOpenFile_[i].bh);

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

    memcpy(bh->szKeyBuf, s, bh->key4Len);

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

    bh->timeStamp = obh->timeStamp;

    bh->keyCount = obh->keyCount;
					// record number
    obh->CurNodePtr = loadBTreeNode(obh, (long)(bh->CurNodePtr));
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
	long   l;
	char   indexType;

	bh->szKeyBuf[bh->keyLen] = '\0';

	indexType = obh->type;
	obh->type = BTREE_FOR_ITSELF;
	if( (l = IndexLocateAndGetRecNo(obh, bh->szKeyBuf)) == LONG_MIN ) {
	    obh->type = indexType;
	    return  1; 	//key is missing
	}

	if( bh->recNo == l ) {
	    obh->type = indexType;
	    return  0;
	}

	while( 1 ) {
	    if( (l = IndexStrEqSkipInside(obh, bh->szKeyBuf, 1)) == LONG_MIN )
	    { //key exist, but recno of df is missing
		obh->type = indexType;
		return  1;
	    }
	    if( bh->recNo == l ) {
		obh->type = indexType;
		return  0;
	    }
	}
	obh->type = indexType;
	return  1;
    }

    obh->nodeCurPos = bh->nodeCurPos;   // position in current node
    return  0;

} //end of restoreBtreeEnv()


/*
-------------------------------------------------------------------------
			IndexRelease()
-------------------------------------------------------------------------*/
_declspec(dllexport) int IndexRelease( bHEAD *bh )
{
    int   i;

#ifdef WSToMT
    EnterCriticalSection( &btreeIndexCE );
#endif
    for(i = 0;  i < _BtreeOpenNum_;  i++ ) {
          IndexClose(_BtreeOpenFile_[i].bh );
    }
    _BtreeOpenNum_ = 0;

#ifdef WSToMT
    LeaveCriticalSection( &btreeIndexCE );
#endif
    return  0;

} //end of IndexRelease()


/*
-------------------------------------------------------------------------
			IndexBAwake()
-------------------------------------------------------------------------*/
_declspec(dllexport) bHEAD * IndexBAwake(char *dbfName, char *fieldName, char *ndxName, \
						       InputDbfType DbfType)
{
    bHEAD *bh, *obh;
    char temp_s[FILENAME_MAX];
    char *filename;
    char *fldName[2];
    char *s;

    strcpy(temp_s, ndxName);
    if( ( filename = strchr(temp_s, '.') ) == NULL ) {
	if( strlen(temp_s) > NAMELEN - EXTLEN ) {
		BtreeErr = 2006;                // name is too long
		return  NULL;
	}
    } else {
	*filename = '\0';
    }
    strcat(temp_s, bIndexExtention);

    if( _BtreeOpenNum_ >= _BtreeOpenHandleNum_ ) {
        return  NULL;
    }

    fldName[0] = fieldName;
    fldName[1] = NULL;
    obh = IndexBuild(dbfName, fldName, ndxName, DbfType);
    if( obh == NULL )
	return  NULL;

#ifdef WSToMT
    EnterCriticalSection( &btreeIndexCE );
#endif

    strcpy(_BtreeOpenFile_[_BtreeOpenNum_].szIndexName, temp_s);

    _BtreeOpenFile_[_BtreeOpenNum_].bh = obh;
    _BtreeOpenFile_[_BtreeOpenNum_].count = 1;
    if( (bh = (bHEAD *)malloc( sizeof(bHEAD) )) == NULL ) {
	    IndexClose(obh);
	    BtreeErr = 2002;
#ifdef WSToMT
	    LeaveCriticalSection( &btreeIndexCE );
#endif
	    return  NULL;
    }
    memcpy(bh, obh, sizeof(bHEAD));
    bh->pbh = obh;

    //saveBtreeEnv
    bh->CurNodePtr = (bTREE *)(obh->CurNodePtr->nodeNo);
    s = &(obh->CurNodePtr->keyBuf[obh->nodeCurPos*(obh->keyStoreLen)]);
    memcpy(bh->szKeyBuf, s, bh->key4Len );
    bh->recNo = *(long *)&s[bh->keyLen];


    //this index if for you!
    if( DbfType == BTREE_FOR_OPENDBF )
	bh->dbfPtr = (dFILE *)dbfName;

    _BtreeOpenNum_++;

#ifdef WSToMT
    LeaveCriticalSection( &btreeIndexCE );
#endif

    return  bh;

} //end of IndexBAwake()


/*********************<end of btreewmt.c>***********************************/