/***************************************************************************
*   BTREE.C   2.5
*
*  Copyright: EastUnion Computer Service Co., Ltd.
*             1991,92,93,94,95
*             Shanghai Tiedao University
*	      CRSC 1995-1998
*  Author:    Yong Hu,  rewrite for special use by Xilong Pei
*  Notice:
*  1995.3.25. all dbf pointer move is come from "skip()" function:
*	      IndexSkip(), IndexEqSkip()
*  1995.3.26. the key is rtrim() in the B+ tree
*  1995.4.    modi
*  1995.8.24  the tree will error when _BtreeKeyBuf[key4Len] = BtreeNO; does not do
*  1995.11.10 add calculate the key prefix version upper to 1.5
*  1997.10.23 let it support multi thread
*  1997.12.14 let it support more open: indexAwake, indexSleep
*  1997.12.22 let the moveDbfPtr fill the _BtreeKeyBuf
*  1998.4.22  strncmp is different from memcmp, for it will stop at '\0',
*             and when the key is binary, it cause an error
*  1998.8.15  moveDbfPtr( b_head..), not moveDbfPtr( pbh...) for the dFILE
*             is attached on b_head
*  1998.10.10 modi an error in memMod()
*  1998.10.10 when a node is locked, loadBtreeNode(itsFather) won't correctly
*             set the flag for this node in bTreeNodeMove()
*  1998.11.12 find an error: restoreBtreeEnv() more than one will cause error
*  1998.12.10 add an item pbtab in bHEAD struct when it is in memory
*  1998.12.12 rewrite the rubbish node manage method
*  1998.12.14 when you have reorganize the node->keyBuf, the reCalPrefixLen()
*             must be call
*  1999.5.17  invalid the df->rec_p (set it to -1) when IndexSeek(),
*	      IndexStrEqSkip()... haven't found a key suit for it
*
****************************************************************************/

//#define DEBUG

#define BtreeProgramBody      2.01

//I found that the memcmp is much faster than my function
#define bStrCmpOptimized

// if define euObjectBtree, it will make the btree support object operate
//#define euObjectBtree

// if define GeneralSupport, it will make the btree support '*' cmp x
//#ifdef GeneralSupport

// this will make the btree give its runing message
//#define RuningMessageOn

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <ctype.h>
#include <malloc.h>
#include <stdlib.h>
#include <dos.h>
#include <string.h>
#include <limits.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "dio.h"
#include "mistring.h"
#include "btree.h"
#include "btreeadd.h"
#include "wst2mt.h"
#include "memutl.h"

#ifdef RuningMessageOn
#include "busyInfo.h"
#endif

/*****
#include "bugstat.h"
*****/

// one node is 1024 bytes large
short BtreeNodeBufferSize = 32;         // buffer number (in blocks)

short MAX_BtreeNodeBufferSize = 100;
short DEF_BtreeNodeBufferSize = 32;

WSToMT char    _BtreeKeyBuf[BTREE_MAX_KEYLEN];
WSToMT short 	BtreeErr = 0;
#define BTREE_REINDEX_PERCENT   0.85
#define BTREE_LAST_KEY          0x02
#define BTREE_FIRST_KEY         0x04
#define BTREE_SEARCH_FOR_FIND   0x20
#define BTREE_KEY_INSERT        'I'
#define BTREE_KEY_DELETE        'D'
#define BTREE_KEY_RUBBISH	'U'
#define BTREE_NODE_BREAK	'B'
#define BTREE_KEY_REPLACE       'R'
#define BTREE_NODENO_CHANGE	'C'
#define BTREE_KEY_UPDATE	'W'


#define btSUCCESS		'\0'
#define btFAIL			'\1'

//--------------
// inside use function protocle
//---------------------------------------------------------------------------
short  ModFather( bHEAD *b_head, bTREE *sonNode, char type);
short  bIndexHeadCheck( bHEAD *b_head );
void   writeTabNode( bHEAD *b_head, bTREE *tempTree );
short  tabInit( bHEAD *b_head );
char   tabMod( bHEAD *b_head, bTREE *nodeAdr, char type );
bTREE *  nodeTurnBuf( bHEAD *b_head, bTREE *node );
signed char  blenStrCmp(const char *s1, const char *s2, short *keyLen, \
							     short prefixLen);
short  binSearch( bHEAD *b_head, bTREE *node, char *str, char type );
char  KeyLocateIntoNodeAndPos( bHEAD *b_head, char *str, char type );
bTREE *  rqMem( bHEAD *b_head, short wRequireForNew );
short  memMod( bHEAD *b_head );
short  locateTabMin( bHEAD *b_head );
short  locateTabNode( bHEAD *b_head, long nodeNo );
void   increaseNodeUseTime( bHEAD *b_head, bTREE *node );
short  locateAllTabNode( bHEAD *b_head, long nodeNo );
void   setNodeWrFlag( bHEAD *b_head, bTREE *node );
short  locateSonNode( bHEAD *b_head, bTREE *bTreeNode, bTREE *bSonNode );
short  lockTabNode( bHEAD *b_head, bTREE *nodeAdr );
void   unlockTabNode( bHEAD *b_head, short id );
signed char   bTreeNodeMove( bHEAD *b_head );
char   bTreeNodeIncorporate( bHEAD *b_head );
short  modSonNode( bHEAD *b_head, bTREE *bTreeNode, char type );
long  getRubbishBlockNo( bHEAD *b_head );
short  putRubbishBlockNo( bHEAD *b_head, long nodeNo );
long int  IndexLocateAndGetRecNo( bHEAD *b_head, char *keyContent );
short  btreeNodeArrange( bHEAD *b_head );
static void moveDbfPtr( bHEAD *b_head, long recNo );
static void invalidDbfPtr( bHEAD *b_head );
long int  IndexStrEqSkipInside( bHEAD *b_head, char *buf, int count );
static void reCalPrefixLen(bTREE *bt, short btKeyLen);
bTREE * deNodeTurnBuf(bHEAD *b_head, bTREE *tempTree );
//static short calPrefixLen(bTREE *bt, unsigned char *str);


#ifndef bStrCmpOptimized
short bStrCmp(const char *s1, const char *s2, short maxlen);
#else
#define	bStrCmp memcmp
#endif


/****************
*                          BtNodeNumSet()
*****************************************************************************/
_declspec(dllexport) short BtNodeNumSet( short NodeNum )
{

    if( NodeNum < 4 ) {
		return  BtreeNodeBufferSize = 4;
    } else {    return  BtreeNodeBufferSize = NodeNum;        }

} // end of function BtNodeNumSet()


/*===============
*                      bIndexHeadCheck()
*===========================================================================*/
short  bIndexHeadCheck( bHEAD *b_head )
{
    if( b_head == NULL )    return  -1;

    if( strcmp(b_head->VerMessage, BTREE_VER_MESSAGE) != 0 ) {
	return  -1;
    }

    if( filelength( b_head->ndxFp ) != \
		(BTREE_BUFMAXLEN*b_head->nodeMaxNo + BTREE_HEADBUF_SIZE) ) {
	return  -1;
    }
    if( b_head->keyLen >= BTREE_MAX_KEYLEN || b_head->keyLen <= 0 ) {
	return  -1;
    }

    if( b_head->keyMaxCount >= BTREE_BUFMAXLEN / 6 || \
						b_head->keyMaxCount <= 0 ) {
	return  -1;
    }
    if( b_head->keyFieldNum > MAX_KEYFIELD_NUM || \
						b_head->keyFieldNum <= 0 ) {
	return  -1;
    }

    return  1;

} // end of function bIndexHeadCheck()


/***************
*                             IndexOpen()
*****************************************************************************/
_declspec(dllexport) bHEAD * IndexOpen(char *dbfName, char *ndxName, InputDbfType DbfType)
{
    short 	i;
    int 	ndxFp;
    char 	*filename;
    char 	temp_s[BTREE_MAX_KEYLEN];
    bHEAD 	*b_head;
    bTREE       *nodeTree;

    if( ndxName == NULL ) {     BtreeErr = 2001;        return  NULL;    }

    // set ndx open environment
    BtreeNodeBufferSize = DEF_BtreeNodeBufferSize;

    if( (b_head = (bHEAD *)zeroMalloc( sizeof(bHEAD) )) == NULL ) {
	BtreeErr = 2002;
	return  NULL;
    }

    strcpy(temp_s, ndxName);
    if( ( filename = strrchr(temp_s, '.') ) == NULL ) {
	if( strlen(temp_s) > NAMELEN - EXTLEN ) {
		BtreeErr = 2006;                // name is too long
		free( b_head );
		return  NULL;
	}
    } else {
	*filename = '\0';
    }
    strcat(temp_s, bIndexExtention);

    if( (ndxFp = open(temp_s, O_RDWR|O_BINARY|SH_DENYRW, S_IWRITE|S_IREAD) ) <= 0 ) {
	BtreeErr = 2004;
	free( b_head );
	return  NULL;
    }

    // read in the head
    if( lseek( ndxFp, 0, SEEK_SET ) != 0 ) {
	close( ndxFp );
	free( b_head );
	return  NULL;
    }

    if( read( ndxFp, b_head, sizeof(bHEAD) ) == -1 ) {
	close( ndxFp );
	free( b_head );
	return  NULL;
    }

    b_head->ndxFp = ndxFp;

    //ndx's filename can be changed!
    strcpy(b_head->ndxName, temp_s);


    b_head->nodeMnFlag = BtreeNO;
    if( tabInit( b_head ) < 0 ) {
	close( ndxFp );
	free( b_head );
	return  NULL;
    }
    b_head->CurNodePtr = NULL;
    b_head->nodeCurPos = -1;

    switch( DbfType ) {
	case BTREE_FOR_CLOSEDBF:
	    if( (b_head->dbfPtr = dAwake(dbfName, \
		(short)(O_RDWR|O_BINARY), (short)SH_DENYNO, (short)-1)) == NULL ) {
		BtreeErr = 2003;
 		close( ndxFp );
		free( b_head );
		return  NULL;
	    }
	    break;
	case BTREE_FOR_OPENDBF:
	    b_head->dbfPtr = (dFILE *)dbfName;
	    break;
	default:
	    b_head->dbfPtr = NULL;
    }

    // check whether the Btree suit for the dbf. add in 1994.12.5 Xilong
    if( (BtreeErr = IndexChk( b_head, b_head->dbfPtr )) != 0 ) {

	if( DbfType == BTREE_FOR_CLOSEDBF ) {
		dSleep( b_head->dbfPtr );
	}

	close( ndxFp );
	free( b_head );
	return  NULL;
    }

    b_head->type = DbfType;
    if( bIndexHeadCheck( b_head ) <= 0 ) {
	BtreeErr = 2005;
	close( ndxFp );

	if( b_head->type == BTREE_FOR_CLOSEDBF ) {
		dSleep( b_head->dbfPtr );
	}
	free( b_head );
	return  NULL;
    }

    for( i = 0;  i < b_head->keyFieldNum;  i++ ) {
	b_head->KeyFldid[i] = GetFldid( b_head->dbfPtr, \
					b_head->bObject.keyName[i].field );
    }

#ifdef WSToMT
    InitializeCriticalSection( &(b_head->dCriticalSection) );
    b_head->inCriticalSection = '\0';
#endif

    if( (nodeTree = (bTREE *)loadBTreeNode( b_head, (long)b_head->ptr)) == NULL ) {

	close( ndxFp );

	if( b_head->type == BTREE_FOR_CLOSEDBF ) {
		dSleep( b_head->dbfPtr );
	}
	free( b_head );
	return  NULL;
    }
    b_head->ptr = (bTREE *)nodeTree;
    b_head->nodeMnFlag = BtreeYES;
    nodeTree->fatherPtr = (bTREE *)b_head;
    nodeTree->fatherPtrFlag = BtreeYES;

    // while repeat until meet the leaf.
    while( nodeTree->nodeFlag != BtreeYES ) {
	// if the last brother node is not in memory.
	if( nodeTree->keyBuf[b_head->key4Len] == BtreeNO ) {
	    if( (nodeTree = (bTREE *)loadBTreeNode( b_head, \
		   *(long *)&(nodeTree->keyBuf[b_head->keyLen]))) == NULL ) {
		close( ndxFp );

		if( b_head->type == BTREE_FOR_CLOSEDBF ) {
		    dSleep( b_head->dbfPtr );
		}
		free( b_head );
		return  NULL;
	    }
	} else {
	    if( (nodeTree = (bTREE *)(*(long *)&nodeTree->keyBuf[b_head->keyLen])) \
								== NULL ) {
		close( ndxFp );

		if( b_head->type == BTREE_FOR_CLOSEDBF ) {
		    dSleep( b_head->dbfPtr );
		}
		free( b_head );
		return  NULL;
	    }
	}

    } //end of while

    b_head->CurNodePtr = nodeTree;
    b_head->nodeCurPos = 0;
    moveDbfPtr(b_head, *(long *)&(nodeTree->keyBuf[b_head->keyLen]));

    return  b_head;

} // end of function IndexOpen()


/*==============
*                                tabInit()
* initialize the memory manage table
*===========================================================================*/
short  tabInit( bHEAD *b_head )
{
    if( (b_head->active_b_tab = (bTAB *)calloc( BtreeNodeBufferSize, sizeof(bTAB) ) ) == \
								     NULL ) {
       BtreeErr = 2002;
       return  -1;
    }

    // set the number of current node in memory
    b_head->nodeCurTab = 0;

    // initial the memory of b_head->active_b_tab
    memset(b_head->active_b_tab, 0, BtreeNodeBufferSize*sizeof(bTAB) );

    return  1;

} // end of function tabInit()


/*==============
*                              loadBTreeNode()
*===========================================================================*/
bTREE * loadBTreeNode(bHEAD *b_head, long nodeNo )
{
    short 	i;
    long	ll;
    bTREE 	*tempTree;
    bTAB 	*b_tab;

    if( nodeNo <= 0 || nodeNo > b_head->nodeMaxNo ) {
	BtreeErr = 2005;
	return  NULL;
    }

    if( ( i = locateAllTabNode(b_head, nodeNo) ) >= 0 ) {
	return  b_head->active_b_tab[i].nodeAdress;
    }

    if( (tempTree = rqMem( b_head, 0 )) == NULL ) {
	return  NULL;
    }

    ll = (nodeNo-1L)*(long)BTREE_BUFMAXLEN+(long)BTREE_HEADBUF_SIZE;
    if( lseek( b_head->ndxFp, ll, SEEK_SET ) != ll ) {
	return  NULL;
    }

    if( read( b_head->ndxFp, tempTree, BTREE_BUFMAXLEN) == -1 ) {
	return  NULL;
    }

    // store into temp variable for speed
    b_tab = b_head->active_b_tab;

    // deal the father node
    if( tempTree->fatherPtr == NULL ) {
	tempTree->fatherPtr = (bTREE *)b_head;
	tempTree->fatherPtrFlag = BtreeYES;
	b_head->nodeMnFlag = BtreeYES;
	b_head->ptr = tempTree;
    } else {
	if( (i = locateAllTabNode( b_head,
				(long)tempTree->fatherPtr ) ) < 0 ) {
		tempTree->fatherPtrFlag = BtreeNO;
	} else {
		tempTree->fatherPtr = b_tab[i].nodeAdress;
		tempTree->fatherPtrFlag = BtreeYES;
		ModFather( b_head, tempTree, BTREE_KEY_INSERT);
	}
    } // end of else

    // deal the last node
    if( (i = locateAllTabNode( b_head, \
				(long)tempTree->lastPtr)) < 0 ) {
	tempTree->lastPtrFlag = BtreeNO;
    } else {
	//
	// tempTree->lastPtr <-----> tempTree
	//
	tempTree->lastPtr = b_tab[i].nodeAdress;
	tempTree->lastPtrFlag = BtreeYES;
	tempTree->lastPtr->nextPtr = tempTree;
	tempTree->lastPtr->nextPtrFlag = BtreeYES;
    }

    // deal the next node
    if( (i = locateAllTabNode( b_head, \
				(long)tempTree->nextPtr)) < 0 ) {
	tempTree->nextPtrFlag = BtreeNO;
    } else {
	//
	// tempTree <-----> tempTree->nextPtr
	//
	tempTree->nextPtr = b_tab[i].nodeAdress;
	tempTree->nextPtrFlag = BtreeYES;
	tempTree->nextPtr->lastPtr = tempTree;
	tempTree->nextPtr->lastPtrFlag = BtreeYES;
    }

    // if it isn't leaf, modify the flag
    if( tempTree->nodeFlag == BtreeNO ) {
	modSonNode( b_head, tempTree, BTREE_KEY_INSERT );
    }

    return  tempTree;

} // end of function loadBTreeNode()


/*==============
*                                tabMod()
*  Function:
*      Modify the manage table, for the node it managed has been:
*
*  BTREE_KEY_REPLACE: reused
*  BTREE_KEY_UPDATE: rewrited
*  BTREE_KEY_RUBBISH: dead
*  BTREE_KEY_DELETE: left memory
*  BTREE_KEY_INSERT: come into memory
*===========================================================================*/
char  tabMod(bHEAD *b_head, bTREE *nodeAdr, char type)
{
    short   i;
    bTAB   *b_tab, *b_tab2;

    // store into temp variable for speed
    b_tab = b_head->active_b_tab;

    if( type == BTREE_KEY_REPLACE ) {

	if( (i = (short)nodeAdr) >= b_head->nodeCurTab ) {
		return  1;
	}

	b_tab[i].nodeNwFlag = BtreeNO;
	b_tab[i].nodeWrFlag = BtreeYES;

#ifdef DEBUG
	memset(b_tab[i].nodeAdress, 0, BTREE_BUFMAXLEN);
#endif

	// incress it, LRU algorithm will manage the memory only, not
	// the content to avoid to be destroyed at once
	if( ++(b_tab[i].nodeUseTimes) >= BTREE_MAX_USETIMES ) {

		short iloop = b_head->nodeCurTab;

		for( i = 0;  i < iloop;  i++ ) {
		    if( b_tab[i].nodeUseTimes > 0 ) {
			b_tab[i].nodeUseTimes = b_tab[i].nodeUseTimes / 3 + 1;
		    }
		} // end of for
	} // end of if

	return  '\0';
    }

    //iloop = b_head->nodeCurTab;
    switch( type ) {
	case BTREE_KEY_UPDATE:
	    nodeAdr->pbtab->nodeWrFlag = BtreeYES;

	    /*
	    for( i = 0;  i < iloop;  i++ ) {
		if( b_tab[i].nodeAdress == nodeAdr )    break;
	    }

	    if( i >= iloop ) {
		  BtreeErr = 2005;
		  return  '\1';
	    }
	    b_tab[i].nodeWrFlag = BtreeYES;
	    */

	    break;
	case BTREE_KEY_RUBBISH:
	    nodeAdr->pbtab->nodeWrFlag = BtreeNO;
	    nodeAdr->pbtab->nodeNwFlag = BtreeNO;
	    nodeAdr->pbtab->nodeUseTimes = 0;     	// set it to no use

	    nodeAdr->nodeNo = 0;

	    /*
	    for( i = 0;  i < iloop;  i++ ) {
		if( b_tab[i].nodeAdress == nodeAdr )    break;
	    }

	    if( i >= iloop ) {
		  BtreeErr = 2005;      // no node to delete
		  return  '\1';
	    }
	    b_tab[i].nodeWrFlag = BtreeNO;
	    b_tab[i].nodeNwFlag = BtreeNO;
	    b_tab[i].nodeUseTimes = 0;     	// set it to no use
	    */

	    break;
	case BTREE_KEY_DELETE:
	    if( nodeAdr->pbtab->nodeWrFlag == BtreeYES ) {
		writeTabNode( b_head, nodeAdr );
	    }

	    nodeAdr->pbtab->nodeUseTimes = 1;     	// set it to less use

	    /*
	    for( i = 0;  i < iloop;  i++ ) {
		if( b_tab[i].nodeAdress == nodeAdr )    break;
	    }

	    if( i >= iloop ) {
		  BtreeErr = 2005;      	// no node to delete
		  return  '\1';
	    }

	    if( b_tab[i].nodeWrFlag == BtreeYES ) {
		writeTabNode( b_head, i );
	    }

	    b_tab[i].nodeUseTimes = 1;     	// set it to less use
	    */

	    break;                              // end of type Delete
	case BTREE_KEY_INSERT:

	    b_tab2 = &b_tab[(b_head->nodeCurTab)++];

	    b_tab2->nodeAdress = nodeAdr;

	    // incress it, LRU algorithm will manage the memory only, not
	    // the content to avoid to be destroyed at once
	    if( ++(b_tab2->nodeUseTimes) >= BTREE_MAX_USETIMES ) {

		short iloop = b_head->nodeCurTab;

		for( i = 0;  i < iloop;  i++ ) {
		    if( b_tab[i].nodeUseTimes > 0 ) {
			b_tab[i].nodeUseTimes = b_tab[i].nodeUseTimes / 3 + 1;
		    }
		} // end of for
	    } // end of if

	    nodeAdr->pbtab = b_tab2;

	    b_tab2->nodeNwFlag = BtreeYES;
	    b_tab2->nodeWrFlag = BtreeYES;
    } // end of switch

    return  '\0';

} // end of function tabMod()


