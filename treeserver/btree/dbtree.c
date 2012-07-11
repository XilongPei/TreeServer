/***************************************************************************
*   DBTREE.C   98
*
*  btree.c single user version
*  Copyright: EastUnion Computer Service Co., Ltd.
*             1991,92,93,94,95
*             Shanghai Tiedao University 1998
* rewrite:
* 	btreeNodeArrange()
* 	getRubbishBlockNo()
* 	putRubbishBlockNo()
* datanode:	next: pointer to next nnode
*               father: left length
*
* 1. strncmp is different from memcmp, for it will stop at '\0', and when the
*    key is binary, it cause an error, 1998.4.22 Xilong
* 2. support variable node size when the G_D_C is created. 1998.5.8
****************************************************************************/

/*^-_-^ ^-_-^ ^-_-^ ^-_-^ ^-_-^ ^-_-^ ^-_-^ ^-_-^ ^-_-^ ^-_-^ ^-_-^ ^-_-^
    //to give c1, c2 an order(not compare as big or little)
    char c, c1, c2;
    c = (unsigned char)c1 - (unsigned char)c2;
    if( c > 0 ) c1 > c2
    ......

    THE UPPER PROGRAM IS ERROR, JUST BECAUSE c IS CHAR DATATYPE

    0-1-------------127--------------255

    132 < 1
    129 < 1

    but:
    132 > 129

 */

//#define DEBUG

//I found that the memcmp is much faster than my function
#define bStrCmpOptimized

#define __DBTREE_C_      "G_C_C_98"
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

#include "wst2mt.h"
#include "mistring.h"
#include "dbtree.h"
#include "memutl.h"

#ifdef RuningMessageOn
#include "busyInfo.h"
#endif

/*****
#include "bugstat.h"
*****/

// one node is 1024 bytes large
#ifdef WSToMT
short BtreeNodeBufferSize = 32;         // buffer number (in blocks)
WSToMT char  _BtreeKeyBuf[BTREE_MAX_KEYLEN];
WSToMT short BtreeErr = 0;
WSToMT long  lastReadSize;
#else
short BtreeNodeBufferSize = 32;         // buffer number (in blocks)
char  _BtreeKeyBuf[BTREE_MAX_KEYLEN];
short BtreeErr = 0;
long  lastReadSize;
#endif

#define      MAX_BtreeNodeBufferSize	(131068/BTREE_BUFMAXLEN)
#define      DEF_BtreeNodeBufferSize	(65536/BTREE_BUFMAXLEN)

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
static short  IndexChk( bHEAD* b_head, void *df );
	/* valid h_head:  0
	*/
#ifndef bStrCmpOptimized
static short bStrCmp(const char *s1, const char *s2, short maxlen);
#else
#define	bStrCmp memcmp
#endif

static char *genDbfBtreeKey(bHEAD *b_head, char *keyBuf);
static int  getRubbishBlockNo( bHEAD *b_head );
static int  putRubbishBlockNo( bHEAD *b_head, long nodeNo );
static bTREE *allocBtreeNode(bHEAD *b_head, long lastNode);
static long int IndexGetKeyRecNo( bHEAD *b_head );
	/*
	*/
static short IndexRecUpdate( bHEAD *b_head, char *src, char *dst, \
				 long int recNo );
	/*
	*/
static long int IndexEqSkip( bHEAD *b_head, long int count );
	/*
	*/
static long int IndexGotoNstEqRec( bHEAD *b_head, char *keyContent, \
					long int count );
	/*
	*/
static long int IndexKeyCount( bHEAD *b_head, char *keyContent );
	/*
	*/
static long int IndexCurRecno( bHEAD* b_head );
	/*
	*/
static short IndexSyncDbf(bHEAD *bh, char *rec_buf);
	/*rec_buf is the record before putrec(), NULL if donnot know.
	*/

static bHEAD * IndexBAwake(char *dbfName, char *fieldName, char *ndxName, \
							   InputDbfType DbfType);
static long IndexRecMove(bHEAD *b_head, char *keyContent, \
						   long recNo, long newRecNo);
bTREE * loadBTreeNode( bHEAD *b_head, long nodeNo );
short  ModFather( bHEAD *b_head, bTREE *sonNode, char type );
short  bIndexHeadCheck( bHEAD *b_head );
void   writeTabNode( bHEAD *b_head, bTREE *tempTree );
short  tabInit( bHEAD *b_head );
char   tabMod( bHEAD *b_head, bTREE *nodeAdr, char type );
bTREE *  nodeTurnBuf( bHEAD *b_head, bTREE *node );
bTREE * deNodeTurnBuf(bHEAD *b_head, bTREE *tempTree );
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
long int  IndexLocateAndGetRecNo( bHEAD *b_head, char *keyContent );
short  btreeNodeArrange( bHEAD *b_head );
static void reCalPrefixLen(bTREE *bt, short btKeyLen);
long int  IndexStrEqSkip( bHEAD *b_head, char *buf, short count );
//static short calPrefixLen(bTREE *bt, unsigned char *str);

/****************
*                          BtNodeNumSet()
*****************************************************************************/
_declspec(dllexport) short  BtNodeNumSet( short NodeNum )
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
		(b_head->nodeSize*b_head->nodeMaxNo + BTREE_HEADBUF_SIZE) ) {
	return  -1;
    }
    if( b_head->keyLen >= BTREE_MAX_KEYLEN || b_head->keyLen <= 0 ) {
	return  -1;
    }

    if( b_head->keyMaxCount < 0 ) {
	return  -1;
    }
    if( b_head->keyFieldNum > MAX_KEYFIELD_NUM || \
						b_head->keyFieldNum < 0 ) {
	return  -1;
    }

    return  1;

} // end of function bIndexHeadCheck()


