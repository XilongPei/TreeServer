/***************************************************************************
*   BTREEADD.C   1.0
*
*  Copyright: East-Union Computer Service Co., Ltd.
*             1994
*  Author:    Xilong Pei
****************************************************************************/

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
#include "strutl.h"
#include "wst2mt.h"

#ifdef __BORLANDC__
unsigned short        maxBtreeSinkSize = 32000;
#else
int        	      maxBtreeSinkSize = 64000;
#endif

//redefine the var 2000/2/12 XilongPei
//WSToMT	static unsigned short keyNumInSink =  0;
//WSToMT	static unsigned short maxSinkKeyNum = 0;

WSToMT	static int    keyNumInSink =  0;
WSToMT	static int    maxSinkKeyNum = 0;
WSToMT	static short 	      pureKeyLen = 0;
WSToMT	static char  *keyBufSink = NULL;


// defined in btree.c pay attention their sync
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
extern char  _BtreeKeyBuf[BTREE_MAX_KEYLEN];
extern short BtreeNodeBufferSize;
extern char   tabMod( bHEAD *b_head, bTREE *nodeAdr, char type );
extern short  tabInit( bHEAD *b_head );
extern bTREE *  rqMem( bHEAD *b_head );
extern char  KeyLocateIntoNodeAndPos( bHEAD *b_head, char *str, char type );
extern char   bTreeNodeMove( bHEAD *b_head );
extern long int  IndexLocateAndGetRecNo( bHEAD *b_head, char *keyContent );
#define BTREE_KEY_UPDATE	'W'
#define BTREE_KEY_INSERT        'I'
#define BTREE_LAST_KEY          0x02
#define btSUCCESS		'\0'
#define btFAIL			'\1'


static int fcmp(const void *a, const void *b);

//*************** initBufSink()
//***************************************************************************
char *initBufSink( bHEAD *bh )
{
    if( bh->dbfPtr == NULL ) 	return  NULL;
/*  remed by NiuJingyu
#ifdef __BORLANDC__
	if( coreleft() <= (BtreeNodeBufferSize + 2 ) * \
					(BTREE_BUFMAXLEN + sizeof(bTAB)) ) {
	return  NULL;
	}
#endif
*/
    // if init ahead, free it.
    if( keyBufSink != NULL ) 		free( keyBufSink );

    maxSinkKeyNum = maxBtreeSinkSize;
    do {
	if( (keyBufSink = malloc( maxSinkKeyNum ) ) == NULL ) {
		if( (maxSinkKeyNum /= 2) == 0 )	break;
	}
    } while( keyBufSink == NULL );

    if( keyBufSink == NULL ) 	return  NULL;

    if( maxSinkKeyNum < BTREE_BUFMAXLEN ) {
	free( keyBufSink );
	maxSinkKeyNum = 0;
	return  keyBufSink = NULL;
    }
    maxSinkKeyNum = (maxSinkKeyNum - 2 ) / bh->key4Len; // minus 2 for safe

    dseek( bh->dbfPtr, 0L, dSEEK_SET );
    keyNumInSink =  0;
    pureKeyLen = bh->keyLen;

    return keyBufSink;

} // end of initBufSink()