/*==============
*                                nodeTurnBuf()
*  Function:
*      turn the state of the node's father node and brother node
*
*      this node will leave memory
*===========================================================================*/
bTREE * nodeTurnBuf( bHEAD *b_head, bTREE *node )
{
    bTREE *tempTree;
    
    // if the node's father node is in memory, turn the flag into not
    if( node->fatherPtrFlag == BtreeYES ) {
	// if it is root node
	if( node->fatherPtr == (bTREE *)b_head ) {
		node->fatherPtr = NULL;
		b_head->nodeMnFlag = BtreeNO;
		b_head->ptr = (bTREE *)(node->nodeNo);
	} else {
		if( node->fatherPtrFlag == BtreeYES ) {
			ModFather( b_head, node, BTREE_KEY_DELETE);
		}
	} // end of else
    }

    // next brother
    if( node->nextPtrFlag == BtreeYES ) {
	tempTree = node->nextPtr;
	tempTree->lastPtr = (bTREE *)(node->nodeNo);
	tempTree->lastPtrFlag = BtreeNO;
	node->nextPtr = (bTREE *)(tempTree->nodeNo);
	node->nextPtrFlag = BtreeNO;
    }

    // last brother
    if(node->lastPtrFlag == BtreeYES) {
	tempTree = node->lastPtr;
	tempTree->nextPtr = (bTREE *)(node->nodeNo);
	tempTree->nextPtrFlag = BtreeNO;
	node->lastPtr = (bTREE *)(tempTree->nodeNo);
	node->lastPtrFlag = BtreeNO;
     }

     // itself, not a leaf of tree
    if( node->nodeFlag == BtreeNO ) {
	// deal the son node when in memory
	modSonNode( b_head, node, BTREE_KEY_DELETE );
    }

    return  node;

} // end of function nodeTurnBuf()


/*==============
*                              writeTabNode()
*  Function:
*      write the current manage table into the index file
*===========================================================================*/
void  writeTabNode( bHEAD *b_head, bTREE *tempTree )
{
    long   nodeNo;
    bTAB  *b_tab2;
    long   ll;

    /*
    if( tabNo < 0  ||  tabNo >= b_head->nodeCurTab )
    {       BtreeErr = 2005;        return  -1;    }
    */

    nodeNo = tempTree->nodeNo;
    b_tab2 = tempTree->pbtab;

    if( b_tab2->nodeNwFlag == BtreeYES ) {
	chsize( b_head->ndxFp, \
		(long)(BTREE_HEADBUF_SIZE) + b_head->nodeMaxNo * \
		BTREE_BUFMAXLEN
	      );
    }

    ll = (long)BTREE_HEADBUF_SIZE + (long)BTREE_BUFMAXLEN * (nodeNo-1L);
    if( lseek( b_head->ndxFp, ll, SEEK_SET ) != ll ) {
	return;
    }

    b_tab2->nodeNwFlag = BtreeNO;
    b_tab2->nodeWrFlag = BtreeNO;
    write( b_head->ndxFp, nodeTurnBuf(b_head, tempTree), BTREE_BUFMAXLEN );

} // end of function writeTabNode();



#ifndef bStrCmpOptimized
/*==============
*                               bStrCmp()
*===========================================================================*/
short bStrCmp(const char *s1, const char *s2, short maxlen)
{
/*    short i;

#ifndef __BORLANDC__
    register signed char _AL, _AH;
#endif
*/
#ifdef GeneralSupport
    for( i = 0;  i < maxlen;  i++ ) {
	if( s1[i] != s2[i] && s1[i] != '*' && s2[i] != '*' )
	{
		if( s1[i] && s2[i] )		return s1[i] - s2[i];
		return	0;
	}
    }
#else
    /*
    for( i = 0;  i < maxlen;  i++ ) {
	_AL = s1[i];
	_AH = s2[i];
	if( _AL && _AH ) {
	    if( _AL != _AH )
	    {
		if( _AL == ' ' ) {
			return  (signed char)1;	// ' ' is the biggest
		}
		if( _AH == ' ' ) {
			return  (signed char)-1;	// ' ' is the biggest
		}
		return _AL - _AH;
	    }
	    //continue to compare the next char
	} else {
	    return	(signed char)0;
	}
    }
    */

    while (--maxlen && *s1 && *s1 == *s2)
    {
	s1++;
	s2++;
    }

    return( *(unsigned char *)s1 - *(unsigned char *)s2 );

#endif

    /*return  (signed char)0;*/


} // end of function bStrCmp()
#endif



/*==============
*                               blenStrCmp()
*===========================================================================*/
signed char  blenStrCmp(const char *s1, const char *s2, short *keyLen, \
							     short prefixLen)
{
    short  i;
    short  maxlen = *keyLen;
    short  flag = 0;

#ifdef GeneralSupport
    for( i = 0;  i < maxlen;  i++ ) {
	if( s1[i] != s2[i] && s1[i] != '*' && s2[i] != '*' && prefixLen != 0 )
	{
		if( i >= prefixLen ) {
			*keyLen -= prefixLen;
			prefixLen = 0;
		}
	}
    }
#else
    for( i = 0;  i < maxlen && s1[i] != '\0';  i++ ) {

	//just deal the first different char
	if( s1[i] != s2[i] && flag == 0 )
	{
		if( i >= prefixLen ) {
			flag = 1;	//they are equal in the scope of prefix
		} else {
			flag = -1;	//they are NOT equal in the scope of prefix
		}
	}
    }
#endif

    if( flag == 1 )  {

	//2000.8.5
	//*keyLen = i - prefixLen;

	if( s1[i] == '\0' )
	    *keyLen = maxlen - prefixLen;
	else
	    *keyLen = i - prefixLen;
	return  '\0';
    }
    //*keyLen = maxlen;
    return  '\1';


} // end of function blenStrCmp()


/*==============
*                               binSearch()
* Function:
*     find the position where the key should insert into
*===========================================================================*/
short  binSearch(bHEAD *b_head, bTREE *node, char *str, char type)
{
    short len, low, mid, high;
    short keyLen;
    short keyNum = node->keyNum;
    int   ci;
    char *sz = node->keyBuf;


    if( (high = keyNum - 1 ) < 0 )        return 0;

    low = 0;
    len = b_head->keyStoreLen;

/*the following is useless for blenStrCmp() will calculate them
    keyLen = strlen(str);
    //1995.09.24 get the shorter one
    if( keyLen > b_head->keyLen ) {
	keyLen = b_head->keyLen;
    }
    //keyLen = b_head->keyLen;  change this line to upper line 1995.9.17 for
    //complex keyword   1234.456  CODE+P_RANK  search for CODE
*/
    keyLen = b_head->keyLen;

    //add the if statement 2000.8.5
    if( node->prefixLen > 0 ) {
	if( blenStrCmp(str, sz, &keyLen, node->prefixLen) == '\0' ) {
	    sz += node->prefixLen;
	    str += node->prefixLen;
	}
    }


#ifdef euObjectBtree
    if( (b_head->type & FOROBJMASK) == 0 )

#endif
    {
	while( low <= high ) {
	    mid = ( low + high ) / 2;
	    if( (ci = bStrCmp(str, &sz[ mid*len ], keyLen)) == 0 ) {
		switch( type|BTREE_SEARCH_FOR_FIND ) {
		case BTREE_LAST_KEY|BTREE_SEARCH_FOR_FIND:
		    for( mid++;    mid < keyNum;     mid++ ) {
			if( bStrCmp( (char *)&sz[ mid * len ], \
							str, keyLen ) > 0 )
				break;
		    }
		    if( type & BTREE_SEARCH_FOR_FIND ) {  return  mid - 1;
		    } else {		return  mid;		    }
		case BTREE_FIRST_KEY|BTREE_SEARCH_FOR_FIND:
		    for( mid--;    mid >= 0;     mid-- ) {
			if( bStrCmp( (char *)&sz[ mid * len ], \
							str, keyLen ) < 0 )
				break;
		    }
		    if( type & BTREE_SEARCH_FOR_FIND ) {  return  mid + 1;
		    } else {		return  mid;		    }
		} // end of switch
	    } // end of if

	    if( ci > 0 )	low = mid + 1;
	    else                high = mid - 1;

	}  // end of while

	/* 1998.4.22 Xilong
	if( type & BTREE_SEARCH_FOR_FIND ) {
	    return  ci < '\0'  ?  high  :  mid;
	}

	return  ci > '\0'  ?  low  :  mid;
	*/
	if( type & BTREE_SEARCH_FOR_FIND ) {
	    return  ci < 0  ?  mid-1  :  mid;
	}

	return  ci > 0  ?  mid+1  :  mid;

    } // end of if( (DbfType & FOROBJMASK) == 0 )

#ifdef euObjectBtree

    // Project orinted
    while( low <= high ) {
	mid = ( low + high ) / 2;
	if( (i = (b_head->bObject.ObjDealFuns.ObjCompFun)(str, &node->keyBuf[ mid*len ], \
							keyLen)) == 0 ) {

	    if( mid <= 0 || mid >= keyNum )     return  mid;

	    switch( type ) {
		case BTREE_LAST_KEY:
		    for( mid++;    mid < keyNum;     mid++ ) {
			if( (OBJCOMPPFD)(b_head->bObject.ObjDealFuns.ObjCompFun)( (char *)&node->keyBuf[ mid * len ], \
							str, keyLen ) != 0 )
				break;
		    }
		    if( type & BTREE_SEARCH_FOR_FIND ) {  return  mid - 1;
		    } else {		return  mid;		    }
		case BTREE_FIRST_KEY:
		    for( mid--;    mid >=0;     mid-- ) {
			if( (OBJCOMPPFD)(b_head->bObject.ObjDealFuns.ObjCompFun)( (char *)&node->keyBuf[ mid * len ], \
							str, keyLen ) != 0 )
				break;
		    }
		    if( type & BTREE_SEARCH_FOR_FIND ) {  return  mid + 1;
		    } else {		return  mid;		    }
	    } // end of switch
	} // end of if

	if( i > 0 )            low = mid + 1;
	else                        high = mid - 1;

    }  // end of while

    if( type & BTREE_SEARCH_FOR_FIND ) {
	return  i < 0  ?  high  :  mid;
    }

    return  i > 0  ?  low  :  mid;

#endif

} // end of function binSearch()



/*==============
*                         KeyLocateIntoNodeAndPos()
* Function:
*     Locate the key into the node pointer and the position in the node
* Return:
*     set the current node pointer and store the position in b_head->nodeCurPos;
*     success return 0 else 1
*===========================================================================*/
char  KeyLocateIntoNodeAndPos( bHEAD *b_head, char *str, char type )
{
    bTREE *nodeTree;
    short  i, keyLen;
    long   nodeNo;

    if( b_head->nodeMnFlag == BtreeNO ) {

	if( (nodeTree = loadBTreeNode(b_head,(long)b_head->ptr)) == NULL ) {
                b_head->CurNodePtr = NULL;
		return  btFAIL;
	}
	b_head->ptr = nodeTree;
	b_head->nodeMnFlag = BtreeYES;
	nodeTree->fatherPtr = (bTREE *)b_head;
	nodeTree->fatherPtrFlag = BtreeYES;
    } else {
	nodeTree = b_head->ptr;

	increaseNodeUseTime(b_head, nodeTree);
	//locateTabNode(b_head, nodeTree->nodeNo);
    }

    keyLen = b_head->keyStoreLen;

    // only the leaf node has the nodeFlag `BtreeYES'
    while( nodeTree->nodeFlag != BtreeYES ) {

/*	if( locateTabNode( b_head, nodeTree->nodeNo ) < 0 ) {
		BtreeErr = 2005;
		return  '\1';
	}
*/
	if( (i = binSearch(b_head, nodeTree, str, \
					(char)(type|(char)BTREE_SEARCH_FOR_FIND))) < (short)0 ) {
		i = 0;
	}

	if( nodeTree->keyBuf[ i*keyLen+b_head->key4Len ] == BtreeNO ) {

	     nodeNo = *(long *)&nodeTree->keyBuf[ \
					i*keyLen+b_head->keyLen ];
	     if( (nodeTree = loadBTreeNode( b_head, nodeNo )) == NULL ) {
			BtreeErr = 2005;
			b_head->CurNodePtr = NULL;
			return  btFAIL;
	     }
	} else {
	     if( (nodeTree = (bTREE *)(*(long *)&nodeTree->keyBuf[ \
			     i * keyLen + b_head->keyLen])) == NULL ) {
			BtreeErr = 2005;
			b_head->CurNodePtr = NULL;
			return  btFAIL;
	     }

	     increaseNodeUseTime(b_head, nodeTree);
	     //locateTabNode(b_head, nodeTree->nodeNo);
	} // end of if

    } // end of while

    // locate into position
    if( (b_head->nodeCurPos = binSearch(b_head, nodeTree, str, type)) < 0 ) {
	b_head->CurNodePtr = nodeTree;
	b_head->nodeCurPos = 0;
	return  btFAIL;
    }

    b_head->CurNodePtr = nodeTree;

    return  btSUCCESS;

} // end of function KeyLocateIntoNodeAndPos()


/*==============
*                                 rqMem()
*  Function:
*      require a node memory
*===========================================================================*/
bTREE *  rqMem( bHEAD *b_head, short wRequireForNew )
{
    bTREE  *memNode;
    short   i;

    if( b_head->nodeCurTab >= BtreeNodeBufferSize ) {
	if( (i = memMod( b_head )) < 0 )	return  NULL;
	if( tabMod( b_head, (bTREE *)i, BTREE_KEY_REPLACE) != '\0' ) {
		return  NULL;
	}

	if( wRequireForNew )
	    b_head->active_b_tab[i].nodeNwFlag = BtreeYES;

	return  b_head->active_b_tab[i].nodeAdress;
    }

    //1998.12.10
    //add an item pbtab in bHEAD struct
    //if( (memNode = (bTREE *)malloc( BTREE_BUFMAXLEN )) == NULL ) {
    if( (memNode = (bTREE *)malloc( sizeof(bTREE) )) == NULL ) {

/*can we do this? 1995.3.30
	// shrink the buffer size recorder
	BtreeNodeBufferSize = b_head->nodeCurTab;
*/
	if( (i = memMod( b_head )) < 0 )	return  NULL;
	if( tabMod( b_head, (bTREE *)i, BTREE_KEY_REPLACE) != 0 ) {
		return  NULL;
	}

	if( wRequireForNew )
	    b_head->active_b_tab[i].nodeNwFlag = BtreeYES;

	return  b_head->active_b_tab[i].nodeAdress;
    }

    if( tabMod( b_head, memNode, BTREE_KEY_INSERT) != 0 ) {
	free( memNode );
	return  NULL;
    }

    return  memNode;

} // end of function rqMem()


/*==============
*                                 memMod()
*  Function:
*      swap the buffer managed by btree for new node
*===========================================================================*/
short  memMod( bHEAD *b_head )
{
    short  min;
    long   ll;
    bTAB  *b_tab, *b_tab2;

    // find a minuse node place for use
    if( (min = locateTabMin( b_head ) ) < 0 )   return  -1;

    // store into temp memory for speed
    b_tab = b_head->active_b_tab;

    b_tab2 = &b_tab[min];
    // swap the node memory
    if( b_tab2->nodeWrFlag == BtreeYES ) {
	if( b_tab2->nodeNwFlag == BtreeYES ) {
		chsize( b_head->ndxFp, \
			(long)(BTREE_HEADBUF_SIZE + b_head->nodeMaxNo * \
			(long)BTREE_BUFMAXLEN) );
		b_tab2->nodeNwFlag = BtreeNO;
	}

	ll = ( b_tab2->nodeAdress->nodeNo-1L )*(long)BTREE_BUFMAXLEN + (long)BTREE_HEADBUF_SIZE;
	if( lseek( b_head->ndxFp, ll, SEEK_SET ) != ll ) {
	    return  -1;
	}

	write( b_head->ndxFp, nodeTurnBuf(b_head, b_tab2->nodeAdress), \
							   BTREE_BUFMAXLEN );
	b_tab2->nodeWrFlag = BtreeNO;
    } else
    { //however, I have to warn that this node will be used by others
      //1998.10.10
	if( b_tab2->nodeUseTimes > 0 ) {
		nodeTurnBuf(b_head, b_tab2->nodeAdress);
	}
    }

//    b_tab[min].nodeUseTimes = 1;

    return  min;

} // end of function memMod()


/*==============
*                              locateTabMin()
*  Function:
*     just locate the minuse node
*===========================================================================*/
short  locateTabMin( bHEAD *b_head )
{
    short 	j, times, min;
    short 	iloop;
    bTAB 	*b_tab;

    b_tab = b_head->active_b_tab;
    min = -1;

    times = BTREE_MAX_USETIMES;
    iloop = b_head->nodeCurTab;
    for( j = 0;  j < iloop; j++ ) {
		// if unlocked and min uses
		if( b_tab[ j ].nodeUseTimes >= 0 && \
				b_tab[ j ].nodeUseTimes < times ) {
			times = b_tab[ min = j ].nodeUseTimes;
		}
    }  // end of search the minuse node

    if( times >= BTREE_MAX_USETIMES ) {
	for( j = 0; j < iloop;  j++ ) {
	    if( b_tab[j].nodeUseTimes > 0 ) {	
		b_tab[j].nodeUseTimes = b_tab[j].nodeUseTimes / 3 + 1;
	    }
	}
	//correct this: 1998.3.31 Xilong
	for( j = 0;  j < iloop; j++ ) {
		// if unlocked
		if( b_tab[ j ].nodeUseTimes >= 0 ) {
			return  j;
		}
	}  // end of search the minuse node
    }  // end of if

    return min;

} // end of function locateTabMin()


/*==============
*                              locateTabNode()
*  Function:
*     just locate the node
*===========================================================================*/
short  locateTabNode( bHEAD *b_head, long nodeNo )
{
    short 	j, i;
    short       iloop;
    bTAB 	*b_tab;

    b_tab = b_head->active_b_tab;

    iloop = b_head->nodeCurTab;
    for( j = 0;  j < iloop;  j++ ) {
	if( b_tab[j].nodeAdress->nodeNo == nodeNo && b_tab[j].nodeUseTimes )
	{
		goto lTNbreak;
	}
    }
    return  -1;

lTNbreak:
    if( b_tab[j].nodeUseTimes >= BTREE_MAX_USETIMES ) {
	for( i = 0;  i < iloop;  i++ ) {
	     if( b_tab[i].nodeUseTimes > 0 ) {
		b_tab[i].nodeUseTimes = b_tab[i].nodeUseTimes / 3 + 1;
	     }
	} // end of for
    } // end of if
    b_tab[j].nodeUseTimes++;

    return j;

} // end of function locateTabNode()





/*==============
*                              increaseNodeUseTime()
*  Function:
*     just locate the node
*===========================================================================*/
void increaseNodeUseTime( bHEAD *b_head, bTREE *node )
{
    short 	i;
    short       iloop;
    bTAB 	*b_tab;

    if( ++(node->pbtab->nodeUseTimes) >= BTREE_MAX_USETIMES ) {

	b_tab = b_head->active_b_tab;
	iloop = b_head->nodeCurTab;

	for( i = 0;  i < iloop;  i++ ) {
	     if( b_tab[i].nodeUseTimes > 0 ) {
		b_tab[i].nodeUseTimes = b_tab[i].nodeUseTimes / 3 + 1;
	     }
	} // end of for

    } // end of if

} // end of function increaseNodeUseTime()



/*==============
*                              locateAllTabNode()
*  Function:
*     just locate the node
*===========================================================================*/
short  locateAllTabNode( bHEAD *b_head, long nodeNo )
{
    short 	j, i;
    short       iloop;
    bTAB 	*b_tab;

    b_tab = b_head->active_b_tab;

    iloop = b_head->nodeCurTab;
    for( j = 0;  j < iloop;  j++ ) {
	if( b_tab[j].nodeUseTimes != 0 && b_tab[j].nodeAdress->nodeNo == nodeNo )
	{
		goto lTNbreak;
	}
    }
    return  -1;

lTNbreak:
    if( ++(b_tab[j].nodeUseTimes) >= BTREE_MAX_USETIMES ) {
	for( i = 0;  i < iloop;  i++ ) {
	     if( b_tab[i].nodeUseTimes > 0 ) {
		b_tab[i].nodeUseTimes = b_tab[i].nodeUseTimes / 3 + 1;
	     }
	} // end of for
    } // end of if

    return j;

} // end of function locateAllTabNode()




