/****************
* ^-_-^
*                               dioext.c
*    Copyright (c) Xilong Pei, 1994 1997 1998
*****************************************************************************/

// this will make the btree give its runing message
//#define RuningMessageOn


#include <dos.h>
#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <share.h>
#include <STDARG.H>
#include <limits.h>
#include <dos.h>

#ifdef __TURBOC__
#include <dir.h>
#include <alloc.h>
#else
#include <malloc.h>
#include <sys\locking.h>
#endif

#include "mistring.h"
#include "dio.h"
#include "dioext.h"
#include "strutl.h"
#include "cfg_fast.h"
#include "btree.h"
#include "memutl.h"
#include "ts_dict.h"
#include "xexp.h"

#ifdef RuningMessageOn
	#include "busyinfo.h"
	#include "msgbar.h"
	#include "msgbox.h"
#endif

extern void DestroydFILE( dFILE *df );
extern void free_dFILE( dFILE *df );
extern dFILE *alloc_dFILE(void);
extern short  try_alloc( dFILE * );

//2 decide by myself, 5 overwrite, 6 not overwrite
short   confirm = 2;

/****************
*                               viewToDbf()
****************************************************************************/
long viewToDbf( char *view, char *dbf )
{
    dFILE       *vdf;
    dFILE       *df;
    dFIELD      *field;
    long        l, recnum;

    dsetbuf( 32000 );
    if( (vdf = dopen( view, (short)(O_RDONLY|O_BINARY), \
				(short)SH_DENYNO, (short)(S_IREAD|S_IWRITE) )) == NULL ) {
	return -1;
    }

    field = dfcopy( vdf, NULL );
    if( (df = dcreate( dbf, field )) == NULL ) {
	dclose( vdf );
	free( field );
	return  -2;
    }
    free( field );

    dseek(vdf, 0L, dSEEK_SET);
    l = 0;
    recnum = getRecNum( vdf );
    while( l < recnum ) {
	getrec( vdf );

#ifdef RuningMessageOn
		euBusyInfo( ++l );
#endif

	PutRecord( df, vdf->rec_buf );
    }

    dclose( vdf );
    dclose( df );

    return  l;

} // end of function viewToDbf()


/****************
*                               getViewDbfname()
****************************************************************************/
char *getViewDbfname( char *view )
{
    int 	handle;
    char	*sz;

    handle = sopen(view, O_RDONLY|O_BINARY, SH_DENYNO, S_IREAD|S_IWRITE);
    if( handle <= 0 ) {
	return  NULL;
    }

    if( strcmp(dGetLine( handle ), szDbfViewFlag) != 0 ) {
	close( handle );
	return  NULL;
    }

    sz = dGetLine( handle );
    close( handle );
    return  sz;


} // end of function getViewDbfname()


/****************
*                               dFILEcmp()
****************************************************************************/
short dFILEcmp( dFILE *df1, dFILE *df2 )
{
    unsigned short  i, k;
    dFIELD *field, *field2;

    if( getFieldNum(df1) != getFieldNum(df2) )      return  SHRT_MIN;

    for( i = 0;   i < getFieldNum(df1);   i++ ) {
	field = getFieldInfo( df1, i );

	// is the field exist in df2
	k = GetFldid( df2, field->field );
	if( k == 0xFFFF )       return  i+1;

	// check type and fielddec
	field2 = getFieldInfo( df2, i );
	if( field->fieldtype != field2->fieldtype || \
		field->fielddec != field2->fielddec )   return  i+1;

	// check 'C' type field len
	if( field->fieldtype == 'C' && \
			field->fieldlen != field2->fieldlen )   return  i+1;
    } // end of for

    return  0;

}  // end of dFILEcmp()

/****************
*                               getdViewViewdbf()
****************************************************************************/
dFILE *getdViewViewdbf( dFILE *df )
{
    dVIEW *dv;

    if( df == NULL )    return  NULL;

    dv = (dVIEW *)df;
    if( ( df->op_flag == VIEWFLAG ) && ( dv->view != NULL ) ) {
	return  dv->view;
    }

    return  NULL;

} // end of function getdViewViewdbf()