//*************** getBtreeKey()
//***************************************************************************
char *getBtreeKey( bHEAD *bh, char *keyCont )
{
    dFILE 	*df;
    dFIELD 	*field, *lastField;
    dFIELD      *fields[MAX_KEYFIELD_NUM];
    short 	keyLen, i;
    int         k, lastField_fieldlen_1;
    short	keyFieldNum;
    int		len;			// bytes writen in buffer
    unsigned short *KeyFldid;
    //char 	buf[FIELDMAXLEN+1];
    char	*sz;

    keyLen = bh->key4Len;

    // if there is no key in buffer, fresh it.
    if( keyNumInSink == 0 ) {

	df = bh->dbfPtr;
	KeyFldid = bh->KeyFldid;
	keyFieldNum = bh->keyFieldNum - 1;
	len = 0;
	lastField = getFieldInfo(df, KeyFldid[keyFieldNum]);
	lastField_fieldlen_1 = lastField->fieldlen-1;
	sz = lastField->fieldstart;

	for( i = 0;    i < keyFieldNum;    i++) {
	    fields[i] = getFieldInfo( df, KeyFldid[i] );
	}

	while( !deof( df ) ) {

	    getrec( df );

	    // generate the keyword
	    for( i = 0;    i < keyFieldNum;    i++) {
		field = fields[i];
		for(k = 0;  k < field->fieldlen;  k++)
		{
		    if( field->fieldstart[k] )
			keyBufSink[len++] = field->fieldstart[k];
		    else
			keyBufSink[len++] = ' ';
		}
	    }   // end of for

	    // the last field should be rtrim(), it is call in get_fld()
	    for( k = lastField_fieldlen_1;  (k >= 0) && (sz[k] == ' ');  k--)
		sz[k] = '\0';
	    memcpy(&keyBufSink[len], sz, lastField->fieldlen);
	    /*1998.11.16
	    if( *rtrim( buf ) == '\0' ) {
		strncpy(&keyBufSink[len], BTREE_BLANKKEY, k);
	    } else {
		strncpy(&keyBufSink[len], buf, k );
	    }*/
	    len += lastField->fieldlen;

	    *(long *)&(keyBufSink[len])= df->rec_no;
	    len += sizeof( long );
/*	    keyBufSink[len] = BtreeNO;
	    len++;*/

	    if( ++keyNumInSink >= maxSinkKeyNum ) break;
	} // end of while

	// quick sort
	if( keyNumInSink > 0 ) {
		//pureKeyLen = bh->keyLen;
		qsort(keyBufSink, keyNumInSink, keyLen, fcmp );
	}

    } // end of if

    if( keyNumInSink == 0 )	return  NULL;

    memcpy(keyCont, &keyBufSink[ (--keyNumInSink) * keyLen ], keyLen);
    keyCont[keyLen] = BtreeNO;

    return  keyCont;

} // end of function getBtreeKey()


//*************** freeBufSink()
//***************************************************************************
void freeBufSink( void )
{
	free( keyBufSink );

	keyNumInSink = 0;
	maxSinkKeyNum = 0;
	keyBufSink = NULL;

} // end of freeBufSink()


//*************** fcmp()
//***************************************************************************
static int fcmp(const void *a, const void *b)
{

   int  i;
//   long l;

   if( (i = -memcmp( (char *)a, (char *)b, pureKeyLen )) != 0 )
   {
	return  i;
   }

   return  *(long *)((char *)b+pureKeyLen) - *(long *)((char *)a+pureKeyLen);

   /*
   if( (l = (*(long *)((char *)a+pureKeyLen) \
			- *(long *)((char *)b+pureKeyLen) )) == 0 )
   {
	return  0;
   }
   if( l < 0 )	return 1;
   return  	-1;
   */

} // end of fcmp()