/*==============
*                              setNodeWrFlag()
*  Function:
*===========================================================================*/
void  setNodeWrFlag( bHEAD *b_head, bTREE *node )
{
    short 	i, iloop;
    bTAB 	*b_tab, *b_tab2;


    b_tab2 = node->pbtab;

    b_tab2->nodeWrFlag = BtreeYES;
    if( ++(b_tab2->nodeUseTimes) >= BTREE_MAX_USETIMES ) {

	b_tab = b_head->active_b_tab;
	iloop = b_head->nodeCurTab;

	for( i = 0;  i < iloop;  i++ ) {
	     if( b_tab[i].nodeUseTimes > 0 ) {
		b_tab[i].nodeUseTimes = b_tab[i].nodeUseTimes / 3 + 1;
	     }
	} // end of for
    } // end of if

} // end of function setNodeWrFlag()


/*==============
*                              locateSonNode()
*  Function:
*     locate the bTreeNode in the (char *)keyBuf of its father node
*===========================================================================*/
short  locateSonNode( bHEAD *b_head, bTREE *bTreeNode, \
				 bTREE *bSonNode )
{
    short  i, j, rec_len;
    char  *s;
    char  *sp4;

    s = bTreeNode->keyBuf + b_head->keyLen;
    sp4 = bTreeNode->keyBuf + b_head->key4Len;
    rec_len = b_head->keyStoreLen;

    for( i = 0,  j = 0;   i < bTreeNode->keyNum;  i++, j += rec_len ) {
		// if its son node is in memory
		if( sp4[ j ] == BtreeYES ) {
			if( bSonNode == (bTREE *)(*(long *)&s[ j ]) ) {
				return  i;
			}
		} // end of if
    } // end of for

    return  -1;

} // end of function locateSonNode()


/*==============
*                              lockTabNode()
*  Function:
*     just locate the node
*===========================================================================*/
short  lockTabNode( bHEAD *b_head, bTREE *nodeAdr )
{
    /*short j, iloop;
    bTAB *b_tab;
    */
    bTAB  *b_tab2;

    b_tab2 = nodeAdr->pbtab;
    if( b_tab2->nodeUseTimes > 0 ) {
	b_tab2->nodeUseTimes = -b_tab2->nodeUseTimes;
    }

    return  ((long)b_tab2 - (long)b_head->active_b_tab) / sizeof(bTAB);

    /*
    b_tab = b_head->active_b_tab;

    iloop = b_head->nodeCurTab;
    for( j = 0;  j < iloop;  j++ ) {
	if( b_tab[ j ].nodeAdress == nodeAdr )
	{
		if( b_tab[j].nodeUseTimes > 0 ) {
		    b_tab[j].nodeUseTimes = -b_tab[j].nodeUseTimes;
		    return j;
		} else {
		    return -1;
		}
	}
    }

    return  -1;
    */

} // end of function lockTabNode()


/*==============
*                              unlockTabNode()
*  Function:
*     just locate the node
*===========================================================================*/
void  unlockTabNode( bHEAD *b_head, short id )
{

    if( id < 0 )        return;

    if( b_head->active_b_tab[id].nodeUseTimes < 0 )
	b_head->active_b_tab[id].nodeUseTimes = -b_head->active_b_tab[id].nodeUseTimes;

} // end of function unlockTabNode()


/***************
**                            IndexBuild()
** if DbfType = BTREE_FOR_OBJ : fieldName;
** fieldName = {"CODE+NAME", NULL} is allowed
*****************************************************************************/
_declspec(dllexport) bHEAD * IndexBuild(char *dbfName, char *fieldName[], char *ndxName, \
						       InputDbfType DbfType)
{
    dFILE  	*dbfPtr;
    bTREE 	*nodeTree;
    bHEAD 	*b_head;
    unsigned short KeyFldid[MAX_KEYFIELD_NUM];
//    short keyNum;
    long int 	recNo;
    short 	ndxFp;
    char  	ndxReallyName[FILENAME_MAX];
    char  	*sz;
    int   	i, j;
    unsigned char tvFlag;
    dFIELD 	*field;
    int    	len;

    // set build ndx environment
    BtreeNodeBufferSize = MAX_BtreeNodeBufferSize;

    if( (dbfName == NULL) || (fieldName[0] == NULL) ) {
	BtreeErr = 2001;
	return  NULL;
    }

    // generate the ndx name
    if( ndxName == NULL || strlen(trim(ndxName)) <= 0 ) {
	switch( DbfType ) {
		case BTREE_FOR_CLOSEDBF:
			strcpy( ndxReallyName, dbfName );
			break;
		case BTREE_FOR_OPENDBF:
			strcpy( ndxReallyName, ((dFILE *)dbfName)->name );
	}
    } else {
	strcpy(ndxReallyName, ndxName);
    }
    if( (sz = strchr(ndxReallyName, '.') ) != NULL ) {
	*sz = '\0';
    }
    strcat( ndxReallyName, bIndexExtention);

    // create the index file
    if( (ndxFp = open( ndxReallyName, O_CREAT|O_RDWR|O_TRUNC|O_BINARY|SH_DENYWR, \
						 S_IREAD|S_IWRITE )) < 0 ) {
	BtreeErr = 2007;
	return  NULL;
    }

    if( (b_head = (bHEAD *)zeroMalloc( sizeof(bHEAD) )) == NULL ) {
	BtreeErr = 2002;
	close( ndxFp );
	unlink( ndxReallyName);
	return  NULL;
    }  /* end of if */
    strcpy(b_head->VerMessage, BTREE_VER_MESSAGE);
    strcpy(b_head->ndxName, ndxReallyName);

#ifdef euObjectBtree

    if( (DbfType & FOROBJMASK) == 0 )
#endif

    {
	switch( DbfType ) {
	    case BTREE_FOR_CLOSEDBF:
		if( (dbfPtr = dAwake(dbfName, (short)(O_RDWR|O_BINARY), (short)SH_DENYNO, \
					       (short)(S_IREAD|S_IWRITE))) == NULL ) {
			BtreeErr = 2003;
			close( ndxFp );
			unlink( ndxReallyName);
                        free(b_head);
			return  NULL;
		}
		break;
	    case BTREE_FOR_OPENDBF:
		dbfPtr = (dFILE *)dbfName;
	} // end of switch

	{
	    char freeFlag = '\0';

	    if( strchr(fieldName[0], '+') != NULL ) {
		fieldName = seperateStr( fieldName[0], '+', NULL );
		freeFlag = '\1';
	    }

	    for( i = j = 0;   fieldName[i] != NULL && i < MAX_KEYFIELD_NUM;   i++ ) {
		if( (b_head->KeyFldid[i] = KeyFldid[i] = \
			     GetFldid( dbfPtr, fieldName[i] )) == 0xFFFF ) {
			BtreeErr = 2008;
			close( ndxFp );
			unlink( ndxReallyName);
			free(b_head);
			return  NULL;
		}
		j += (getFieldInfo( dbfPtr, KeyFldid[i]))->fieldlen;
	    } // end of for

	    if( freeFlag == '\1' )	free( fieldName );

	    if( j >= BTREE_MAX_KEYLEN ) {
		BtreeErr = 2009;		// key is too long
		close( ndxFp );
		unlink( ndxReallyName);
		free(b_head);
		return  NULL;
	    }
	}
    } /* end of if( DbfType & FOROBJMASK == 0 ) */

#ifdef euObjectBtree

    else {
	j = ((bOBJDEALFUN *)fieldName)->ObjLen;
    } // end of else (if) object orinted

#endif

    // allocate the active_b_tab memory
    if( tabInit( b_head ) < 0 ) {
	close( ndxFp );
	unlink( ndxReallyName);
	free(b_head);
	return  NULL;
    }

    b_head->keyLen = j;
    b_head->keyStoreLen = j + 5;
    b_head->key4Len = j + 4;
    b_head->keyFieldNum = i;
    b_head->keyMaxCount = (BTREE_BUFSIZE - 5) / (b_head->keyStoreLen);

    b_head->nodeBreakPos = (short)(b_head->keyMaxCount * DEF_BREAKPOS);
    b_head->upperKeyNum = (short)(b_head->keyMaxCount * UPPER_BREAKPOS);
    b_head->nodeCorpNum = (short)(b_head->keyMaxCount * CORP_KEYPER);

    b_head->ptr = NULL;
    b_head->nodeMnFlag = BtreeNO;
    b_head->CurNodePtr = NULL;
    b_head->nodeCurPos = -1;
    b_head->keyCount = 0;
    b_head->ndxFp = ndxFp;
    b_head->type = DbfType;
    b_head->rubbishBlockNum = 0;

#ifdef euObjectBtree
    if( (DbfType & FOROBJMASK) == 0 )
#endif
    {
	b_head->dbfPtr = dbfPtr;
	//1998.7.13
	//strcpy(b_head->keyDbfName, dbfPtr->name);

	// i is fieldnum now
	for( i--;    i >= 0;     i-- ) {
	     memcpy(&((b_head->bObject).keyName[i]), \
			 getFieldInfo(dbfPtr, KeyFldid[i]), sizeof(dFIELD));
	}  // end of for
    } /* end of if */

#ifdef euObjectBtree
    else {
	b_head->dbfPtr = NULL;
	b_head->bObject.ObjDealFuns.ObjInitFun = ((bOBJDEALFUN *)fieldName)->ObjInitFun;
	b_head->bObject.ObjDealFuns.ObjConFun = ((bOBJDEALFUN *)fieldName)->ObjConFun;
	b_head->bObject.ObjDealFuns.ObjDeconFun = ((bOBJDEALFUN *)fieldName)->ObjDeconFun;
	b_head->bObject.ObjDealFuns.ObjCompFun = ((bOBJDEALFUN *)fieldName)->ObjCompFun;
	// initialize the object
	((bOBJDEALFUN *)fieldName)->ObjInitFun();
    } // end of else (if) object orinted
#endif
/*    // ***** create the btree head. *****
    lseek( ndxFp, 0, SEEK_SET);
    write( ndxFp, b_head, BTREE_HEADBUF_SIZE );
*/
    if( (nodeTree = rqMem( b_head, 1 )) == NULL ) {
	BtreeErr = 2002;
	IndexClose( b_head );
	return  NULL;
    } // end of if

    nodeTree->nodeNo = b_head->nodeMaxNo = 1L;

    b_head->nodeMnFlag = BtreeYES;
    b_head->ptr = nodeTree;
    nodeTree->fatherPtr = (bTREE *)b_head;
    nodeTree->nextPtr = NULL;
    nodeTree->lastPtr = NULL;
    nodeTree->nextPtrFlag = BtreeNO;
    nodeTree->lastPtrFlag = BtreeNO;
    nodeTree->fatherPtrFlag = BtreeYES;
    nodeTree->keyNum = 0;
    nodeTree->prefixLen = 0;
    nodeTree->nodeFlag = BtreeYES;

#ifdef euObjectBtree
    if( (DbfType & FOROBJMASK) == 0 )
#endif
    {

	tvFlag = dbfPtr->op_flag;
	dbfPtr->op_flag = TABLEFLAG;

	// if the buffer is useable
	if( initBufSink( b_head ) == NULL ) {
	    dseek( dbfPtr, 0, dSEEK_SET );
	    while( !deof(dbfPtr) ) {

		recNo = dbfPtr->rec_p;
		getrec( dbfPtr );

#ifdef RuningMessageOn
		euBusyInfo( recNo );
#endif

		// generate the keyword
		//memset(_BtreeKeyBuf, 0, j );

		len = 0;
		for( i = 0;    i < b_head->keyFieldNum;    i++) {
			int  k;

			field = getFieldInfo(dbfPtr, KeyFldid[i]);
			for(k = 0;  k < field->fieldlen;  k++)
			{
			    if( field->fieldstart[k] )
				_BtreeKeyBuf[len++] = field->fieldstart[k];
			    else
				_BtreeKeyBuf[len++] = ' ';
			}
		}   // end of for
		_BtreeKeyBuf[len] = '\0';
		rtrim(_BtreeKeyBuf);

		/*1998.11.16
		if( *rtrim(_BtreeKeyBuf) == '\0' ) {
			strncpy(_BtreeKeyBuf, BTREE_BLANKKEY, j);
		}*/
/*error!!! when the field is not full
	    for( i = 0;    i < b_head->keyFieldNum;    i++) {
		strcat(_BtreeKeyBuf, get_fld(dbfPtr, KeyFldid[i], temp_s));
	    }   // end of for
*/
		if( KeyLocateIntoNodeAndPos( b_head, _BtreeKeyBuf, \
						 BTREE_LAST_KEY ) != btSUCCESS ) {
			IndexClose( b_head );
			return  NULL;
		}

		*(long *)&(_BtreeKeyBuf[b_head->keyLen])= recNo;
		_BtreeKeyBuf[b_head->key4Len] = BtreeNO;
		if( bTreeNodeMove( b_head ) != '\0' ) {
			IndexClose( b_head );
			return  NULL;
		}
	    }  // end of while

	} else {
	    // local sorted
	    b_head->upperKeyNum = \
	    b_head->nodeBreakPos = (short)(b_head->keyMaxCount * BTREE_REINDEX_PERCENT);
	    recNo = 0;

	    while( getBtreeKey( b_head, _BtreeKeyBuf ) != NULL ) {

#ifdef RuningMessageOn
		euBusyInfo( ++recNo );
#endif
#ifdef DEBUG
	if( heapcheck() < 0 ) {
		puts("heap Error");
	}
	if( recNo > b_head->dbfPtr->rec_num ) {
		puts("recNo error");
	}
#endif
		if( KeyLocateIntoNodeAndPos( b_head, _BtreeKeyBuf, \
						 BTREE_LAST_KEY ) != btSUCCESS ) {
			freeBufSink();
			IndexClose( b_head );
			return  NULL;
		}

		if( bTreeNodeMove( b_head ) != '\0' ) {
			freeBufSink();
			IndexClose( b_head );
			return  NULL;
		}

	    }  // end of while

	    // free the buffer memory
	    freeBufSink();

	} // end of else

	dbfPtr->op_flag = tvFlag;


	//1998.8.20 to avoid IndexClose optimize
	b_head->type = BTREE_FOR_ITSELF;
	IndexClose( b_head );

#ifdef RuningMessageOn
		euBusyInfo( 0 );
#endif

	switch( DbfType ) {
	    case BTREE_FOR_CLOSEDBF:
		return  IndexOpen( dbfName, ndxReallyName, BTREE_FOR_CLOSEDBF );
	    case BTREE_FOR_OPENDBF:
		return  IndexOpen( (char *)dbfPtr, ndxReallyName, BTREE_FOR_OPENDBF );
	    default:
		return  NULL;
	}

    } // end of if( DbfType & FOROBJMASK == 0 )
#ifdef euObjectBtree
    else {
	recNo = 0;
	while( ((bOBJDEALFUN *)fieldName)->ObjConFun( \
		_BtreeKeyBuf, &recNo, ((bOBJDEALFUN *)fieldName)->ObjLen)
	     ) {

	    if( KeyLocateIntoNodeAndPos( b_head, _BtreeKeyBuf, \
						 BTREE_LAST_KEY ) != btSUCCESS ) {
		IndexClose( b_head );
		return  NULL;
	    }

	    *(long *)&(_BtreeKeyBuf[b_head->keyLen])= recNo;
	    _BtreeKeyBuf[b_head->key4Len] = BtreeNO;
	    if( bTreeNodeMove( b_head ) != '\0' ) {
		IndexClose( b_head );
		return  NULL;
	    }

	}  // end of while

	IndexClose( b_head );

	if( ((bOBJDEALFUN *)fieldName)->ObjDeconFun != NULL ) {
	    ((bOBJDEALFUN *)fieldName)->ObjDeconFun( \
		_BtreeKeyBuf, &recNo, ((bOBJDEALFUN *)fieldName)->ObjLen);
	} // deconstruct the key

	return  IndexOpen( dbfName, ndxReallyName, DbfType );

    } // end of else of if( DbfType & FOROBJMASK == 0 )
#endif
} // end of function IndexBuild()


/*==============
*                                ModFather()
* Function:
*     add one node then call this function to deal with its father storing
* flag
*===========================================================================*/
short  ModFather( bHEAD *b_head, bTREE *SonNode, char type )
{
    short    i, j, rec_len;
    bTREE   *FatherNode;
    char    *sf;
    char    *sf4;
    long int nodeNo;

    FatherNode = SonNode->fatherPtr;
    sf = FatherNode->keyBuf + b_head->keyLen;
    sf4 = FatherNode->keyBuf + b_head->key4Len;

    rec_len = b_head->keyStoreLen;

    switch( type ) {
	case BTREE_KEY_INSERT:
	    if( (bHEAD *)FatherNode == (bHEAD *)b_head) {
		//SonNode->fatherPtr = (bTREE *)b_head;
		SonNode->fatherPtrFlag = BtreeYES;
		b_head->ptr = SonNode;
		b_head->nodeMnFlag = BtreeYES;
	    } else {
		nodeNo = SonNode->nodeNo;
		for( i = j = 0;  i < FatherNode->keyNum;  i++, j+=rec_len ) {
		    if( sf4[ j ] == BtreeNO && *(long *)&sf[ j ] == nodeNo )
			break;
		}
		if( i >= FatherNode->keyNum ) {
		      BtreeErr = 2005;
		      return  -1;
		}
		*(long *)&sf[ j ] = (long)SonNode;
		sf4[ j ] = BtreeYES;

		SonNode->fatherPtr = (bTREE *)FatherNode;
		SonNode->fatherPtrFlag = BtreeYES;
	    }
	    break;
	case BTREE_KEY_DELETE:
	    nodeNo = SonNode->nodeNo;
	    if( (bHEAD *)FatherNode == (bHEAD *)b_head) {
		b_head->ptr = (bTREE *)nodeNo;
		b_head->nodeMnFlag = BtreeNO;
	    } else {
		for( i = j = 0;  i < FatherNode->keyNum;  i++, j+=rec_len ) {
			if( sf4[ j ] == BtreeYES && \
					*(long *)&sf[ j ] == (long)SonNode )
			{
				break;
			}
		}

		if( i >= FatherNode->keyNum ) {
		      BtreeErr = 2005;
		      return  -1;
		}

		*(long *)&sf[ j ] = SonNode->nodeNo;
		sf4[ j ] = BtreeNO;
		SonNode->fatherPtr = (bTREE *)(FatherNode->nodeNo);
		SonNode->fatherPtrFlag = BtreeNO;

	    }
    }

    return  1;

}  // end of function ModFather()


