/****************
* tabtools.c
* 1995.11.1
* author: Xilong Pei
* copyright (c) EastUnion Computer Service Co., Ltd.
*****************************************************************************/

#include <string.h>
#include <io.h>
#include <limits.h>

#include "dio.h"
#include "strutl.h"
#include "btree.h"
#include "mistring.h"
#include "tabtools.h"

// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
short changeFldName(dFILE *df, short fldId, char *fldName)
{
    short  i;
    unsigned char   buf[32];

    if( fldId < 0 ) {
	for(i = 0;   i < df->field_num;    i++) {
		if( strnicmp(df->field[i].field, "T______", 7) == 0 )
			break;
	}
	if( i < df->field_num )
		fldId = i;
	else
		return  -1;
    }

    lseek(df->fp, 32L * (fldId+1), SEEK_SET);

    memset( buf, '\0', 32 );
    strncpy(buf, stoUpper(fldName), 11);
    buf[11] = df->field[fldId].fieldtype;
    buf[16] = (unsigned char)df->field[fldId].fieldlen;
    if( df->field[fldId].fieldtype == 'N' )
	    buf[17] = (unsigned char) df->field[fldId].fielddec;
    write(df->fp, buf, 32);

    return  fldId;

} // end of changeFldName()


// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
// the key field must be the first field
// bh if IndexOpen with OPEN
short fillTable(bHEAD *bh, char *key, char *dataFld, void *data)
{
    dFILE *df;
    long   l;
    short  i;

    df = bh->dbfPtr;

    i = GetFldid(df, dataFld);
    if( i < 0 ) {
	i = changeFldName(df, -1, dataFld);
	if( i < 0 )
		return  -1;
    }

    l = IndexSeek(bh, key);
    if( l == LONG_MIN ) {
	NewRec(df);
	dseek(df, 0, dSEEK_END);
	PutField(df, 0, key);
	IndexSyncDbf(bh, NULL);
    } else {
	get1rec(df);
    }
    PutField(df, i, data);
    put1rec(df);

    return  i;

}

/*
main()
{
	dFILE *df;
	bHEAD *bh;
	char *field[] = {"NUM", NULL};

	df = dopen("pcz.dbf", DOPENPARA);
	bh = IndexBuild(df, field, df->name, BTREE_FOR_OPENDBF);

	fillTable(bh, "1234567", "UM_MASK", "abc");
	fillTable(bh, "01", "UM_MASK", "def");
	//changeFldName(df, -1, "Xilong");
	dclose(df);
	IndexClose(bh);
}*/