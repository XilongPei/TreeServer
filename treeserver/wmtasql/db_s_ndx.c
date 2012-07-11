/****************
 * db_s_ndx.c
 * ^-_-^  1999.1.13
 ***************************************************************************/

//#include "dio.h"
//#include "btree.h"
#include "db_s_ndx.h"
#include "mistring.h"
#include "btreeext.h"


bHEAD * mIndexBuildKeyExpr(dFILE *df, char  *szExpr, \
					       short  wKeyLen, \
					       char  *ndxName);

int dbfOrderSyncToNdx(dFILE *df, bHEAD *bh, short wSkipAdjust)
{
    long   lno;
    long   recno;
    char   buf[256], buf1[256];
    char   indexType;

    //to avoid moveDbfPtr(pbh)
    indexType = bh->type;
    bh->type = BTREE_FOR_ITSELF;

    if( wSkipAdjust == 0 )
	IndexGoTop( bh );
    else
	IndexGoBottom( bh );

    for( lno = 1;  lno <= df->rec_num;  lno++ ) {
	recno = IndexCurRecno( bh );

	/*test code
	dseek(df, recno-1, dSEEK_SET);
	get1rec(df);
	df->rec_buf[40] = '\0';
	printf("%s\n", df->rec_buf);
	IndexSkip(bh,1);
	continue;
	test code*/

	/*
	dbf             ndx
	+--+
	|1 |<---------   8
	|2 |       /     3
	|3 |      /      10
	|4 |     |       4
	|5 |     /
	|6 |    |  <===========swap 8 and 1
	|7 |    /
	|8 |<---
	|9 |
	|10|
	+--+
	*/

	if( recno != lno )
	{ //swap them
	    dseek(df, lno-1, dSEEK_SET);
	    get1rec( df );
	    memcpy(df->rec_tmp, df->rec_buf, df->rec_len);

	    dseek(df, recno-1, dSEEK_SET);
	    get1rec( df );
	    dseek(df, lno-1, dSEEK_SET);
	    put1rec( df );

	    if( bh->vp == NULL )
		genDbfBtreeKey(bh, buf1);
	    else {
		char *sz;

		sz = (char *)CalExpr( (MidCodeType *)(bh->vp) );
		if( sz == NULL ) {
		    break;
		}

		memset(buf1, 0, bh->keyLen);
		strncpy(buf1, sz, BTREE_MAX_KEYLEN);
	    }

	    memcpy(df->rec_buf, df->rec_tmp, df->rec_len);
	    dseek(df, recno-1, dSEEK_SET);
	    put1rec( df );

	    IndexRecMove(bh, buf1, recno, -1);

	    if( bh->vp == NULL )
		genDbfBtreeKey(bh, buf);
	    else {
		char *sz;

		sz = (char *)CalExpr( (MidCodeType *)(bh->vp) );
		if( sz == NULL ) {
		    break;
		}

		memset(buf, 0, bh->keyLen);
		strncpy(buf, sz, BTREE_MAX_KEYLEN);
	    }
	    if( IndexRecMove(bh, buf, lno, recno) == LONG_MIN ) {
		;
	    }

	    //move the record and locate it
	    if( IndexRecMove(bh, buf1, -1, lno) == LONG_MIN ) {
		//error
		;
	    }

	}

	if( IndexSkip( bh, 1+wSkipAdjust ) == LONG_MIN ) {
	    ;
	}
    }

    bh->type = indexType;
    return  0;

} //end of dbfOrderSyncToNdx()


/*
* orderbyExec()
*****************************************************************************/
int orderbyExec(FromToStru *fFrTo)
{
	char   *FieldName[2];
	char    szGbNdx[MAXPATH];
	bHEAD  *orderbyBh;
extern  char    tmpPath[MAXPATH];

	sprintf(szGbNdx, "ORDBY%03X.NDX", intOfThread&0xFFF);
	makefilename(szGbNdx, tmpPath, szGbNdx);

	//build the index, no before index for reference
	if( fFrTo->wObKeyLen == SHRT_MAX ) {
	    FieldName[0] = fFrTo->cOrderbyKey;
	    FieldName[1] = NULL;

	    orderbyBh = IndexBuild((char *)(fFrTo->targefile), FieldName, szGbNdx, BTREE_FOR_OPENDBF);
	} else {
	    orderbyBh = mIndexBuildKeyExpr(fFrTo->targefile, fFrTo->cOrderbyKey, \
						fFrTo->wObKeyLen, \
						szGbNdx);
	}

	if( orderbyBh == NULL ) {
	    qsError = 3029;
	    return  FALSE;
	}

	dbfOrderSyncToNdx(fFrTo->targefile, orderbyBh, \
					fFrTo->iOrderbyScopeSkipAdjust);

	IndexClose( orderbyBh );
	unlink( szGbNdx );

	return  0;

} //end of orderbyExec()



/***************
**                            mIndexBuildKeyExpr()
*****************************************************************************/
bHEAD * mIndexBuildKeyExpr(dFILE *df, char  *szExpr, \
					       short  wKeyLen, \
					       char  *ndxName)
{
    bHEAD       *bh;
    MidCodeType *mx;
    char        *sz;
    char        buf[BTREE_MAX_KEYLEN];

    mx = WordAnalyse( szExpr );
    if( mx == NULL )
	return  NULL;

    if( SymbolRegister(mx, df, NULL, 0, NULL, 0) != NULL ) {
	FreeCode( mx );
	return  NULL;
    }

    bh = BppTreeBuild(ndxName, wKeyLen);
    if( bh == NULL ) {
	FreeCode( mx );
	return  NULL;
    }

    dseek(df, 0, dSEEK_SET);
    while( !deof(df) ) {
	getrec(df);

	sz = (char *)CalExpr( mx );
	if( sz == NULL ) {
	    break;
	}

	memset(buf, 0, bh->keyLen);
	strncpy(buf, sz, BTREE_MAX_KEYLEN);

	IndexRecIns(bh, buf, df->rec_no);
    }

    //FreeCode( mx );

    bh->dbfPtr = df;
    bh->type = BTREE_FOR_OPENDBF;
    bh->vp = mx;

    return  bh;

} //end of mIndexBuildKeyExpr()


void test( void )
{
    dFILE *df;
    bHEAD *bh;
    char *fieldName[2] = {"branch", NULL};

    df = dAwake("e:\\wmtasql\\doctor.dbf", DOPENPARA);
    bh  = mIndexBuildKeyExpr(df, "substr(branch,1,2)", \
						2, \
						"ls");
    //bh = IndexBuild(df, fieldName, "ls", BTREE_FOR_OPENDBF);

    dbfOrderSyncToNdx(df, bh, 0);

    dSleep(df);

    FreeCode( (MidCodeType *)(bh->vp) );
    IndexClose( bh );

} 