/*==============
*                                modSonNode()
* Function:
*     set the son node storing flag, when the father node will:
* BTREE_KEY_DELETE:     miss its memory, swap into disk
* BTREE_KEY_INSERT:     get its memory
* TREE_KEY_REPLACE:
*===========================================================================*/
short  modSonNode( bHEAD *b_head, bTREE *bTreeNode, char type)
{
    bTREE   *tempTree;
    long     nodeNo;
    long     ll;
    short    rec_len, i, j, k;
    char    *s, *s4;
    bTAB    *b_tab = b_head->active_b_tab;

    if( bTreeNode->nodeFlag == BtreeYES )       return  1;
    if( bTreeNode->keyNum <= 0 )        return  1;

    s = bTreeNode->keyBuf + b_head->keyLen;
    s4 = bTreeNode->keyBuf + b_head->key4Len;
    rec_len = b_head->keyStoreLen;

    switch( type ) {
	// father node miss its memory
	case BTREE_KEY_DELETE:
	    nodeNo = bTreeNode->nodeNo;
	    for( i = j = 0;  i < bTreeNode->keyNum;  i++, j+=rec_len ) {
		// if its son node is in memory
		if( s4[ j ] == BtreeYES ) {

		    if( (tempTree = (bTREE *)(*(long *)&s[ j ])) != NULL ) {
			// set its father pointer with nodeNo, for its father
			// node will miss its memory
			//
			//   bTreeNode	father node
			//       /\
			//       ||     <-reset their relation method
			//	 ||
			//    tempTree  son node
			//
			tempTree->fatherPtr = (bTREE *)nodeNo;
			tempTree->fatherPtrFlag = BtreeNO;
			*(long *)&s[ j ] = tempTree->nodeNo;
			s4[ j ] = BtreeNO;
		    }

		} // end of if
	    } // end of for
	    break;

	// father node get its memory
	case BTREE_KEY_INSERT:
	    for( i = j = 0;   i < bTreeNode->keyNum;   i++,  j+=rec_len ) {
		if( (k = locateAllTabNode( b_head, \
					*(long *)&s[ j ] )) >= 0 ) {
			//
			//   bTreeNode	father node
			//       /\
			//       ||     <-reset their relation method
			//	 ||
			//    tempTree,b_tab[k].nodeAdress  son node
			//
			tempTree = b_tab[k].nodeAdress;
			tempTree->fatherPtr = bTreeNode;
			tempTree->fatherPtrFlag = BtreeYES;
			*(long *)&s[ j ] = (long)tempTree;
			s4[ j ] = BtreeYES;
		} else {
			s4[ j ] = BtreeNO;
		}
	    } // end of for
	    break;

	// father node change its nodeNo
	case BTREE_NODENO_CHANGE:
	    for( i = j = 0;   i < bTreeNode->keyNum;   i++, j+=rec_len ) {
		// if its son node is in memory
		if( s4[ j ] == BtreeNO ) {

		    if( (nodeNo = *(long *)&s[ j ]) != 0 ) {
			// set its father pointer with nodeNo, for its father
			// node will change its nodeNo
			ll = (long)BTREE_HEADBUF_SIZE + \
				     (long)BTREE_FATHERNODE_POSITION + \
				     (nodeNo-1L)*(long)BTREE_BUFMAXLEN;
			if( lseek(b_head->ndxFp, ll, SEEK_SET) != ll ) {
			    return  -1;
			}
			if( write( b_head->ndxFp, &(bTreeNode->nodeNo), sizeof(long)) == -1 ) {
			    return  -1;
			}
		    }

		} // end of if
	    } // end of for
	    break;

	// father break
	// now the son node of bTreeNode come from other node
	case BTREE_NODE_BREAK:
	    for( i = j = 0;   i < bTreeNode->keyNum;   i++, j+=rec_len ) {

		//    bTreeNode
		//       ||
		//       ||    <-tell them their new father is bTreeNode
		//	\||/
		// :::::::::::::::

		if( s4[ j ] == BtreeYES ) {
			if( (tempTree = (bTREE *)*(long *)&s[ j ]) != NULL ) {
				tempTree->fatherPtr = bTreeNode;
			}
		} else {
			if( (nodeNo = *(long *)&s[ j ] ) != 0 ) {
				ll = (long)BTREE_HEADBUF_SIZE + \
					(long)BTREE_FATHERNODE_POSITION + \
					(nodeNo-1L)*(long)BTREE_BUFMAXLEN;
				if( lseek(b_head->ndxFp, ll, SEEK_SET) != ll ) {
				    return  -1;
				}
				if( write( b_head->ndxFp, &(bTreeNode->nodeNo), sizeof(long)) == -1 ) {
				    return  -1;
				}
			}  // end of if
		}
	    } // end of for

    } // end of switch

    return  1;

}  // end of function modSonNode()


/***************
*                                 IndexClose()
*  Function:
*      free head memory and write the btree head message
*****************************************************************************/
short  IndexClose( bHEAD *b_head )
{
    bTREE   *TempTree;
    short    i;
    long     ll;
    long     nodeNo;
    bTAB    *b_tab;
    int	     success = 1;

    if( b_head == NULL ) {      BtreeErr = 2001;        return( -1 );    }

    if( b_head->type == BTREE_FOR_OPENDBF ) {
	if( b_head->dbfPtr->write_flag.file_write == 0 ) {

	    b_tab = b_head->active_b_tab;

	    for( i = 0; i < b_head->nodeCurTab; i++) {
		free( b_tab[i].nodeAdress );
	    }

	    free( b_tab );
	    close( b_head->ndxFp );

	    free( b_head );

	    return  1;
	}
    } /////


    b_tab = b_head->active_b_tab;
    // first, adjust size of the tree file
    chsize( b_head->ndxFp, (long)(BTREE_HEADBUF_SIZE + \
					b_head->nodeMaxNo * BTREE_BUFMAXLEN) );

    for( i = 0; i < b_head->nodeCurTab; i++) {
    
	    TempTree = b_tab[i].nodeAdress;

	    //2000.12.25
	    if( b_tab[i].nodeUseTimes == 0 ) {
		free( TempTree );
		continue;
	    }

	    if( (b_tab[i].nodeWrFlag == BtreeYES) || \
						(b_head->ptr == TempTree) )  {
		nodeNo = TempTree->nodeNo;

		ll = (long)(BTREE_HEADBUF_SIZE) + \
				(nodeNo-1L)*(long)BTREE_BUFMAXLEN;
		if( lseek( b_head->ndxFp, ll, SEEK_SET) != ll ) {
		    //error, how to do
		    success = 0;
		}
		if( write( b_head->ndxFp, nodeTurnBuf(b_head, TempTree), \
						BTREE_BUFMAXLEN) == -1 ) {
		    success = 0;
		}
            }

            //2000.12.21
            else {
                nodeTurnBuf(b_head, TempTree);
            }

	    /*if( b_head->ptr == TempTree ) {
		b_head->ptr =(bTREE *)(TempTree->nodeNo);
	    }*/

	    free( TempTree );

    }  // end of for

    if( lseek( b_head->ndxFp, 0, SEEK_SET ) != 0 ) {
	//error
	;
    }
    if( write( b_head->ndxFp, b_head, sizeof(bHEAD)) == -1 ) {
	//error
	;
    }

    free( b_tab );

    if( b_head->type == BTREE_FOR_CLOSEDBF ) {
	dSleep( b_head->dbfPtr );
    }

    // close the file
    close( b_head->ndxFp );

    free( b_head );

    return  1;

} // end of function IndexClose()


/***************
*                                 IndexDispose()
*  Function:
*      free head memory but do not write the btree head message
*****************************************************************************/
_declspec(dllexport) short IndexDispose(bHEAD *b_head)
{
    short  i;
    bTAB  *b_tab;

    if( b_head == NULL ) {      BtreeErr = 2001;        return( -1 );    }

    b_tab = b_head->active_b_tab;

    for( i = 0; i < b_head->nodeCurTab; i++) {
	    free( b_tab[i].nodeAdress );
    }  // end of for
    free( b_tab );

    if( b_head->type == BTREE_FOR_CLOSEDBF ) {
	dSleep( b_head->dbfPtr );
    }

    // close the file
    close( b_head->ndxFp );

    free( b_head );

    return  1;

} // end of function IndexDispose()


/*==============
*                               bTreeNodeMove()
* Function:
*      manager the node when new keyword is insert.
*
*==========================================================================*/
signed char  bTreeNodeMove( bHEAD *b_head )
{
    bTREE 	*tempTree, *tempPtr, *node2, *tTree;
    short   	mid, rec_len, breakPos, insPos;
    long	nodeNo;
    //short 	MnPos;
//    bTAB 	*b_tab;
    char 	*sn;
    char 	*st;                           // tempTree->keyBuf
    char 	buf[BTREE_MAX_KEYLEN];
    long 	idRubbish;
    char  	*sz;
    short 	i, j, k, ilock;
    short 	keyLen = b_head->keyLen;
    short	nKnum;
    long	ll;
//    bTREE 	*remNode;

    bTREE *node = b_head->CurNodePtr;
    short strPosition = b_head->nodeCurPos;

//    b_tab = b_head->active_b_tab;
    sn = node->keyBuf;
    nKnum = node->keyNum;

/* inside use module, need not check,
    if( strPosition < 0  ||  strPosition > node->keyNum )
    {     BtreeErr = 2005;        return  '\1';    }

    if( b_head->ndxFp < 0 ) {   BtreeErr = 2005;        return  '\1';    }
*/
    rec_len = b_head->keyStoreLen;

    // the tree has more than one level and now we operate the first keyword
    // replace it and then operate it which is the old one
    if( strPosition <= 0 && b_head->nodeMaxNo > 1 ) {
	     setNodeWrFlag(b_head, node);
	     k = lockTabNode(b_head, (node2 = node));
	     memcpy(buf, node->keyBuf, rec_len);

	     memcpy(node->keyBuf, _BtreeKeyBuf, rec_len );
	     reCalPrefixLen(node, keyLen);

//	     b_tab[ locateTabNode( b_head, node->nodeNo ) ].nodeWrFlag = BtreeYES;

	     // modify the key begin from node's father node
	     do {

		//remNode = node2;
		nodeNo = (long)node2;

		//
		//     node2
		//      /\
		//      || \fathernode
		//      \/
		//     nodeNo
		//

		if( node2->fatherPtrFlag == BtreeYES ) {

			//1998.12.12
			if( node2->fatherPtr == (bTREE *)b_head )  break;

			node2 = node2->fatherPtr;
		} else {
			ilock = lockTabNode(b_head, node2);
			if( (node2 = loadBTreeNode( b_head, \
				(long)(node2->fatherPtr))) == NULL ) {
				BtreeErr = 2001;        // cannot load node
				return  '\1';
			}
			unlockTabNode( b_head, ilock );
		} // end of else

		//if( node2 == (bTREE *)b_head )  break;

		setNodeWrFlag(b_head, node2);
		//b_tab[locateTabNode(b_head, node2->nodeNo)].nodeWrFlag = BtreeYES;

		// find the buf position
		sz = node2->keyBuf+keyLen;
		for( i = j = 0;  i < node2->keyNum;  i++, j+=rec_len ) {
			/*if( sz[ j+4 ] == BtreeYES ) {
				if( *(long *)&sz[ j ] == (long)remNode )
					break;
			} else {
				if( *(long *)&sz[ j ] == remNode->nodeNo )
					break;
			}*/
			if( *(long *)&sz[ j ] == nodeNo )
				break;
		}
		if( i >= node2->keyNum ) {
			BtreeErr = 2005;
			return  -1;
		}

		/*//1998.10.10
		sz[ j+4 ] = BtreeYES;
		*(long *)&sz[ j ] = (long)remNode;
		remNode->fatherPtrFlag = BtreeYES;
		remNode->fatherPtr = node2;
		*/

		memcpy(node2->keyBuf+j, _BtreeKeyBuf, keyLen);
		reCalPrefixLen(node2, keyLen);

		//1998.10.11
		//if( node2->fatherPtr == (bTREE *)b_head )  break;

	     } while( i == 0 );

	     memcpy(_BtreeKeyBuf, buf, rec_len );
	     unlockTabNode( b_head, k );
	     strPosition = 1;
    } // end of if

    if( nKnum < b_head->keyMaxCount) {

	// if there is enough space
	// insert this key directly.
	(char *)sn += strPosition * rec_len;

	//1998.10.10
	if( nKnum > strPosition )
	    memmove(sn + rec_len, sn, (nKnum - strPosition) * rec_len);

	memcpy(sn, _BtreeKeyBuf,  rec_len);
	b_head->keyCount++;
	node->keyNum++;

	setNodeWrFlag(b_head, node);

	if( memcmp(node->keyBuf, _BtreeKeyBuf, node->prefixLen) != 0 ) {
		reCalPrefixLen(node, keyLen);
	}

	return  0;

     } // else
     // node will break

     ilock = lockTabNode( b_head, node );

     // Modi 1995.5.14
     // check wether we can borrow from last node
     //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     if( node->lastPtr == NULL ) 	goto  BtreeJMP_1;
     if( node->lastPtrFlag == BtreeYES ) {
	 tempTree = node->lastPtr;
     } else {
	 tempTree = loadBTreeNode(b_head, (long)(node->lastPtr) );
     }
     if( tempTree->keyNum < b_head->upperKeyNum ) {

	// borrow from last node
	memcpy(buf, sn, rec_len);
	setNodeWrFlag(b_head, node);
	unlockTabNode( b_head, ilock );

	// 0 1 2 3 4 5 6 7 8 9
	// 0 is move out, strPosition is 6
	// algorithm: move 012345 forward, strPosition is 6-1=5
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if( strPosition != 0 ) {
	    memmove(sn, sn+rec_len, (--strPosition)*rec_len);
	}

	memcpy(sn+strPosition*rec_len, _BtreeKeyBuf,  rec_len);
	reCalPrefixLen(node, keyLen);

        //tell the node's father that I have changed my mark key
        //buf->_BtreeKeyBuf
	memcpy(_BtreeKeyBuf, sn, rec_len);

	k = lockTabNode(b_head, tempTree);

	do {
	   ilock = lockTabNode(b_head, node);
	   nodeNo = (long)node;
	   if( node->fatherPtrFlag == BtreeYES ) {
	       //1998.10.11
	       if( node->fatherPtr == (bTREE *)b_head ) {
		   //2000.12.25
		   unlockTabNode( b_head, ilock );
		   break;
	       }

	       node = node->fatherPtr;
	   } else {
	       if( (node = loadBTreeNode( b_head, \
				(long)(node->fatherPtr))) == NULL ) {
		      BtreeErr = 2001;        // cannot load node
		      return  '\1';
	       }
	   } // end of else
	   unlockTabNode( b_head, ilock );

	   /*if( node == (bTREE *)b_head )
	       break;
	   */

	   setNodeWrFlag(b_head, node);

	   sz = node->keyBuf+keyLen;
	   for( i = j = 0;  i < node->keyNum;  i++, j+=rec_len ) {
		if( *(long *)&sz[ j ] == (long)nodeNo )
				break;
	   }
	   memcpy(node->keyBuf+j, _BtreeKeyBuf, keyLen);

	   if( memcmp(node->keyBuf, _BtreeKeyBuf, node->prefixLen) != 0 ) {
		reCalPrefixLen(node, keyLen);
	   }
	} while( i == 0 );

	unlockTabNode( b_head, k );

	memcpy(_BtreeKeyBuf, buf, rec_len);
	b_head->CurNodePtr = tempTree;
	b_head->nodeCurPos = tempTree->keyNum;

	return  bTreeNodeMove( b_head );

     }

BtreeJMP_1:

     // check wether we can borrow from next node
     //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     if( node->nextPtr == NULL ) 	goto  BtreeJMP_2;
     if( node->nextPtrFlag == BtreeYES ) {
	 tempTree = node->nextPtr;
     } else {
	tempTree = loadBTreeNode(b_head, (long)(node->nextPtr) );
     }
     if( tempTree->keyNum < b_head->upperKeyNum ) {
	// borrow from next node

	unlockTabNode( b_head, ilock );
	if( strPosition < nKnum ) {

		nKnum--;

		// 0 1 2 3 4 5 6 7 8 9
		// 9 is move out, 10-1=9, strPosition is 6
		// algorithm: move 678 backward, strPosition is 6-1=5
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		memcpy(buf, sn+nKnum*rec_len, rec_len);

		(char *)sn += strPosition * rec_len;
		memmove(sn + rec_len, sn, (nKnum - strPosition) * rec_len);
		memcpy(sn, _BtreeKeyBuf,  rec_len);

		if( memcmp(node->keyBuf, _BtreeKeyBuf, node->prefixLen) != 0 ) {
			reCalPrefixLen(node, keyLen);
		}

		// find the node position
		setNodeWrFlag(b_head, node);
		//b_tab[ locateTabNode( b_head, node->nodeNo ) ].nodeWrFlag = BtreeYES;

		memcpy(_BtreeKeyBuf, buf, rec_len);
	}

	b_head->CurNodePtr = tempTree;
	b_head->nodeCurPos = 0;

	return  bTreeNodeMove( b_head );

     }

BtreeJMP_2:
     b_head->keyCount++;

     if( (tempTree = rqMem( b_head, 1 )) == NULL )        return  '\1';
     unlockTabNode( b_head, ilock );

     if( (idRubbish = getRubbishBlockNo( b_head )) < 0 ) {
	tempTree->nodeNo = ++(b_head->nodeMaxNo);
     } else {
	tempTree->nodeNo = idRubbish;
     }

     st = tempTree->keyBuf;

     // get the break position
     breakPos = mid = b_head->nodeBreakPos;

     // break the node
     // node -> node, tempTree
     tempTree->nodeFlag = BtreeYES;

     // 0 1 2 3 4 5 6 7 8 9
     // break from 6
     // algorithm: 6789 to tempTree
     // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

     memmove(st, &sn[ breakPos * rec_len ], (nKnum-breakPos) * rec_len );

     // if the key should insert position is behind the mid
     if( strPosition >= mid ) {
	 tempTree->keyNum = nKnum - mid;
	 node->keyNum = mid;
	 strPosition -= mid;
	 sz = (char *)&st[strPosition * rec_len];
	 memmove(sz+rec_len, sz, ((tempTree->keyNum++) - strPosition)*rec_len );
	 memcpy( sz, _BtreeKeyBuf, rec_len);
     } else {
	 tempTree->keyNum = nKnum - mid;
	 node->keyNum = mid + 1;
	 sz = (char *)&sn[strPosition * rec_len];
	 memmove(sz + rec_len, sz, ( mid - strPosition ) * rec_len );
	 memcpy(sz, _BtreeKeyBuf, rec_len);
     }
     reCalPrefixLen(node, keyLen);
     reCalPrefixLen(tempTree, keyLen);

     // if the next node is in memory
     if( node->nextPtrFlag == BtreeYES ) {
	 tempTree->nextPtr = tTree = node->nextPtr;
	 tempTree->nextPtrFlag = BtreeYES;
	 tempTree->lastPtr = node;
	 tempTree->lastPtrFlag = BtreeYES;
	 node->nextPtr = tempTree;
	 node->nextPtrFlag = BtreeYES;
	 tTree->lastPtr = tempTree;
	 tTree->lastPtrFlag = BtreeYES;
	 setNodeWrFlag(b_head, tTree);
	 //b_tab[ locateTabNode( b_head, tTree->nodeNo ) ].nodeWrFlag = BtreeYES;
     } else {
	 if( node->nextPtr != NULL )  {
		nodeNo = (long)node->nextPtr;
		ll = (long)BTREE_HEADBUF_SIZE + \
			(long)BTREE_LASTNODE_POSITION+(nodeNo-1L)*(long)BTREE_BUFMAXLEN;
		if( lseek(b_head->ndxFp, ll, SEEK_SET) != ll ) {
			BtreeErr = 2003;
			return  '\1';
		}
		if( write( b_head->ndxFp, &(tempTree->nodeNo), sizeof(long)) == -1 ) {
			BtreeErr = 2003;
			return  '\1';
		}
	 }  // end of if
	 tempTree->nextPtr = node->nextPtr;
	 tempTree->nextPtrFlag = BtreeNO;
	 tempTree->lastPtr = node;
	 tempTree->lastPtrFlag = BtreeYES;
	 node->nextPtr = tempTree;
	 node->nextPtrFlag = BtreeYES;
     } //  end of if
     setNodeWrFlag(b_head, node);
     //b_tab[ locateTabNode( b_head, node->nodeNo ) ].nodeWrFlag = BtreeYES;

     do {

	    // if the node's father node is the tree's head
	    // this will call this tree to increase its level
	    if( node->fatherPtr == (bTREE *)b_head ) {

		// lock it
		i = lockTabNode( b_head, tempTree );
		j = lockTabNode( b_head, node );

		if( (tTree = rqMem( b_head, 1 )) == NULL ) {
			BtreeErr = 2003;
			return  '\1';
		}

		// unlock it
		unlockTabNode( b_head, i );
		unlockTabNode( b_head, j );

		if( (idRubbish = getRubbishBlockNo( b_head )) < 0 ) {
			tTree->nodeNo = ++(b_head->nodeMaxNo);
		} else {
			tTree->nodeNo = idRubbish;
		}

		tTree->keyNum = 2;      // the son neast to father break
					// will make two node has no father
		b_head->ptr = tTree;
		b_head->nodeMnFlag = BtreeYES;
		tTree->nodeFlag = BtreeNO;
		tTree->fatherPtr = (bTREE *)b_head;
		tTree->fatherPtrFlag = BtreeYES;
		tTree->nextPtr = NULL;
		tTree->nextPtrFlag = BtreeNO;
		tTree->lastPtr = NULL;
		tTree->lastPtrFlag = BtreeNO;
		tempTree->fatherPtr = tTree;
		tempTree->fatherPtrFlag = BtreeYES;
		node->fatherPtr = tTree;
		node->fatherPtrFlag = BtreeYES;

		// fill the new node with keyContent, recno and memMark.
		// node
		memmove( tTree->keyBuf, sn, keyLen );
		*(long *)&(tTree->keyBuf[ keyLen ]) = (long)node;
		tTree->keyBuf[ b_head->key4Len ] = BtreeYES;

		// tempTree
		memmove( &tTree->keyBuf[rec_len], st, keyLen );
		*(long *)&(tTree->keyBuf[ rec_len + keyLen ]) = \
							      (long)tempTree;
		tTree->keyBuf[ rec_len+b_head->key4Len ] = BtreeYES;
		reCalPrefixLen(tTree, keyLen);

		return  btSUCCESS;

	    } // end of if the node's father node is the tree's head

	    // copy the keyword which stored both in this node and its father
	    // into buf
	    memmove( buf, st, keyLen );

	    if( node->fatherPtrFlag == BtreeYES ) {
		node2 = node->fatherPtr;
	    } else {

		// lock it
		i = lockTabNode( b_head, tempTree );
		j = lockTabNode( b_head, node );

		if( (node2 = loadBTreeNode( b_head, \
				(long)(node->fatherPtr))) == NULL ) {
			BtreeErr = 2001;        // cannot load node
			return  '\1';
		}

		// unlock it
		unlockTabNode( b_head, i );
		unlockTabNode( b_head, j );
	    } // end of else

	    tempTree->fatherPtr = node2;
	    tempTree->fatherPtrFlag = BtreeYES;

	    // find the node position it in its father keyBuf, then
	    // get the position of tempTree to insert into
	    if( (insPos = locateSonNode( b_head, node2, node ) + 1 ) < 1 ) {
			return  '\1';
	    }

	    // set the write flag
	    setNodeWrFlag(b_head, node2);
	    //b_tab[ locateTabNode( b_head, node2->nodeNo ) ].nodeWrFlag = BtreeYES;

	    // tempTree now stored the node which want to find its father node
	    *(long *)&(buf[ keyLen ]) = (long)tempTree;
	    buf[ b_head->key4Len ] = BtreeYES;
	    if( node2->keyNum < b_head->keyMaxCount ) {
		    sz = (char *)&node2->keyBuf[ insPos*rec_len ];
		    memmove(sz+rec_len, sz, ((node2->keyNum++)-insPos)*rec_len);
		    memcpy(sz, buf, rec_len);

		    if( memcmp(node2->keyBuf, buf, node2->prefixLen) != 0 ) {
			reCalPrefixLen(node2, keyLen);
		    }

		    break;            // stop `while'

	    } else {

		    // the upper level will break, the father node is node2 now

		    // lock it
		    i = lockTabNode( b_head, tempTree );
		    j = lockTabNode( b_head, node );
		    k = lockTabNode( b_head, node2 );
		    if( (tempPtr = rqMem( b_head, 1 )) == NULL )   return  -1;
		    // unlock it
		    unlockTabNode( b_head, i );
		    unlockTabNode( b_head, j );
		    unlockTabNode( b_head, k );

		    if( (idRubbish = getRubbishBlockNo( b_head )) < 0 ) {
			tempPtr->nodeNo = ++(b_head->nodeMaxNo);
		    } else {
			tempPtr->nodeNo = idRubbish;
		    }

		    tempPtr->nodeFlag = BtreeNO;
		    memmove( tempPtr->keyBuf, \
			     &node2->keyBuf[ breakPos * rec_len ], \
			     ( node2->keyNum - breakPos ) * rec_len );

		    tempPtr->fatherPtr = node2->fatherPtr;
		    tempPtr->lastPtr = node2;
		    tempPtr->nextPtr = node2->nextPtr;

		    if( node2->nextPtrFlag == BtreeYES ) {
			node2->nextPtr->lastPtr = tempPtr;
			node2->nextPtr->lastPtrFlag = BtreeYES;

			//2000.12.25
			setNodeWrFlag(b_head, node2->nextPtr);
		    } else {
			if( node2->nextPtr != NULL ) {
			     nodeNo= (long)node2->nextPtr;

			     ll = (long)BTREE_HEADBUF_SIZE + \
				    (long)BTREE_LASTNODE_POSITION + (nodeNo-1L) * \
				    (long)BTREE_BUFMAXLEN;
			     if( lseek( b_head->ndxFp, ll, SEEK_SET ) != ll ) {
				return  '\1';
			     }
			     if( write( b_head->ndxFp, &(tempPtr->nodeNo), sizeof(long)) == -1 ) {
				return  '\1';
			     }
			} // end of if
		    } // end of else
		    node2->nextPtr = tempPtr;

		    // set the storaging flag
		    tempPtr->fatherPtrFlag = node2->fatherPtrFlag;
		    tempPtr->lastPtrFlag = BtreeYES;
		    tempPtr->nextPtrFlag = node2->nextPtrFlag;
		    node2->nextPtrFlag = BtreeYES;

		// 0 1 2 3 4 5 6 7 8 9
		// breakPos is 6
		// algorithm: move 6789 to tempPtr,
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		    if( insPos >= breakPos ) {
			tempPtr->keyNum = node2->keyNum - breakPos;
			node2->keyNum = breakPos;
			insPos -= breakPos;
			sz = &(tempPtr->keyBuf[ insPos * rec_len]);
			memmove( sz+rec_len, sz, ((tempPtr->keyNum++) - insPos) * rec_len );
			memmove( sz, buf, rec_len );
		    } else {
			tempPtr->keyNum = node2->keyNum - breakPos;
			node2->keyNum = breakPos + 1;
			sz = (char *)&node2->keyBuf[ insPos*rec_len ];
			memmove(sz+rec_len, sz, (breakPos-insPos) * rec_len);
			memmove(sz, buf, rec_len);
		   } // end of if

		   modSonNode( b_head, tempPtr, BTREE_NODE_BREAK );

		   reCalPrefixLen(node2, keyLen);
		   reCalPrefixLen(tempPtr, keyLen);

		   sn = (node = node2)->keyBuf;
		   st = (tempTree = tempPtr)->keyBuf;

	    } // end of else

     } while( 1 );

    return  0;

} // end of function nodeBufMove()