/***************
**                            IndexReBuild()
*****************************************************************************/
short  IndexReBuild( char *ndxName, float breakPercent )
{
    bTREE *nodeTree;
    bHEAD *b_head;
    long int recNo;
    short ndxFp;
    char ndxReallyName[FILENAME_MAX], ndxTmpName[BTREE_MAX_KEYLEN];
    short nodeBreadPos;
    char *s;
    bHEAD *bh;

    strcpy( ndxReallyName, ndxName );
    if( (s = strchr( ndxReallyName, '.' )) != NULL ) {
	strcpy( s, bIndexExtention );
    } else {
	strcat( ndxReallyName, bIndexExtention );
    }
    strcpy( ndxTmpName, ndxReallyName );
    strcpy( strchr(ndxTmpName, '.'), ".TRP" );

    unlink( ndxTmpName );
    rename( ndxReallyName, ndxTmpName );
    if( (bh = IndexOpen( NULL, ndxTmpName, BTREE_FOR_ITSELF )) == NULL ) {
	BtreeErr = 2007;
	return  -1;
    }

    // create the index file
    if( (ndxFp = open( ndxReallyName, O_CREAT|O_RDWR|O_TRUNC|O_BINARY, \
						 S_IREAD|S_IWRITE )) < 0 ) {
	BtreeErr = 2007;
	return  -1;
    }

    if( (b_head = (bHEAD *)malloc( sizeof(bHEAD) )) == NULL ) {
	BtreeErr = 2002;
	close( ndxFp );
	unlink( ndxReallyName);
	return  -1;
    }  // end of if

    memcpy(b_head, bh, sizeof(bHEAD) );
    b_head->ndxFp = ndxFp;
    b_head->rubbishBlockNum = -1;

    // allocate the active_b_tab memory
    tabInit( b_head );

    if( (nodeTree = rqMem( b_head )) == NULL ) {
	BtreeErr = 2002;
	IndexClose( b_head );
	return  -1;
    } // end of if

    nodeTree->nodeNo = b_head->nodeMaxNo = 1L;

    if( tabMod(b_head, nodeTree, BTREE_KEY_INSERT) != '\0' ) {
	IndexClose( b_head );
	free( nodeTree );
	return  -1;
    } // end of if

    b_head->nodeMnFlag = BtreeYES;
    b_head->ptr = nodeTree;
    nodeTree->fatherPtr = (bTREE *)b_head;
    nodeTree->nextPtr = NULL;
    nodeTree->lastPtr = NULL;
    nodeTree->nextPtrFlag = BtreeNO;
    nodeTree->lastPtrFlag = BtreeNO;
    nodeTree->fatherPtrFlag = BtreeYES;
    nodeTree->keyNum = 0;
    nodeTree->nodeFlag = BtreeYES;

    nodeBreadPos = b_head->nodeBreakPos;
    b_head->nodeBreakPos = (short)(bh->keyMaxCount * breakPercent);

    IndexGoTop( bh );
    while( !IndexEof( bh ) ) {

	IndexGetKeyContent(bh);		// this function's result in _BtreeKeyBuf
	recNo = IndexGetKeyRecNo(bh);

	if( KeyLocateIntoNodeAndPos( b_head, _BtreeKeyBuf, \
						 BTREE_LAST_KEY ) != btSUCCESS ) {
		IndexClose( b_head );
		return  -1;
	}

	*(long *)&(_BtreeKeyBuf[b_head->key4Len])= recNo;
	_BtreeKeyBuf[b_head->key4Len] = BtreeNO;
	if( bTreeNodeMove( b_head ) != '\0' ) {
		IndexClose( b_head );
		return  -1;
	}
	IndexSkip(bh, 1);

    }  // end of while

    b_head->nodeBreakPos = nodeBreadPos;
    IndexClose( bh );
    unlink( ndxTmpName );
    IndexClose( b_head );

    return 1;

} // end of function IndexReBuild()


/***************
**                            IndexPhySyncUpdate(()
*****************************************************************************/
long IndexPhySyncUpdate( bHEAD *b_head )
{
    dFILE *vdf;
    dFILE *df;
    dFIELD *field;
    long l;

    dsetbuf( 32000 );
    if( b_head == NULL ) {   BtreeErr = 2001;    return  -1;    }

    if( (b_head->type != BTREE_FOR_OPENDBF) && (b_head->type == BTREE_FOR_CLOSEDBF) )
    {
	BtreeErr = 2001;
	return  -1;
    }

    vdf = b_head->dbfPtr;

    field = dfcopy( vdf, NULL );
    if( (df = dcreate( "dbf", field )) == NULL ) {
	dclose( vdf );
	free( field );
	return  -2;
    }
    free( field );

    dseek(vdf, 0L, dSEEK_SET);
    l = vdf->rec_num;
    IndexGoTop( b_head );
    while( l-- ) {
	getrec( vdf );
	PutRecord( df, vdf->rec_buf );
	IndexSkip( b_head, 1 );
    }

    l = vdf->rec_num;
    dclose( vdf );
    dclose( df );

    return  l;

} // end of function viewToDbf()