/****************
*                               dwriteDbfInfo()
****************************************************************************/
short dwriteDbfInfo( dFILE *df, char *path )
{
    short i;
    dVIEW *dv;

    if( df == NULL )    return  0;
    { // for less stack
	char buf[NAMELEN];

	makeTrimFilename(buf, path, df->name);
	i = open(buf, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IWRITE);
	if( i <= 0 )    return  1;
    }


    dv = (dVIEW *)df;
    if( ( df->op_flag == VIEWFLAG ) && ( dv->view != NULL ) ) {

	dFILE dFile;

	memcpy(&dFile, df, sizeof(dFILE) );
	dFile.rec_num = dv->view->rec_num;
	dFile.op_flag = TABLEFLAG;
	write( i, &dFile, sizeof(dFILE) );

    } else {
	write( i, df, sizeof(dFILE) );
    }
    return  close( i );

} // end of dwriteDbfInfo()


/****************
*                               dreadDbfInfo()
****************************************************************************/
dFILE *dreadDbfInfo( dFILE *df, char *path )
{
    short i;

    if( df == NULL ) {
	if( (df = (dFILE *)malloc( sizeof( dFILE ) )) == NULL ) return NULL;
    }

    i = open(path, O_RDONLY|O_CREAT|O_TRUNC|O_BINARY, S_IWRITE);
    if( i <= 0 )        return  NULL;

    read( i, df, sizeof(dFILE) );
    close( i );

    return  df;

} // end of dreadDbfInfo()


/****************
*                               dbfCopy()
****************************************************************************/
long dbfCopy( dFILE *sdf, dFILE *tdf )
{
    long recno, recnum;

    if( sdf == NULL || tdf == NULL )    return  -1;

    recnum = getRecNum( sdf );
    for( recno = 1;  recno <= recnum;  recno++ ) {
	drecopy( sdf, recno, tdf, recno );
    }

    return  recnum;

} // end of dbfCopy()


short dAccess( char *filename, short amode )
{
    char buf[NAMELEN];

    MK_name( strcpy(buf, filename) );

    return  access( buf, amode );

}

/****************
*       tgCarryDbfRec()
* function:
*   get the field from another field which has the same "name"
* return
*   copied field number
****************************************************************************/
short tgCarryDbfRec( dFILE *tdf, dFILE *sdf )
{
	short   i, k;
	unsigned short j;

	k = 0;
	for( i = 0;   i < tdf->field_num;    i++ ) {
		j = GetFldid( sdf, tdf->field[i].field );
		if( j != 0xFFFF ) {
			k++;
			// here i, j perhaps be virtual field name, so we can not use
			// put_fld to put this field, else we should get field i's
			// virtual field name
			memcpy( tdf->field[i].fieldstart, \
					sdf->field[j].fieldstart, \
					tdf->field[i].fieldlen );
		}
	}

	return  k;

} // end of tgCarryDbfRec()



/****************
*       dModiCreate()
****************************************************************************/
dFILE *dModiCreate( char *filename, dFIELD field[] )
{
    dFILE  *df;
    dFILE  *tdf;
    char    dioTmpFile[NAMELEN];

    sprintf(dioTmpFile, "MODISTRU.%03X", intOfThread&0xFFF);

    //make the temp file and filename at the same location
    makefilename(dioTmpFile, filename, dioTmpFile);

    unlink( dioTmpFile );
    if( rename( filename, dioTmpFile ) != 0 )
	return  NULL;

    dsetbuf( 64000 );
    df = dopen( dioTmpFile, (short)(O_RDONLY|O_BINARY), (short)SH_DENYNO, (short)(S_IREAD|S_IWRITE) );
    if( df == NULL ) {
	rename( dioTmpFile, filename );
	return  NULL;
    }

    tdf = dcreate( filename, field );
    if( tdf == NULL ) {
	dclose( df );
	rename( dioTmpFile, filename );
	return  NULL;
    }

    dbfCopy( df, tdf );
    dclose( df );
    unlink( dioTmpFile );

    return  tdf;

} // end of dModiCreate()