/*==============
*                               bTreeNodeIncorporate()
* Function:
*      manager the node when new keyword is insert.
*===========================================================================*/
char  bTreeNodeIncorporate( bHEAD *b_head )
{
    bTREE   	*fatherNode, *brotherNode;
    short 	rec_len;
//    long nodeNo;
//    bTAB *b_tab;
//    short idRubbish;
    char  	buf[BTREE_MAX_KEYLEN];
    short 	i, j;
    bTREE 	*node;
    short 	strPos;
    char  	*sz;

    node = b_head->CurNodePtr;
    strPos = b_head->nodeCurPos;
//    b_tab = b_head->active_b_tab;

/* inside use module
    if( strPos < 0 || strPos > b_head->keyMaxCount || strPos > node->keyNum )
    {     BtreeErr = 2005;        return  -1;    }

    if( b_head->ndxFp < 0 ) {   BtreeErr = 2005;        return  -1;    }
*/
    rec_len = b_head->keyStoreLen;
    b_head->keyCount--;

    do {

	// if there is enough key to delete or the node has just one key
	sz = &node->keyBuf[ strPos * rec_len ];
	memmove( sz, sz + rec_len, ( --node->keyNum - strPos ) * rec_len );


	//
	// brotherNode <==last== node
	//
	// test wether we can corporate the two node
	//
	//
	if( node->nodeFlag == BtreeYES && node->keyNum > 0 && \
					node->keyNum < b_head->nodeCorpNum )
	{ /////_^_/

            //last <<< last <<< last <<< last <<< last <<<
	    if( node->lastPtrFlag == BtreeYES ) {
		brotherNode = node->lastPtr;
	    } else {
		if( node->lastPtr != NULL ) {
			i = lockTabNode( b_head, node );
			brotherNode = loadBTreeNode( b_head, (long)(node->lastPtr) );
			unlockTabNode( b_head, i );
		} else {
			brotherNode = NULL;
		}
	    }

	    if( brotherNode != NULL ) {
		if( brotherNode->keyNum + node->keyNum <= b_head->upperKeyNum ) {

		   memcpy(&brotherNode->keyBuf[rec_len*brotherNode->keyNum],\
			   node->keyBuf, \
			   rec_len*node->keyNum);

		   brotherNode->keyNum += node->keyNum;
		   setNodeWrFlag(b_head, brotherNode);

		   node->keyNum = 0;

		   goto  bTreeNodeIncorporate_DEL_NODE;
		}
	    }

	    //next >>> next >>> next >>> next >>> next >>>
	    if( node->nextPtrFlag == BtreeYES ) {
		brotherNode = node->nextPtr;
	    } else {
		if( node->lastPtr != NULL ) {
			i = lockTabNode( b_head, node );
			brotherNode = loadBTreeNode( b_head, (long)(node->nextPtr) );
			unlockTabNode( b_head, i );
		} else {
			brotherNode = NULL;
		}
	    }

	    if( brotherNode != NULL ) {
		if( brotherNode->keyNum + node->keyNum <= b_head->upperKeyNum ) {

		   memcpy(&node->keyBuf[rec_len * node->keyNum],\
			   brotherNode->keyBuf, \
			   rec_len * brotherNode->keyNum);

		   node->keyNum += brotherNode->keyNum;
		   setNodeWrFlag(b_head, node);

		   reCalPrefixLen(node, b_head->keyLen);

		   if( strPos <= 0 ) {

			//keep the relation key into buf
			memcpy(buf, node->keyBuf, b_head->keyLen);

			while( node->fatherPtr != (bTREE *)b_head ) {

			    //fathererNode is old node, son node
			    fatherNode = node;

			    if( node->fatherPtrFlag == BtreeYES ) {
				node = node->fatherPtr;
			    } else {
				i = lockTabNode(b_head, fatherNode);
				j = lockTabNode(b_head, brotherNode);
				if( (node = loadBTreeNode( b_head, \
					(long)(node->fatherPtr))) == NULL ) {
				    BtreeErr = 2001;        // cannot load node
				    return  '\1';
				}
				unlockTabNode(b_head, i);
				unlockTabNode(b_head, j);
			    } // end of else

			    //replace the key in father node
			    //
			    //          node
			    //           | find the position in its fathernode
			    //           | replace it into the new key
			    //           ^
			    //       fatherNode

			    strPos = locateSonNode(b_head, node, fatherNode);
			    memcpy(&(node->keyBuf[strPos*rec_len]), buf, b_head->keyLen);

			    reCalPrefixLen(node, b_head->keyLen);
			    setNodeWrFlag(b_head, node);

			    //if not the first key, goto return
			    if( strPos > 0 )	break;

			} // end of while

		   } // end of if

		   (node = brotherNode)->keyNum = 0;

		   goto  bTreeNodeIncorporate_DEL_NODE;
		}
	    }

	} /////_^_/


	// the tree has more than on level
	if( node->keyNum >= 1 ) {

	    setNodeWrFlag(b_head, node);

	    if( strPos <= 0 ) {

		memcpy(buf, node->keyBuf, rec_len );

		while( node->fatherPtr != (bTREE *)b_head ) {

			//fatherNode is old node
			fatherNode = node;

			if( node->fatherPtrFlag == BtreeYES ) {
			     node = node->fatherPtr;
			} else {
			     i = lockTabNode(b_head, fatherNode);
			     if( (node = loadBTreeNode( b_head, \
				(long)(node->fatherPtr))) == NULL ) {
				BtreeErr = 2001;        // cannot load node
				return  '\1';
			     }
			     unlockTabNode(b_head, i);
			} // end of else

			strPos = locateSonNode(b_head, node, fatherNode);
			memcpy(&(node->keyBuf[strPos*rec_len]), buf, b_head->keyLen);

			reCalPrefixLen(node, b_head->keyLen);
			setNodeWrFlag(b_head, node);

			if( strPos > 0 )	break;

		} // end of while
	    } // end of if

	    return  btSUCCESS;

	} // end of if, ELSE ELSE ELSE ELSE ELSE ELSE ELSE ELSE ELSE ELSE

bTreeNodeIncorporate_DEL_NODE:

	// else
	// the node has been empty, put it nito rubbish to wait for reuse
	//
	// if the node's father node is the tree's head
	// this will call this tree to decrease its level
	if( node->fatherPtr == (bTREE *)b_head ) {

		// empty B+ tree has one empty node in my B+ tree
		if( b_head->keyCount <= 0 ) {
			return  btSUCCESS;
		}
		b_head->ptr = brotherNode;
		b_head->nodeMnFlag = BtreeYES;
		brotherNode->fatherPtr = (bTREE *)b_head;
		brotherNode->fatherPtrFlag = BtreeYES;

		putRubbishBlockNo( b_head, node->nodeNo );
		/*
		if( putRubbishBlockNo( b_head, node->nodeNo ) < 0 ) {
			// reindex the tree
			i = lockTabNode( b_head, node );
			if( btreeNodeArrange( b_head ) < 0 ) {
				return  '\1';
			}
			unlockTabNode( b_head, i );
		}*/
		tabMod( b_head, node, BTREE_KEY_RUBBISH);

		return  btSUCCESS;

	} // end of if the node's father node is the tree's head

	// node will be incorporated
	if( node->lastPtrFlag == BtreeYES ) {
		brotherNode = node->lastPtr;
	} else {
		if( node->lastPtr != NULL ) {
			i = lockTabNode( b_head, node );
			brotherNode = loadBTreeNode( b_head, (long)(node->lastPtr) );
			unlockTabNode( b_head, i );
		} else {
			brotherNode = NULL;
		}
	}
	// brotherNode is the base
	if( brotherNode != NULL ) {
		brotherNode->nextPtrFlag = node->nextPtrFlag;
		if( (brotherNode->nextPtr = node->nextPtr) != NULL ) {
			if( node->nextPtrFlag == BtreeYES ) {
				node->nextPtr->lastPtr = brotherNode;
				node->nextPtr->lastPtrFlag = BtreeYES;

				setNodeWrFlag(b_head, node->nextPtr);

			} else {
				long  ll;

				ll = (long)BTREE_HEADBUF_SIZE + \
					(long)BTREE_LASTNODE_POSITION + \
					((long)node->nextPtr-1L) * \
					(long)BTREE_BUFMAXLEN;
				if( lseek(b_head->ndxFp, ll, SEEK_SET) != ll ) {
				    return  '\1';
				}
				if( write( b_head->ndxFp, &(brotherNode->nodeNo), sizeof(long)) == -1 ) {
				    return  '\1';
				}
			}
		} // end of if
	} else {		// brotherNode is NULL, get the next node as base
		if( node->nextPtrFlag == BtreeYES ) {
			brotherNode = node->nextPtr;
		} else {
			if( node->nextPtr != NULL ) {
				i = lockTabNode( b_head, node );
				brotherNode = loadBTreeNode( b_head, (long)(node->nextPtr) );
				unlockTabNode( b_head, i );
			} else {
				brotherNode = NULL;
			}
		}
		if( brotherNode != NULL ) {
			brotherNode->lastPtrFlag = node->lastPtrFlag;
			if( (brotherNode->lastPtr = node->lastPtr) != NULL ) {
				if( node->lastPtrFlag == BtreeYES ) {
					node->lastPtr->nextPtr = brotherNode;
					node->lastPtr->nextPtrFlag = BtreeYES;

					setNodeWrFlag(b_head, node->lastPtr);

				} else {
					long  ll;

					ll = (long)BTREE_HEADBUF_SIZE + \
						(long)BTREE_NEXTNODE_POSITION + \
						((long)node->lastPtr-1L) * \
						(long)BTREE_BUFMAXLEN;
					if( lseek(b_head->ndxFp, ll, SEEK_SET) != ll ) {
					    return  '\1';
					}
					if( write( b_head->ndxFp, &(brotherNode->nodeNo), sizeof(long)) == -1 ) {
					    return  '\1';
					}
				}
			}
		} // end of if
	} // end of else

	if( brotherNode != NULL ) {
	    setNodeWrFlag(b_head, brotherNode);
	} else
	{ //both of the node's lastNode and nextNode is NULL
	  //decrease one level
	  //
	  //      fatherNode's father
	  //           |           \   point to this directly
	  //      fatherNode         \
	  //           |              \|
	  //   NO  <+++node+++> NO     |
	  //                       ----+
	  //
	  
	    //save the leaf
	    brotherNode = node;

	    do {

		if( node->fatherPtrFlag == BtreeYES ) {

		    if( node->fatherPtr == (bTREE *)b_head ) {
			b_head->ptr = brotherNode;
			b_head->nodeMnFlag = BtreeYES;
			
			//brotherNode->nodeFlag = BtreeYES;
			brotherNode->fatherPtrFlag = BtreeYES;
			brotherNode->fatherPtr = (bTREE *)b_head;

			/*if( brotherNode != node ) {
			    putRubbishBlockNo( b_head, node->nodeNo );
			    tabMod( b_head, node, BTREE_KEY_RUBBISH );
			}*/

			if( brotherNode->keyNum <= 0 ) {
			    brotherNode->prefixLen = 0;
			}

			setNodeWrFlag(b_head, brotherNode);
			break;
		    }

		    fatherNode = node->fatherPtr;

		} else {
		    i = lockTabNode( b_head, node );
		    j = lockTabNode( b_head, brotherNode );
		    fatherNode = loadBTreeNode( b_head, (long)(node->fatherPtr));
		    unlockTabNode( b_head, i );
		    unlockTabNode( b_head, j );
		}
		if( fatherNode == NULL ) {
		    BtreeErr = 2001;        // cannot load node
		    return  '\1';
		}

		putRubbishBlockNo( b_head, fatherNode->nodeNo );
		tabMod( b_head, fatherNode, BTREE_KEY_RUBBISH );

		node = fatherNode;
	    } while ( 1 );

	    return  '\0';

	} //end of if( brotherNode == NULL )


	if( node->fatherPtrFlag == BtreeYES ) {
		fatherNode = node->fatherPtr;
	} else {
		i = lockTabNode( b_head, node );
		j = lockTabNode( b_head, brotherNode );
		fatherNode = loadBTreeNode( b_head, (long)(node->fatherPtr));
		unlockTabNode( b_head, i );
		unlockTabNode( b_head, j );
	}
	if( fatherNode == NULL ) {
		BtreeErr = 2001;        // cannot load node
		return  '\1';
	}
	/*else {
		node->fatherPtrFlag = BtreeYES;
	}*/

	if( (strPos = locateSonNode( b_head, fatherNode, node ) ) < 0 ) {
		BtreeErr = 3004;
		return  '\1';
	} // end of if

	// set the write flag
	//Error,1997.3.25. setNodeWrFlag(b_head, brotherNode->nodeNo);
	//setNodeWrFlag(b_head, fatherNode);

	putRubbishBlockNo( b_head, node->nodeNo );
	/*
	if( putRubbishBlockNo( b_head, node->nodeNo ) < 0 ) {
		// reindex the tree
		i = lockTabNode( b_head, node );
		if( btreeNodeArrange( b_head ) < 0 ) {
			return  '\1';
		}
		unlockTabNode( b_head, i );
	}*/
	tabMod( b_head, node, BTREE_KEY_RUBBISH);

	node = fatherNode;

    } while( 1 );

    return  '\0';

} // end of function bTreeNodeIncorporate()



/*==============
*                               getRubbishBlockNo()
*===========================================================================*/
long  getRubbishBlockNo( bHEAD *b_head )
{
    /*
    if( b_head->rubbishBlockNum >= 0 ) {
		return (b_head->rubbishBlockNum)--;
    }
    return  -1;
    */
    long  nodeNo;
    long  ll;

    if( b_head->rubbishBlockNum <= 0 )
	return  -1;

    nodeNo = b_head->rubbishBlockNo[0];

    ll = (nodeNo-1)*(long)BTREE_BUFMAXLEN+(long)BTREE_HEADBUF_SIZE;
    if( lseek( b_head->ndxFp, ll, SEEK_SET ) != ll ) {
	return  -1;
    }

    read( b_head->ndxFp, &b_head->rubbishBlockNo[0], sizeof(long));
    (b_head->rubbishBlockNum)--;

    return  nodeNo;


} // end of function getRubbishBlockNo()


/*==============
*                               putRubbishBlockNo()
*===========================================================================*/
short  putRubbishBlockNo( bHEAD *b_head, long nodeNo )
{
    /*
    if( b_head->rubbishBlockNum < BTREE_RUBBISHBLOCKNO ) {
	b_head->rubbishBlockNo[ ++(b_head->rubbishBlockNum) ] = nodeNo;
	return  b_head->rubbishBlockNum;
    }

    return  -1;
    */
    long  ll;

    ll = (nodeNo-1) * (long)BTREE_BUFMAXLEN + (long)BTREE_HEADBUF_SIZE;
    if( lseek( b_head->ndxFp, ll, SEEK_SET ) != ll ) {
	return  -1;
    }

    if( write( b_head->ndxFp, &b_head->rubbishBlockNo[0], sizeof(long)) == -1 ) {
	return  -1;
    }

    b_head->rubbishBlockNo[0] = nodeNo;

    return  ++(b_head->rubbishBlockNum);

}  // end of function putRubbishBlockNo()


