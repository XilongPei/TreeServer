
int buildIndexAndRegThem(char *szDataBase, dFILE *df, char *ndxName, char *keys)
{
    int    k, kc;
    char   buf[256];
    char   nameBuf2[64];
    char   nameBuf[256];
    bHEAD *bh;

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
    bh = IndexBAwake((char *)df, keys, buf, BTREE_FOR_OPENDBF);
    if( bh == NULL )
	return  2;

    IndexSleep( bh );

    sprintf(buf, "NDX%d", k+1);
    sprintf(nameBuf2, "KEY%d", k+1);
    WriteCfgKey(csuDataDictionary, szDataBase, nameBuf, buf);
    WriteCfgKey(csuDataDictionary, szDataBase, nameBuf, nameBuf2);

} //end of buildIndexAndRegThem()