/****************
*       dDbfToTxt()
****************************************************************************/
long dDbfToTxt( char *dbfName, char *txtName )
{
    dFILE       *df;
    FILE        *out;
    long        l;
    char        buf[FIELDMAXLEN+1];
    unsigned short i;

    df = dAwake( dbfName, (short)(O_RDONLY|O_BINARY), (short)SH_DENYNO, (short)(S_IREAD|S_IWRITE) );
    if( df == NULL )    return  -1;
    out = fopen(txtName, "wb");
    if( out == NULL ) {
	dSleep( df );
	return  -2;
    }

    while( !deof( df ) ) {
	getrec(df);
	for( i = 0; i < df->field_num; i++) {
		memcpy( buf, df->field[i].fieldstart, df->field[i].fieldlen );
		buf[df->field[i].fieldlen] = '\0';
		strToSourceC( buf, buf, '"' );
		if( i < df->field_num+1 )       strcat(buf, ",");
		fprintf(out, "%s", buf);
	}
	fprintf(out, "\n");
    }
    l = getRecNum( df );
    dSleep( df );
    fclose( out );

    return  l;

} // end of dDbfToTxt()



/*
-------------------------------------------------------------------------
!!                      dIsAwake()
--------------------------------------------------------------------------*/
int dIsAwake(char *dbfName)
{
    int i;
    char temp[NAMELEN];

    if( dbfName == NULL )    return  -1;

    strncpy(temp, dbfName, NAMELEN);
    MakeName( temp, "DBF", NAMELEN);
#ifdef WSToMT
    EnterCriticalSection( &dofnCE );
#endif
    for(i = 0;  i < _DioOpenFileNum_;  i++ ) {
	if( (/*_DioOpenFile_[i].op_flag == TABLEFLAG && */ \
	     stricmp(_DioOpenFile_[i].df->name, temp) == 0 ) ||
	     (_DioOpenFile_[i].op_flag == VIEWFLAG && \
	     stricmp(((dVIEW *)_DioOpenFile_[i].df)->name, temp) == 0 ) )
        {

#ifdef WSToMT
		LeaveCriticalSection( &dofnCE );
#endif
                return _DioOpenFile_[i].df->write_flag.SleepDBF;
	}
    }

#ifdef WSToMT
    LeaveCriticalSection( &dofnCE );
#endif

    return  0;

} /* end of function dIsAwake() */


/*
-------------------------------------------------------------------------
!                       joinCreateDbf
! the return should be dSleep()
------------------------------------------------------------------------*/
dFILE *joinCreateDbf( short DbfNum, char *TargetDbf, char *dbf[] )
{
    dFILE  *df;
    dFIELD *dField;
    short i, j, k;
    short FieldNum;
    short firstFld;

    dsetbuf( SYSSIZE );
    for(i = 0;   i < DbfNum;  i++ ) {
	df = dAwake(dbf[i], (short)(O_RDWR|O_BINARY), (short)SH_DENYNO, (short)(S_IREAD|S_IWRITE));
	if( df == NULL ) {
		if( dField == NULL )    free( dField );
		return  NULL;
	}
	if( i == 0 ) {
	   dField = dfcopy(df, NULL);
           for(firstFld = 0;  firstFld < df->field_num && \
                        dField[firstFld].field[0] != '\0';  firstFld++);

	   FieldNum = firstFld + 1;
	} else {
		FieldNum += df->field_num;
		dField = realloc(dField, FieldNum*sizeof(dFIELD));
		for( k = 0;  k < df->field_num;  k++ ) {

		    for(j = 0;   j < firstFld && \
			stricmp(dField[j].field, df->field[k].field)!=0;
		    j++);

		    if( j >= firstFld ) {
			   memcpy( &(dField[firstFld++]), &(df->field[k]), sizeof(dFIELD) );
		    }
		}
	}
	dSleep( df );
    }
    dField[firstFld].field[0] = '\0';

    if( (df = dcreate(TargetDbf, dField)) != NULL ) {
	free( dField );
	if( dSetAwake(df, &df) != 1 ) {
	    dclose(df);
	    return  NULL;
	}
	return  df;
    }
    free( dField );

    return  NULL;

} // end of joinCreateDbf()

