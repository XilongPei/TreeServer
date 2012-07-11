/****************
* ^-_-^
* btreeext.c
* copyright (c) 1995
*****************************************************************************/
#include <string.h>

#define  __BTREEEXT_C__

#include "mistring.h"
#include "btree.h"
#include "btreeext.h"
#include "xexp.h"


//
//string fieldname comp
///////////////////////////////////////////////////////////////////////////
int keyFieldCmp(bHEAD *bh, char *fieldName)
{
    int    i;
    char   buf[BTREE_MAX_KEYLEN];

    if( bh == NULL )
        return  1;

    buf[0] = '\0';
    for(i = 0;   i < bh->keyFieldNum;   i++) {
          strcat(buf, bh->bObject.keyName[i].field);
          strcat(buf, "+");
    }
    buf[ strlen(buf)-1 ] = '\0';

    i = strlen(shrink(fieldName));

    //1999.1.3
    //return  stricmp(buf, fieldName);
    return  strnicmp(buf, fieldName, i) == 0 && \
					(buf[i] == '+' || buf[i] == '\0');

} // end of keyFieldCmp()


//
//stringS fieldname comp
///////////////////////////////////////////////////////////////////////////
short sKeyFieldCmp(bHEAD *bh, char **fieldName)
{
    short  i;
    char   buf[BTREE_MAX_KEYLEN];
    char   buf1[BTREE_MAX_KEYLEN];

    if( bh == NULL )
	return  1;

    buf[0] = '\0';
    for(i = 0;   i < bh->keyFieldNum;   i++) {
	  strcat(buf, bh->bObject.keyName[i].field);
	  strcat(buf, "+");
    }
    buf[ strlen(buf)-1 ] = '\0';

    buf1[0] = '\0';
    for(i = 0;  fieldName[i] != NULL;   i++) {
	  strcat(buf1, fieldName[i]);
	  strcat(buf1, "+");
    }
    buf1[ strlen(buf1)-1 ] = '\0';

    shrink(buf1);

    return  stricmp(buf, buf1);

} // end of sKeyFieldCmp()


//
// keyFieldPartCmp
///////////////////////////////////////////////////////////////////////////
short keyFieldPartCmp(bHEAD *bh, char *fieldName)
{
    short  i;
    char   *fldName[32];

    if( bh == NULL )
	return  1;

    if( seperateStr(fieldName, '+', fldName) == NULL )
	return  2;

    for(i = 0;   i < bh->keyFieldNum && fldName[i] != NULL;   i++) {
	  if( stricmp(trim(fldName[i]), bh->bObject.keyName[i].field) != 0 )
		break;
    }

    if( fldName[i] != NULL )
	return  3;
    return  0;

} // end of keyFieldPartCmp()



/***************
**                            IndexBuildKeyExpr()
*****************************************************************************/
_declspec(dllexport) bHEAD * IndexBuildKeyExpr(dFILE *df, \
					       char  *szExpr, \
					       short  wKeyLen, \
					       char  *ndxName)
{
    bHEAD       *bh;
    MidCodeType *mx;
    char        *sz;
    char        buf[BTREE_MAX_KEYLEN];

    if( df == NULL || szExpr == NULL || wKeyLen >= BTREE_MAX_KEYLEN || ndxName == NULL )
	return  NULL;
    
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

	//2000.8.5
	//remark it again, for the expression will do this, Xilong Pei
	//rtrim(buf);

	IndexRecIns(bh, buf, df->rec_no);
    }

    FreeCode( mx );

    bh->dbfPtr = df;
    bh->type = BTREE_FOR_OPENDBF;

    return  bh;

} //end of IndexBuildKeyExpr()




/////////////////////////////////////////////////////////////////////////////