/***************
*                              IndexRecIns()
*****************************************************************************/
_declspec(dllexport) short IndexRecIns( bHEAD *b_head, char *keyContent, long int recNo )
{
    bHEAD *pbh;

    if( (b_head == NULL)  ||  (keyContent == NULL) ) {
	BtreeErr = 2001;
	return  0;
    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );

	b_head->timeStamp = ++(pbh->timeStamp);

	//restoreBtreeEnv(b_head);
    } else
#endif
      {
           pbh = b_head;
#ifdef WSToMT
	   EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    if( KeyLocateIntoNodeAndPos(pbh, keyContent, BTREE_LAST_KEY) != btSUCCESS ) {
#ifdef WSToMT
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  0;
    }

    //1998.8.4
    //should clear the key buffer
    //memmove( _BtreeKeyBuf, keyContent, pbh->keyLen );
    memset(_BtreeKeyBuf, 0, pbh->keyLen);
    strncpy(_BtreeKeyBuf, keyContent, BTREE_MAX_KEYLEN);

    *(long *)&_BtreeKeyBuf[ pbh->keyLen ] = recNo;
    _BtreeKeyBuf[pbh->key4Len] = BtreeNO;

    if( bTreeNodeMove( pbh ) != btSUCCESS ) {
#ifdef WSToMT
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  0;
    }

#ifdef WSToMT
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  1;

} // end of function IndexRecIns()


/***************
*                                 IndexRecDel()
* RETURN:
*    the recno of the last delete
*****************************************************************************/
_declspec(dllexport) long int IndexRecDel(bHEAD *b_head, char *keyContent, long int recNo)
{
    long int  tmpRecNo, tmpRecNo2;
    bHEAD    *pbh;
    char      buf[BTREE_MAX_KEYLEN];

    if( (b_head == NULL) || (keyContent == NULL) ) {
	BtreeErr = 2001;
	return  LONG_MIN;
    }

    memset(buf, 0, b_head->keyLen);
    strncpy(buf, keyContent, BTREE_MAX_KEYLEN);

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );

	b_head->timeStamp = ++(pbh->timeStamp);

	//restoreBtreeEnv(b_head);
    } else
#endif
      {
           pbh = b_head;
#ifdef WSToMT
	   EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    if( (tmpRecNo = IndexLocateAndGetRecNo(pbh, buf)) == LONG_MIN ) {
#ifdef WSToMT
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  LONG_MIN;
    }

    if( recNo != LONG_MIN ) {
	for( ; ; ) {
		if( recNo == tmpRecNo ) {
			break;
		}
		if( (tmpRecNo = IndexStrEqSkipInside(pbh, buf, 1)) == LONG_MIN ) {
#ifdef WSToMT
                        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
			return  LONG_MIN;
		}
	} // end of for
	bTreeNodeIncorporate(pbh);
#ifdef WSToMT
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  tmpRecNo;
    }

    for( ; ; ) {
	bTreeNodeIncorporate(pbh);
	if( (tmpRecNo2 = IndexLocateAndGetRecNo(pbh, buf)) == \
								LONG_MIN ) {
		break;
	}
	tmpRecNo = tmpRecNo2;
    } // end of for

#ifdef WSToMT
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  tmpRecNo;

} // end of function IndexRecDel()


/***************
*                               IndexRecUpdate()
*****************************************************************************/
_declspec(dllexport) short IndexRecUpdate(bHEAD *b_head, char *src, char *dst, long int recNo)
{
    bHEAD *pbh;

    if( (b_head == NULL) || (src == NULL) || (dst == NULL) ) {
	BtreeErr = 2001;
	return  SHRT_MIN;
    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );
	//restoreBtreeEnv(b_head);
    } else
#endif
      {
	   pbh = b_head;
#ifdef WSToMT
	   EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    if( (recNo = IndexRecDel(pbh, src, recNo)) == LONG_MIN ) {
#ifdef WSToMT
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  SHRT_MIN;
    }

#ifdef WSToMT
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  IndexRecIns( b_head, dst, recNo );

} // end of function IndexRecUpdate()


/*==============
*                               IndexLocateAndGetRecNo()
*===========================================================================*/
long int  IndexLocateAndGetRecNo( bHEAD *b_head, char *keyContent )
{
    short  	recLen;
    short  	keyLen;
    char 	*sz;

    if( KeyLocateIntoNodeAndPos( b_head, keyContent, \
			BTREE_FIRST_KEY|BTREE_SEARCH_FOR_FIND ) != btSUCCESS ) {
	if( b_head->type != BTREE_FOR_ITSELF ) {
	    invalidDbfPtr( b_head );
	}
	return  LONG_MIN;
    }

    //1998.2.19
    if( b_head->nodeCurPos >= b_head->CurNodePtr->keyNum ) {
	if( b_head->type != BTREE_FOR_ITSELF ) {
	    invalidDbfPtr( b_head );
	}
	return  LONG_MIN;
    }

    keyLen = b_head->keyLen;
    recLen = b_head->keyStoreLen;

    sz = &(b_head->CurNodePtr->keyBuf[b_head->nodeCurPos*recLen]);
    if( b_head->type != BTREE_FOR_ITSELF ) {
	moveDbfPtr( b_head, *(long *)(sz+keyLen) );
    }

#ifdef euObjectBtree
    if( (b_head->type & FOROBJMASK) == 0 )
#endif

    {
	if( bStrCmp( sz, keyContent, keyLen ) != 0 ) {
		if( b_head->type != BTREE_FOR_ITSELF ) {
		    invalidDbfPtr( b_head );
		}
		return  LONG_MIN;
	}
	if( b_head->nodeCurPos < 1 ) {
		// skip to the first equal keyword
		while( IndexStrEqSkipInside( b_head, keyContent, -1 ) != LONG_MIN );
	}
    }

#ifdef euObjectBtree

    else {
	if( (b_head->bObject.ObjDealFuns.ObjCompFun)( &(b_head->CurNodePtr->keyBuf[ \
			b_head->nodeCurPos * recLen ]), \
			keyContent, strlen( keyContent )) != 0 ) {
		return  LONG_MIN;
	}
    }
#endif
    return  *(long *)&(b_head->CurNodePtr->keyBuf[keyLen + \
						b_head->nodeCurPos*recLen]);

} // end of function IndexLocateAndGetRecNo()


/*==============
*                               btreeNodeArrange()
*  Function:
*      make all the spilt node into the end of the tree then deal them.
*
* !!!this function has no use now.
*===========================================================================*/
short  btreeNodeArrange( bHEAD *b_head )
{
    bTREE    *node;
    long int  newNodeNo;
    long      ll;

    return  1;

    for( ;  b_head->rubbishBlockNum >= 0;  (b_head->rubbishBlockNum)-- ) {

	newNodeNo = b_head->rubbishBlockNo[b_head->rubbishBlockNum];

	if( (node = loadBTreeNode( b_head, b_head->nodeMaxNo )) == NULL ) {
		return  -1;
	}

	node->nodeNo = newNodeNo;
	(b_head->nodeMaxNo)--;

	// if the node's father node is in memory, turn the flag into not
	if( node->fatherPtrFlag == BtreeNO && node->fatherPtr != NULL && \
					node->fatherPtr != (bTREE *)b_head ) {
		ll = (long)BTREE_HEADBUF_SIZE + \
				     (long)BTREE_FATHERNODE_POSITION + \
				     ((long)node->fatherPtr-1L)*(long)BTREE_BUFMAXLEN;
		if( lseek(b_head->ndxFp, ll, SEEK_SET) != ll ) {
		    return  -1;
		}
		if( write( b_head->ndxFp, &(node->nodeNo), sizeof(long)) == -1 ) {
		    return  -1;
		}
	}  // end of if

	// next brother
	if( node->nextPtrFlag == BtreeNO && node->nextPtr != NULL ) {
		ll = (long)BTREE_HEADBUF_SIZE + \
				     (long)BTREE_LASTNODE_POSITION + \
				     ((long)node->nextPtr-1L)*(long)BTREE_BUFMAXLEN;
		if( lseek(b_head->ndxFp, ll, SEEK_SET) != ll ) {
		    return  -1;
		}
		if( write( b_head->ndxFp, &(node->nodeNo), sizeof(long)) == -1 ) {
		    return  -1;
		}
	}  // end of if

	// last brother
	if(node->lastPtrFlag == BtreeNO && node->lastPtr != NULL ) {
		ll = (long)BTREE_HEADBUF_SIZE + \
				     (long)BTREE_NEXTNODE_POSITION + \
				     ((long)node->lastPtr-1L)*(long)BTREE_BUFMAXLEN;
		if( lseek(b_head->ndxFp, ll, SEEK_SET) != ll ) {
		    return  -1;
		}
		if( write( b_head->ndxFp, &(node->nodeNo), sizeof(long)) == -1 ) {
		    return  -1;
		}
	}

	// itself, not a leaf of tree
	if( node->nodeFlag == BtreeNO ) {
		// deal the son node when it is not in memory
		modSonNode( b_head, node, BTREE_NODENO_CHANGE );
	}

    } // end of for

    // change the index file size
    chsize( b_head->ndxFp, (long)(BTREE_HEADBUF_SIZE) + b_head->nodeMaxNo * \
						BTREE_BUFMAXLEN );
    return  1;

} // end of function btreeNodeArrange()


/****************
*                                  IndexSeek()
*****************************************************************************/
_declspec(dllexport) long int IndexSeek( bHEAD *b_head, char *keyContent )
{
    bHEAD *pbh;
    long   l;
    char   indexType;
    char   buf[BTREE_MAX_KEYLEN];

    if( b_head == NULL || keyContent == NULL ) {
	BtreeErr = 2001;
	return  LONG_MIN;
    }

    memset(buf, 0, b_head->keyLen);
    strncpy(buf, keyContent, BTREE_MAX_KEYLEN);
    /*if( *keyContent == '\0' ) {
	keyContent = BTREE_BLANKKEY;
    }*/

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	//restoreBtreeEnv(b_head);
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );
    } else
#endif
      {
	   pbh = b_head;
#ifdef WSToMT
	   EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
      }

    if( pbh->keyCount <= 0 ) {
#ifdef WSToMT
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	if( b_head->type != BTREE_FOR_ITSELF ) {
	    invalidDbfPtr( b_head );
	}
	return  LONG_MIN;
    }

    //to avoid moveDbfPtr(pbh)
    indexType = pbh->type;
    pbh->type = BTREE_FOR_ITSELF;
    l = IndexLocateAndGetRecNo(pbh, buf);
    pbh->type = indexType;

#ifdef WSToMT
    saveBtreeEnv(b_head);
    if( l == LONG_MIN ) {
	if( b_head->type != BTREE_FOR_ITSELF ) {
	    invalidDbfPtr( b_head );
	}
	/*if( pbh->CurNodePtr != NULL ) {
	    moveDbfPtr(b_head, *(long *)&(pbh->CurNodePtr->keyBuf[pbh->keyLen]));
	}*/
    } else {
	moveDbfPtr(b_head, l);
    }
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  l;

/*this is run in IndexSkip() whick is called with IndexLocateAndGetRecNo()
    if( (b_head->type == BTREE_FOR_OPENDBF) || (b_head->type == BTREE_FOR_CLOSEDBF) ) {
	dseek( b_head->dbfPtr, (recNo - 1L), dSEEK_SET );
    }
*/

} // end of function IndexSeek()


/****************
*                                  IndexLocate()
*****************************************************************************/
_declspec(dllexport) long int IndexLocate( bHEAD *b_head, char *keyContent )
{
    bHEAD *pbh;
    long   l;
    char   indexType;
    char   buf[BTREE_MAX_KEYLEN];

    if( b_head == NULL || keyContent == NULL ) {
	BtreeErr = 2001;
	return  LONG_MIN;
    }

    memset(buf, 0, b_head->keyLen);
    strncpy(buf, keyContent, BTREE_MAX_KEYLEN);
    /*if( *keyContent == '\0' ) {
	keyContent = BTREE_BLANKKEY;
    }*/

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	//restoreBtreeEnv(b_head);
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );
    } else
#endif
      {
	   pbh = b_head;
#ifdef WSToMT
	   EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
      }

    if( pbh->keyCount <= 0 ) {
#ifdef WSToMT
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	moveDbfPtr(b_head, 0);
	return  LONG_MIN;
    }

    //to avoid moveDbfPtr(pbh)
    indexType = pbh->type;
    pbh->type = BTREE_FOR_ITSELF;
    l = IndexLocateAndGetRecNo(pbh, buf);
    pbh->type = indexType;

#ifdef WSToMT
    saveBtreeEnv(b_head);
    if( l == LONG_MIN ) {
	if( pbh->CurNodePtr != NULL ) {
	    moveDbfPtr(b_head, *(long *)&(pbh->CurNodePtr->keyBuf[pbh->keyLen]));
	}
    } else {
	moveDbfPtr(b_head, l);
    }
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  l;

} // end of function IndexLocate()



/****************
*                                 IndexSkip()
* Return:
*      Success:         the record no
*      Fail:            0 (error no in BtreeErr), negative of the record
*                       no(cannot skip so much steps, skip to the limit)
*****************************************************************************/
_declspec(dllexport) long int IndexSkip( bHEAD *b_head, long int count )
{
    bHEAD       *pbh;
    bTREE 	*nodeTree;
    short 	keyNum, nodeCurPos;
    long int 	recNo;
    char 	flag = BtreeNO;

    if( b_head == NULL ) {
	BtreeErr = 2001;
	return  LONG_MIN;
    }
#ifdef WSToMT
    if( b_head->pbh != NULL ) {
        pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );
	restoreBtreeEnv(b_head);
    } else
#endif
      {
           pbh = b_head;
#ifdef WSToMT
           EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }
    nodeTree = pbh->CurNodePtr;
    nodeCurPos = pbh->nodeCurPos;

    if( count == 1 && nodeCurPos <= nodeTree->keyNum - 2 ) {

	short 	i = (++(pbh->nodeCurPos)) * pbh->keyStoreLen;

	recNo = *(long *)&(nodeTree->keyBuf[i + pbh->keyLen]);
	if( pbh->type != BTREE_FOR_ITSELF ) {
	    moveDbfPtr(b_head, recNo);
	}

#ifdef WSToMT
	saveBtreeEnv(b_head);
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  recNo;
    }

    if( count == -1 && nodeCurPos > 0 ) {

	short 	i = (--(pbh->nodeCurPos)) * pbh->keyStoreLen;

	recNo = *(long *)&(nodeTree->keyBuf[i + pbh->keyLen]);
	if( pbh->type != BTREE_FOR_ITSELF ) {
	    moveDbfPtr(b_head, recNo );
	}

#ifdef WSToMT
	saveBtreeEnv(b_head);
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  recNo;
    }

    if( count > 0 ) {

	// in this {} we use recPos as keyNum in this node
	// nodeCurPos is the position in this node
	keyNum = nodeTree->keyNum;
	while( (nodeTree->nextPtr != NULL) && (keyNum <= nodeCurPos+count) ) {
		count -= keyNum - nodeCurPos;
		if( nodeTree->nextPtrFlag == BtreeYES ) {
			keyNum = (nodeTree = nodeTree->nextPtr)->keyNum;

			increaseNodeUseTime(pbh, nodeTree);
			//locateTabNode(pbh, nodeTree->nodeNo);
		} else {
			if( (nodeTree = (bTREE *)loadBTreeNode( pbh, \
					(long)nodeTree->nextPtr))  == NULL ) {
#ifdef WSToMT
				LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
				BtreeErr = 2001;        // cannot load node

				if( b_head->type != BTREE_FOR_ITSELF ) {
					invalidDbfPtr( b_head );
				}
				return  LONG_MIN;
			}
			keyNum = nodeTree->keyNum;
	       }
	       nodeCurPos = 0;
	} // end of while

	// if the tree cannot skip so much steps. goto the bottom
	if( keyNum > nodeCurPos+count ) {
		nodeCurPos += (short)count;
	} else {
		nodeCurPos = keyNum - 1;
		flag = BtreeYES;
	}

    } else if( count < 0 ) {

	while( (nodeTree->lastPtr != NULL) && (nodeCurPos+count < 0) ) {
		count += nodeCurPos + 1;
		if( nodeTree->lastPtrFlag == BtreeYES ) {
			nodeTree = (bTREE *)nodeTree->lastPtr;

			increaseNodeUseTime(pbh, nodeTree);
			//locateTabNode(pbh, nodeTree->nodeNo);
		} else {
			if( (nodeTree = (bTREE *)loadBTreeNode( pbh, \
					(long)nodeTree->lastPtr) ) == NULL ) {
#ifdef WSToMT
                                LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
				BtreeErr = 2001;        // cannot load node

				if( b_head->type != BTREE_FOR_ITSELF ) {
					invalidDbfPtr( b_head );
				}
				return  LONG_MIN;
			}
		}
		nodeCurPos = nodeTree->keyNum - 1;
	}

	// if the tree cannot skip so much steps. goto the head
	if( nodeCurPos+count < 0 ) {
		flag = BtreeYES;
		nodeCurPos = 0;
	} else {
		nodeCurPos += (short)count;
	}

    } else {    //if( count == 0 )


#ifdef WSToMT
	saveBtreeEnv(b_head);
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return *(long *)&(nodeTree->keyBuf[\
			nodeCurPos*(pbh->keyStoreLen)+pbh->keyLen]);

    }

    pbh->CurNodePtr = nodeTree;
    pbh->nodeCurPos = nodeCurPos;
    if( nodeCurPos < nodeTree->keyNum ) {
	recNo = *(long *)&nodeTree->keyBuf[ nodeCurPos*(pbh->keyStoreLen)+pbh->keyLen ];
	if( pbh->type != BTREE_FOR_ITSELF ) {
	    moveDbfPtr(b_head, recNo);
	}
    }

#ifdef WSToMT
    saveBtreeEnv(b_head);
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    if( flag == BtreeNO ) 	return  recNo;


    if( b_head->type != BTREE_FOR_ITSELF ) {
	invalidDbfPtr( b_head );
    }

    return  LONG_MIN;

} // end of function IndexSkip()