/*
-------------------------------------------------------------------------
!                       dAppendFile()
------------------------------------------------------------------------*/
long dAppendFile(char *sDbf, char *tDbf, bHEAD *bh, short uniqueOrFree)
{
    dFILE *sdf, *tdf;
    long  s_recnum, t_recnum, recnum, l;
    char  keyBuf[BTREE_MAX_KEYLEN];
    int   i;
    int   overWrite = 4;

    sdf = dAwake(sDbf, (short)(O_RDONLY|O_BINARY), (short)SH_DENYNO, (short)(S_IREAD|S_IWRITE));
    if( sdf == NULL )	return  -1;

    tdf = dAwake(tDbf, DOPENPARA);
    if( tdf == NULL )
    {
	return  -2;
/*	dFIELD  *field = dfcopy(sdf, NULL);
	tdf = dcreate(tDbf, field);
	free(field);
	if( tdf == NULL ) {
		dSleep( sdf );
		return  -2;
	}
	dSetAwake(tdf);*/
    }

    if( bh == NULL )
	uniqueOrFree = dKEY_FREE;

    s_recnum = 1;
    recnum = getRecNum(sdf);
    t_recnum = getRecNum(tdf) + 1;

    if( uniqueOrFree == dKEY_UNIQUE ) {

	dFIELD	*field[MAX_KEYFIELD_NUM];
	char	*p;

	if( confirm < 3 ) {
	    p = GetCfgKey(csuDataDictionary, "eucfg", NULL,"AP_OVERWRITE");
	    if( p != NULL ) {
		overWrite = atoi(p);
		confirm = 1;
	    } else {
		confirm = 0;
	    }
	} else {
	    overWrite = confirm - 3;
	}

	for( i = 0;    i < bh->keyFieldNum;    i++) {
		field[i] = getFieldInfo(sdf, \
					GetFldid(sdf, bh->bObject.keyName[i].field));
	}

	while( s_recnum <= recnum ) {
#ifdef RuningMessageOn
		euBusyInfo( s_recnum );
#endif
		//in dio component, use it directly
		sdf->rec_p = s_recnum;
		get1rec(sdf);
		{ //gen the key
		    int  	len;
		    short 	i;

		    len = 0;
		    for( i = 0;    i < bh->keyFieldNum;    i++) {
			strncpy(&keyBuf[len], field[i]->fieldstart, field[i]->fieldlen);
			len += field[i]->fieldlen;
		    }   // end of for
		    keyBuf[len] = '\0';

		    if( *rtrim(keyBuf) == '\0' ) {
			strcpy(keyBuf, BTREE_BLANKKEY);
		    }
		}
		if( (l = IndexSeek(bh, keyBuf)) != LONG_MIN )
		{ //modi this record
#ifdef RuningMessageOn
			if( overWrite > 1 && !confirm ) {
			    setMBName("~A~全部", cmYes);
			    setMBName("~Y~本记录", cmNo);
			    setMBName("~N~不覆盖", cmCancel);
			    switch( multiMessageBox(mfConfirmation|mfYesNoCancel, \
				  "相同关键字[%s]记录已存在, 是否覆盖?\n", \
				  keyBuf) )
			    {
				case cmYes:
					overWrite = 1;
					break;
				case cmNo:
					overWrite = 2;
					break;
				default:
					overWrite = 3;
			    }
			    setMBName(NULL, -1);
			}
#endif
			if( overWrite <= 2 )
			{
			    drecopy(sdf, s_recnum++, tdf, l);
                            dseek(tdf, -1, dSEEK_CUR);
			    IndexSyncDbf(bh, NULL);
			} else {
			    s_recnum++;
			    dseek(sdf, 1, dSEEK_CUR);
			}
		} else {
			drecopy(sdf, s_recnum++, tdf, t_recnum++);
		}
	} //end of while
    } else {
	while( s_recnum <= recnum ) {

#ifdef RuningMessageOn
		euBusyInfo( s_recnum );
#endif
		drecopy(sdf, s_recnum++, tdf, t_recnum++);
		if( bh != NULL ) {
			IndexSyncDbf(bh, NULL);
		}
	}
    }

    dSleep( sdf );
    dSleep( tdf );

    return  s_recnum;

} // end of dAppendFile()



