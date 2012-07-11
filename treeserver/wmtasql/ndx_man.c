/*************
 *
 * ndx manage utility
 *
 * Writen by Xilong Pei   Jan. 27  1999
 *
 **************************************************************************/

#define __NDX_MAN_C__

#include "wst2mt.h"
#include "dio.h"
#include "btree.h"
#include "ts_dict.h"
#include "mistring.h"
#include "ndx_man.h"


int buildIndexAndRegThem(char *szDataBase, dFILE *df, char *ndxName, char *keys)
{
    int    k;
    char   buf[256];
    char   nameBuf2[64];
    char   nameBuf[256];
    bHEAD *bh;
    char  *s;

    strcpy(nameBuf, TrimFileName(df->name));

    for(k = 0;   k < DIO_MAX_SYNC_BTREE;   k++) {
	sprintf(buf, "NDX%d", k+1);

	s = GetCfgKey(csuDataDictionary, szDataBase, nameBuf, buf);
	if( s == NULL )
		break;

	if( *s == '\0' )
		continue;
    }

    if( k >= DIO_MAX_SYNC_BTREE ) {
	return  1;
    }

    makefilename(buf, df->name, ndxName);

#ifdef WSToMT
    EnterCriticalSection( &(df->dCriticalSection) );
#endif
    bh = IndexBAwake((char *)df, keys, buf, BTREE_FOR_OPENDBF);
#ifdef WSToMT
    LeaveCriticalSection( &(df->dCriticalSection) );
#endif
    if( bh == NULL )
	return  2;

    IndexSleep( bh );

    sprintf(buf, "NDX%d", k+1);
    sprintf(nameBuf2, "KEY%d", k+1);
    WriteCfgKey(csuDataDictionary, szDataBase, nameBuf, buf, ndxName);
    WriteCfgKey(csuDataDictionary, szDataBase, nameBuf, nameBuf2, keys);

    return  0;

} //end of buildIndexAndRegThem()



int dropIndexAndRegThem(char *szDataBase, dFILE *df, char *ndxName)
{
    int    k;
    char   buf[256];
    char   nameBuf2[64];
    char   nameBuf[256];
    char  *s;

    strcpy(nameBuf, TrimFileName(df->name));

    for(k = 0;   k < DIO_MAX_SYNC_BTREE;   k++) {
	sprintf(buf, "NDX%d", k+1);

	s = GetCfgKey(csuDataDictionary, szDataBase, nameBuf, buf);
	if( s == NULL )
		break;

	if( stricmp(s, ndxName) == 0 ) {

	    WriteCfgKey(csuDataDictionary, szDataBase, nameBuf, buf, "");
	    sprintf(nameBuf2, "KEY%d", k+1);
	    WriteCfgKey(csuDataDictionary, szDataBase, nameBuf, nameBuf2, "");

	    makefilename(buf, df->name, ndxName);
	    _unlink( buf );

	    return  0;
	}
    }

    return  1;

} //end of dropIndexAndRegThem()