/***************
*                             dbTreeOpen()
*****************************************************************************/
_declspec(dllexport) bHEAD * dbTreeOpen(char *ndxName)
{
    int 	ndxFp;
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
    /*if( ( filename = strrchr(temp_s, '.') ) == NULL ) {
	if( strlen(temp_s) > 200 ) {
		BtreeErr = 2006;                // name is too long
		free( b_head );
		return  NULL;
	}
    } else {
	*filename = '\0';
    }
    strcat(temp_s, bIndexExtention);
    */

    if( (ndxFp = open(temp_s, O_RDWR|O_BINARY|SH_DENYWR, S_IWRITE|S_IREAD) ) <= 0 ) {
	BtreeErr = 2004;
	free( b_head );
	return  NULL;
    }

    // read in the head
    lseek( ndxFp, 0L, SEEK_SET );
    read( ndxFp, b_head, sizeof(bHEAD) );
    b_head->ndxFp = ndxFp;

    b_head->nodeMnFlag = BtreeNO;
    if( tabInit( b_head ) < 0 ) {
	BtreeErr = 2005;
	close( ndxFp );
	free( b_head );
	return  NULL;
    }
    b_head->CurNodePtr = NULL;
    b_head->nodeCurPos = -1;

    b_head->dbfPtr = NULL;

    // check whether the Btree suit for the dbf. add in 1994.12.5 Xilong
    if( (BtreeErr = IndexChk( b_head, b_head->dbfPtr )) != 0 ) {

	BtreeErr = 2006;
	close( ndxFp );
	free( b_head );
	return  NULL;
    }

    b_head->type = BTREE_FOR_OBJ;
    if( bIndexHeadCheck( b_head ) <= 0 ) {
	BtreeErr = 2007;
	close( ndxFp );

	free( b_head );
	return  NULL;
    }

    //for compitable with previous version to recalculate it
    b_head->nodeCorpNum = (short)(b_head->keyMaxCount * CORP_KEYPER);


#ifdef WSToMT
    InitializeCriticalSection( &(b_head->dCriticalSection) );
    b_head->inCriticalSection = '\0';
#endif

    if( (nodeTree = (bTREE *)loadBTreeNode( b_head, (long)b_head->ptr)) == NULL ) {

		BtreeErr = 2008;
		close( ndxFp );
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
		BtreeErr = 2009;
		close( ndxFp );
		return  NULL;
	    }
	} else {
	    if( (nodeTree = (bTREE *)(*(long *)&nodeTree->keyBuf[b_head->keyLen])) \
								== NULL ) {
		BtreeErr = 2010;
		close( ndxFp );
		return  NULL;
	    }
	}

    } //end of while

    b_head->CurNodePtr = nodeTree;
    b_head->nodeCurPos = 0;

    return  b_head;

} // end of function dbTreeOpen()


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
* load a B++ tree node into memory
*===========================================================================*/
bTREE * loadBTreeNode(bHEAD *b_head, long nodeNo )
{
    short 	i;
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

    lseek( b_head->ndxFp, \
	   (nodeNo-1)*(long)b_head->nodeSize+(long)BTREE_HEADBUF_SIZE, \
	   SEEK_SET );
    if( read( b_head->ndxFp, tempTree, b_head->nodeSize) == -1 ) {
	return  NULL;
    }

    //1998.2.19 Xilong
    if( tempTree->nodeFlag == BtreeDATA ) {
	return  tempTree;
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
    short  i/*, iloop*/;
    bTAB  *b_tab, *b_tab2;

    // store into temp variable for speed
    b_tab = b_head->active_b_tab;

    if( type == BTREE_KEY_REPLACE ) {

	if( (i = (short)nodeAdr) >= b_head->nodeCurTab ) {
		return  1;
	}
	b_tab[i].nodeNwFlag = BtreeNO;
	b_tab[i].nodeWrFlag = BtreeYES;

#ifdef DEBUG
	memset(b_tab[i].nodeAdress, 0, b_head->nodeSize);
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

	return  0;
    }

    //iloop = b_head->nodeCurTab;
    switch( type ) {
	case BTREE_KEY_UPDATE:
	    b_tab2 = (bTAB *)*(long *)((char *)nodeAdr + b_head->nodeSize);
	    b_tab2->nodeWrFlag = BtreeYES;

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
	    b_tab2 = (bTAB *)*(long *)((char *)nodeAdr + b_head->nodeSize);
	    b_tab2->nodeWrFlag = BtreeNO;
	    b_tab2->nodeNwFlag = BtreeNO;
	    b_tab2->nodeUseTimes = 0;     	// set it to no use

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
	    b_tab2 = (bTAB *)*(long *)((char *)nodeAdr + b_head->nodeSize);
	    if( b_tab2->nodeWrFlag == BtreeYES ) {
		writeTabNode( b_head, nodeAdr );
	    }

	    b_tab2->nodeUseTimes = 1;     	// set it to less use

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

	    //iloop = b_head->nodeCurTab;
	    b_tab2 = &b_tab[b_head->nodeCurTab++];

	    b_tab2->nodeAdress = nodeAdr;
	    *(long *)((char *)nodeAdr+b_head->nodeSize) = (long)b_tab2;

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

	    b_tab2->nodeNwFlag = BtreeYES;
	    b_tab2->nodeWrFlag = BtreeYES;
    } // end of switch

    return  0;

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
    bTAB   *b_tab2;

    //1998.3.31, the BtreeDATA node has no father, last, next
    if( node->nodeFlag == BtreeDATA || node->nodeNo <= 0 ) {
	return  node;
    }

    if( node->nodeNo == 9431 ) 
	printf(" ");

    b_tab2 = (bTAB *)*(long *)((char *)node + b_head->nodeSize);


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

	//       next
	// node <------> tempTree
	//

	tempTree->lastPtr = (bTREE *)(node->nodeNo);
	tempTree->lastPtrFlag = BtreeNO;
	node->nextPtr = (bTREE *)(tempTree->nodeNo);
	node->nextPtrFlag = BtreeNO;
    }

    // last brother
    if(node->lastPtrFlag == BtreeYES) {
	tempTree = node->lastPtr;

	//            last
	// tempTree <------> node
	//

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
    long    nodeNo;
    bTAB   *b_tab2;

    b_tab2 = (bTAB *)*(long *)((char *)tempTree + b_head->nodeSize);
    nodeNo = tempTree->nodeNo;

    if( b_tab2->nodeNwFlag == BtreeYES ) {
	chsize( b_head->ndxFp, \
		(long)(BTREE_HEADBUF_SIZE) + b_head->nodeMaxNo * \
		b_head->nodeSize);
    }
    lseek( b_head->ndxFp, \
	     (long)BTREE_HEADBUF_SIZE + (long)b_head->nodeSize * (nodeNo-1), \
	     SEEK_SET );

    b_tab2->nodeNwFlag = BtreeNO;
    b_tab2->nodeWrFlag = BtreeNO;
    write( b_head->ndxFp, nodeTurnBuf(b_head, tempTree), b_head->nodeSize );

    /*
    if( tabNo < 0  ||  tabNo >= b_head->nodeCurTab )
    {       BtreeErr = 2005;        return  -1;    }

    nodeNo = (tempTree = b_head->active_b_tab[ tabNo ].nodeAdress)->nodeNo;

    if( b_head->active_b_tab[tabNo].nodeNwFlag == BtreeYES ) {
	chsize( b_head->ndxFp, \
		(long)(BTREE_HEADBUF_SIZE) + b_head->nodeMaxNo * \
		b_head->nodeSize);
    }
    lseek( b_head->ndxFp, \
	       (long)BTREE_HEADBUF_SIZE + (long)b_head->nodeSize * (nodeNo-1), \
	       SEEK_SET );

    b_head->active_b_tab[tabNo].nodeNwFlag = BtreeNO;
    b_head->active_b_tab[tabNo].nodeWrFlag = BtreeNO;
    write( b_head->ndxFp, nodeTurnBuf(b_head, tempTree), b_head->nodeSize );
    */

} // end of function writeTabNode();


#ifndef bStrCmpOptimized
/*==============
*                               bStrCmp()
*===========================================================================*/
short bStrCmp(const char *s1, const char *s2, short maxlen)
{
    short i;

#ifndef __BORLANDC__
//    register signed char _AL, _AH;
#endif

#ifdef GeneralSupport
    for( i = 0;  i < maxlen;  i++ ) {
	if( s1[i] != s2[i] && s1[i] != '*' && s2[i] != '*' )
	{
		if( s1[i] && s2[i] )		return s1[i] - s2[i];
		return	0;
	}
    }

    return  0;

#else
    maxlen--;
    for( i = 0;  i < maxlen;  i++ ) {
	if( s1[i] != s2[i] )
	{
	    return (unsigned char)s1[i] - (unsigned char)s2[i];
//this change is for dbtree. 1998.4.8
/*
		if( _AL == ' ' ) {
			if( _AH != '\0' )
				return  (signed char)1;	// ' ' is the biggest
			else	return  (signed char)0;
		}
		if( _AH == ' ' ) {
			if( _AL != '\0' )
				return  (signed char)-1;	// ' ' is the biggest
			else	return  (signed char)0;
		}
		if( _AL && _AH )		return _AL - _AH;
		return	(signed char)0;
*/
	}
    }

    return (unsigned char)s1[i] - (unsigned char)s2[i];

#endif

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
//this change is for dbtree. 1998.4.8
/*    for( i = 0;  i < maxlen && s1[i] != '\0';  i++ ) {
    for( i = 0;  i < maxlen;  i++ ) {

	//just deal the first different char
	if( s1[i] != s2[i] && flag == 0 )
	{
		if( i >= prefixLen ) {
			flag = 1;
		} else {
			flag = -1;
		}
	}
    }
-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
    for( i = 0;  (i < prefixLen) && (s1[i] == s2[i]);  i++ );

    if( i < prefixLen )
	return  '\1';

    *keyLen = maxlen - prefixLen;
    return '\0';
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#endif

    if( flag == 1 )  {
	*keyLen = i - prefixLen;
	return  '\0';
    }
    //*keyLen = i;
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


    if( (high = keyNum - 1 ) < 0 )        return  0;

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
    if( blenStrCmp(str, sz, &keyLen, node->prefixLen) == '\0' ) {
	sz += node->prefixLen;
	str += node->prefixLen;
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
		    for( mid--;    mid >=0;     mid-- ) {
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

	/*
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
		return  '\1';
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
					(char)(type|(char)BTREE_SEARCH_FOR_FIND))) < 0 ) {
		i = 0;
	}

	if( nodeTree->keyBuf[ i*keyLen+b_head->key4Len ] == BtreeNO ) {

	     nodeNo = *(long *)&nodeTree->keyBuf[ \
					i*keyLen+b_head->keyLen ];
	     if( (nodeTree = loadBTreeNode( b_head, nodeNo )) == NULL ) {
			BtreeErr = 2005;
                        b_head->CurNodePtr = NULL;
			return  '\1';
	     }
	} else {
	     if( (nodeTree = (bTREE *)(*(long *)&nodeTree->keyBuf[ \
			     i * keyLen + b_head->keyLen])) == NULL ) {
			BtreeErr = 2005;
                        b_head->CurNodePtr = NULL;
			return  '\1';
	     }

	     increaseNodeUseTime(b_head, nodeTree);
	     //locateTabNode(b_head, nodeTree->nodeNo);
	} // end of if

    } // end of while

    // locate into position
    if( (b_head->nodeCurPos = binSearch(b_head, nodeTree, str, type)) < 0 ) {
        b_head->CurNodePtr = nodeTree;
        b_head->nodeCurPos = 0;
	return  '\1';
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
    bTREE *memNode;
    short i;

    if( b_head->nodeCurTab >= BtreeNodeBufferSize ) {
	if( (i = memMod( b_head )) < 0 )	return  NULL;
	if( tabMod( b_head, (bTREE *)i, BTREE_KEY_REPLACE) != '\0' ) {
	    return  NULL;
	}

	if( wRequireForNew )
	    b_head->active_b_tab[i].nodeNwFlag = BtreeYES;

	return  b_head->active_b_tab[i].nodeAdress;
    }

    if( (memNode = (bTREE *)malloc(b_head->nodeSize + 4)) == NULL ) {

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
			(long)b_head->nodeSize) );
		b_tab2->nodeNwFlag = BtreeNO;
	}
	lseek( b_head->ndxFp, \
	      ( b_tab2->nodeAdress->nodeNo-1 )*(long)b_head->nodeSize + (long)BTREE_HEADBUF_SIZE, \
	      SEEK_SET );

	write( b_head->ndxFp, nodeTurnBuf(b_head, b_tab2->nodeAdress), \
							   b_head->nodeSize );
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
		b_tab[j].nodeUseTimes = b_tab[j].nodeUseTimes / 3 + 1;
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
	if( b_tab[j].nodeUseTimes > 0 && b_tab[j].nodeAdress->nodeNo == nodeNo )
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
*===========================================================================*/
void increaseNodeUseTime( bHEAD *b_head, bTREE *node )
{
    short 	i;
    short       iloop;
    bTAB 	*b_tab, *b_tab2;

    b_tab2 = (bTAB *)*(long *)((char *)node + b_head->nodeSize);


    if( ++(b_tab2->nodeUseTimes) >= BTREE_MAX_USETIMES ) {

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
void setNodeWrFlag( bHEAD *b_head, bTREE *node )
{
    short 	i, iloop;
    bTAB 	*b_tab, *b_tab2;

    b_tab2 = (bTAB *)*(long *)((char *)node + b_head->nodeSize);

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
short  locateSonNode( bHEAD *b_head, bTREE *bTreeNode, bTREE *bSonNode )
{
    short i, j, rec_len;
    char *s;
    char *sp4;

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
    bTAB  *b_tab;
    */
    bTAB  *b_tab2;

    b_tab2 = (bTAB *)*(long *)((char *)nodeAdr + b_head->nodeSize);
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

    //1998.10.10
    if( b_head->active_b_tab[id].nodeUseTimes < 0 )
	b_head->active_b_tab[id].nodeUseTimes = -b_head->active_b_tab[id].nodeUseTimes;

} // end of function unlockTabNode()


/***************
**                            dbTreeBuild()
*****************************************************************************/
_declspec(dllexport) int dbTreeBuild(char *ndxName, short keyLen, short nodeSize)
{
    bTREE 	*nodeTree;
    bHEAD 	*b_head;
    short 	ndxFp;
    char  	ndxReallyName[FILENAME_MAX];
    
    // set build ndx environment
    BtreeNodeBufferSize = MAX_BtreeNodeBufferSize;

    strcpy(ndxReallyName, ndxName);

    /*if( (sz = strchr(ndxReallyName, '.') ) != NULL ) {
	*sz = '\0';
    }
    strcat( ndxReallyName, bIndexExtention);
    */

    // create the index file
    if( (ndxFp = open( ndxReallyName, O_CREAT|O_RDWR|O_TRUNC|O_BINARY|SH_DENYWR, \
						 S_IREAD|S_IWRITE )) < 0 ) {
	return  BtreeErr = 2007;
    }

    if( (b_head = (bHEAD *)zeroMalloc( sizeof(bHEAD) )) == NULL ) {
	close( ndxFp );
	unlink( ndxReallyName);
	return  BtreeErr = 2002;
    }  /* end of if */
    strcpy(b_head->VerMessage, BTREE_VER_MESSAGE);

    // allocate the active_b_tab memory
    if( tabInit( b_head ) < 0 ) {
	close( ndxFp );
	unlink( ndxReallyName);
	return  BtreeErr = 2002;
    }

    b_head->nodeSize = nodeSize;
    b_head->nodeBufSize = nodeSize-24;

    b_head->keyLen = keyLen;
    b_head->keyStoreLen = keyLen + 5;
    b_head->key4Len = keyLen + 4;
    b_head->keyFieldNum = 0;
    b_head->keyMaxCount = b_head->nodeBufSize / b_head->keyStoreLen;

    b_head->nodeBreakPos = (short)(b_head->keyMaxCount * DEF_BREAKPOS);
    b_head->upperKeyNum = (short)(b_head->keyMaxCount * UPPER_BREAKPOS);
    b_head->nodeCorpNum = (short)(b_head->keyMaxCount * CORP_KEYPER);

    b_head->CurNodePtr = NULL;
    b_head->nodeCurPos = -1;
    b_head->keyCount = 0;
    b_head->ndxFp = ndxFp;
    b_head->type = BTREE_FOR_OBJ;
    b_head->rubbishBlockNum = 0;

    b_head->dbfPtr = NULL;
/*    // ***** create the btree head. *****
    lseek( ndxFp, 0L, SEEK_SET);
    write( ndxFp, b_head, BTREE_HEADBUF_SIZE );
*/
    if( (nodeTree = rqMem( b_head, 1 )) == NULL ) {
	dbTreeClose( b_head );
	return  BtreeErr = 2002;
    } // end of if

    nodeTree->nodeNo = b_head->nodeMaxNo = 1;

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
    return  dbTreeClose( b_head );

} // end of function dbTreeBuild()


/*==============
*                                ModFather()
* Function:
*     add one node then call this function to deal with its father storing
* flag
*===========================================================================*/
short  ModFather( bHEAD *b_head, bTREE *SonNode, char type )
{
    short	i, j, rec_len;
    bTREE 	*FatherNode;
    char  	*sf;
    char  	*sf4;
    long int 	nodeNo;

    FatherNode = SonNode->fatherPtr;
    sf = FatherNode->keyBuf + b_head->keyLen;
    sf4 = FatherNode->keyBuf + b_head->key4Len;

    rec_len = b_head->keyStoreLen;

    switch( type ) {
	case BTREE_KEY_INSERT:
	    if( (bHEAD *)FatherNode == (bHEAD *)b_head) {
//		SonNode->fatherPtr = (bTREE *)b_head;
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
    bTREE  *tempTree;
    long    nodeNo;
    short   rec_len, i, j, k;
    char   *s, *s4;
    bTAB   *b_tab = b_head->active_b_tab;

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
			lseek(b_head->ndxFp, (long)BTREE_HEADBUF_SIZE + \
				     (long)BTREE_FATHERNODE_POSITION + \
				     (nodeNo-1)*(long)b_head->nodeSize, \
				     SEEK_SET);
			write( b_head->ndxFp, &(bTreeNode->nodeNo), sizeof(long));
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
			if( (nodeNo = *(long *)&s[ j ] ) != 0L ) {
				lseek(b_head->ndxFp, (long)BTREE_HEADBUF_SIZE + \
					(long)BTREE_FATHERNODE_POSITION + \
					(nodeNo-1)*(long)b_head->nodeSize, \
					SEEK_SET);
				write( b_head->ndxFp, &(bTreeNode->nodeNo), sizeof(long));
			}  // end of if
		}
	    } // end of for

    } // end of switch

    return  1;

}  // end of function modSonNode()


/***************
*                                 dbTreeClose()
*  Function:
*      free head memory and write the btree head message
*****************************************************************************/
_declspec(dllexport) int dbTreeClose( bHEAD *b_head )
{
    bTREE	*TempTree;
    short  	i;
    long int 	nodeNo;
    bTAB 	*b_tab;

    if( b_head == NULL ) {      BtreeErr = 2001;        return( -1 );    }

    b_tab = b_head->active_b_tab;
    // first, adjust size of the tree file
    chsize( b_head->ndxFp, (long)(BTREE_HEADBUF_SIZE + \
					b_head->nodeMaxNo * b_head->nodeSize) );

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
		lseek( b_head->ndxFp, (long)(BTREE_HEADBUF_SIZE) + \
				(nodeNo-1)*(long)b_head->nodeSize, SEEK_SET);
		write( b_head->ndxFp, nodeTurnBuf(b_head, TempTree), \
							b_head->nodeSize);
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

    lseek( b_head->ndxFp, 0L, SEEK_SET );
    write( b_head->ndxFp, b_head, sizeof(bHEAD));

    free( b_tab );

    /*if( b_head->type == BTREE_FOR_CLOSEDBF ) {
	dSleep( b_head->dbfPtr );
    }*/

    // close the file
    close( b_head->ndxFp );

    free( b_head );

    return  0;

} // end of function dbTreeClose()


/***************
*                                 IndexDispose()
*  Function:
*      free head memory but do not write the btree head message
*****************************************************************************/
_declspec(dllexport) short dbTreeDispose(bHEAD *b_head)
{
    short i;
//    unsigned short nodeNum;
    bTAB *b_tab;

    if( b_head == NULL ) {      BtreeErr = 2001;        return( -1 );    }

    b_tab = b_head->active_b_tab;

    for( i = 0; i < b_head->nodeCurTab; i++) {
	    free( b_tab[i].nodeAdress );
    }  // end of for
    free( b_tab );

    /*if( b_head->type == BTREE_FOR_CLOSEDBF ) {
	dSleep( b_head->dbfPtr );
    }*/

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
//    short 	MnPos;
//    bTAB 	*b_tab;
    char 	*sn;
    char 	*st;                           // tempTree->keyBuf
    char 	buf[BTREE_MAX_KEYLEN];
    int 	idRubbish;
    char  	*sz;
    short 	i, j, k, ilock;
    short 	keyLen = b_head->keyLen;
    short	nKnum;
//    bTREE	*remNode;

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
    //if( strPosition <= 0 && b_head->nodeMaxNo > 1 && b_head->keyCount > 0 ) {
    if( strPosition <= 0 && b_head->ptr != node || b_head->nodeMnFlag == BtreeNO ) {
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

			//1998.10.11
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
			}
			*/
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

	     } while( i == 0 );

	     memcpy(_BtreeKeyBuf, buf, rec_len );
	     unlockTabNode( b_head, k );
	     strPosition = 1;
    } // end of if

    if( nKnum < b_head->keyMaxCount) {

	// if there is enough space
	// insert this key directly.
	(char *)sn += strPosition * rec_len;

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
	memcpy( buf, sn, rec_len );
	setNodeWrFlag( b_head, node );

	unlockTabNode( b_head, ilock );

	// 0 1 2 3 4 5 6 7 8 9
	// 0 is move out, strPosition is 6
	// algorithm: move 012345 forward, strPosition is 6-1=5
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if( strPosition != 0 ) {
	    memmove(sn, sn+rec_len, (--strPosition)*rec_len);
	}

	memcpy(sn + strPosition * rec_len,  _BtreeKeyBuf,  rec_len);
	reCalPrefixLen(node, keyLen);

	//tell the node's father that I have changed my mark key
	//buf->_BtreeKeyBuf
	memcpy(_BtreeKeyBuf, sn, rec_len);

	k = lockTabNode(b_head, tempTree);

	do
	{ //change node's mark key, from bottom bo top

	   ilock = lockTabNode(b_head, node);
	   nodeNo = (long)node;
	   if( node->fatherPtrFlag == BtreeYES ) {

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

	   /*
	   if( node == (bTREE *)b_head )
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
	//tempTree->nodeNo = b_head->rubbishBlockNo[ idRubbish ];
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
		lseek(b_head->ndxFp, (long)BTREE_HEADBUF_SIZE + \
			(long)BTREE_LASTNODE_POSITION+(nodeNo-1)*(long)b_head->nodeSize, \
			SEEK_SET);
		write( b_head->ndxFp, &(tempTree->nodeNo), sizeof(long));
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
			//tTree->nodeNo = b_head->rubbishBlockNo[ idRubbish ];
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
		memcpy( tTree->keyBuf, sn, keyLen );
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
	    memcpy( buf, st, keyLen );

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

		/*$$$
		node->fatherPtr = node2;
		node->fatherPtrFlag = BtreeYES;
		*/

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
			     lseek( b_head->ndxFp, (long)BTREE_HEADBUF_SIZE + \
				    (long)BTREE_LASTNODE_POSITION + (nodeNo-1) * \
				    (long)b_head->nodeSize, SEEK_SET );
			     write( b_head->ndxFp, &(tempPtr->nodeNo), sizeof(long));
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
    bTREE      *fatherNode, *brotherNode;
    short 	rec_len;
    char  	buf[BTREE_MAX_KEYLEN];
    short 	i, j;
    bTREE      *node;
    short 	strPos;
    char       *sz;

    node = b_head->CurNodePtr;
    strPos = b_head->nodeCurPos;

/* inside use module, needn't check this
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

		   memcpy(&brotherNode->keyBuf[rec_len * brotherNode->keyNum],\
			   node->keyBuf, \
			   rec_len * node->keyNum);

		   brotherNode->keyNum += node->keyNum;
		   setNodeWrFlag(b_head, brotherNode);

		   node->keyNum = 0;

		   reCalPrefixLen(brotherNode, b_head->keyLen);

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


	// the tree has more than on level, perhaps
	if( node->keyNum >= 1 ) {

	    setNodeWrFlag(b_head, node);

	    if( strPos <= 0 ) {

		//keep the relation key into buf
		memcpy(buf, node->keyBuf, b_head->keyLen);

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

	    return  btSUCCESS;

	} // end of if, ELSE ELSE ELSE ELSE ELSE ELSE ELSE ELSE ELSE ELSE


bTreeNodeIncorporate_DEL_NODE:

	// else,
	// the node has been empty, put it nito rubbish to wait for reuse
	//
	// if the node's father node is the tree's head
	// this will call this tree to decrease its level
	if( node->fatherPtr == (bTREE *)b_head ) {

		// empty B+ tree has one empty node in my B+ tree
		if( b_head->keyCount <= 0 ) {
			return  btSUCCESS;
		}

		//?????
		b_head->ptr = brotherNode;
		b_head->nodeMnFlag = BtreeYES;
		brotherNode->fatherPtr = (bTREE *)b_head;
		brotherNode->fatherPtrFlag = BtreeYES;

		putRubbishBlockNo( b_head, node->nodeNo );
		/*needn't compress the Btree
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
				lseek(b_head->ndxFp, (long)BTREE_HEADBUF_SIZE + \
					(long)BTREE_LASTNODE_POSITION + \
					((long)node->nextPtr-1) * \
					(long)b_head->nodeSize, \
					SEEK_SET);
				write( b_head->ndxFp, &(brotherNode->nodeNo), sizeof(long));
			}
		} // end of if
	} else
	{ // brotherNode is NULL, get the next node as base
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
					lseek(b_head->ndxFp, (long)BTREE_HEADBUF_SIZE + \
						(long)BTREE_NEXTNODE_POSITION + \
						((long)node->lastPtr-1) * \
						(long)b_head->nodeSize, \
						SEEK_SET);
					write( b_head->ndxFp, &(brotherNode->nodeNo), sizeof(long));
				}
			}
		} // end of if
	} // end of else

	if( brotherNode != NULL ) {
	    setNodeWrFlag(b_head, brotherNode);
	} else
	{ //both of the node's lastNode and nextNode is NULL
	  //decrease level(s)
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


	//!!!!!
	if( (strPos = locateSonNode( b_head, fatherNode, node ) ) < 0 ) {
	//if( locateSonNode(b_head, fatherNode, node) < 0 )
		BtreeErr = 3004;
		return  '\1';
	} // end of if

	// set the write flag
	//Error,1997.3.25. setNodeWrFlag(b_head, brotherNode->nodeNo);
	//???setNodeWrFlag(b_head, fatherNode);

	putRubbishBlockNo( b_head, node->nodeNo );
	/*needn't compress the Btree
	if( putRubbishBlockNo( b_head, node->nodeNo ) < 0 ) {
		// reindex the tree
		i = lockTabNode( b_head, node );
		if( btreeNodeArrange( b_head ) < 0 ) {
			return  '\1';
		}
		unlockTabNode( b_head, i );
	}*/
	tabMod( b_head, node, BTREE_KEY_RUBBISH );

	node = fatherNode;

    } while( 1 );

} // end of function bTreeNodeIncorporate()



/*==============
*                               getRubbishBlockNo()
*===========================================================================*/
int  getRubbishBlockNo( bHEAD *b_head )
{
    /*
    if( b_head->rubbishBlockNum >= 0 ) {
		return (b_head->rubbishBlockNum)--;
    }
    return  -1;
    */
    long nodeNo;

    if( b_head->rubbishBlockNum <= 0 )
	return  -1;
    nodeNo = b_head->rubbishBlockNo[0];
    lseek( b_head->ndxFp, \
	   (nodeNo-1)*(long)b_head->nodeSize+(long)BTREE_HEADBUF_SIZE, \
	   SEEK_SET );
    read( b_head->ndxFp, &b_head->rubbishBlockNo[0], sizeof(long));
    (b_head->rubbishBlockNum)--;

    return  nodeNo;

} // end of function getRubbishBlockNo()


/*==============
*                               putRubbishBlockNo()
*===========================================================================*/
int  putRubbishBlockNo( bHEAD *b_head, long nodeNo )
{
    /*
    if( b_head->rubbishBlockNum < BTREE_RUBBISHBLOCKNO ) {
	b_head->rubbishBlockNo[ ++(b_head->rubbishBlockNum) ] = nodeNo;
	return  b_head->rubbishBlockNum;
    }

    return  -1;
    */

    lseek( b_head->ndxFp, \
	   (nodeNo-1)*(long)b_head->nodeSize+(long)BTREE_HEADBUF_SIZE, \
	   SEEK_SET );
    write( b_head->ndxFp, &b_head->rubbishBlockNo[0], sizeof(long));
    b_head->rubbishBlockNo[0] = nodeNo;

    return  ++(b_head->rubbishBlockNum);

}  // end of function putRubbishBlockNo()


/***************
*                              IndexRecIns()
*****************************************************************************/
_declspec(dllexport) short dbTreeRecIns( bHEAD *b_head, char *keyContent, long int recNo )
{
    bHEAD *pbh;
//    int    k;

    if( (b_head == NULL)  ||  (keyContent == NULL) ) {
	BtreeErr = 2001;
	return  0;
    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	restoreBtreeEnv(b_head);
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

    if( KeyLocateIntoNodeAndPos(pbh, keyContent, BTREE_LAST_KEY) != btSUCCESS ) {
#ifdef WSToMT
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  0;
    }

    memmove( _BtreeKeyBuf, keyContent, pbh->keyLen );
    *(long *)&_BtreeKeyBuf[ pbh->keyLen ] = recNo;
    _BtreeKeyBuf[pbh->key4Len] = BtreeNO;

    if( bTreeNodeMove( pbh ) != btSUCCESS ) {
#ifdef WSToMT
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  0;
    }

//////////////1998.10.10
/*
    for(k = 0;  k < b_head->nodeCurTab;  k++)
	if( b_head->active_b_tab[k].nodeUseTimes < 0 && (*((b_head->active_b_tab[k]).nodeAdress)).nodeFlag != 'D' )
	    printf("ERROR");
*/

#ifdef WSToMT
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  1;

} // end of function IndexRecIns()


/***************
*                                 dbTreeRecDel()
* RETURN:
*    the recno of the last delete
*****************************************************************************/
_declspec(dllexport) long int dbTreeRecDel(bHEAD *b_head, char *keyContent, long int recNo)
{
    long int tmpRecNo, tmpRecNo2;
    bHEAD   *pbh;

    if( (b_head == NULL) || (keyContent == NULL) ) {
	BtreeErr = 2001;
	return  LONG_MIN;
    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	restoreBtreeEnv(b_head);
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

    if( (tmpRecNo = IndexLocateAndGetRecNo(pbh, keyContent)) == LONG_MIN ) {
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
		if( (tmpRecNo = IndexStrEqSkip(pbh, keyContent, 1)) == LONG_MIN ) {
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
	if( (tmpRecNo2 = IndexLocateAndGetRecNo(pbh, keyContent)) == \
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
short IndexRecUpdate(bHEAD *b_head, char *src, char *dst, long int recNo)
{
    bHEAD *pbh;

    if( (b_head == NULL) || (src == NULL) || (dst == NULL) ) {
	BtreeErr = 2001;
	return  SHRT_MIN;
    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	restoreBtreeEnv(b_head);
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

    if( (recNo = dbTreeRecDel(pbh, src, recNo)) == LONG_MIN ) {
#ifdef WSToMT
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  SHRT_MIN;
    }

#ifdef WSToMT
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  dbTreeRecIns( b_head, dst, recNo );

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
	return  LONG_MIN;
    }

    //1998.2.19
    if( b_head->nodeCurPos >= b_head->CurNodePtr->keyNum ) {
	return  LONG_MIN;
    }

    keyLen = b_head->keyLen;
    recLen = b_head->keyStoreLen;


    sz = &(b_head->CurNodePtr->keyBuf[b_head->nodeCurPos*recLen]);

#ifdef euObjectBtree
    if( (b_head->type & FOROBJMASK) == 0 )
#endif

    {
	if( bStrCmp( sz, keyContent, keyLen ) != 0 ) {
		return  LONG_MIN;
	}
	if( b_head->nodeCurPos < 1 ) {
		// skip to the first equal keyword
		while( IndexStrEqSkip( b_head, keyContent, -1 ) != LONG_MIN );
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
*===========================================================================*/
short  btreeNodeArrange( bHEAD *b_head )
{

    return  1;

    /*bTREE *node;
    long int newNodeNo;

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
		lseek(b_head->ndxFp, (long)BTREE_HEADBUF_SIZE + \
				     (long)BTREE_FATHERNODE_POSITION + \
				     ((long)node->fatherPtr-1)*(long)BTREE_BUFMAXLEN, \
				     SEEK_SET);
		write( b_head->ndxFp, &(node->nodeNo), sizeof(long));
	}  // end of if

	// next brother
	if( node->nextPtrFlag == BtreeNO && node->nextPtr != NULL ) {
		lseek(b_head->ndxFp, (long)BTREE_HEADBUF_SIZE + \
				     (long)BTREE_LASTNODE_POSITION + \
				     ((long)node->nextPtr-1)*(long)BTREE_BUFMAXLEN, \
				     SEEK_SET);
		write( b_head->ndxFp, &(node->nodeNo), sizeof(long));
	}  // end of if

	// last brother
	if(node->lastPtrFlag == BtreeNO && node->lastPtr != NULL ) {
		lseek(b_head->ndxFp, (long)BTREE_HEADBUF_SIZE + \
				     (long)BTREE_NEXTNODE_POSITION + \
				     ((long)node->lastPtr-1)*(long)BTREE_BUFMAXLEN, \
				     SEEK_SET);
		write( b_head->ndxFp, &(node->nodeNo), sizeof(long));
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
    */
} // end of function btreeNodeArrange()


/****************
*                                  IndexSeek()
*****************************************************************************/
_declspec(dllexport) long int dbTreeSeek( bHEAD *b_head, char *keyContent )
{
    bHEAD *pbh;
    long   l;

    if( b_head == NULL || keyContent == NULL ) {
	BtreeErr = 2001;
	return  LONG_MIN;
    }

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
	return  LONG_MIN;
    }

    l = IndexLocateAndGetRecNo(pbh, keyContent);

#ifdef WSToMT
    saveBtreeEnv(b_head);
    /*if( l == LONG_MIN ) {
	if( b_head->CurNodePtr != NULL ) {
	    moveDbfPtr(b_head, *(long *)&(b_head->CurNodePtr->keyBuf[b_head->keyLen]));
	}
    } else {
	moveDbfPtr(b_head, l);
    }*/
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  l;

/*this is run in IndexSkip() whick is called with IndexLocateAndGetRecNo()
    if( (b_head->type == BTREE_FOR_OPENDBF) || (b_head->type == BTREE_FOR_CLOSEDBF) ) {
	dseek( b_head->dbfPtr, (recNo - 1), dSEEK_SET );
    }
*/

} // end of function IndexSeek()


/****************
*                                 IndexSkip()
* Return:
*      Success:         the record no
*      Fail:            0 (error no in BtreeErr), negative of the record
*                       no(cannot skip so much steps, skip to the limit)
*****************************************************************************/
_declspec(dllexport) long int dbTreeSkip( bHEAD *b_head, long int count )
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
	restoreBtreeEnv(b_head);
        EnterCriticalSection( &(pbh->dCriticalSection) );
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

#ifdef WSToMT
       saveBtreeEnv(b_head);
       LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  recNo;
    }

    if( count == -1 && nodeCurPos > 0 ) {

	short 	i = (--(pbh->nodeCurPos)) * pbh->keyStoreLen;

	recNo = *(long *)&(nodeTree->keyBuf[i + pbh->keyLen]);

#ifdef WSToMT
       saveBtreeEnv(b_head);
       LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  recNo;
    }

    if( count > 0L ) {

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

    } else if( count < 0L ) {

	while( (nodeTree->lastPtr != NULL) && (nodeCurPos+count < 0L) ) {
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
				return  LONG_MIN;
			}
		}
		nodeCurPos = nodeTree->keyNum - 1;
	}

	// if the tree cannot skip so much steps. goto the head
	if( nodeCurPos+count < 0 ) {
		flag = BtreeYES;
		nodeCurPos = 0L;
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
    }


#ifdef WSToMT
    saveBtreeEnv(b_head);
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    if( flag == BtreeNO ) 	return  recNo;

    return  LONG_MIN;

} // end of function IndexSkip()


/*---------------
*                                 IndexGoto1stEqRec()
*---------------------------------------------------------------------------*/
long int IndexGotoNstEqRec(bHEAD *b_head, char *keyContent, long int count)
{
    long int recNo;
    bHEAD *pbh;

    if( b_head == NULL ) {
	BtreeErr = 2001;
	return  LONG_MIN;
    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	restoreBtreeEnv(b_head);
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

    if( (recNo = dbTreeSeek(pbh, keyContent)) == LONG_MIN ) {
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

    if( (recNo = dbTreeSkip(pbh, count)) == LONG_MIN ) {
#ifdef WSToMT
        saveBtreeEnv(b_head);
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return LONG_MIN;
    }

    if( bStrCmp(keyContent, &pbh->CurNodePtr->keyBuf[pbh->nodeCurPos*\
			(pbh->keyStoreLen)], pbh->keyLen) != 0 ) {
	dbTreeSkip(b_head, -count);
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
long int IndexEqSkip( bHEAD *b_head, long int count )
{

    char     buf[BTREE_MAX_KEYLEN];
    long int recNo;
    bHEAD   *pbh;

    if( b_head == NULL ) {
	BtreeErr = 2001;
	return  LONG_MIN;
    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	restoreBtreeEnv(b_head);
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

    if( count == 1 && pbh->nodeCurPos <= pbh->CurNodePtr->keyNum - 2 ) {

	char 	*s = pbh->CurNodePtr->keyBuf;
	short 	i = (pbh->nodeCurPos+1) * pbh->keyStoreLen;

	if( bStrCmp(&s[i], &s[i-pbh->keyStoreLen], pbh->keyLen) != 0 )
	{
#ifdef WSToMT
            saveBtreeEnv(b_head);
            LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	    return	LONG_MIN;
	}

	(pbh->nodeCurPos)++;
	recNo = *(long *)&s[i + pbh->keyLen];
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
	    return	LONG_MIN;
	}

	(pbh->nodeCurPos)--;
	recNo = *(long *)&s[i + pbh->keyLen];
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
    if( (recNo = dbTreeSkip(pbh, count)) == LONG_MIN ) {
#ifdef WSToMT
        saveBtreeEnv(b_head);
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return LONG_MIN;
    }

    if( bStrCmp(buf, &pbh->CurNodePtr->keyBuf[pbh->nodeCurPos*\
			(pbh->keyStoreLen)], pbh->keyLen) != 0 ) {
	dbTreeSkip(pbh, -count);
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

} // end of function IndexEqSkip()


/****************
*                                 IndexStrEqSkip()
*****************************************************************************/
long int  IndexStrEqSkip( bHEAD *b_head, char *buf, short count )
{

    long int recNo;
//this change is for dbtree. 1998.4.8
//    short    keyLen = strlen(buf);
    short    keyLen = b_head->keyLen;

    if( count == -1 && b_head->nodeCurPos > 0 ) {

	char 	*s = b_head->CurNodePtr->keyBuf;
	short 	i = (b_head->nodeCurPos-1) * b_head->keyStoreLen;

	if( bStrCmp(&s[i], buf, keyLen) != 0 )
	{
	    return	LONG_MIN;
	}

	(b_head->nodeCurPos)--;
	recNo = *(long *)&s[i + b_head->keyLen];
	return  recNo;
    }

    if( count == 1 && b_head->nodeCurPos <= b_head->CurNodePtr->keyNum - 2 ) {

	char 	*s = b_head->CurNodePtr->keyBuf;
	short 	i = (b_head->nodeCurPos+1) * b_head->keyStoreLen;

	if( bStrCmp(&s[i], buf, keyLen) != 0 )
	{
	    return	LONG_MIN;
	}

	(b_head->nodeCurPos)++;
	recNo = *(long *)&s[i + b_head->keyLen];
	return  recNo;
    }

    // now dbf pointer move come from IndexSkip()
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if( (recNo = dbTreeSkip(b_head, count)) == LONG_MIN ) {
	return LONG_MIN;
    }

    if( bStrCmp(buf, &b_head->CurNodePtr->keyBuf[b_head->nodeCurPos*\
			(b_head->keyStoreLen)], keyLen) != 0 ) {
	dbTreeSkip(b_head, -count);
	return  LONG_MIN;
    }

    return recNo;

} // end of function IndexStrEqSkip()


/****************
*                        IndexKeyCount()
* function:
*     count the key number and point the pointer to the last key
*****************************************************************************/
long int IndexKeyCount( bHEAD *b_head, char *keyContent )
{

    long int l;
    short keyLen;
    short keyStoreLen;
    bHEAD *pbh;

    // change the type for not move the dFILE rec_p
    //typeReserved = b_head->type;

    //sometime we use IndexKeyCount() to move the pointer to the last equal
    //keyword
    //b_head->type = BTREE_FOR_ITSELF;

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

    if( dbTreeSeek( pbh, keyContent) == LONG_MIN ) {
	//pbh->type = typeReserved;
#ifdef WSToMT
        saveBtreeEnv(b_head);
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return 0L;
    }
    l = 1;

    keyLen = pbh->keyLen;
    keyStoreLen = pbh->keyStoreLen;
    while( 1 ) {
	if( dbTreeSkip(pbh, 1) == LONG_MIN ) {
		break;
	}

	if( bStrCmp(keyContent, &pbh->CurNodePtr->keyBuf[ \
				pbh->nodeCurPos*keyStoreLen], \
				keyLen) != 0 ) {
		dbTreeSkip(pbh, -1);
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
_declspec(dllexport) char * dbTreeGetKeyContent(bHEAD *b_head)
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
long int IndexGetKeyRecNo( bHEAD *b_head )
{
    bHEAD *pbh;
    long  l;

    if( b_head == NULL ) {      BtreeErr = 2001;        return  0;    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	restoreBtreeEnv(b_head);
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
_declspec(dllexport)long int dbTreeGoTop( bHEAD *b_head )
{
    bHEAD *pbh;
    bTREE *nodeTree;
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
		return  LONG_MIN;
	    }
	} else {
	    if( (nodeTree = (bTREE *)(*(long *)&nodeTree->keyBuf[pbh->keyLen])) \
								== NULL ) {
#ifdef WSToMT
                LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
		return  LONG_MIN;
	    }
	}

    } // end of while

    recNo = *(long *)&(nodeTree->keyBuf[pbh->keyLen]);
    pbh->CurNodePtr = nodeTree;
    pbh->nodeCurPos = 0;

#ifdef WSToMT
    saveBtreeEnv(b_head);
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  recNo;

} // end of function IndexGoTop()


/****************
*                              IndexGoBottom()
*****************************************************************************/
_declspec(dllexport) long int dbTreeGoBottom( bHEAD *b_head )
{
    bHEAD *pbh;
    bTREE *nodeTree;
    short   i, rec_len;
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
		return  LONG_MIN;
	}

	if( nodeTree->keyBuf[ (i=(nodeTree->keyNum-1)*rec_len)+\
					     pbh->key4Len] == BtreeNO ) {
	    if( (nodeTree = loadBTreeNode( pbh, \
				*(long *)&(nodeTree->keyBuf[i+pbh->keyLen]) )) == NULL ) {
#ifdef WSToMT
                        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
			return  LONG_MIN;
	    }
	} else {
	    if( (nodeTree = (bTREE *)(*(long *)&nodeTree->keyBuf[i+pbh->keyLen])) == \
								     NULL ) {
#ifdef WSToMT
                LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
		return  LONG_MIN;
	    }
	} // end of else

    } // end of while

    pbh->nodeCurPos = nodeTree->keyNum - 1;
    pbh->CurNodePtr = nodeTree;
    recNo = *(long *)&(nodeTree->keyBuf[ pbh->nodeCurPos * rec_len + pbh->keyLen]);

#ifdef WSToMT
    saveBtreeEnv(b_head);
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  recNo;

} // end of function IndexGoBottom()


/****************
*                             IndexCurRecno( )
*****************************************************************************/
long int IndexCurRecno( bHEAD *b_head )
{
    bHEAD *pbh;
    long   l;

    if( b_head == NULL ) {       BtreeErr = 2001;        return  -1;    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	restoreBtreeEnv(b_head);
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
_declspec(dllexport) short dbTreeEof( bHEAD *b_head )
{
    bHEAD *pbh;

    if( b_head == NULL ) {       BtreeErr = 2001;        return  -1;    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	restoreBtreeEnv(b_head);
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
short  IndexChk( bHEAD *b_head, void *df )
{
    if( df == NULL )		return  0;
    if( b_head == NULL )	return  1;

    return  0;

} // end of IndexChk()


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
    short    i, j;
    unsigned char     c;
    short    keyStoreLen = btKeyLen+5;

    for(i = 0;   i < btKeyLen;  i++, s++) {
	c = *s;
	for(j = 1;  j < bt->keyNum;  j++) {
		if( c != s[j*keyStoreLen] ) {
			bt->prefixLen = i;
			return;
		}
	}
    }

    bt->prefixLen = i;

} //endof reCalPrefixLen()


/***************
*                                 IndexRecMove()
* RETURN:
*    the recno of the last delete
*****************************************************************************/
long IndexRecMove(bHEAD *b_head, char *keyContent, \
						   long recNo, long newRecNo)
{
    long int tmpRecNo;
    bHEAD *pbh;

    if( (b_head == NULL) || (keyContent == NULL) ) {
	BtreeErr = 2001;
	return  LONG_MIN;
    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	restoreBtreeEnv(b_head);
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

    if( (tmpRecNo = IndexLocateAndGetRecNo(pbh, keyContent)) == LONG_MIN ) {
#ifdef WSToMT
        LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
	return  LONG_MIN;
    }

    for( ; ; ) {
	if( recNo == tmpRecNo ) {
		break;
	}
	if( (tmpRecNo = IndexStrEqSkip(pbh, keyContent, 1 )) == LONG_MIN ) {
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



/*==============
*                              deNodeTurnBuf()
*===========================================================================*/
bTREE * deNodeTurnBuf(bHEAD *b_head, bTREE *tempTree )
{
    short 	i;
    bTAB 	*b_tab;

    if( tempTree->nodeFlag == BtreeDATA ) {
	return  tempTree;
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




/***************
*                                 allocBtreeNode()
* RETURN:
*****************************************************************************/
bTREE *allocBtreeNode(bHEAD *b_head, long lastNode)
{
    bTREE *nodeTree;

    long nodeNo = getRubbishBlockNo(b_head);

    if( nodeNo < 0 ) {
	nodeNo = ++(b_head->nodeMaxNo);
    }

    if( lastNode != 0 ) {
	nodeTree = loadBTreeNode(b_head, lastNode);
	nodeTree->nextPtr = (bTREE *)nodeNo;
    }
    if( (nodeTree = rqMem( b_head, 1 )) == NULL )        return  NULL;

    memset(nodeTree, 0, b_head->nodeSize);
    nodeTree->nodeNo = nodeNo;
    nodeTree->nodeFlag = BtreeDATA;

    return  nodeTree;

} //end of allocBtreeNode()


/***************
*                                 writeBtreeData()
* RETURN:
*****************************************************************************/
_declspec(dllexport) int writeBtreeData(bHEAD *b_head, char *keyBuf, char *data, int dataLen)
{
    long  lastNode;
    int   iBytes;
    bTREE *tempTree, *tempTree1;
    long  nodeNo;
    short k;

    if( b_head == NULL )
	return  1;

    nodeNo = dbTreeSeek(b_head, keyBuf);

    if( dataLen <= 0 ) {
	if( nodeNo == LONG_MIN ) {
	    dbTreeRecIns(b_head, keyBuf, 0);
	} else {
	    IndexRecMove(b_head, keyBuf, nodeNo, 0);
	}
	return  dataLen;
    }

    lastNode = 0;
    for(iBytes = dataLen;  iBytes > 0;  iBytes -= b_head->nodeBufSize) {

	tempTree = allocBtreeNode(b_head, lastNode);

	if( lastNode == 0 ) {
	    k=lockTabNode(b_head, tempTree);
	    if( nodeNo == LONG_MIN ) {
		dbTreeRecIns(b_head, keyBuf, tempTree->nodeNo);
	    } else {
		IndexRecMove(b_head, keyBuf, nodeNo, tempTree->nodeNo);

		//if it has already exist
		//free the old data node thread

		//load a datanode
		tempTree1 = loadBTreeNode(b_head, nodeNo);
		while( tempTree1 != NULL ) {
			nodeNo = (long)(tempTree1->nextPtr);
			if( putRubbishBlockNo(b_head, tempTree1->nodeNo) < 0 ) {
				return  1;
			}
			tabMod(b_head, tempTree1, BTREE_KEY_RUBBISH);

			if( nodeNo == 0 )	break;

			tempTree1 = loadBTreeNode(b_head, nodeNo);
		} //end of while
	    }
	    unlockTabNode(b_head, k);
	}
	(long)(tempTree->fatherPtr) = iBytes;
	lastNode = tempTree->nodeNo;

	if( iBytes < b_head->nodeBufSize )
	    memcpy(tempTree->keyBuf, data, iBytes);
	else
	    memcpy(tempTree->keyBuf, data, b_head->nodeBufSize);
	data += b_head->nodeBufSize;
    }

    dbTreeFlush( b_head );

    return  0;

} //end of writeBtreeData()



/***************
*                                 freeBtreeData()
* RETURN:
*    the recno of the last delete
*****************************************************************************/
_declspec(dllexport) int freeBtreeData(bHEAD *b_head, char *keyBuf)
{
    long   nodeNo;
    bTREE *tempTree;
    int    i;

    nodeNo = dbTreeSeek(b_head, keyBuf);
    if( nodeNo == LONG_MIN )
	return  -1;

    //free all keyword equal to this key
    dbTreeRecDel(b_head, keyBuf, LONG_MIN);

    i = 0;
    //load a datanode
    tempTree = loadBTreeNode(b_head, nodeNo);

    while( tempTree != NULL ) {
	i++;
	nodeNo = (long)(tempTree->nextPtr);
	if( putRubbishBlockNo(b_head, tempTree->nodeNo) < 0 ) {
		return  -2;
	}
	tabMod(b_head, tempTree, BTREE_KEY_RUBBISH);

	if( nodeNo == 0 )
	    break;

	tempTree = loadBTreeNode(b_head, nodeNo);
    }

    dbTreeFlush( b_head );

    return  i;

} //end of freeBtreeData()


/***************
*                                 readBtreeData()
* RETURN:
*    the recno of the last delete
*****************************************************************************/
_declspec(dllexport) char * readBtreeData(bHEAD *b_head, char *keyBuf, char *data, int dataLen)
{
    long   nodeNo;
    bTREE *tempTree;
    int    iRead, iShouldRead;
    char  *buf;

    nodeNo = dbTreeSeek(b_head, keyBuf);
    if( nodeNo == LONG_MIN )
	return  NULL;

    if( nodeNo <= 0 )
	return  NULL;

    tempTree = loadBTreeNode(b_head, nodeNo);
    iShouldRead = (long)(tempTree->fatherPtr);

    //remember the datasize
    lastReadSize = iShouldRead;

    if( data == NULL ) {
	buf = malloc(iShouldRead+16);
	if( buf == NULL )
	    return  NULL;

	dataLen = iShouldRead;
    } else {
	buf = data;
    }

    iRead = 0;
    while( iRead < iShouldRead ) {
	nodeNo = (long)(tempTree->nextPtr);

	if( dataLen < b_head->nodeBufSize ) {
	    memcpy(&buf[iRead], tempTree->keyBuf, dataLen);
	    iRead += dataLen;
	} else {
	    memcpy(&buf[iRead], tempTree->keyBuf, b_head->nodeBufSize);
	    iRead += b_head->nodeBufSize;
	}

	if( iRead >= iShouldRead )
	    break;

	dataLen -= b_head->nodeBufSize;

	if( dataLen <= 0 )
	    break;
	tempTree = loadBTreeNode(b_head, nodeNo);
	if( tempTree == NULL )
	    break;
    }

    return  buf;

} //end of readBtreeData()


/***************
*                                 getBtreeDataLen()
* RETURN:
*    the length of data
*****************************************************************************/
_declspec(dllexport) long getBtreeDataLen(bHEAD *b_head, char *keyBuf)
{
    long  nodeNo;
    bTREE *tempTree;

    nodeNo = dbTreeSeek(b_head, keyBuf);
    if( nodeNo == LONG_MIN )
	return  -1;

    tempTree = loadBTreeNode(b_head, nodeNo);
    if( tempTree == NULL )
	return  -2;

    return (long)(tempTree->fatherPtr);

} //end of getBtreeDataLen()


/***************
*                                 freeBtreeMem()
* mention:
*    the DLL work address thread is not the same EXE's
*****************************************************************************/
_declspec(dllexport) void freeBtreeMem(char *s)
{
    free(s);
} //end of freeBtreeMem()


/***************
*                                 getBtreeError()
* mention:
*    the DLL work address thread is not the same EXE's
*****************************************************************************/
_declspec(dllexport) long getBtreeError( void )
{
    return  BtreeErr;
} //end of getBtreeError()


/***************
*                                 getBtreelastReadSize()
* mention:
*    the DLL work address thread is not the same EXE's
*****************************************************************************/
_declspec(dllexport) long getBtreeLastReadSize( void )
{
    return  lastReadSize;
} //end of getBtreelastReadSize()


/*when we use the bTREE v1.5 we can just comp little keywords
* at the binCmp, add the bef+bt->prefixLen and len-=bt->prefixLen
* this cause the compare speed up.
*/


/***************
*                                 dbTreeFlush()
*  Function:
*      free head memory and write the btree head message
*****************************************************************************/
_declspec(dllexport) int dbTreeFlush( bHEAD *b_head )
{
    bTREE    *TempTree;
    short     i;
    long int  nodeNo;
    bTAB     *b_tab;
    bHEAD    *pbh;

    if( b_head == NULL ) {      BtreeErr = 2001;        return  -1;    }

#ifdef WSToMT
    if( b_head->pbh != NULL ) {
	restoreBtreeEnv(b_head);
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

    b_tab = b_head->active_b_tab;
    // first, adjust size of the tree file
    chsize( b_head->ndxFp, (long)(BTREE_HEADBUF_SIZE + \
					b_head->nodeMaxNo * b_head->nodeSize) );

    for( i = 0; i < b_head->nodeCurTab; i++) {
	    
	    //2000.12.25
	    if( b_tab[i].nodeUseTimes == 0 ) {
		continue;
	    }

	    TempTree = b_tab[i].nodeAdress;
	    if( (b_tab[i].nodeWrFlag == BtreeYES) || \
						(b_head->ptr == TempTree) )  {
		nodeNo = TempTree->nodeNo;
		lseek( b_head->ndxFp, (long)(BTREE_HEADBUF_SIZE) + \
				(nodeNo-1)*(long)b_head->nodeSize, SEEK_SET);
		write( b_head->ndxFp, nodeTurnBuf(b_head, TempTree), \
							b_head->nodeSize);
		deNodeTurnBuf(b_head, TempTree);
		//b_tab[i].nodeUseTimes = 0;
		//b_tab[i].nodeNwFlag = BtreeNO;
		b_tab[i].nodeWrFlag = BtreeNO;
	    }
	    /*if( b_head->ptr == TempTree ) {
		b_head->ptr =(bTREE *)(TempTree->nodeNo);
	    }*/
    }  // end of for

    lseek(b_head->ndxFp, 0L, SEEK_SET);
    if( b_head->nodeMnFlag == BtreeYES ) {
	TempTree = b_head->ptr;
	b_head->ptr = (bTREE *)TempTree->nodeNo;
	write(b_head->ndxFp, b_head, sizeof(bHEAD));
	b_head->ptr = TempTree;
    } else {
	write(b_head->ndxFp, b_head, sizeof(bHEAD));
    }

#ifdef WSToMT
    LeaveCriticalSection( &(pbh->dCriticalSection) );
#endif
    return  0;

} // end of function dbTreeFlush()





/******************     end of this software dbtree.c     *******************/