/****************
*                               dbfFlagCopy()
****************************************************************************/
long dbfFlagCopy( char *sdbf, char *tpath, char *flagFld )
{
    long 	recno, recnum, trecno;
    dFILE 	*df, *tdf;
    dFIELD	*field;
    char	buf[256];
    char	c;
    short	fldid;

    if( sdbf == NULL || tpath == NULL )    return  -1;

    df = dAwake(sdbf, (short)(O_RDONLY|O_BINARY), (short)SH_DENYNO, (short)(S_IREAD|S_IWRITE));
    if( df == NULL )	return  -1;

    fldid = GetFldid(df, flagFld);
    if( fldid < 0 ) {
	dSleep(df);
	return  0;
    }

    field = dfcopy(df, NULL);
    makeTrimFilename(buf, tpath, sdbf);
    tdf = dcreate(buf, field);
    if( tdf == NULL ) {
	dSleep(df);
	return  -2;
    }
    free(field);

    recnum = getRecNum( df );
    trecno = 0;
    for( recno = 0;  recno < recnum;  recno++ ) {
	dseek(df, recno, dSEEK_SET);
	getrec(df);
	get_fld(df, fldid, buf);
	c = *ltrim(buf);
	if( c != '\0' && c != '0' ) {
		drecopy( df, recno+1, tdf, ++trecno );
	}
    }

    dSleep(df);
    dclose(tdf);

    return  trecno;

} // end of dbfCopy()



/****************
*                               dsetFldFlag()
****************************************************************************/
long dsetFldFlag( char *sdbf, char *flagFld, char *cont )
{
    long 	recno, recnum;
    dFILE 	*df;
    short	fldid;
    long	l;

    if( sdbf == NULL )    return  -1;

    df = dAwake(sdbf, DOPENPARA);
    if( df == NULL )	return  -1;

    fldid = GetFldid(df, flagFld);
    if( fldid < 0 ) {
	dSleep(df);
	return  0;
    }

    recnum = getRecNum( df );
    l = 0;
    for( recno = 0;  recno < recnum;  recno++ ) {
	dseek(df, recno, dSEEK_SET);
	getrec(df);
	put_fld(df,fldid,cont);
	dseek(df, recno, dSEEK_SET);
	putrec(df);
    }

    dSleep(df);

    return  l;

} // end of dsetFldFlag()


/****************
*				batchAppend()
****************************************************************************/
#ifdef DOS_COMPATABLE
unsigned short batchAppend( char *spath, char *files, char *dpath )
{
    char 	temp[MAXPATH];
    char 	temp1[MAXPATH];
    struct 	find_t fblk;
    unsigned short count = 0;
    dFILE  	*sdf, *tdf;

    strcpy( temp, spath );
    strcat( temp, files );
    if( !_dos_findfirst(temp, _A_ARCH, &fblk) )
    {
	do {

	    count++;

	    strcpy( temp, spath );
	    strcat( temp, fblk.name );

	    strcpy( temp1, dpath );
	    strcat( temp1, fblk.name );

#ifdef RuningMessageOn
	messageBar( temp );
#endif
	    dAppendFile(temp, temp1, NULL, dKEY_FREE);
	} while( !_dos_findnext(&fblk) );
    }

    return  count;

} // end of batchCopy()
#endif


/****************
*				saveDfileEnv()
****************************************************************************/
dFILEENV *saveDfileEnv(dFILE *df, dFILEENV *env)
{
    if( df == NULL || env == NULL )
	return  NULL;

    env->rec_p = df->rec_p;
    return  env;
} //end of saveDfileEnv()