/*---------------
*                                 IndexGoto1stEqRec()
*---------------------------------------------------------------------------*/
_declspec(dllexport) long int IndexGotoNstEqRec(bHEAD *b_head, char *keyContent, long int count)
{
    long int recNo;
    bHEAD   *pbh;

    if( b_head == NULL ) {
	BtreeErr = 2001;
	return  LONG_MIN;
    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	pbh = b_head->pbh;
        EnterCriticalSection( &(pbh->dCriticalSection) );
	restoreBtreeEnv(b_head);
    } else
#endif
      {
           pbh = b_head;
#ifdef WSToMT
           EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    if( (recNo = IndexSeek(pbh, keyContent)) == LONG_MIN ) {
#ifdef WSToMT
	saveBtreeEnv(b_head);
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return LONG_MIN;
    }

    if( count <= 0 ) {
#ifdef WSToMT
	saveBtreeEnv(b_head);
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  recNo;
    }

    if( (recNo = IndexSkip(pbh, count)) == LONG_MIN ) {
#ifdef WSToMT
	saveBtreeEnv(b_head);
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return LONG_MIN;
    }

    if( bStrCmp(keyContent, &pbh->CurNodePtr->keyBuf[pbh->nodeCurPos*\
			(pbh->keyStoreLen)], pbh->keyLen) != 0 ) {
	IndexSkip(b_head, -count);
#ifdef WSToMT
        saveBtreeEnv(b_head);
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  LONG_MIN;
    }

#ifdef WSToMT
    saveBtreeEnv(b_head);
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return recNo;

} // end of function IndexGoto1stEqRec()


/****************
*                                 IndexEqSkip()
*****************************************************************************/
_declspec(dllexport) long int IndexEqSkip( bHEAD *b_head, long int count )
{

    char      buf[BTREE_MAX_KEYLEN];
    long int  recNo;
    bHEAD     *pbh;

    if( b_head == NULL ) {
	BtreeErr = 2001;
	return  LONG_MIN;
    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );
	restoreBtreeEnv(b_head);
    } else
#endif
      {
	   pbh = b_head;
#ifdef WSToMT
	   EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    if( count == 1 && pbh->nodeCurPos <= pbh->CurNodePtr->keyNum - 2 ) {

	char 	*s = pbh->CurNodePtr->keyBuf;
	short 	i = (pbh->nodeCurPos+1) * pbh->keyStoreLen;

	if( bStrCmp(&s[i], &s[i-pbh->keyStoreLen], pbh->keyLen) != 0 )
	{
#ifdef WSToMT
	    saveBtreeEnv(b_head);
	    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

	    if( b_head->type != BTREE_FOR_ITSELF ) {
		invalidDbfPtr( b_head );
	    }
	    return	LONG_MIN;
	}

	(pbh->nodeCurPos)++;
	recNo = *(long *)&s[i + pbh->keyLen];

	if( pbh->type != BTREE_FOR_ITSELF ) {
	    moveDbfPtr(b_head, recNo);
	}

#ifdef WSToMT
	saveBtreeEnv(b_head);
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  recNo;
    }

    if( count == -1 && pbh->nodeCurPos > 0 ) {

	char 	*s = pbh->CurNodePtr->keyBuf;
	short 	i = (pbh->nodeCurPos-1) * pbh->keyStoreLen;

	if( bStrCmp(&s[i], &s[i+pbh->keyStoreLen], pbh->keyLen) != 0 )
	{
#ifdef WSToMT
            saveBtreeEnv(b_head);
            LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

	    if( b_head->type != BTREE_FOR_ITSELF ) {
		invalidDbfPtr( b_head );
	    }
	    return	LONG_MIN;
	}

	(pbh->nodeCurPos)--;
	recNo = *(long *)&s[i + pbh->keyLen];

	if( pbh->type != BTREE_FOR_ITSELF ) {
	    moveDbfPtr(b_head, recNo);
	}
#ifdef WSToMT
	saveBtreeEnv(b_head);
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  recNo;
    }

    memcpy( buf, &pbh->CurNodePtr->keyBuf[ \
		 pbh->nodeCurPos*(pbh->keyStoreLen)], pbh->keyLen );

    // now dbf pointer move come from IndexSkip()
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if( (recNo = IndexSkip(pbh, count)) == LONG_MIN ) {
#ifdef WSToMT
	saveBtreeEnv(b_head);
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

	if( b_head->type != BTREE_FOR_ITSELF ) {
	    invalidDbfPtr( b_head );
	}
	return LONG_MIN;
    }

    if( bStrCmp(buf, &pbh->CurNodePtr->keyBuf[pbh->nodeCurPos * \
				(pbh->keyStoreLen)], pbh->keyLen) != 0 ) {
	IndexSkip(pbh, -count);
#ifdef WSToMT
	saveBtreeEnv(b_head);
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

	if( b_head->type != BTREE_FOR_ITSELF ) {
	    invalidDbfPtr( b_head );
	}
	return  LONG_MIN;
    }

#ifdef WSToMT
    saveBtreeEnv(b_head);
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return recNo;

} // end of function IndexEqSkip()


/****************
*                                 IndexStrEqSkip()
* inside use function, now I have to take it support ohtside, for asqlana.c use
*****************************************************************************/
long int  IndexStrEqSkip( bHEAD *b_head, char *buf, int count )
{

    long int recNo;
    //short    keyLen = strlen(buf);
    short    keyLen;
    bHEAD   *pbh;
    char     indexType;
    char     buff[BTREE_MAX_KEYLEN];

    /*if( *buf == '\0' ) {
	buf = BTREE_BLANKKEY;
	keyLen = 1;
    } else
	if( keyLen > b_head->keyLen )
		keyLen = b_head->keyLen;
    */

    keyLen = b_head->keyLen;
    memset(buff, 0, keyLen);
    strncpy(buff, buf, BTREE_MAX_KEYLEN);

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );

	if( restoreBtreeEnv(b_head) != 0 )
	{ //the old node & position is lost
	    LeaveCriticalSection( &(pbh->dCriticalSection) );

	    if( b_head->type != BTREE_FOR_ITSELF ) {
		invalidDbfPtr( b_head );
	    }
	    return	LONG_MIN;
	}

    } else
#endif
      {
	   pbh = b_head;
#ifdef WSToMT
	   EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    if( count == -1 && pbh->nodeCurPos > 0 ) {

	char 	*s = pbh->CurNodePtr->keyBuf;
	short 	i = (pbh->nodeCurPos-1) * pbh->keyStoreLen;

	if( bStrCmp(&s[i], buff, keyLen) != 0 )
	{
#ifdef WSToMT
	    saveBtreeEnv(b_head);
	    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

	    if( b_head->type != BTREE_FOR_ITSELF ) {
		invalidDbfPtr( b_head );
	    }
	    return	LONG_MIN;
	}

	(pbh->nodeCurPos)--;
	recNo = *(long *)&s[i + pbh->keyLen];

	if( pbh->type != BTREE_FOR_ITSELF ) {
	    moveDbfPtr( b_head, recNo );
	}
#ifdef WSToMT
	saveBtreeEnv(b_head);
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  recNo;
    }

    if( count == 1 && pbh->nodeCurPos <= pbh->CurNodePtr->keyNum - 2 ) {

	char 	*s = pbh->CurNodePtr->keyBuf;
	short 	i = (pbh->nodeCurPos+1) * pbh->keyStoreLen;

	if( bStrCmp(&s[i], buff, keyLen) != 0 )
	{
#ifdef WSToMT
	    saveBtreeEnv(b_head);
	    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

	    if( b_head->type != BTREE_FOR_ITSELF ) {
		invalidDbfPtr( b_head );
	    }
	    return	LONG_MIN;
	}

	(pbh->nodeCurPos)++;
	recNo = *(long *)&s[i + pbh->keyLen];

	if( pbh->type != BTREE_FOR_ITSELF ) {
	    moveDbfPtr( b_head, recNo );
	}
#ifdef WSToMT
	saveBtreeEnv(b_head);
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  recNo;
    }

    // now dbf pointer move come from IndexSkip()
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    indexType = pbh->type;
    pbh->type = BTREE_FOR_ITSELF;
    if( (recNo = IndexSkip(pbh, count)) == LONG_MIN ) {
	pbh->type = indexType;
#ifdef WSToMT
	saveBtreeEnv(b_head);
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

	if( b_head->type != BTREE_FOR_ITSELF ) {
	    invalidDbfPtr( b_head );
	}
	return LONG_MIN;
    }

    if( bStrCmp(buff, &pbh->CurNodePtr->keyBuf[pbh->nodeCurPos * \
				     (pbh->keyStoreLen)], keyLen) != 0 ) {
	IndexSkip(pbh, -count);
	pbh->type = indexType;
#ifdef WSToMT
	saveBtreeEnv(b_head);
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

	if( b_head->type != BTREE_FOR_ITSELF ) {
	    invalidDbfPtr( b_head );
	}
	return  LONG_MIN;
    }

    pbh->type = indexType;
    
    //2000.12.5
    if( indexType != BTREE_FOR_ITSELF ) {
	moveDbfPtr( b_head, recNo );
    }

#ifdef WSToMT
    saveBtreeEnv(b_head);
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return recNo;

} // end of function IndexStrEqSkip()


/****************
*                                 IndexStrEqSkipInside()
* inside use function, now I have to take it support ohtside, for asqlana.c use
*****************************************************************************/
long int  IndexStrEqSkipInside( bHEAD *b_head, char *buf, int count )
{

    long int recNo;
    //short    keyLen = strlen(buf);
    short    keyLen;
    bHEAD   *pbh;

    /*if( *buf == '\0' ) {
	buf = BTREE_BLANKKEY;
	keyLen = 1;
    } else*/
    //if( keyLen > b_head->keyLen )
	keyLen = b_head->keyLen;

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	pbh = b_head->pbh;
    } else
#endif
      {
	   pbh = b_head;
    }

    if( count == -1 && pbh->nodeCurPos > 0 ) {

	char 	*s = pbh->CurNodePtr->keyBuf;
	short 	i = (pbh->nodeCurPos-1) * pbh->keyStoreLen;

	if( bStrCmp(&s[i], buf, keyLen) != 0 )
	{

	    if( b_head->type != BTREE_FOR_ITSELF ) {
		invalidDbfPtr( b_head );
	    }
	    return	LONG_MIN;
	}

	(pbh->nodeCurPos)--;
	recNo = *(long *)&s[i + pbh->keyLen];

	if( pbh->type != BTREE_FOR_ITSELF ) {
	    moveDbfPtr( b_head, recNo );
	}
	return  recNo;
    }

    if( count == 1 && pbh->nodeCurPos <= pbh->CurNodePtr->keyNum - 2 ) {

	char 	*s = pbh->CurNodePtr->keyBuf;
	short 	i = (pbh->nodeCurPos+1) * pbh->keyStoreLen;

	if( bStrCmp(&s[i], buf, keyLen) != 0 )
	{

	    if( b_head->type != BTREE_FOR_ITSELF ) {
		invalidDbfPtr( b_head );
	    }
	    return	LONG_MIN;
	}

	(pbh->nodeCurPos)++;
	recNo = *(long *)&s[i + pbh->keyLen];

	if( pbh->type != BTREE_FOR_ITSELF ) {
	    moveDbfPtr( b_head, recNo );
	}
	return  recNo;
    }

    // now dbf pointer move come from IndexSkip()
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if( (recNo = IndexSkip(pbh, count)) == LONG_MIN ) {
	if( b_head->type != BTREE_FOR_ITSELF ) {
	    invalidDbfPtr( b_head );
	}
	return LONG_MIN;
    }

    if( bStrCmp(buf, &pbh->CurNodePtr->keyBuf[pbh->nodeCurPos * \
				     (pbh->keyStoreLen)], keyLen) != 0 ) {
	IndexSkip(pbh, -count);

	if( b_head->type != BTREE_FOR_ITSELF ) {
	    invalidDbfPtr( b_head );
	}
	return  LONG_MIN;
    }

    return recNo;

} // end of function IndexStrEqSkipInside()



/****************
*                        IndexKeyCount()
* function:
*     count the key number and point the pointer to the last key
*****************************************************************************/
_declspec(dllexport) long int IndexKeyCount( bHEAD *b_head, char *keyContent )
{

    long int 	l;
    short 	keyLen;
    short 	keyStoreLen;
    bHEAD 	*pbh;
    char        buf[BTREE_MAX_KEYLEN];

    // change the type for not move the dFILE rec_p
    //typeReserved = b_head->type;

    //sometime we use IndexKeyCount() to move the pointer to the last equal
    //keyword
    //b_head->type = BTREE_FOR_ITSELF;

    memset(buf, 0, b_head->keyLen);
    strncpy(buf, keyContent, BTREE_MAX_KEYLEN);
    /*if( *keyContent == '\0' ) {
	keyContent = BTREE_BLANKKEY;
    }*/

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	//restoreBtreeEnv(b_head);
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );
    } else
#endif
      {
	   pbh = b_head;
#ifdef WSToMT
	   EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    if( IndexSeek( pbh, keyContent) == LONG_MIN ) {
	//pbh->type = typeReserved;
#ifdef WSToMT
	saveBtreeEnv(b_head);
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return 0;
    }
    l = 1L;

    keyLen = pbh->keyLen;
    keyStoreLen = pbh->keyStoreLen;
    while( 1 ) {
	if( IndexSkip(pbh, 1) == LONG_MIN ) {
		break;
	}

	if( bStrCmp(keyContent, &pbh->CurNodePtr->keyBuf[ \
				pbh->nodeCurPos*keyStoreLen], \
				keyLen) != 0 ) {
		IndexSkip(pbh, -1);
		break;
	}
	l++;
    }

    //b_head->type = typeReserved;
#ifdef WSToMT
    saveBtreeEnv(b_head);
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return l;

} // end of function IndexKeyCount()


/****************
*                                IndexGetKeyContent()
*****************************************************************************/
_declspec(dllexport) char * IndexGetKeyContent(bHEAD *b_head)
{
    bHEAD *pbh;

    if( b_head == NULL ) {      BtreeErr = 2001;        return  NULL;    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );
	restoreBtreeEnv(b_head);
    }
#endif
      else {
           pbh = b_head;
#ifdef WSToMT
           EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    if( (pbh->nodeCurPos < 0) || (pbh->nodeCurPos >= pbh->keyMaxCount) ) {
	BtreeErr = 2005;
#ifdef WSToMT
	saveBtreeEnv(b_head);
	LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  NULL;
    }

    memcpy( _BtreeKeyBuf, &pbh->CurNodePtr->keyBuf[ \
		 pbh->nodeCurPos*(pbh->keyStoreLen)], pbh->keyLen );

    _BtreeKeyBuf[ pbh->keyLen ] = '\0';

#ifdef WSToMT
    saveBtreeEnv(b_head);
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  _BtreeKeyBuf;

}  // end of function IndexGetKeyContent()


/****************
*                                IndexGetKeyRecNo()
*****************************************************************************/
_declspec(dllexport) long int IndexGetKeyRecNo( bHEAD *b_head )
{
    bHEAD  *pbh;
    long    l;

    if( b_head == NULL ) {      BtreeErr = 2001;        return  0;    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );
	restoreBtreeEnv(b_head);
    } else
#endif
      {
           pbh = b_head;
#ifdef WSToMT
           EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    if( (pbh->nodeCurPos < 0) || (pbh->nodeCurPos >= pbh->keyMaxCount) ) {
	BtreeErr = 2005;
#ifdef WSToMT
        saveBtreeEnv(b_head);
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  0;
    }

    l = *(long *)&( pbh->CurNodePtr->keyBuf[ pbh->keyLen + \
				pbh->nodeCurPos * (pbh->keyStoreLen) ] );
#ifdef WSToMT
    saveBtreeEnv(b_head);
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

    return  l;

}  // end of function IndexGetKeyRecNo()



/****************
*                                IndexGoTop()
*****************************************************************************/
_declspec(dllexport) long int IndexGoTop( bHEAD *b_head )
{
    bHEAD   *pbh;
    bTREE   *nodeTree;
    long int recNo;

    if( b_head == NULL ) {
	BtreeErr = 2001;
	return  LONG_MIN;
    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	//restoreBtreeEnv(b_head);
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );

	//when function change the current node and position infomation
	//should change this value, else needn't
	//
	//b_head->timeStamp = ++(pbh->timeStamp);
	//
    } else
#endif
      {
	   pbh = b_head;
#ifdef WSToMT
	   EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    if( pbh->nodeMnFlag == BtreeNO ) {
	if( (nodeTree = (bTREE *)loadBTreeNode( pbh, \
				(long)pbh->ptr)) == NULL ) {
#ifdef WSToMT
		LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
		return  LONG_MIN;
	}
	pbh->ptr = (bTREE *)nodeTree;
	pbh->nodeMnFlag = BtreeYES;
	nodeTree->fatherPtr = (bTREE *)pbh;
	nodeTree->fatherPtrFlag = BtreeYES;
    }   else  nodeTree = pbh->ptr;

    // while repeat until meet the leaf.
    while( nodeTree->nodeFlag != BtreeYES ) {

	/*if( locateTabNode( pbh, nodeTree->nodeNo ) < 0 ) {
#ifdef WSToMT
		LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
		return  LONG_MIN;
	}*/

	// if the last brother node is not in memory.
	if( nodeTree->keyBuf[pbh->key4Len] == BtreeNO ) {
	    if( (nodeTree = (bTREE *)loadBTreeNode( pbh, \
		   *(long *)&(nodeTree->keyBuf[pbh->keyLen]))) == NULL ) {
#ifdef WSToMT
		LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

		if( b_head->type != BTREE_FOR_ITSELF ) {
		    invalidDbfPtr( b_head );
		}
		return  LONG_MIN;
	    }
	} else {
	    if( (nodeTree = (bTREE *)(*(long *)&nodeTree->keyBuf[pbh->keyLen])) \
								== NULL ) {
#ifdef WSToMT
		LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

		if( b_head->type != BTREE_FOR_ITSELF ) {
		    invalidDbfPtr( b_head );
		}
		return  LONG_MIN;
	    }
	}

    } // end of while

    recNo = *(long *)&(nodeTree->keyBuf[pbh->keyLen]);
    pbh->CurNodePtr = nodeTree;
    pbh->nodeCurPos = 0;

    if( pbh->type != BTREE_FOR_ITSELF ) {
	moveDbfPtr(b_head, recNo);
    }

#ifdef WSToMT
    saveBtreeEnv(b_head);
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  recNo;

} // end of function IndexGoTop()


/****************
*                              IndexGoBottom()
*****************************************************************************/
_declspec(dllexport) long int IndexGoBottom( bHEAD *b_head )
{
    bHEAD   *pbh;
    bTREE   *nodeTree;
    short    i, rec_len;
    long int recNo;

    if( b_head == NULL ) {
	BtreeErr = 2001;
	return  LONG_MIN;
    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	//restoreBtreeEnv(b_head);
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );

	//when function change the current node and position infomation
	//should change this value, else needn't
	//
	//b_head->timeStamp = ++(pbh->timeStamp);
	//
    } else
#endif
      {
	   pbh = b_head;
#ifdef WSToMT
	   EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    if( pbh->nodeMnFlag == BtreeNO ) {
	if( (nodeTree = (bTREE *)loadBTreeNode( pbh, \
				     (long)pbh->ptr)) == NULL ) {

		if( b_head->type != BTREE_FOR_ITSELF ) {
		    invalidDbfPtr( b_head );
		}
		return  LONG_MIN;
	}
	pbh->ptr = nodeTree;
	pbh->nodeMnFlag = BtreeYES;
	nodeTree->fatherPtr = (bTREE *)pbh;
	nodeTree->fatherPtrFlag = BtreeYES;
    } else {
	nodeTree = pbh->ptr;
    }

    rec_len = pbh->keyStoreLen;

    while( nodeTree->nodeFlag != BtreeYES ) {

	if( locateTabNode( pbh, nodeTree->nodeNo ) < 0 ) {
#ifdef WSToMT
		LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

		if( b_head->type != BTREE_FOR_ITSELF ) {
		    invalidDbfPtr( b_head );
		}
		return  LONG_MIN;
	}

	if( nodeTree->keyBuf[ (i=(nodeTree->keyNum-1)*rec_len)+\
					     pbh->key4Len] == BtreeNO ) {
	    if( (nodeTree = loadBTreeNode( pbh, \
				*(long *)&(nodeTree->keyBuf[i+pbh->keyLen]) )) == NULL ) {
#ifdef WSToMT
			LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

			if( b_head->type != BTREE_FOR_ITSELF ) {
			    invalidDbfPtr( b_head );
			}
			return  LONG_MIN;
	    }
	} else {
	    if( (nodeTree = (bTREE *)(*(long *)&nodeTree->keyBuf[i+pbh->keyLen])) == \
								     NULL ) {
#ifdef WSToMT
		LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

		if( b_head->type != BTREE_FOR_ITSELF ) {
		    invalidDbfPtr( b_head );
		}
		return  LONG_MIN;
	    }
	} // end of else

    } // end of while

    pbh->nodeCurPos = nodeTree->keyNum - 1;
    pbh->CurNodePtr = nodeTree;
    recNo = *(long *)&(nodeTree->keyBuf[ pbh->nodeCurPos * rec_len + pbh->keyLen]);

    if( pbh->type != BTREE_FOR_ITSELF ) {
	moveDbfPtr(b_head, recNo );
    }

#ifdef WSToMT
    saveBtreeEnv(b_head);
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  recNo;

} // end of function IndexGoBottom()


/****************
*                             IndexCurRecno( )
*****************************************************************************/
_declspec(dllexport) long int IndexCurRecno( bHEAD *b_head )
{
    bHEAD *pbh;
    long   l;

    if( b_head == NULL ) {       BtreeErr = 2001;        return  -1;    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );
	restoreBtreeEnv(b_head);
    } else
#endif
      {
	   pbh = b_head;
#ifdef WSToMT
	   EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    l = *(long *)&pbh->CurNodePtr->keyBuf[ pbh->keyLen + \
				pbh->nodeCurPos * pbh->keyStoreLen ];

#ifdef WSToMT
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

    return  l;

} // end of function IndexCurRecno()


/****************
*                                 IndexEof()
*****************************************************************************/
_declspec(dllexport) short IndexEof(bHEAD *b_head)
{
    bHEAD *pbh;

    if( b_head == NULL ) {       BtreeErr = 2001;        return  -1;    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );
	restoreBtreeEnv(b_head);
    } else
#endif
      {
           pbh = b_head;
#ifdef WSToMT
           EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    if( pbh->CurNodePtr == NULL ) {
#ifdef WSToMT
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
        return  -2;
    }
    if( ( pbh->CurNodePtr->nextPtr == NULL ) && \
		( pbh->CurNodePtr->keyNum == pbh->nodeCurPos+1 ) ) {
#ifdef WSToMT
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  1;
    }

#ifdef WSToMT
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  0;

} // end of function IndexEof()