/****************
*				setDfileEnv()
****************************************************************************/
dFILE *setDfileEnv(dFILE *df, dFILEENV *env)
{
    if( df == NULL || env == NULL )
	return  NULL;

    df->rec_p = env->rec_p;
    return  df;
} //end of setDfileEnv()



/****************
*                               recDelPack()
* the rec_buf must be the content of current record!
**************************************************************************/
long recDelPack(dFILE *df, bHEAD *bh)
{
    long rec_p = df->rec_p;
    char buf[256];
    int  i;
    int  k;

    if( df == NULL )         return  -1;

/*in _ASK_RecPack(), the lock is called
#ifdef WSToMT
    wmtDbfLock(df);
#endif
*/
    if( rec_p > df->rec_num )
	rec_p = df->rec_num;

    if( rec_p < df->rec_num )
    { //keep the rec_buf, for the calculator will use it anymore, else
      //the NEW record will never be calculated.

	memcpy(df->rec_tmp, df->rec_buf, df->rec_len);
	for(k = 0;   k < df->syncBhNum;   k++) {
	    IndexRecDel((bHEAD *)(df->bhs[k]), \
				genDbfBtreeKey((bHEAD *)(df->bhs[k]), buf), rec_p);
	}
	if( bh != NULL ) {
	    IndexSyncDbf(bh, df->rec_tmp);
	    IndexRecDel(bh, genDbfBtreeKey(bh, buf), df->rec_num);
	}

	dseek(df, -1, dSEEK_END);
	get1rec(df);
	dseek(df, rec_p-1, dSEEK_SET);
	put1rec(df);
	memcpy(df->rec_buf, df->rec_tmp, df->rec_len);
    } else {
	for(k = 0;   k < df->syncBhNum;   k++) {
	    IndexRecDel((bHEAD *)(df->bhs[k]), \
				genDbfBtreeKey((bHEAD *)(df->bhs[k]), buf), rec_p);
	}
    }

    if( df->pdf != NULL )
	(df->pdf->rec_num)--;
    df->rec_num--;

    dflush(df);
    if( IsMemoExist(df) == 0 ) {
	for(i = 0;  i < df->field_num;  i++) {
	      if( df->field[i].fieldtype == 'M' || df->field[i].fieldtype == 'G' ) {
                  PhyDbtDelete(df, atol(substr(df->field[i].fieldstart, 0, 10)));
			  }
        }
    }

    df->rec_p = 0-df->rec_p;
    //dseek(df, rec_p-2, dSEEK_SET);

/*in _ASK_RecPack(), the lock is called
#ifdef WSToMT
	wmtDbfUnLock(df);
#endif
*/
    return  rec_p;

} //end of recDelPack()


/*
-------------------------------------------------------------------------
!!                      dTmpCreate()
--------------------------------------------------------------------------*/
dFILE *dTmpCreate(dFIELD field[])
{
    dFILE *df;
    unsigned short i, k;
    int    j;


    if ( ( df = alloc_dFILE() ) == NULL )
	return  NULL;

    //df->write_flag.creat = 1;
    df->op_flag = TABLEFLAG;

    for(i = 0; field[i].field[0] != '\0'; i++ );
    df->field_num = i;

    df->field = (dFIELD *)zeroMalloc(i*sizeof(dFIELD));
    df->fld_id = (short *)calloc( i, sizeof ( short ) );
    if( df->field == NULL || df->fld_id == NULL ) {
	dERROR  = 1001; /* ENOMEM */
	free_dFILE( df );
	return( NULL );
    }

    df->dbf_flag = 3;
    for( j = 1, i = 0, k = 0;  i < df->field_num; i++ ) {

	//shrink the VIEW_RECNO_FIELD
	if( stricmp(field[i].field, VIEW_RECNO_FIELD) == 0 && i > 0 )
		continue;

	memcpy(df->field[k].field, stoUpper(field[i].field), FIELDNAMELEN );
	if( ( df->field[k].fieldtype = toupper(field[i].fieldtype) ) == 'M' )
		df->dbf_flag = 0x83;
	if( df->field[k].fieldtype == 'G' )
		df->dbf_flag = 0x83;

	/*in virtual dbf file, to compatible to ODBC data source, the 'D'
	  can be larger than 8 chars 1998.7.12
	if( df->field[k].fieldtype == 'D' ) {
	    field[i].fieldlen = 8;
	    field[i].fielddec = 0;
	}
	*/

	df->field[k].fieldlen = field[i].fieldlen;
	df->field[k].fielddec = field[i].fielddec;
	df->field[k].fieldstart = (unsigned char *)j;
	df->fld_id[k] = i;
	j += field[i].fieldlen;

	k++;
    }
    df->rec_len = j;
    df->headlen = ( df-> field_num + 1 ) * 32 + 1;

    df->rec_buf = (unsigned char *)zeroMalloc( 2*(j+1) );
    if( df->rec_buf == NULL ) {
	dERROR = 1001;
	free_dFILE( df );
	return( NULL );
    }
    df->rec_buf[j] = '\0';

    //1997.12.18
    df->rec_tmp = &(df->rec_buf[j + 1]);

    for( i = 0;  i < df->field_num; i++ )
	 df->field[i].fieldstart = (unsigned short)df->field[i].fieldstart + \
								df->rec_buf;

    df->buf_rsize = 0;
    df->buf_bsize = 0;
    /*1998.10.19
    if( try_alloc( df ) != dOK ) {
	free_dFILE( df );
	return( NULL );
    }*/

    {
      struct tm *ntime;
      time_t ltime;

      time ( &ltime );
      ntime = gmtime ( &ltime );
      df->last_modi_time[0] = (unsigned char)ntime->tm_year;
      df->last_modi_time[1] = (unsigned char)ntime->tm_mon + 1;
      df->last_modi_time[2] = (unsigned char)ntime->tm_mday;
    }

    df->oflag = (short)(O_RDONLY|O_BINARY);	//readonly
    df->pmode = (short)S_IWRITE;

    df->write_flag.dbt_write = 0;
    df->dbt_buf = NULL;
    df->rec_num = 0;
    df->rec_no = 1;
    df->rec_p = 1;
    df->rec_end = 1;

    df->rec_beg = 1;
    df->buf_freshtimes = 1;
    df->write_flag.buf_write = 0;
    df->write_flag.file_write = df->write_flag.append = 1;
    df->buf_methd = 'S';
    df->buf_usetimes = IniBufUseTimes;

    return ( df );

}/* end of function dTmpCreat() */


/*
----------------------------------------------------------------------------
!!                      dTmpClose()
----------------------------------------------------------------------------*/
short dTmpClose(dFILE *df)
{
    if( df == NULL )	return  2;
    if( df->op_flag == VIEWFLAG ) {

	dVIEW *dv = (dVIEW *)df;

	DestroydFILE( df );
	if( dv->ViewField != NULL ) {
		free( dv->ViewField );
		free( dv->szfield );
	}
	free( dv->rec_buf );
	free( dv );
	return  0;

    } // ELSE

    free_dFILE( df );

    return  0;

}/* end of function dTmpClose() */


int checkRecValid(dFILE *df)
{
    ///////////
    //check record
    if( df->isRecCheck ) {
	int  i;

	for(i = 0;  i < df->field_num;  i++) {
	    if( df->field[i].px != NULL ) {
		if( (short)CalExpr((MidCodeType *)df->field[i].px) == 0 ) {
			return  i+1;
		}
	    }
	}
    }

    return  0;

} //checkRecValid()


/*long viewUpdateDbf( char *view )
{
    long        l;
    static char *dbf = "ILOVEVEW.VKY";
    char        buf[MAXPATH];

    l = viewToDbf( view, dbf );
    unlink(

}
*/

/* self test main function
main( int argc, char *argv[] )
{
	int i;
	dFILE *df1, *df2;

	df1 = dopen("SUB0", DOPENPARA);
	df2 = dopen("SUB00", DOPENPARA);

	i = dFILEcmp( df1, df2 );
	printf("%d", i);

	dclose( df1 );
	dclose( df2 );


}
*/