/****************
*                                 IndexChk()
*****************************************************************************/
short  IndexChk( bHEAD* b_head, dFILE *df )
{
    if( df == NULL )		return  0;
    if( b_head == NULL )	return  1;

    // here we needn't pay attention to dVIEW or dFILE
    if( b_head->keyCount != df->rec_num )	return  2;

    return  0;

} // end of IndexChk()


/****************
*                                 moveDbfPtr()
*****************************************************************************/
static void moveDbfPtr( bHEAD *b_head, long recNo )
{
#ifdef WSToMT
    bHEAD *pbh;

    if( b_head->pbh != NULL )
	pbh = b_head->pbh;
    else
	pbh = b_head;
#endif

    if( (b_head->type == BTREE_FOR_OPENDBF) || (b_head->type == BTREE_FOR_CLOSEDBF) ) {

	//dbfPtr should not be pbh->dbfPtr
	//for dbfPtr counld be a private pointer of dFILE in my bHEAD
	dFILE *dbfPtr = b_head->dbfPtr;
	char tvFlag;

	tvFlag = dbfPtr->op_flag;

	dbfPtr->op_flag = TABLEFLAG;

	if( dbfPtr->pdf == NULL )
	{ //
	  // move original ptr should lock it first
	  //
	    EnterCriticalSection( &(dbfPtr->dCriticalSection) );
	    dseek(dbfPtr, (recNo - 1L), dSEEK_SET);
	    LeaveCriticalSection( &(dbfPtr->dCriticalSection) );
	} else {
	    dseek(dbfPtr, (recNo - 1L), dSEEK_SET);
	}


	dbfPtr->op_flag = tvFlag;

#ifdef WSToMT
	memcpy(_BtreeKeyBuf, &pbh->CurNodePtr->keyBuf[ \
			      pbh->nodeCurPos*(pbh->keyStoreLen)], pbh->keyLen);
	_BtreeKeyBuf[ pbh->keyLen ] = '\0';
#endif
    }

} // end of moveDbfPtr()


/****************
*                                 invalidDbfPtr()
*****************************************************************************/
static void invalidDbfPtr( bHEAD *b_head )
{
#ifdef WSToMT
    bHEAD *pbh;

    if( b_head->pbh != NULL )
	pbh = b_head->pbh;
    else
	pbh = b_head;
#endif

    if( (b_head->type == BTREE_FOR_OPENDBF) || (b_head->type == BTREE_FOR_CLOSEDBF) ) {

	//dbfPtr should not be pbh->dbfPtr
	//for dbfPtr counld be a private pointer of dFILE in my bHEAD
	dFILE *dbfPtr = b_head->dbfPtr;
	char tvFlag;

	tvFlag = dbfPtr->op_flag;

	dbfPtr->op_flag = TABLEFLAG;

	if( dbfPtr->pdf == NULL )
	{ //
	  // move original ptr should lock it first
	  //
	    EnterCriticalSection( &(dbfPtr->dCriticalSection) );
	    dbfPtr->rec_p = -1;
	    LeaveCriticalSection( &(dbfPtr->dCriticalSection) );
	} else {
	    dbfPtr->rec_p = -1;
	}


	dbfPtr->op_flag = tvFlag;

    }

} // end of invalidDbfPtr()



/****************
*                                 IndexSyncDbf()
* char *rec_buf: new value
*****************************************************************************/
short IndexSyncDbf(bHEAD *b_head, char *rec_buf)
{
    short	i;
    char 	keyBuf_1[BTREE_MAX_KEYLEN];             //old value
    char 	keyBuf_2[BTREE_MAX_KEYLEN];		//new value
    dFILE	*df = b_head->dbfPtr;
    char  	typeReserved;

    if( b_head == NULL ) {       BtreeErr = 2001;        return  -1;    }

    if( (b_head->type != BTREE_FOR_OPENDBF) && (b_head->type != BTREE_FOR_CLOSEDBF) )
    {
	BtreeErr = 2001;
	return  -1;
    }

    // change the type for not move the dFILE rec_p
    typeReserved = b_head->type;
    b_head->type = BTREE_FOR_ITSELF;

    //1999/12/22
    saveBtreeEnv(b_head);

    if( df->write_flag.append || deof( df ) ) {

	if( rec_buf != NULL ) {
	    memcpy(df->rec_tmp, df->rec_buf, df->rec_len);
	    memcpy(df->rec_buf, rec_buf, df->rec_len);
	}

	genDbfBtreeKey( b_head, keyBuf_1 );
	i = IndexRecIns( b_head, keyBuf_1, df->rec_p );
	b_head->type = typeReserved;

	if( rec_buf != NULL ) {
	    memcpy(df->rec_buf, df->rec_tmp, df->rec_len);
	}

	//1999/12/22
	restoreBtreeEnv(b_head);

	return  i;
    }

//    l = getRecP( df );
    // save the new recBuf

    memcpy(df->rec_tmp, df->rec_buf, df->rec_len);
    genDbfBtreeKey( b_head, keyBuf_1 );

    if( rec_buf == NULL ) {
	get1rec( df );
    } else {
	memcpy(df->rec_buf, rec_buf, df->rec_len);
    }
    genDbfBtreeKey(b_head, keyBuf_2);

    // restore record buffer
    memcpy(df->rec_buf, df->rec_tmp, df->rec_len);

    if( bStrCmp(keyBuf_1, keyBuf_2, b_head->keyLen ) != 0 ) {
	i = IndexRecUpdate(b_head, keyBuf_1, keyBuf_2, df->rec_no );
	//dseek( df, l-1, dSEEK_SET );
	b_head->type = typeReserved;

	//1999/12/22
	restoreBtreeEnv(b_head);

	return	i;
    }

    //dseek( df, l-1, dSEEK_SET );
    b_head->type = typeReserved;

    //1999/12/22
    restoreBtreeEnv(b_head);

    return  0;

} // end of IndexSyncDbf()


/****************
*                                 genDbfBtreeKey()
*****************************************************************************/
char *genDbfBtreeKey(bHEAD *b_head, char *keyBuf)
{
    int  	len;
    short 	i, j;
    dFIELD	*field;
    dFILE	*df = b_head->dbfPtr;

    len = 0;
    for( i = 0;    i < b_head->keyFieldNum;    i++) {
	field = &(df->field[ df->fld_id[b_head->KeyFldid[i]] ]);
	
	//ommited, 2000.8.5
	/*
	for(j = 0;  j < field->fieldlen;  j++)
		keyBuf[len++] = field->fieldstart[j];
	*/

	for(j = 0;  j < field->fieldlen && field->fieldstart[j];  j++)
		keyBuf[len++] = field->fieldstart[j];
	for( ;  j < field->fieldlen;  j++)
		keyBuf[len++] = ' ';
    }   // end of for
    keyBuf[len] = '\0';

    rtrim(keyBuf);
    /*if( *rtrim(keyBuf) == '\0' ) {
	strcpy(keyBuf, BTREE_BLANKKEY);
    }*/

    return  keyBuf;

} // end of genDbfBtreeKey()


/* calculate the prefixLen in bTREE
*  this algorithm is not usefull when the key is not the same length
*/

/*
static short calPrefixLen(bTREE *bt, unsigned char *str)
{

    short  i, len;
    unsigned char   *s;

    if( memcmp((s=bt->keyBuf), str, (len=bt->prefixLen)) == 0 ) {
	return;
    }

    for(i = 0;  i < len;  i++) {
	if( s[i] != str[i] )	break;
    }
    bt->prefixLen = i;

} //end of calPrefixLen()
*/

/* recalculate the length
*  this occured when the node is break
*/
static void reCalPrefixLen(bTREE *bt, short btKeyLen)
{
    unsigned char *s = bt->keyBuf;
    short          i, j;
    unsigned char  c;
    short    keyStoreLen = btKeyLen+5;

    for(i = 0;   i < btKeyLen;  i++, s++) {
	c = *s;
	for(j = 1;  j < bt->keyNum;  j++) {
		if( c != s[j * keyStoreLen] ) {
			bt->prefixLen = i;
			return;
		}
	}
    }

    bt->prefixLen = i;

} //endof reCalPrefixLen()


/***************
*                               Index2View()
*****************************************************************************/
long Index2View(char *szIndexName, char *lowerKey, char *upperKey, \
		      char *szViewName, char *szDbfName, char *recFileName)
{
    bHEAD 	*b_head;
    dFILE       *df;
    short  	recLen;
    short  	keyLen;
    char 	*sz;
    long 	l;
    char        buf1[BTREE_MAX_KEYLEN], buf2[BTREE_MAX_KEYLEN];



    b_head = IndexOpen(NULL, szIndexName, BTREE_FOR_ITSELF);
    if( b_head == NULL ) {
	return  LONG_MIN;
    }

    memset(buf1, 0, b_head->keyLen);
    lowerKey = strncpy(buf1, lowerKey, BTREE_MAX_KEYLEN);
    memset(buf2, 0, b_head->keyLen);
    upperKey = strncpy(buf2, upperKey, BTREE_MAX_KEYLEN);


    df = dcreate(recFileName, viewField);
    if( df == NULL ) {
	IndexClose(b_head);
	return  LONG_MIN;
    }

    keyLen = b_head->keyLen;
    recLen = b_head->keyStoreLen;

    if( KeyLocateIntoNodeAndPos( b_head, lowerKey, \
					BTREE_FIRST_KEY|BTREE_SEARCH_FOR_FIND ) != btSUCCESS ) {
	return  LONG_MIN;
    }

    sz = &(b_head->CurNodePtr->keyBuf[b_head->nodeCurPos*recLen]);

    if( bStrCmp(sz, lowerKey, keyLen) == 0 ) {

	if( b_head->nodeCurPos < 1 ) {
		// skip to the first equal keyword
		while( IndexStrEqSkipInside( b_head, lowerKey, -1 ) != LONG_MIN );
	}
    }

    //get the btree first
    while( !IndexEof(b_head) ) {
	sz = &(b_head->CurNodePtr->keyBuf[b_head->nodeCurPos*recLen]);
	if( bStrCmp(sz, upperKey, keyLen) > 0 ) {
		break;
	}
	PutField(df, 0, sz+keyLen);
	putrec(df);
	/*printf("%s %ld\n", substr(sz, 0,keyLen) , *(long *)&( b_head->CurNodePtr->keyBuf[ b_head->keyLen + \
				b_head->nodeCurPos * (b_head->keyStoreLen) ]) );
				*/
	IndexSkip(b_head, 1);
    }

    IndexClose(b_head);

    l = df->rec_num;
    dclose(df);
    makeView(szViewName, szDbfName, recFileName, NULL);

    return  l;

} // end of function Index2View()


/***************
*                                 IndexRecMove()
* RETURN:
*    the recno of the last delete
*****************************************************************************/
_declspec(dllexport) long IndexRecMove(bHEAD *b_head, char *keyContent, \
						   long recNo, long newRecNo)
{
    long int  tmpRecNo;
    bHEAD    *pbh;
    char      buf[BTREE_MAX_KEYLEN];

    if( (b_head == NULL) || (keyContent == NULL) ) {
	BtreeErr = 2001;
	return  LONG_MIN;
    }

    memset(buf, 0, b_head->keyLen);
    strncpy(buf, keyContent, BTREE_MAX_KEYLEN);

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );
	restoreBtreeEnv(b_head);
    } else
#endif
      {
           pbh = b_head;
#ifdef WSToMT
           EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    if( (tmpRecNo = IndexLocateAndGetRecNo(pbh, buf)) == LONG_MIN ) {
#ifdef WSToMT
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  LONG_MIN;
    }

    for( ; ; ) {
	if( recNo == tmpRecNo ) {
		break;
	}
	if( (tmpRecNo = IndexStrEqSkipInside(pbh, buf, 1 )) == LONG_MIN ) {
#ifdef WSToMT
		LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
		return  LONG_MIN;
	}
    } // end of for

    *(long *)&pbh->CurNodePtr->keyBuf[ \
		 pbh->nodeCurPos*(pbh->keyStoreLen)+pbh->keyLen] = newRecNo;
    tabMod(pbh, pbh->CurNodePtr, BTREE_KEY_UPDATE);

#ifdef WSToMT
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif

    return  0;

} // end of function IndexRecMove()



/***************
**                            BppTreeBuild()
*****************************************************************************/
_declspec(dllexport) bHEAD *BppTreeBuild(char *ndxName, short keyLen)
{
    bTREE  *nodeTree;
    bHEAD  *b_head;
    short   ndxFp;
    char   *sz;
    char    ndxReallyName[FILENAME_MAX];
    
    // set build ndx environment
    BtreeNodeBufferSize = MAX_BtreeNodeBufferSize;

    strcpy(ndxReallyName, ndxName);

    if( (sz = strchr(ndxReallyName, '.') ) != NULL ) {
	*sz = '\0';
    }
    strcat( ndxReallyName, bIndexExtention);

    // create the index file
    if( (ndxFp = open( ndxReallyName, O_CREAT|O_RDWR|O_TRUNC|O_BINARY|SH_DENYWR, \
						 S_IREAD|S_IWRITE )) < 0 ) {
	BtreeErr = 2007;
	return  NULL;
    }

    if( (b_head = (bHEAD *)zeroMalloc( sizeof(bHEAD) )) == NULL ) {
	close( ndxFp );
	unlink( ndxReallyName);
	BtreeErr = 2002;
	return  NULL;
    }  /* end of if */
    strcpy(b_head->VerMessage, BTREE_VER_MESSAGE);

    // allocate the active_b_tab memory
    if( tabInit( b_head ) < 0 ) {
	close( ndxFp );
	unlink( ndxReallyName);
	BtreeErr = 2002;
	return  NULL;
    }

    b_head->keyLen = keyLen;
    b_head->keyStoreLen = keyLen + 5;
    b_head->key4Len = keyLen + 4;
    b_head->keyFieldNum = 0;
    b_head->keyMaxCount = (BTREE_BUFSIZE - 5) / (b_head->keyStoreLen);
    b_head->nodeBreakPos = (short)(b_head->keyMaxCount * DEF_BREAKPOS);
    b_head->upperKeyNum = (short)(b_head->keyMaxCount * UPPER_BREAKPOS);
    b_head->CurNodePtr = NULL;
    b_head->nodeCurPos = -1;
    b_head->keyCount = 0;
    b_head->ndxFp = ndxFp;
    b_head->type = BTREE_FOR_OBJ;
    b_head->rubbishBlockNum = -1;

    b_head->dbfPtr = NULL;
/*    // ***** create the btree head. *****
    lseek( ndxFp, 0, SEEK_SET);
    write( ndxFp, b_head, BTREE_HEADBUF_SIZE );
*/
    if( (nodeTree = rqMem( b_head, 1 )) == NULL ) {
	IndexClose( b_head );
	BtreeErr = 2002;
	return  NULL;
    } // end of if

    nodeTree->nodeNo = b_head->nodeMaxNo = 1L;

    b_head->nodeMnFlag = BtreeYES;
    b_head->ptr = nodeTree;
    nodeTree->fatherPtr = (bTREE *)b_head;
    nodeTree->nextPtr = NULL;
    nodeTree->lastPtr = NULL;
    nodeTree->nextPtrFlag = BtreeNO;
    nodeTree->lastPtrFlag = BtreeNO;
    nodeTree->fatherPtrFlag = BtreeYES;
    nodeTree->keyNum = 0;
    nodeTree->prefixLen = 0;
    nodeTree->nodeFlag = BtreeYES;

#ifdef RuningMessageOn
    euBusyInfo( 0 );
#endif

    //ndx's filename can be changed!
    strcpy(b_head->ndxName, ndxReallyName);

#ifdef WSToMT
    InitializeCriticalSection( &(b_head->dCriticalSection) );
    b_head->inCriticalSection = '\0';
#endif

    return  b_head;

} // end of function BppTreeBuild()



/***************
*                                 IndexFlush()
*  Function:
*      free head memory and write the btree head message
*****************************************************************************/
_declspec(dllexport) int IndexFlush( bHEAD *b_head )
{
    bTREE   *TempTree;
    short    i;
    long int nodeNo;
    bTAB    *b_tab;
    bHEAD   *pbh;
    long     ll;

    if( b_head == NULL ) {      BtreeErr = 2001;        return  -1;    }

    if( b_head->type == BTREE_FOR_OPENDBF && \
				b_head->dbfPtr->write_flag.file_write == 0 ) {
	return  0;
    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	pbh = b_head->pbh;
	EnterCriticalSection( &(pbh->dCriticalSection) );
	restoreBtreeEnv(b_head);
    } else
#endif
      {
	   pbh = b_head;
#ifdef WSToMT
	   EnterCriticalSection( &(pbh->dCriticalSection) );
#endif
    }

    b_tab = pbh->active_b_tab;
    // first, adjust size of the tree file
    chsize( pbh->ndxFp, (long)(BTREE_HEADBUF_SIZE + \
					pbh->nodeMaxNo * BTREE_BUFMAXLEN) );

    for( i = 0; i < pbh->nodeCurTab; i++) {

	    //2000.12.25
	    if( b_tab[i].nodeUseTimes == 0 )
		continue;

	    TempTree = b_tab[i].nodeAdress;
	    if( (b_tab[i].nodeWrFlag == BtreeYES) || \
						(pbh->ptr == TempTree) )  {
		nodeNo = TempTree->nodeNo;
		ll = (long)(BTREE_HEADBUF_SIZE) + \
				      (nodeNo-1)*BTREE_BUFMAXLEN;
		if( lseek( pbh->ndxFp, ll, SEEK_SET) != ll ) {
#ifdef WSToMT
		    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
		    return  -1;
		}
		if( write( pbh->ndxFp, nodeTurnBuf(pbh, TempTree), \
							BTREE_BUFMAXLEN) == -1 ) {
#ifdef WSToMT
		    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
		    return  -1;
		}
		deNodeTurnBuf(pbh, TempTree);
		//b_tab[i].nodeUseTimes = 0;
		//b_tab[i].nodeNwFlag = BtreeNO;
		b_tab[i].nodeWrFlag = BtreeNO;
	    }
	    /*if( b_head->ptr == TempTree ) {
		b_head->ptr =(bTREE *)(TempTree->nodeNo);
	    }*/
    }  // end of for

    if( lseek(pbh->ndxFp, 0, SEEK_SET) != 0 ) {
	//error
	;
    }
    write(pbh->ndxFp, pbh, sizeof(bHEAD));

#ifdef WSToMT
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  0;

} // end of function IndexFlush()



/*==============
*                              deNodeTurnBuf()
*===========================================================================*/
bTREE * deNodeTurnBuf(bHEAD *b_head, bTREE *tempTree )
{
    short 	i;
    bTAB 	*b_tab;

    // store into temp variable for speed
    b_tab = b_head->active_b_tab;

    // deal the father node
    if( tempTree->fatherPtr == NULL ) {
	tempTree->fatherPtr = (bTREE *)b_head;
	tempTree->fatherPtrFlag = BtreeYES;
	b_head->nodeMnFlag = BtreeYES;
	b_head->ptr = tempTree;
    } else {
	if( (i = locateAllTabNode( b_head,
				(long)tempTree->fatherPtr ) ) < 0 ) {
		tempTree->fatherPtrFlag = BtreeNO;
	} else {
		tempTree->fatherPtr = b_tab[i].nodeAdress;
		tempTree->fatherPtrFlag = BtreeYES;
		ModFather( b_head, tempTree, BTREE_KEY_INSERT);
	}
    } // end of else

    // deal the last node
    if( (i = locateAllTabNode( b_head, \
				(long)tempTree->lastPtr)) < 0 ) {
	tempTree->lastPtrFlag = BtreeNO;
    } else {
	tempTree->lastPtr = b_tab[i].nodeAdress;
	tempTree->lastPtrFlag = BtreeYES;
	tempTree->lastPtr->nextPtr = tempTree;
	tempTree->lastPtr->nextPtrFlag = BtreeYES;
    }

    // deal the next node
    if( (i = locateAllTabNode( b_head, \
				(long)tempTree->nextPtr)) < 0 ) {
	tempTree->nextPtrFlag = BtreeNO;
    } else {
	tempTree->nextPtr = b_tab[i].nodeAdress;
	tempTree->nextPtrFlag = BtreeYES;
	tempTree->nextPtr->lastPtr = tempTree;
	tempTree->nextPtr->lastPtrFlag = BtreeYES;
    }

    // if it isn't leaf, modify the flag
    if( tempTree->nodeFlag == BtreeNO ) {
	modSonNode( b_head, tempTree, BTREE_KEY_INSERT );
    }

    return  tempTree;

} // end of function deNodeTurnBuf()


/*when we use the bTREE v1.5 we can just comp little keywords
* at the binCmp, add the bef+bt->prefixLen and len-=bt->prefixLen
* this cause the compare speed up.
*/

/******************     end of this software btree.c     *******************/
