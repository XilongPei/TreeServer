/*********
 *  DIOFILE.C   MIS lower supportment ( I ) -- Data I/O
 *
 *              Main modular.
 *
 * Copyright: (c) Shanghai Institute of Railway Technology  1991, 1992
 *                CRSC 1997, 1998
 *                Shanghai Withub Vision Computer Software Co., Ltd.
 *		  1999-2000
 * Notice:
 *    1997.6.21  add informix support
 *    1997.10.24 add windows multi-thread support
 *    1997.11.5  allow the fieldname exced 10 chars long, upper to 24
 *    1997.12.3  enhance the dbt, let it support block to be deleted,
 *               the block data structure is DBTBLOCK
 *    1997.12.18 now, I can support sync B++,
 *    1998.1     modify dpack(), support sync B++, and keep_physical_order
 *    1998.6.21  support RJ
 *    1998.7.1   let odbcOpen(), and dAwake() use different temp name, to
 *		 support multi work
 *    1998.11.10 check memo field, when the dbf has no memo flag
 *    1998.12.9  dFILE's original version keep the expression's string
 *                       the colon version keep the expression inside code
 *    1998.12.30 SHAREABLE support for OPTIMIZED_FILE_MAN
 *    1999.9.10  put1rec() in dpack() is changed to:
 *		 df->slowPutRec = 0, and then change it back
 *		 for when df->slowPutRec is not 0, the 'M','G' field willn't
 *		 be writen directly
 *    1999.11.20
 *		let the index use for check only, DONNOT
 *		affecting the table operation
 *		df->bhs[kc]->type = BTREE_FOR_ITSELF;
 *    2000.2.12
 *      	df->mainkey_fields added
 *              MAINKEY_DEF if asqlwmt.cfg added insted of MAINKEY_NUM
 *	        to define mainkey with field_list such as: code,name,sex
 *		insted to define mainkey with field_num
 *    2000.3.21 if( df->write_flag.buf_write == 1 ) added in dflush()
 *              sometime a file is writen, but the current buffer hasn't
 *		been writen!
 *    2000.4.20 lseek() may cause error when the disk is error, check it
 *              to avoid data missing
 *    2000.6.12 supprt an ASQL call before open an view
 *
 *
 * Caution:
 *    1. 0xFFFF(unsigned short) == -1(short)       1994.12.29 Xilong
 *    2. in multi thread the dbt_buf should use less,
 *       and the dbt information has no cache
 * Modify Memo:
 *     field_ident and GetFldid modified        1995.5.4 Xilong
 *
 *  1999.5.11, add "really type"
 *  DIO field description:
 *  [0......10], 11  , 12,13,14,15           ,16       ,17
 *   field name, type,         , really type ,fieldlen ,dev
 *
 *
 *
 ****************************************************************************/


#undef  RAM_ENOUGH
#define RJ_SUPPORT

//#define TREESVR_SL
//OPTIMIZED_FILE_MAN:
//whether the file will be optimized managed by TreeServer
//in standalone version, #undef this
#ifdef TREESVR_SL
#pragma message("TreeServer Standalone")
    #undef OPTIMIZED_FILE_MAN
#else
    #define OPTIMIZED_FILE_MAN		1
#endif

#define DBT_IS_GDC
#define TOTAL_LOCKS_PER_DF	1024

#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <search.h>
#include <limits.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <share.h>
#include <STDARG.H>
#include "dir.h"

#ifdef __TURBOC__
#include <dir.h>
#include <alloc.h>
#else
#include <malloc.h>
#include <sys\locking.h>
#endif

#define STD_EXTNAME     ".dbf"
#define IS_DioMAIN      "Windows"
#define DBT_GDC_KEY_LEN sizeof(long)

#include "mistring.h"
#include "arrayutl.h"
#include "dio.h"
#include "diodbt.h"
#include "dioext.h"
#include "memutl.h"
#include "strutl.h"
#include "dir.h"
#include "wst2mt.h"
#include "cfg_fast.h"
#include "sql_dio.h"
#include "odbc_dio.h"
#include "ts_dict.h"
#include "xexp.h"
#include "filetool.h"
#include "dbtree.h"

//include for support call ASQL before open an view
#include "asqlana.h"
#include "wmtasql.h"
#include "ts_const.h"


#ifdef __cplusplus
short min(short value1, short value2);
#endif
short  try_alloc( dFILE * );
//static int    longcomp( const void *, const void * );
static unsigned char *CryptStr = (unsigned char *)"Cryptic! TGMIS\n\x1A";
static dFILE *ReadDbfHead( dFILE * );
static dFILE *dInitDF( char *, short, short, short );
dFILE *alloc_dFILE(void);
void DestroydFILE( dFILE *df );
void free_dFILE( dFILE *df );
static char **ReadViewFieldName( int, dVIEW * );
static unsigned short bindfield( dVIEW *dv );
static short modViewForPhydelRec( dFILE *df, long recno );
static short getVirtualId(dFILE *df,short i);
static int   dIncreaseCount(dFILE *df);

WSToMT static dFILE *tempTableList[_DIOFILENUM_];
WSToMT static short  tempTableNum = 0;

#ifdef WSToMT
dFILE *wsmDf(dFILE *df, short oflag, short shflag, short pmode);
#endif

//2000.6.12
int calASQLforView(char *szDataBase, dVIEW *dv );

#ifdef __cplusplus
short min(short value1, short value2)
{
     return ( (value1 < value2) ? value1 : value2);
}
#endif


/*
-----------------------------------------------------------------------
		MK_name()
-----------------------------------------------------------------------*/
void MK_name( char *filename )
{
/* old version
    char *s;

    trim( filename );
    s = strrchr( filename, '\\' );    //  search reversor
    if( s == NULL ) s = filename;
    s = strchr( s, '.' );
    if( s == NULL ){
	if( strlen ( filename ) <= NAMELEN - EXTLEN ) {
	    strcat( filename, STD_EXTNAME );
	} else {
	    dERROR = 1012;
	    return( dNOT_OK );
	}
    }
    return( dOK );
*/
    char drive[MAXDRIVE];
    char dir[MAXDIR];
    char file[MAXFILE];
    char ext[MAXEXT];
    char path[_MAX_PATH];
    char *sz;

	
    //2000.7.10
    if( filename[0] == '"' ) {
	char *sz1;

        sz = filename+1;
        sz1 = strchr(filename, '"');
	if( sz1 != NULL )
	    *sz1 = '\0';
    } else {
	sz = filename;
    }

   _splitpath(sz, drive, dir, file, ext);
   if( ext[0] == '\0' ) strcpy(ext, STD_EXTNAME );
   _makepath(path,drive,dir,file,ext);

   _fullpath(filename, path, _MAX_PATH );
 
}


/*
-------------------------------------------------------------------------
		free_dFILE()
--------------------------------------------------------------------------*/
void free_dFILE( dFILE *df )
{
	DestroydFILE( df );
	free( df );

} // end of function free_dFILE()

/*
---------------------------------------------------------------------------
	    dsetbuf()
---------------------------------------------------------------------------*/
/* SYSSIZE  defined in DIO.H */
unsigned int  BUFSINKSIZE = SYSSIZE;
void dsetbuf( int dBufSize )
{ 
	if( dBufSize <= 1 ) {
		BUFSINKSIZE = SYSSIZE;
	} else {
		BUFSINKSIZE = dBufSize;
	}
}/* end  of dsetbuf() */

/*
--------------------------------------------------------------------------
	    try_alloc()
--------------------------------------------------------------------------*/
short try_alloc( dFILE *df )
{
    int rsize;

    if( (df->dbf_flag & 0x83) == 0x83 ) {
	if( ( df->dbt_buf = (unsigned char *)malloc( DBTBLOCKSIZE+1 ) ) == NULL ) {
		/* add 1 byte for hold tail '\0' */
		return( dNOT_OK );
	}
	df->dbt_buf[DBTBLOCKSIZE] = '\0';
	if( (df->dbt_head=(long *)zeroMalloc(DBTDELMEMORYSIZE*sizeof(long))) == NULL ) {
		free( df->dbt_buf );
		return( dNOT_OK );
	}
    } else {
	df->dbt_head = NULL;
	df->dbt_buf = NULL;
    }

    rsize = BUFSINKSIZE / df->rec_len;
    if( rsize <= 0 ) {
	df->error = 1013;
	if( (df->dbf_flag & 0x83) == 0x83 ) {
		free( df->dbt_head );
		free( df->dbt_buf );
	}
	return( dNOT_OK );
    }

    do {
	df->buf_sink = (unsigned char *) malloc( rsize * df->rec_len );
	if( df->buf_sink != NULL ) {
		df->buf_pointer = (dBUFPOINTER *)calloc(rsize, sizeof(dBUFPOINTER));
		if( df->buf_pointer == NULL ) {
			rsize /= 2;
			free( df->buf_sink );
		}
	} else {
		rsize /= 2;
		df->buf_pointer = NULL;
	}
    } while ( ( df->buf_pointer == NULL ) && ( rsize != 0 ) );

    if( rsize == 0 ) {
	df->error = 1001;
	if( (df->dbf_flag & 0x83) == 0x83 ) {
		free( df->dbt_head );
		free( df->dbt_buf );
	}
	return( dNOT_OK );
    }
/*else*/ df->buf_rsize = rsize;
     df->buf_bsize = rsize * df->rec_len;

     return( dOK );

}

/*
--------------------------------------------------------------------------
	    dresetbuf()
--------------------------------------------------------------------------*/
short dresetbuf( dFILE *df, int dBufSize )
{
    int   rsize;
    void  *p1, *p2;
    int   i;

    dflush(df);
    rsize = dBufSize / df->rec_len;
    if( rsize == 0 ) {
	df->error = 1013;
	return( 1 );
    }

    p1 = (char *)realloc( df->buf_sink, rsize * df->rec_len );
    if( p1 == NULL )    return( 2 );
/*ELSE
*/  df->buf_sink = (unsigned char *)p1;
    p2 = realloc( df->buf_pointer, rsize*sizeof( dBUFPOINTER ) );
    if( p2 == NULL ) {
	df->buf_sink = (unsigned char *)realloc(p1, df->buf_rsize*df->rec_len);
	p2 = realloc(df->buf_pointer, df->buf_rsize*sizeof( dBUFPOINTER ) );
	if( p2 == NULL || df->buf_sink == NULL )    return( -1 );
		    /* FATAL ERROR: old memory is missed */
	return( 3 );
    }
    df->buf_pointer = (dBUFPOINTER *)p2;
    df->buf_bsize = rsize * df->rec_len;
    if( df->buf_methd == 'S' ) {
	if( rsize < df->buf_rsize ) {
		df->rec_end = (df->rec_end + rsize) - df->buf_rsize;
	}
	df->buf_rsize = rsize;
	return( 0 );
    }
/*ELSE df->buf_methd == 'P'
 */
    for( i=df->buf_rsize; i<rsize; i++ )      df->buf_pointer[i].times = 0;
    df->buf_rsize = rsize;
    return( 0 );
}

/*
----------------------------------------------------------------------------
!!                        dopen()
!! add:
!! sql:(sqlname) select * from table where 1=1
----------------------------------------------------------------------------*/
dFILE *dopen( char *filename, short oflag, short shflag, short pmode )
{
    dFILE *df;
    unsigned char buf[32];

    if( strnicmp(filename, "SQL:", 4) == 0 ) {
	if( filename[4] == '{' ) {
		short i;
		char *s = &filename[5];
		for(i = 0;  i < 31 && s[i] != '\0' && s[i] != '}';  i++)
			buf[i] = s[i];

		if( s[i] != '}' )
			return  NULL;

		buf[i++] = '\0';
		return TsqlOpen(buf, s+i);
	} else {
		return TsqlOpen("TS_SQL", &filename[4]);
	}
    }
    if( strnicmp(filename, "ODBC:", 5) == 0 ) {
	if( filename[5] == '{' ) {
		short i;
		char *s = &filename[6];
		for(i = 0;  i < 31 && s[i] != '\0' && s[i] != '}';  i++)
			buf[i] = s[i];

		if( s[i] != '}' )
			return  NULL;

		buf[i++] = '\0';
		return odbcOpen(buf, s+i);
	} else {
		return odbcOpen("TS_ODBC", &filename[5]);
	}
    }

    if( (df = dInitDF( filename, oflag, shflag, pmode )) == NULL ) {
	return( NULL );
    }

    if( read(df->fp, buf, 32) != 32 ) {
	return  NULL;
    }
#ifndef WSToMT
    if( strncmp((char *)buf, (char *)CryptStr, 16) == 0 ) {
	memcpy(buf, buf+16, 16);
	df->pmode = '1';
    } else
#endif
        df->pmode = '0';

    if( strncmp( (char *)buf, DBFVIEWFLAG, sizeof(DBFVIEWFLAG)-sizeof("") ) == 0 ) {

	// it is a view we want to open
	char SourceDBFName[NAMELEN], ViewDBFName[NAMELEN];
	dVIEW *dv;

	ReadViewFileName(df->fp, SourceDBFName, ViewDBFName);
	if( *SourceDBFName == '\0' || *ViewDBFName == '\0' ) {
	    return NULL;
	}
	makefilename( SourceDBFName, df->name, SourceDBFName );
	makefilename( ViewDBFName, df->name, ViewDBFName );

	dv = (dVIEW *)zeroMalloc( sizeof( dVIEW ) );
	if( dv == NULL ) {
		return ( NULL );
	}

	dv->szfield = ReadViewFieldName( df->fp, dv );
/*      if( (dv->szfield = ReadViewFieldName( df->fp, dv ) ) == NULL ) {
		free( dv );
		return  NULL;
	}
*/
	// researved the view name
	strZcpy(dv->name, df->name, NAMELEN);

	close( df->fp );
	free_dFILE( df );

#ifdef WSToMT
	if( dIsAwake( SourceDBFName ) ) {
	    df = dAwake(SourceDBFName, oflag, shflag, pmode);
	    //dIncreaseCount(df);

	    memcpy(&(dv->source), df, sizeof(dFILE));
	    dv->awakedSource = df;

	    dv->source.op_flag = VIEWFLAG;

	    //clear the name of virtual source
	    dv->source.name[0] = '\0';

	    //free( df );
	} else
#endif
	{
	    df = dInitDF(SourceDBFName, oflag, shflag, pmode);
	    if( df == NULL ) {
		free( dv );
		return  NULL;
	    }

	    if( ReadDbfHead( df ) == NULL ) {
		return  NULL;
	    }

	    // make view flag
	    df->op_flag = VIEWFLAG;
	    memcpy(&(dv->source), df, sizeof(dFILE));

	    dv->awakedSource = NULL;

	    free( df );
	}

	if( stricmp( ViewDBFName, "NULL" ) != 0 ) {

#ifdef WSToMT
	    if( dIsAwake( ViewDBFName ) ) {
		dv->view = dAwake(ViewDBFName, oflag, shflag, pmode);
		//dIncreaseCount(dv->view);

		dv->awakedView = dv->view;

	    } else
#endif
	    {
		if( (dv->view = dInitDF( ViewDBFName, oflag, shflag, pmode )) ==\
								    NULL ) {
			free( dv->szfield );
			free( dv->ViewField );
			if( dv->szASQL )
			    free( dv->szASQL );
			DestroydFILE( &(dv->source) );
			free( dv );
			return( NULL );
		}
		if( (dv->view = ReadDbfHead( dv->view )) == NULL ) {
			free( dv->szfield );
			free( dv->ViewField );
			if( dv->szASQL )
			    free( dv->szASQL );
			DestroydFILE( &(dv->source) );
			free( dv );
			return( NULL );
		}
		dv->awakedView = NULL;
	    }

	    if( (dv->RecnoFldid = GetFldid(dv->view, viewField[0].field)) \
								== 0xFFFF ) {
			free( dv->szfield );
			free( dv->ViewField );
			if( dv->szASQL )
			    free( dv->szASQL );
			DestroydFILE( &(dv->source) );
			free( dv );
			return( NULL );
	    }

	    if( (dv->rec_buf = malloc( dv->view->rec_len )) == NULL ) {
			free( dv->szfield );
			free( dv->ViewField );
			if( dv->szASQL )
			    free( dv->szASQL );
			DestroydFILE( &(dv->source) );
			free( dv );
			return( NULL );
	    }

	    //bind the dfield
	    bindfield( dv );

	    if( dv->szfield != NULL ) {

		char **ss = dv->szfield;
//		short  i;

		if( field_ident( (dFILE *)dv, dv->szfield ) == NULL ) {
			free( dv->szfield );
			free( dv->ViewField );
			if( dv->szASQL )
			    free( dv->szASQL );
			DestroydFILE( &(dv->source) );
			free( dv );
			return  NULL;
		}
		/*
		for( i = 0;  ss[i] != NULL;   i++);
		if( i > 0 )
		{ //just really select field is OK
			dv->source.field_num = i;
		} */
	    }
	    dv->view->op_flag = TABLEFLAG;
	} else {
	    dv->view = NULL;
	}

	// add 1995.05.23 1.00am
	// remark it. 1997.1.7 Xilong
	if( dAbsDbf != 0 ) {
	    dSyncView( dv );
	}

	dseek( (dFILE *)dv, 0, dSEEK_SET );
	return  (dFILE *)dv;
    } else {
       if( lseek( df->fp, 0, SEEK_SET ) != 0 ) {
	   DestroydFILE( df );
	   free( df );
	   return  NULL;
       }

       df->op_flag = TABLEFLAG;
       df = ReadDbfHead( df );
    }

    return  df;

} /* end of function dopen() */


/*
-------------------------------------------------------------------------
!!                      dAwake()
--------------------------------------------------------------------------*/
dFILE *dAwake( char *FileName, short oflag, short shflag, short pmode )
{
    int   i;
    char  temp[NAMELEN];
    char  szDataBase[32];
    char  *sz;
    dFILE *df, *df1;
    int   sqlFlag = 0;
    int   icmp1, icmp2;
    unsigned char buf[32];


    if( strnicmp(FileName, "SQL:", 4) == 0 ) {
	if( FileName[4] == '{' ) {
		short i;
		char *s = &FileName[5];
		for(i = 0;  i < 31 && s[i] != '\0' && s[i] != '}';  i++)
			buf[i] = s[i];

		if( s[i] != '}' )
			return  NULL;

		buf[i++] = '\0';
		strZcpy(temp, buf, NAMELEN);
	} else {
		strZcpy(temp, "SQL", NAMELEN);
	}
	sqlFlag = 1;
    } else if( strnicmp(FileName, "ODBC:", 5) == 0 ) {
	if( FileName[5] == '{' ) {
		short i;
		char *s = &FileName[6];
		for(i = 0;  i < 31 && s[i] != '\0' && s[i] != '}';  i++)
			buf[i] = s[i];

		if( s[i] != '}' )
			return  NULL;

		buf[i++] = '\0';
		strZcpy(temp, buf, NAMELEN);
	} else {
		strZcpy(temp, "ODBC", NAMELEN);
	}
	sqlFlag = 1;
    } else {
	sz = strchr(FileName, '*');
	if( sz == NULL ) {
	    strcpy(szDataBase, "DBROOT");
	    strZcpy(temp, FileName, NAMELEN);
	} else {
	    *sz = '\0';
	    strZcpy(szDataBase, FileName, 32);
	    strZcpy(temp, sz+1, NAMELEN);
	}
	MakeName(temp, "DBF", NAMELEN);
    }

#ifdef WSToMT
    EnterCriticalSection( &dofnCE );
#endif
    if( sqlFlag ) 
    {
	i = _DioOpenFileNum_;
	goto  ODBC_SQL_TABLE_JMP;
    }

    for(i = 0;  i < _DioOpenFileNum_;  i++ ) {

	/*
	if( (//_DioOpenFile_[i].op_flag == TABLEFLAG &&  \
	     stricmp(_DioOpenFile_[i].df->name, temp) == 0 ) ||
	     (_DioOpenFile_[i].op_flag == VIEWFLAG && \
	     stricmp(((dVIEW *)_DioOpenFile_[i].df)->name, temp) == 0 ) ) {
	*/

	icmp1 = (stricmp(_DioOpenFile_[i].df->name, temp) == 0);
	icmp2 = ((_DioOpenFile_[i].op_flag == VIEWFLAG) && \
			(stricmp(((dVIEW *)_DioOpenFile_[i].df)->name, temp)) == 0);

	if( icmp1 || icmp2 ) {

		unsigned char old_opflag;

		df = _DioOpenFile_[i].df;


#ifndef WSToMT
// add the following two lines 1994.10.9 Xilong
		_DioOpenFile_[i].sleepFlag[df->write_flag.SleepDBF]=df->op_flag;
		// reserved the old op_flag
		df->op_flag = _DioOpenFile_[i].op_flag;
#endif
		//df->write_flag.append = 0;
		df->write_flag.SleepDBF++;
#ifdef WSToMT
		/*1998.3.8
		//perhaps other thread has locked it to modify the data
		//as rec_num

		wmtDbfLock(df);
		wmtDbfUnLock(df);
		*/
		old_opflag = df->op_flag;

		//
		//to allow a view to be used as a table
		//match the _DioOpenFile_ with table type name, it is a
		//table, else it is a view
		//

		/*
		if( //_DioOpenFile_[i].op_flag == TABLEFLAG &&  \
			stricmp(_DioOpenFile_[i].df->name, temp) == 0 )
		*/

		if( icmp1 )
		{
		    df->op_flag = TABLEFLAG;
		} else {
		    df->op_flag = VIEWFLAG;
		}

		df1 = wsmDf(df, oflag, shflag, pmode);

		df->op_flag = old_opflag;

		LeaveCriticalSection( &dofnCE );
#endif
		return( df1 );
	}
    }

#ifdef OPTIMIZED_FILE_MAN
    if( i >= _DioFileHandleNum_ )
    { //release one
	for(i = 0;  i < _DioOpenFileNum_;  i++ ) {
	    if( _DioOpenFile_[i].df->write_flag.SleepDBF < 1 ) {
		dRelease( _DioOpenFile_[i].df );
		break;
	    }
	}
    }
#endif

    //direct jump ------------------------------<<<<<<<<<<<<<<<<<<<<<<
ODBC_SQL_TABLE_JMP:

    if( i < _DioFileHandleNum_ ) {
	if( sqlFlag )
	{
	    if( ( df = dopen( FileName, oflag, shflag, pmode ) ) == NULL ) {
#ifdef WSToMT
		LeaveCriticalSection( &dofnCE );
#endif
		return( NULL );
	    }

#ifdef OPTIMIZED_FILE_MAN
	    _DioOpenFile_[_DioOpenFileNum_].FileManOpable = 0;
#endif

	} else {
#ifdef WSToMT
	    char nameBuf[256];
	    char *s;
	    char szTid[256];
	    int  icheck_cond;

	    if( ( df = dopen(temp, (short)(O_RDWR|O_BINARY), \
		   (short)SH_DENYNO, (short)(S_IREAD|S_IWRITE)) ) == NULL ) {
		LeaveCriticalSection( &dofnCE );
		return( NULL );
	    }

	    strcpy(nameBuf, TrimFileName(df->name));

	    s = GetCfgKey(csuDataDictionary, szDataBase, nameBuf, "TID");
	    if( s != NULL ) {
		df->tbid = atoi(s);
		strZcpy(szTid, s, 32);
	    } else {
		df->tbid = 0;

		//default value is 0, needn't set it
		//df->wlog = 0;
		//df->dpack_auto = 0;

		//2000.2.12 XilongPei
		//df->mainkey_num = 1;

		df->mainkey_num = 0;
		df->keep_physical_order = 1;

		goto DIO_NO_DICT;

		//strZcpy(szTid, nameBuf, 256);
	    }

	    s = GetCfgKey(csuDataDictionary, szDataBase, nameBuf, "WLOG");
	    if( s != NULL && rjFile != NULL ) {
		df->wlog = atoi(s);
	    } else {
		df->wlog = 0;
	    }

	    //2000.2.12 XilongPei
	    df->mainkey_num = 0;
//	    s = GetCfgKey(csuDataDictionary, szDataBase, nameBuf, "MAINKEY_NUM");
	    s = GetCfgKey(csuDataDictionary, szDataBase, nameBuf, "MAINKEY_DEF");
	    if( s != NULL ) {
		char 		*sp;
		unsigned short   w;

		sp = strtok(s, ",+;");
		if( sp != NULL ) {
		    w = GetFldid(df, sp);
		    if( w == 0xFFFF ) {
			dclose( df );
			dERROR = 10001;
			return  NULL;
		    }

		    df->mainkey_fields[df->mainkey_num++] = w;
		}

		sp = strtok(NULL, ",+;");
		while( sp != NULL && df->mainkey_num <= dioMAX_KEYFIELD_NUM )
		{
		    w = GetFldid(df, sp);
		    if( w == 0xFFFF ) {
			dclose( df );
			dERROR = 10001;
			return  NULL;
		    }

		    df->mainkey_fields[df->mainkey_num++] = w;
		    sp = strtok(NULL, ",+;");
		}

	    } //end of if

	    s = GetCfgKey(csuDataDictionary, szDataBase, nameBuf, "LOCK_NUM");
	    if( s != NULL ) {
		df->lock_num = atoi(s);

		if( df->lock_num < 1 )
		{
			df->lock_num = TOTAL_LOCKS_PER_DF;
		}
	    }

	    s = GetCfgKey(csuDataDictionary, szDataBase, nameBuf, "KEEP_PHYSICAL_ORDER");
	    if( s != NULL ) {
		df->keep_physical_order = atoi(s);
	    } else {
		df->keep_physical_order = 1;
	    }

	    s = GetCfgKey(csuDataDictionary, szDataBase, nameBuf, "DPACK_AUTO");
	    if( s != NULL ) {
		df->dpack_auto = atoi(s);
	    } else {
		df->dpack_auto = 0;
	    }

	    s = GetCfgKey(csuDataDictionary, szDataBase, nameBuf, "CHECK_VALID");
	    if( s != NULL ) {
		icheck_cond = atoi(s);
	    } else {
		icheck_cond = 0;
	    }

	    if( icheck_cond )
	    { ///////////// field check_cond
	      //isRecCheck and df->field[k].px has their default value 0
	      //
		int k;
		for(k = 0;  k < df->field_num;  k++) {
		    s = GetCfgKey(csuDataDictionary, szTid, df->field[k].field, "CHECK_COND");
		    if( s != NULL ) {

			if( *s == '\0' )
			    continue;

			df->isRecCheck = 1;

			df->field[k].px = strdup(s);
			/*
			df->field[k].px = WordAnalyse(s);
			if( SymbolRegister( (MidCodeType *)df->field[k].px, \
					    df, \
					    NULL, \
					    0, \
					    NULL, \
					    0) != NULL ) {
				df->field[k].px = NULL;
			}
			*/
		    }
		    /* needn't to do this
		    ** for the df is malloced by zeroMalloc()
		    **
		    ** else
		    **     df->field[k].px = NULL;
		    */
		}
	    } else {
		df->isRecCheck = 0;
	    }

	    if( pmode != -1 ) {
		int   k, kc;
		char  buf[256];
		char  nameBuf2[64];

		for(k = 0, kc = 0;   k < DIO_MAX_SYNC_BTREE;   k++) {
		    sprintf(buf, "NDX%d", k+1);

		    s = GetCfgKey(csuDataDictionary, szDataBase, nameBuf, buf);
		    if( s == NULL )
			break;

		    if( *s == '\0' )
		    { //this dictionary item is blank
		      //so we use kc to calculate the ndx num
			continue;
		    }

		    makefilename(buf, df->name, s);
		    df->bhs[kc] = IndexAwake((char *)df, buf, BTREE_FOR_OPENDBF);
		    if( df->bhs[kc] == NULL ) {
			sprintf(nameBuf2, "KEY%d", k+1);
			s = GetCfgKey(csuDataDictionary, szDataBase, nameBuf, nameBuf2);
			if( s == NULL )
				break;
			if( *s == '\0' )
				continue;

			df->bhs[kc] = IndexBAwake((char *)df, s, buf, BTREE_FOR_OPENDBF);
			//index open error, set error?
			if( df->bhs[kc] == NULL )
				break;
		    }

		    //1999.11.20
		    //let the index use for check only, DONNOT
		    //affecting the table operation
		    //((bHEAD *)df->bhs[kc])->type = BTREE_FOR_ITSELF;
		    //ERROR for we depend on the FOR_OPENDBF to update
		    //the index

		    kc++;
		}
		df->syncBhNum = kc;
	    }

DIO_NO_DICT:

#ifdef OPTIMIZED_FILE_MAN
	    if( df->tbid > 0 )
	    { //maned by my dictionary
	      //can be optimized
		s = GetCfgKey(csuDataDictionary, szDataBase, nameBuf, "SHAREABLE");
		if( s != NULL )
		    _DioOpenFile_[_DioOpenFileNum_].FileManOpable = (atoi(s)?0:1);
		else
		    _DioOpenFile_[_DioOpenFileNum_].FileManOpable = 0;
	    } else {
		_DioOpenFile_[_DioOpenFileNum_].FileManOpable = 0;
	    }
#endif
	    //null statement
	    ;

#else
	    if( ( df = dopen( temp, oflag, shflag, pmode ) ) == NULL ) {
		return  NULL;
	    }
#endif
	}
	if( df->op_flag == VIEWFLAG ) {
//1996.7.1      dSetAwake( (dFILE *)&( ((dVIEW *)df) ->source) );
		if( dSetAwake( ((dVIEW *)df)->view, &(((dVIEW *)df)->view) ) != 1 ) {
		    dclose(df);
		    return  NULL;
		}
//              return  df;
	}

#ifndef WSToMT
	_DioOpenFile_[_DioOpenFileNum_].sleepFlag[0] = df->op_flag;
#endif

	df->write_flag.SleepDBF = 1;
	_DioOpenFile_[_DioOpenFileNum_].op_flag = df->op_flag;
	/*_DioOpenFileNum_++;                     */
	_DioOpenFile_[_DioOpenFileNum_++].df = df;

#ifdef WSToMT
	df = wsmDf(df, oflag, shflag, pmode);
	LeaveCriticalSection( &dofnCE );
#endif
	if( df->op_flag == VIEWFLAG ) {
	    calASQLforView(szDataBase, (dVIEW *)df );
	}

	return( df );
    }

    dERROR = 10001;
#ifdef WSToMT
    LeaveCriticalSection( &dofnCE );
#endif
    return( NULL );

} /* end of function dAwake() */


/*
-------------------------------------------------------------------------
!!                      dSetAwake()
!! just for table not for view
!! when the df has already open, return the old dFILE pointer
--------------------------------------------------------------------------*/
short dSetAwake(dFILE *df, dFILE **tdf)
{
    short i;
    char  temp[NAMELEN];

    if( df == NULL )    return  -1;

    strncpy(temp, df->name, NAMELEN);
#ifdef WSToMT
    EnterCriticalSection( &dofnCE );
#endif
    for(i = 0;  i < _DioOpenFileNum_;  i++ ) {

	if( (/*_DioOpenFile_[i].op_flag == TABLEFLAG && */\
	     stricmp(_DioOpenFile_[i].df->name, temp) == 0 ) ||
	     (_DioOpenFile_[i].op_flag == VIEWFLAG && \
	     stricmp(((dVIEW *)_DioOpenFile_[i].df)->name, temp) == 0 ) ) {

	/*if( (_DioOpenFile_[i].op_flag == TABLEFLAG && \
	     stricmp(_DioOpenFile_[i].df->name, temp) == 0 ) ||
	     (_DioOpenFile_[i].op_flag == VIEWFLAG && \
	     stricmp(((dVIEW *)_DioOpenFile_[i].df)->name, temp) == 0 ) ) */

/*1998.7.24,  already open, cannot be dSetAwake()
		df = _DioOpenFile_[i].df;
#ifndef WSToMT
// add the following two lines 1994.10.9 Xilong
		_DioOpenFile_[i].sleepFlag[df->write_flag.SleepDBF]=df->op_flag;
		// reserved the old op_flag
		df->op_flag = _DioOpenFile_[i].op_flag;
#else
		LeaveCriticalSection( &dofnCE );
		*tdf = wsmDf(df, df->oflag, df->shflag, df->pmode);
#endif
		return  ++(df->write_flag.SleepDBF);
*/
		if( _DioOpenFile_[i].df->write_flag.SleepDBF == 0 ) {

		    //
		    //release the old one
		    //
		    dclose( _DioOpenFile_[i].df );

#ifdef OPTIMIZED_FILE_MAN
		    if( *TrimFileName( temp ) == '#' )
			_DioOpenFile_[i].FileManOpable = 1;
		    else
			_DioOpenFile_[i].FileManOpable = 0;
#endif

		    _DioOpenFile_[i].df = df;
		    _DioOpenFile_[i].df->write_flag.SleepDBF = 1;
		    /*i++;                     */
		    _DioOpenFile_[i].op_flag = df->op_flag;
#ifdef WSToMT
		    *tdf = wsmDf(df, df->oflag, df->shflag, df->pmode);
		    LeaveCriticalSection( &dofnCE );
#endif
		    return  1;
		}
		return  2;
	}
    }

    if( i < _DioFileHandleNum_ ) {

#ifdef OPTIMIZED_FILE_MAN
	if( *TrimFileName( temp ) == '#' ) {
	    _DioOpenFile_[_DioOpenFileNum_].FileManOpable = 1;
	    tempTableList[tempTableNum++] = df;
	} else
	    _DioOpenFile_[_DioOpenFileNum_].FileManOpable = 0;
#endif

	_DioOpenFile_[_DioOpenFileNum_].df = df;
	_DioOpenFile_[_DioOpenFileNum_].df->write_flag.SleepDBF = 1;
	/*_DioOpenFileNum_++;                     */
	_DioOpenFile_[_DioOpenFileNum_++].op_flag = df->op_flag;
#ifdef WSToMT
	*tdf = wsmDf(df, df->oflag, df->shflag, df->pmode);
	LeaveCriticalSection( &dofnCE );
#endif
	return  1;
    }

    dERROR = 10001;
#ifdef WSToMT
    LeaveCriticalSection( &dofnCE );
#endif
    return  0;

} /* end of function dSetAwake() */


/*
-------------------------------------------------------------------------
!!                      dcreate()
--------------------------------------------------------------------------*/
dFILE *dcreate( char *filename, dFIELD field[] )
{
    dFILE *df;
    char *s, cs;
    unsigned char buf[512];
    int  i, j, k;

    if( filename == NULL ) {            /* reserved for TV's create */
	df = (dFILE *)field;
	goto DioCreatePart_JMP;
    }

    if ( ( df = alloc_dFILE() ) == NULL )
	return( NULL );

    df->write_flag.creat = 1;
    df->op_flag = TABLEFLAG;

    if( strlen( filename ) > NAMELEN -1 ) { /* '-1' FOR HOLD '\0'*/
	df->error = 1012;  /* name too long */
	free_dFILE ( df );
	return( NULL );
    }

    strcpy( df->name, filename );
    MK_name( df->name );

    for(i = 0; field[i].field[0] != '\0'; i++ );
    df->field_num = i;

    df->field = (dFIELD *)zeroMalloc(i * sizeof(dFIELD));
    df->fld_id = (short *)calloc( i, sizeof( short ) );
    if( df->field == NULL || df->fld_id == NULL ) {
	df->error  = 1001; /* ENOMEM */
	free_dFILE( df );
	return( NULL );
    }

    //df->pmode = '0';
    df->dbf_flag = 3;
    df->del_link_id = 0xFFFF;
    df->fldTimeStampId = 0xFFFF;
    for( j = 1, i = 0, k = 0;  i < df->field_num; i++ ) {

	//shrink the VIEW_RECNO_FIELD
	if( stricmp(field[i].field, VIEW_RECNO_FIELD) == 0 && i > 0 )
		continue;

	if( stricmp(field[i].field, "DEL___LINK") == 0 ) {
		df->del_link_id = i;
	}

	if( stricmp(field[i].field, "TIME_STAMP") == 0 ) {
		df->fldTimeStampId = i;
		df->fldTimeStampOffset = j;
		df->fldTimeStampLen = field[i].fieldlen;
	}

	memcpy(df->field[k].field, stoUpper(field[i].field), FIELDNAMELEN );

	/*in database system, the field can't be appeared more than once
	  but in TreeSVR, this is allowed? 1999.2.25

	{ //to check wether the field has already exist
	  //
	    int n;
	    char *sz = field[i].field;
	    char *sz1;

	    for( n = 0;  n < k;  n++ ) {
		sz1 = df->field[n].field;

		if( *sz != *sz1 )
			continue;

		if( strcmp(sz1, sz) == 0 ) {
			df->error  = 1001;
			free_dFILE( df );
			return( NULL );
		}
	    }
	} //end of check
	*/

	if( ( df->field[k].fieldtype = toupper(field[i].fieldtype) ) == 'M' )
	{
	    df->dbf_flag = 0x83;
	    field[i].fieldlen = 10;
	    field[i].fielddec = 0;
	}
	if( df->field[k].fieldtype == 'G' ) {
	    df->dbf_flag = 0x83;
	    field[i].fieldlen = 10;
	    field[i].fielddec = 0;
	}

	if( df->field[k].fieldtype == 'D' ) {
	    field[i].fieldlen = 8;
	    field[i].fielddec = 0;
	}

	df->field[k].fieldlen = field[i].fieldlen;
	df->field[k].fielddec = field[i].fielddec;
	df->field[k].fieldstart = (unsigned char *)j;
	df->fld_id[k] = i;
	j += field[i].fieldlen;

	k++;
    }
    df->rec_len = j;
    df->headlen = ( df-> field_num + 1 ) * 32 + 1;

    df->rec_buf = (unsigned char *)zeroMalloc( 2*(df->rec_len+1) );
    if( df->rec_buf == NULL ) {
	df->error = 1001;
	free_dFILE( df );
	return( NULL );
    }
    df->rec_buf[ df->rec_len ] = '\0';

    //1997.12.18
    df->rec_tmp = &(df->rec_buf[df->rec_len + 1]);

    for( i = 0;  i < df->field_num; i++ )
	 df->field[i].fieldstart = (unsigned short)df->field[i].fieldstart + \
								df->rec_buf;

    if( try_alloc( df ) != dOK ) {
	free_dFILE( df );
	return( NULL );
    }

    {
      struct tm *ntime;
      time_t ltime;

      time ( &ltime );
      ntime = gmtime ( &ltime );
      df->last_modi_time[0] = (unsigned char)ntime->tm_year;
      df->last_modi_time[1] = (unsigned char)ntime->tm_mon + 1;
      df->last_modi_time[2] = (unsigned char)ntime->tm_mday;

      //1997.12.19 for compitable with delphi desktop
      if( df->del_link_id != 0xFFFF )
	  df->firstDel = LONG_MAX;
      else
	  df->firstDel = 0;
    }

DioCreatePart_JMP:

    df->fp = open(df->name, O_RDWR|O_CREAT|O_TRUNC|O_BINARY|SH_DENYWR, S_IWRITE);
    if( df->fp < 0 ) {
	dERROR = errno;
//      df->error = 1003; /* file not found */
	free_dFILE ( df );
	return ( NULL );
    }

    df->oflag = (short)(O_RDWR|O_CREAT|O_TRUNC|O_BINARY);
    df->pmode = (short)S_IWRITE;

    memset( buf, '\0', 32 );
    buf[0] = df->dbf_flag;
    buf[1] = df->last_modi_time[0];
    buf[2] = df->last_modi_time[1];
    buf[3] = df->last_modi_time[2];

    *(long *)&buf[4] = df->rec_num;
    *(unsigned short *)&buf[10] = df->rec_len;
    *(unsigned short *)&buf[8] = (unsigned short)df->headlen;

    //Add 1997.11.13
    *(long *)&buf[12] = df->firstDel;     /* buf[12] ~ buf[15]   */

    write( df->fp, buf, 32 );

    for( i = 0; i < df->field_num; i++ ){
	memset( buf, 0, 32 );

	strncpy((char *)buf, (char *)df->field[i].field, 10);
        strncpy(&buf[18], (char *)&(df->field[i].field[10]), 14);

	buf[11] = df->field[i].fieldtype;

	if( buf[11] == 'M' || buf[11] == 'G' || \
					buf[11] == 'O' || buf[11] == 'B' ) {
		buf[15] = buf[11];
		buf[11] = 'M';
	}

	buf[16] = (unsigned char)df->field[i].fieldlen;
	if ( df->field[i].fieldtype == 'N' )
	    buf[17] = (unsigned char) df->field[i].fielddec;
	write(df->fp, buf, 32);
    }

    buf[0] = 0x0D;
    write ( df->fp, buf, 1 );

    if( ( df->dbf_flag & 0x83 ) == 0x83 ) { /**/
	s = strrchr( df->name, '.' );
	cs = s[3];
	s[3] = 'T';                   /* make the name to be DBT from DBF */

#ifdef DBT_IS_GDC

	if( dbTreeBuild(df->name, DBT_GDC_KEY_LEN, DBTBLOCKSIZE) != 0 )
	{
		dERROR = 4001; /* file not found */
		free_dFILE( df );
		return( NULL );
	}

	if( ( df->dp = (long)dbTreeOpen( df->name ) ) == 0 ) {
		dERROR = 4001; /* file not found */
		free_dFILE( df );
		return( NULL );
	}

	df->cdbtIsGDC = 1;

#else
	df->dp = open( df->name, O_RDWR|O_CREAT|O_TRUNC|O_BINARY|SH_DENYWR, S_IWRITE);
	if( df->dp < 0 ) {
		dERROR = 4001; /* file not found */
		free_dFILE( df );
		return( NULL );
	}
	memset(buf, '\0', 512);
	buf[0] = 1;
	df->dbt_head[0] = 1;
	df->write_flag.dbt_write = 1;
	write(df->dp, buf, 512);
#endif
	s[3] = cs;
    } else {
	df->write_flag.dbt_write = 0;
	df->dbt_buf = NULL;
    }

    if( filename != NULL ) {
	df->rec_num = 0;
	df->rec_no = df->rec_p = df->rec_end = 1;
    }
    df->rec_beg = 1;
    df->buf_freshtimes = 1;
    df->write_flag.buf_write = 0;
    df->write_flag.file_write = df->write_flag.append = 1;
    df->buf_methd = 'S';
    df->buf_usetimes = IniBufUseTimes;

    df->slowPutRec = ((df->fldTimeStampId != 0xFFFF) || (df->dbf_flag & 0x83) == 0x83);

#ifdef WSToMT
    InitializeCriticalSection( &(df->dCriticalSection) );
    df->inCriticalSection = '\0';
#endif

    return ( df );

}/* end of function dcreat() */

/*
---------------------------------------------------------------------------
!                       dfcopy()
! Sucess return tdFIELD pointer
--------------------------------------------------------------------------*/
dFIELD *dfcopy( dFILE *sdFILE, dFIELD *tdFIELD )
{

    unsigned short i, k;

    if( sdFILE == NULL )
	return  NULL;

    if( tdFIELD == NULL )
	tdFIELD = (dFIELD *)calloc( sdFILE->field_num+1, sizeof( dFIELD ) );
    if( tdFIELD == NULL ) {
	dERROR = 1001; /* ENOMEM */
	return ( NULL );
    }

    k = 0;
    for ( i = 0; i < sdFILE->field_num; i++ ) {
	if( stricmp((char *)sdFILE->field[sdFILE->fld_id[i]].field, viewField[0].field) == 0 )
		continue;
	if( sdFILE->field[sdFILE->fld_id[i]].sdword < 0 )
		continue;

	memcpy(&(tdFIELD[k]), &(sdFILE->field[sdFILE->fld_id[i]]), sizeof(dFIELD));
	/*strcpy((char *)tdFIELD[k].field, (char *)sdFILE->field[i].field);
	tdFIELD[k].fieldtype = sdFILE->field[i].fieldtype;
	tdFIELD[k].fieldlen = sdFILE->field[i].fieldlen;
	tdFIELD[k].fielddec = sdFILE->field[i].fielddec;*/
	k++;
    }

    tdFIELD[k].field[0] = '\0';

    return( tdFIELD );

} /* Function End */


/*
---------------------------------------------------------------------------
!                       dflush()
---------------------------------------------------------------------------*/
short dflush( dFILE *df )
{
    unsigned char   buf[32];
    long            beg, end;
    int  	    i, rsize;
    short           k;

    if( df == NULL )         return  1;

    if( df->dbf_flag == SQL_TABLE || df->dbf_flag == ODBC_TABLE )
	return  2;

#ifdef WSToMT
    if( df->pdf != NULL ) {
	df = df->pdf;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( df->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    df = df->pdf;
	}

    }
    EnterCriticalSection( &(df->dCriticalSection) );
#endif

    beg = df->rec_beg;
    end = df->rec_end;
    rsize = df->buf_rsize;

    if( df->write_flag.file_write == 0 ) {
#ifdef WSToMT
       LeaveCriticalSection( &(df->dCriticalSection) );
#endif
       return( 0 );
    }

    if( df->buf_methd == 'S' ) {

	  if( df->write_flag.buf_write == 1 )
	  { //2000.3.21 Xilong Pei

	     long  ll;

	     df->write_flag.buf_write = 0;

	     ll = df->headlen + ( beg - 1 ) * df->rec_len;
	     if( lseek( df->fp, ll, SEEK_SET ) != ll ) {
#ifdef WSToMT
		LeaveCriticalSection( &(df->dCriticalSection) );
#endif
		return( 1 );
	     }

	     if( ( dERROR = write( df->fp, df->buf_sink, \
			(unsigned short)( end - beg + 1) * df->rec_len ) ) == -1 )
	     {
#ifdef WSToMT
		LeaveCriticalSection( &(df->dCriticalSection) );
#endif
		return( 1 );
	     }
	  }
    } else {
/* buf_methd='P' */
	     for( i = 0; i < rsize; i++ )
		if( df->buf_pointer[i].buf_write == 1 ) {
		    long ll;

		    ll = df->headlen + (df->buf_pointer[i].rec_num\
					- 1 ) * df->rec_len;
		    if( lseek( df->fp, ll, SEEK_SET ) != ll ) {
#ifdef WSToMT
			LeaveCriticalSection( &(df->dCriticalSection) );
#endif
			return( -1 );
		    }

		    dERROR = write( df->fp, &(df->buf_sink[ i * \
					df->rec_len ]), df->rec_len );
		    if( dERROR == -1 ) {
#ifdef WSToMT
			LeaveCriticalSection( &(df->dCriticalSection) );
#endif
			return( dERROR );
		    }
		    df->buf_pointer[i].buf_write = 0;
		}
    }

    if( df->write_flag.file_write == 1 ) {
	if( lseek( df->fp, 0, SEEK_SET ) != 0 ) {
#ifdef WSToMT
	    LeaveCriticalSection( &(df->dCriticalSection) );
#endif
	    return( 1 );
	}

	read( df->fp, buf, 32 );

	{
	   struct tm *ntime;
	   time_t ltime;

	   time ( &ltime );
	   ntime = gmtime ( &ltime );
	   buf[1] = (unsigned char)ntime->tm_year;
	   buf[2] = (unsigned char)ntime->tm_mon + 1;
	   buf[3] = (unsigned char)ntime->tm_mday;
	}

	*(long *)&buf[4] = df->rec_num;

	if( lseek( df->fp, 0, SEEK_SET ) != 0 ) {
#ifdef WSToMT
	    LeaveCriticalSection( &(df->dCriticalSection) );
#endif
	    return( 1 );
	}

	dERROR = write( df->fp, buf, 32 );

	if( df->write_flag.dbt_write ) {
	    if( df->cdbtIsGDC ) {
		dbTreeFlush( (bHEAD *)(df->dp) );
	    } else {
		*(long *)buf = df->dbt_head[0];
		if( lseek( df->dp, 0, SEEK_SET) != 0 ) {
#ifdef WSToMT
			LeaveCriticalSection( &(df->dCriticalSection) );
#endif
			return( 1 );
		}
		dERROR = write( df->dp, buf, 4);
	    }
	}
    }

    df->write_flag.file_write = 0;
#ifdef WSToMT
    for(k = 0;  k < df->syncBhNum;  k++) {
	IndexFlush( (bHEAD *)(df->bhs[k]) );
    }

    LeaveCriticalSection( &(df->dCriticalSection) );
#endif
    return( 0 );

} /* end of function dflush() */


/*
---------------------------------------------------------------------------
!                       drflush()
---------------------------------------------------------------------------*/
short drflush( dFILE *df )
{
    long beg, end;
    unsigned short  i, rsize;
    char buf[36];

    if( df == NULL )         return  1;
    if( df->dbf_flag == SQL_TABLE || df->dbf_flag == ODBC_TABLE )
	return  2;

#ifdef WSToMT
    if( df->pdf != NULL ) {
	df = df->pdf;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( df->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    df = df->pdf;
	}

    }
    EnterCriticalSection( &(df->dCriticalSection) );
#endif

    beg = df->rec_beg;
    end = df->rec_end;
    rsize = df->buf_rsize;

    if( lseek( df->fp, 0, SEEK_SET ) != 0 ) {
	return  1;
    }
    read( df->fp, buf, 32 );
#ifndef WSToMT
    if( strncmp((char *)buf, (char *)CryptStr, 16) == 0 ) {
	memcpy(buf, buf+16, 16);
	df->pmode = '1';
    } else
#endif
	df->pmode = '0';

    df->last_modi_time[0] = buf[1];
    df->last_modi_time[1] = buf[2];
    df->last_modi_time[2] = buf[3];

    df->rec_num = *(long *)&buf[4];         /* buf[4] ~ buf[7]   */

    //set this value will speed up the dpack()
    df->firstDel = *(long *)&buf[12];     /* buf[12] ~ buf[15]   */

    if( df->buf_methd == 'S' ) {
	     long  ll;

	     ll = df->headlen + ( beg - 1 ) * df->rec_len;
	     if( lseek( df->fp, ll, SEEK_SET ) != ll ) {
#ifdef WSToMT
                LeaveCriticalSection( &(df->dCriticalSection) );
#endif
		return( 1 );
	     }

	     if( ( dERROR = read( df->fp, df->buf_sink, \
			(unsigned short)( end - beg + 1) * df->rec_len ) ) == -1 )
             {
#ifdef WSToMT
                LeaveCriticalSection( &(df->dCriticalSection) );
#endif
		return( 1 );
             }
    } else {
/* buf_methd='P' */
	     for( i = 0; i < rsize; i++ ) {
		    long  ll;

		    ll = df->headlen + (df->buf_pointer[i].rec_num\
			- 1 ) * df->rec_len;
		    if( lseek( df->fp, ll, SEEK_SET ) != ll ) {
#ifdef WSToMT
			LeaveCriticalSection( &(df->dCriticalSection) );
#endif
			return  -1;
		    }

		    dERROR = read( df->fp, &(df->buf_sink[ i * \
					df->rec_len ]), df->rec_len );
		    if( dERROR == -1 ) {
#ifdef WSToMT
			LeaveCriticalSection( &(df->dCriticalSection) );
#endif
			return( dERROR );
		    }
		}
    }

#ifdef WSToMT
    LeaveCriticalSection( &(df->dCriticalSection) );
#endif
    return( 0 );

} /* end of function drflush() */


/*
----------------------------------------------------------------------------
!!                      dclose()
----------------------------------------------------------------------------*/
short dclose( dFILE *df )
{
    unsigned char   buf[32];
    unsigned short  i;

    if( df == NULL )	return  2;
    if( df->dbf_flag == SQL_TABLE ) {
	return  TsqlClose(df);
    }
    if( df->dbf_flag == ODBC_TABLE ) {
	return  odbcClose(df);
    }


    if( df->pwLocksByThread != NULL )
    {
	free( df->pwLocksByThread );
    }

    if( df->op_flag == VIEWFLAG ) {

	dVIEW *dv = (dVIEW *)df;

#ifdef WSToMT
	if( dv->view != NULL && dv->awakedSource != NULL ) {
	    dSleep( dv->awakedSource );
	} else
#endif
	{ //close source

	  if( df->write_flag.buf_write == 1 ) {
	    if( df->buf_methd == 'S' ) {
	      long  ll;

	      ll = df->headlen + ( df->rec_beg - 1 ) * df->rec_len;
	      if( lseek( df->fp, ll, SEEK_SET ) != ll ) {
		  return  0;
	      }

	      write ( df->fp, df->buf_sink, \
			( df->rec_end - df->rec_beg + 1) * df->rec_len );
	    } else {
	      for( i = 0; i < df->buf_rsize; i++ )
		if( df->buf_pointer[i].buf_write == 1 ) {
		    long  ll;

		    ll = df->headlen + (df->buf_pointer[i].rec_num\
							- 1 ) * df->rec_len;
		    if( lseek( df->fp, ll, SEEK_SET ) != ll ) {
			return  0;
		    }

		    write(df->fp, &(df->buf_sink[ i * df->rec_len ]), \
			df->rec_len );
		}
	    }
	  }

	  i = 0;              //df->write_flag.file_write
	  if( df->write_flag.file_write == 1 ) {

	    i = 1;
	    lseek( df->fp, 0, SEEK_SET );
	    read( df->fp, buf, 32 );

#ifndef WSToMT
	    if( df->pmode == '1' ) {
		memcpy(buf, buf+16, 16);
	    }
#endif
	    {
		struct tm *ntime;
		time_t ltime;

		time ( &ltime );
		ntime = gmtime ( &ltime );
		buf[1] = (unsigned char)ntime->tm_year;
		buf[2] = (unsigned char)ntime->tm_mon + 1;
		buf[3] = (unsigned char)ntime->tm_mday;

		//Add 1997.11.13
		if( df->del_link_id != 0xFFFF )
		    *(long *)&buf[12] = df->firstDel;     /* buf[12] ~ buf[15]   */
		else
		    *(long *)&buf[12] = 0;
	    }

	    *(long *)&buf[4] = df->rec_num;

#ifndef WSToMT
	    if( df->pmode == '1' ) {
		memcpy(buf+16, buf, 16);
		memcpy(buf, CryptStr, 16);
	    }
#endif
	    lseek( df->fp, 0, SEEK_SET );
	    write( df->fp, buf, 32 );

	    if( ( df->dbf_flag & 0x83 ) == 0x83 ) {  /* write the DBT file */
	      if( df->cdbtIsGDC == 0 && df->write_flag.dbt_write ) {
		lseek( df->dp, 0, SEEK_SET );
		write( df->dp, df->dbt_head, DBTBLOCKSIZE );
		chsize( df->dp, df->dbt_head[0]*DBTBLOCKSIZE);
	      }
	    }

	    chsize( df->fp, (long)df->headlen + df->rec_num * df->rec_len );

	  }

	  close( df->fp );
	  if( ( df->dbf_flag & 0x83 ) == 0x83 ) {  /* close the DBT file */
		close( df->dp );
	  }
	}

	if( dv->view != NULL ) {
#ifdef WSToMT
	    if( dv->awakedView )
		dSleep( dv->view );
	    else
#endif
		dclose( dv->view );
	}

	if( dv->view == NULL || dv->awakedSource == NULL )
	{ //now, the mem in df->source not come from other memory 1998.8.1
		DestroydFILE( df );
	}

	if( dv->ViewField != NULL ) {
		free( dv->ViewField );
		free( dv->szfield );
	}
	
	if( dv->szASQL )
		free( dv->szASQL );

	free( dv->rec_buf );
	free( dv );
	return  0;

    } // ELSE ELSE ELSE ELSE ELSE ELSE ELSE ELSE ELSE ELSE

    //not a view
    if( df->write_flag.buf_write == 1 ) {
	if( df->buf_methd == 'S' ) {
	    long  ll;

	    ll = df->headlen + ( df->rec_beg - 1 ) * df->rec_len;
	    if( lseek ( df->fp, ll, SEEK_SET ) != ll ) {
		return  0;
	    }

	    write ( df->fp, df->buf_sink, \
			( df->rec_end - df->rec_beg + 1) * df->rec_len );
	} else {
	    for( i = 0; i < df->buf_rsize; i++ )
		if( df->buf_pointer[i].buf_write == 1 ) {
		    long  ll;

		    ll = df->headlen + (df->buf_pointer[i].rec_num\
			- 1 ) * df->rec_len;
		    if( lseek( df->fp, ll, SEEK_SET ) != ll ) {
			return  0;
		    }
		    write(df->fp, &(df->buf_sink[ i * df->rec_len ]), \
			df->rec_len );
		}
	}
    }

    i = 0;              //df->write_flag.file_write
    if( df->write_flag.file_write == 1 ) {

	i = 1;
	if( lseek( df->fp, 0, SEEK_SET ) != 0 )
	    return  0;

	read( df->fp, buf, 32 );

#ifndef WSToMT
	if( df->pmode == '1' ) {
		memcpy(buf, buf+16, 16);
	}
#endif
	{
	   struct tm *ntime;
	   time_t ltime;

	   time ( &ltime );
	   ntime = gmtime ( &ltime );
	   buf[1] = (unsigned char)ntime->tm_year;
	   buf[2] = (unsigned char)ntime->tm_mon + 1;
	   buf[3] = (unsigned char)ntime->tm_mday;

           //Add 1997.11.13
	   if( df->del_link_id != 0xFFFF )
	       *(long *)&buf[12] = df->firstDel;     /* buf[12] ~ buf[15]   */
	   else
	       *(long *)&buf[12] = 0;     	     /* buf[12] ~ buf[15]   */
	}

	*(long *)&buf[4] = df->rec_num;

#ifndef WSToMT
	if( df->pmode == '1' ) {
		memcpy(buf+16, buf, 16);
		memcpy(buf, CryptStr, 16);
	}
#endif
	if( lseek( df->fp, 0, SEEK_SET ) != 0 )
	    return  0;

	write( df->fp, buf, 32 );

	if( ( df->dbf_flag & 0x83 ) == 0x83 ) {  /* write the DBT file */
	    if( df->cdbtIsGDC == 0 && df->write_flag.dbt_write ) {
		if( lseek( df->dp, 0, SEEK_SET ) != 0 )
		    return  0;

		write( df->dp, df->dbt_head, DBTBLOCKSIZE );
		chsize( df->dp, df->dbt_head[0] * DBTBLOCKSIZE );
	    }
	}

	chsize( df->fp, (long)df->headlen + df->rec_num * df->rec_len );

    }

    close( df->fp );
    if( ( df->dbf_flag & 0x83 ) == 0x83 ) {  /* close the DBT file */
	if( df->cdbtIsGDC ) {
	    dbTreeClose( (bHEAD *)(df->dp) );
	} else {
	    close( df->dp );
	}
    }

    free_dFILE( df );

    return  i;

}/* end of function dclose() */


/*
-------------------------------------------------------------------------
			dSleep()
PURPOSE: dSleep will never close file.
-------------------------------------------------------------------------*/

__declspec(dllexport) short dSleep( dFILE *df )
{
    register short i;
#ifdef WSToMT
    dFILE *odf;
#endif

    if( df == NULL ) {
#ifdef WSToMT
#else
	// sleep all
	for(i = 0;   i < _DioOpenFileNum_;  i++ ) {
		df = _DioOpenFile_[i].df;
		if( df->write_flag.SleepDBF > 1 ) {
			if( dflush( df ) != 0 ) return( 1 );
			maintainRecP( df, -1 );
			df->write_flag.SleepDBF--;
		} else {
			dclose( df );
			_DioOpenFile_[i].df = NULL;
		}
	}
	for(i = j = 0;   i < _DioOpenFileNum_;  i++ ) {
		if( _DioOpenFile_[i].df != NULL ) {
			_DioOpenFile_[j++].df = df;
		}
	}
	_DioOpenFileNum_ = j;
#endif
	return( 0 );
    }

#ifdef WSToMT
    if( (df->dbf_flag == ODBC_TABLE) || (df->dbf_flag == SQL_TABLE) ) 
    { //ODBC_TABLE and SQL_TABLE isn't managed by DIO
        return  dRelease( df );
    }

    EnterCriticalSection( &dofnCE );
    if( df->pdf != NULL ) {
	odf = df;
	if( df->op_flag == VIEWFLAG ) {

		dFILE *vdf = ((dVIEW *)df)->view;

		//1998.12.9
		if( vdf->isRecCheck )
		{
		    for(i = 0;  i < vdf->field_num;  i++) {
			if( vdf->field[i].px != NULL )
			FreeCode( (MidCodeType *)vdf->field[i].px );
		    }
		}

		free( vdf->rec_buf );
		free( vdf->field );
		free( vdf->fld_id );
		free( vdf );
	}
	df = df->pdf;

	//1998.12.9
	if( odf->isRecCheck )
	{
	    for(i = 0;  i < odf->field_num;  i++) {
		if( odf->field[i].px != NULL )
		FreeCode( (MidCodeType *)odf->field[i].px );
	    }
	}

	free(odf->rec_buf);
	free(odf->field);
	free(odf->fld_id);

	free(odf);
    }
#endif
    for(i = 0;   i < _DioOpenFileNum_ && _DioOpenFile_[i].df != df;  i++ );

    if( i >= _DioOpenFileNum_ ) {
#ifdef WSToMT
	LeaveCriticalSection( &dofnCE );
#endif
	return  -1;
    } else {
#ifdef OPTIMIZED_FILE_MAN
	if( _DioOpenFile_[i].FileManOpable ) {
	    if( df->write_flag.SleepDBF > 0 ) {

		(df->write_flag.SleepDBF)--;

		if( dflush( df ) != 0 ) {
#ifdef WSToMT
			LeaveCriticalSection( &dofnCE );
#endif
			return  -2;
		}
		maintainRecP( df, -1 );
#ifndef WSToMT
// this sentense is add at 1994.10.9
		df->op_flag = _DioOpenFile_[i].sleepFlag[df->write_flag.SleepDBF];
#endif
	    }
	} else
#endif
	if( --(df->write_flag.SleepDBF) >= 1 ) {
		if( dflush( df ) != 0 ) {
#ifdef WSToMT
			LeaveCriticalSection( &dofnCE );
#endif
			return  -2;
		}
		maintainRecP( df, -1 );
#ifndef WSToMT
// this sentense is add at 1994.10.9
		df->op_flag = _DioOpenFile_[i].sleepFlag[df->write_flag.SleepDBF];
#endif
	} else {
		dRelease( df );

		//df is free now!!! 1997.08.01 Xilong
#ifdef WSToMT
		LeaveCriticalSection( &dofnCE );
#endif
		return( 0 );
/* this two statements is writen older 1994.9.8
//                       shrink the memory
			dresetbuf( df, df->rec_len * 2 );
*/
	}
    }

#ifdef WSToMT
    LeaveCriticalSection( &dofnCE );
#endif
    return  df->write_flag.SleepDBF;

} /* end of function dSleep() */


/*
-------------------------------------------------------------------------
			dIncreaseCount()
PURPOSE: dIncreaseCount() will increase the write_flag.SleepDBF
-------------------------------------------------------------------------*/
static int   dIncreaseCount(dFILE *df)
{
    register short i;

    if( df == NULL ) {
	return  -1;
    }

#ifdef WSToMT
    EnterCriticalSection( &dofnCE );
    if( df->pdf != NULL ) {
	df = df->pdf;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( df->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    df = df->pdf;
	}

    }
#endif

    for(i = 0;   i < _DioOpenFileNum_ && _DioOpenFile_[i].df != df;  i++ );

    if( i >= _DioOpenFileNum_ ) {
#ifdef WSToMT
	LeaveCriticalSection( &dofnCE );
#endif
	return  -2;
    } else {
	(df->write_flag.SleepDBF)++;
    }

#ifdef WSToMT
    LeaveCriticalSection( &dofnCE );
#endif
    return  df->write_flag.SleepDBF;

} /* end of function dIncreaseCount() */



/*
-------------------------------------------------------------------------
			dDecreaseCount()
PURPOSE: dDecreaseCount() will decrease the write_flag.SleepDBF
-------------------------------------------------------------------------*/
int   dDecreaseCount(dFILE *df)
{
    register short i;

    if( df == NULL ) {
	return  -1;
    }

#ifdef WSToMT
    EnterCriticalSection( &dofnCE );
    if( df->pdf != NULL ) {
	df = df->pdf;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( df->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    df = df->pdf;
	}

    }
#endif

    for(i = 0;   i < _DioOpenFileNum_ && _DioOpenFile_[i].df != df;  i++ );

    if( i >= _DioOpenFileNum_ ) {
#ifdef WSToMT
	LeaveCriticalSection( &dofnCE );
#endif
	return  -2;
    } else {
	if( df->write_flag.SleepDBF <= 1 ) {
	    dRelease( df );
#ifdef WSToMT
	    LeaveCriticalSection( &dofnCE );
#endif
	    return  0;
	} else {
	    (df->write_flag.SleepDBF)--;
	}
    }

#ifdef WSToMT
    LeaveCriticalSection( &dofnCE );
#endif
    return  df->write_flag.SleepDBF;

} /* end of function dDecreaseCount() */



/*
-------------------------------------------------------------------------
			dRelease()
-------------------------------------------------------------------------*/
short dRelease( dFILE *df )
{
    short   i;
    dVIEW  *dv;
    int     k;

    if( df == NULL ) {
	for(_DioOpenFileNum_--; _DioOpenFileNum_ >= 0;  _DioOpenFileNum_-- ) {
	   dclose( _DioOpenFile_[_DioOpenFileNum_].df );
	   _DioOpenFile_[_DioOpenFileNum_].df = NULL;
	}
	_DioOpenFileNum_ = 0;
    } else {
#ifdef WSToMT
	EnterCriticalSection( &dofnCE );
#endif
	for(i = 0;   i < _DioOpenFileNum_ && _DioOpenFile_[i].df != df;  i++ );

	if( i >= _DioOpenFileNum_ ) {
#ifdef WSToMT
		LeaveCriticalSection( &dofnCE );
#endif
		return  -1;
	} else {
		// shrink the _DioOpenFile_
		for( i++; i < _DioOpenFileNum_; i++ )
			_DioOpenFile_[i-1] = _DioOpenFile_[i];
		_DioOpenFileNum_--;

		if( (df->dbf_flag == ODBC_TABLE) || (df->dbf_flag == SQL_TABLE) ) 
		{
    		    goto  ODBC_SQL_JMP2;
		}

		if( df->op_flag == VIEWFLAG ) {

			dv = (dVIEW *)df;

			// release view
			if( dv->view != NULL ) {
				for(i = 0;   i < _DioOpenFileNum_ && _DioOpenFile_[i].df != dv->view->pdf;  i++ );
				if( i >= _DioOpenFileNum_ ) {
#ifdef WSToMT
				    LeaveCriticalSection( &dofnCE );
#endif
				    return   1;
				}

				if( dv->view->pdf->write_flag.SleepDBF > 1 ) {
				    dv->awakedView = dv->view->pdf;
				} else {
				    for( i++;   i < _DioOpenFileNum_;   i++ ) {
					_DioOpenFile_[i-1] = _DioOpenFile_[i];
				    }
				    _DioOpenFileNum_--;
				}
			}
		}

		//1998.1.7
		if( df->dpack_auto )
		    dpack(df);

		for(k = 0;  k < df->syncBhNum;  k++) {
		    IndexSleep( (bHEAD *)(df->bhs[k]) );
		}

		for(k = 0;  k < df->field_num;  k++) {
		    if( df->field[k].px != NULL )

			free(df->field[k].px);

			//1998.12.9
			//FreeCode( (MidCodeType *)df->field[k].px );
		}

ODBC_SQL_JMP2:		//-----------------<<<<<<<<<<<<<<<<<<<<<<<<<<
		dclose( df );
	}
#ifdef WSToMT
        LeaveCriticalSection( &dofnCE );
#endif
    }

    return  0;

} /* end of function dRelease() */


/*
-------------------------------------------------------------------------
	    alloc_dFILE()
-------------------------------------------------------------------------*/
dFILE *alloc_dFILE(void)
{
    dFILE *df;

    df = (dFILE *)malloc( sizeof ( dFILE ) );
    if ( df == NULL ) {
	df->error = 1001; 	/* ENOMEM */
	return  NULL;
    }

    // initialize the memory
    memset( df, 0, sizeof(dFILE) );

    return  df;
}

/*
----------------------------------------------------------------------------
				   dzap()
---------------------------------------------------------------------------*/
short  dzap( dFILE *df )
{
    if( df == NULL ) return( 1 );

    if( df->dbf_flag == SQL_TABLE || df->dbf_flag == ODBC_TABLE ) {
	df->error = 2001;
	return  2;
    }
    if( (df->oflag & 0xF) == O_RDONLY ) {
	df->error = 2000;
	return  1;
    }

#ifdef WSToMT
    if( df->pdf != NULL ) {
	df = df->pdf;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( df->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    df = df->pdf;
	}

    }
#endif

    df->rec_num = 0;
    df->rec_beg = df->rec_p = df->rec_end = df->buf_freshtimes = 1;
    df->write_flag.buf_write = 0;
    df->write_flag.file_write = df->write_flag.append = 1;
    df->buf_methd = 'S';
    df->buf_usetimes = IniBufUseTimes;

    dERROR = errno;

    return( 0 );

}


/*
----------------------------------------------------------------------------
				   dpack()
Caution:
    if you delete ONE record in a view, this will correct the view's pointer,
else it make mistake to a view.
    correct to a DBF all the time.
---------------------------------------------------------------------------*/
short  dpack( dFILE *df )
{
    long put_recnum;
    long get_recnum;
    unsigned char *p;
//    long    recno;
#ifdef RAM_ENOUGH
    unsigned short oldBsize;
#endif
    int 	  i;
    unsigned char tvFlag;
//    char    c;
    int     memoExist;
    char    buf[256];
    int	    slowPutRec;

    if( df == NULL )    return  1;

    if( df->dbf_flag == SQL_TABLE || df->dbf_flag == ODBC_TABLE ) {
	df->error = 2001;
	return  2;
    }
    if( (df->oflag & 0xF) == O_RDONLY ) {
	return  df->error = 2000;
    }

#ifdef WSToMT
    if( df->pdf != NULL ) {
	df = df->pdf;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( df->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    df = df->pdf;
	}

    }
#endif

    if( df->firstDel == LONG_MAX ) {
        return  0;
    }

#ifdef WSToMT
    wmtDbfLock(df);
    /*if( dIsAwake(df->name) > 1 )
    { //more than one user is using the dbf
	wmtDbfUnLock(df);
	return  df->error = 2008;
    }*/
#endif

#ifdef RAM_ENOUGH
    // if the old buffer is very small
    if( (oldBsize = df->buf_bsize) < 16000 ) {
	dresetbuf( df, 32000 );
    }
#endif

    /* if DBT file exist delete the Memo field */
    if( (df->dbf_flag & 0x83) == 0x83 ) {
	memoExist = 1;
    } else {
	memoExist = 0;
    }
    tvFlag = df->op_flag;
    df->op_flag = TABLEFLAG;

if( df->del_link_id != 0xFFFF ) {

    put_recnum = df->rec_num;		//point to the tail
    if( df->firstDel < 0 ) {            //df->firstDel set error
	df->firstDel = LONG_MAX;
	wmtDbfUnLock(df);
	return  0;
    }
    get_recnum = df->firstDel;		//point to the thread head

    while( get_recnum > 0 && get_recnum < LONG_MAX )	//repeat untail thread end
    {
	int    k;
	long   nextI;

	df->rec_p = get_recnum;
	if( ( p = get1rec( df ) ) != NULL ) {
	    if( p[0] != '*' ) {		//thread set error, ignore
		get_recnum = *(long *)(df->field[df->del_link_id].fieldstart);
		continue;
	    }
	} else {
	    goto  dPackExit;		//read error
	}

	//delete current record         //delete memo
	if( memoExist ) {
	   for(i = 0;  i < df->field_num;  i++) {
	      if( df->field[i].fieldtype == 'M' ) {
		  long  ll;

		  ll = atol(subcopy(df->field[i].fieldstart, 0, 10));
		  if( ll > 0 )
		    PhyDbtDelete(df, ll);
	      }
	   }
	}
	for(k = 0;   k < df->syncBhNum;   k++) {     //delete index key
	    IndexRecDel((bHEAD *)(df->bhs[k]), \
			 genDbfBtreeKey((bHEAD *)(df->bhs[k]), buf), get_recnum);
	}

	nextI = *(long *)(df->field[df->del_link_id].fieldstart);

	//if it is the dbf end now
	if( df->rec_p < df->rec_num ) {
	   while( put_recnum > get_recnum ) {     //search the following record
		df->rec_p = put_recnum;
		p = get1rec(df);
		if( p == NULL )
			goto  dPackExit;
		if( p[0] != '*' ) {
			break;
		} else {
			put_recnum--;
		}
	   }

	   if( put_recnum > get_recnum ) {        //if found a record
		for(k = 0;   k < df->syncBhNum;   k++) {
		    IndexRecMove((bHEAD *)(df->bhs[k]), \
				  genDbfBtreeKey((bHEAD *)(df->bhs[k]), buf),\
				  put_recnum, get_recnum);
		}
		put_recnum--;
		df->rec_p = get_recnum;

		slowPutRec = df->slowPutRec;
		df->slowPutRec = 0;    	//force to write the dbt field id
		put1rec(df);
		df->slowPutRec = slowPutRec;

	   }
	}
	get_recnum = nextI;
	df->rec_num--;
    }
    dflush(df);

} else if( df->keep_physical_order ) {
    //all sync index should be reindexed

    if( df->firstDel > 0 ) {
	get_recnum = df->firstDel;
	put_recnum = df->firstDel - 1;
    } else {
	put_recnum = 0;
	get_recnum = 1;
    }

    p = df->rec_buf;            /* not NULL value, NULL means read error */
    while( get_recnum <= df->rec_num && p != NULL ) {
	df->rec_p = get_recnum;
	if( ( p = get1rec( df ) ) != NULL ) {
	    if( p[0] != '*' ) {
		put_recnum++;
		if( get_recnum != put_recnum )
		{
		    int    k;

		    df->rec_p = put_recnum;

		    slowPutRec = df->slowPutRec;
		    df->slowPutRec = 0;    	//force to write the dbt field id
		    p = put1rec( df );
		    df->slowPutRec = slowPutRec;

		    for(k = 0;   k < df->syncBhNum;   k++) {
			IndexRecMove((bHEAD *)(df->bhs[k]), \
				     genDbfBtreeKey((bHEAD *)(df->bhs[k]), buf), \
				     get_recnum, put_recnum);
		    }
		}
	    } else {
		int  k;
		for(k = 0;   k < df->syncBhNum;   k++) {
		    IndexRecDel((bHEAD *)(df->bhs[k]), \
				genDbfBtreeKey((bHEAD *)(df->bhs[k]), buf), get_recnum);
		}
		/* if DBT file exist delete the Memo field */
		if( memoExist ) {
		    for( i = 0;  i < df->field_num;  i++ ) {
			if( df->field[i].fieldtype == 'M' ) {
			    long  ll;

			    ll = atol( (char *)subcopy((char *)df->field[i].fieldstart, 0, 10) );
			    if( ll > 0 )
				PhyDbtDelete( df, ll );
			}
		    }
		}
	    } /* end of else */
	}
	get_recnum++;
    }
    df->rec_num = put_recnum;
} else {

    if( df->firstDel > 0 ) {
	get_recnum = df->firstDel;
	put_recnum = df->firstDel - 1;
    } else {
	put_recnum = 0;
	get_recnum = 1;
    }

    for(;   get_recnum < df->rec_num;  get_recnum++)
    { //keep the rec_buf, for the calculator will use it anymore, else
      //the NEW record will never be calculated.
	int    k;
	bHEAD *bh;

	df->rec_p = get_recnum;
	if( ( p = getrec( df ) ) != NULL ) {
	    if( p[0] != '*' ) {
		continue;
	    }
	} else {
	    goto  dPackExit;
	}

	//delete current record
	if( memoExist ) {
	   for(i = 0;  i < df->field_num;  i++) {
	      if( df->field[i].fieldtype == 'M' ) {
		  long ll;

		  ll = atol(subcopy(df->field[i].fieldstart, 0, 10));
		  if( ll > 0 )
		    PhyDbtDelete(df, ll);
	      }
	   }
	}
	for(k = 0;   k < df->syncBhNum;   k++) {
	    bh = df->bhs[k];
	    IndexRecDel(bh, genDbfBtreeKey(bh, buf), get_recnum);
	}

	while( 1 ) {		//get a really record
	   dseek(df, -1, dSEEK_END);
	   if( df->rec_p <= get_recnum )
		break;

	   p = get1rec(df);
	   if( p == NULL )
		goto  dPackExit;
	   if( p[0] == '*' ) {
		for(k = 0;   k < df->syncBhNum;   k++) {
		    bh = df->bhs[k];
		    IndexRecDel(bh, genDbfBtreeKey(bh, buf), df->rec_no);
		}
		
		if( memoExist ) {
		    for(i = 0;  i < df->field_num;  i++) {
			if( df->field[i].fieldtype == 'M' ) {
			    long  ll;

			    ll = atol(subcopy(df->field[i].fieldstart, 0, 10));
			    if( ll > 0 )
				PhyDbtDelete(df, ll);
			}
		    }
		}
		df->rec_num--;
	   } else
		break;
	}

	if( df->rec_p > get_recnum ) {
	    for(k = 0;   k < df->syncBhNum;   k++) {
		bh = df->bhs[k];
		IndexRecDel(bh, genDbfBtreeKey(bh, buf), get_recnum);
		IndexRecMove(bh, genDbfBtreeKey(bh, buf), df->rec_num, get_recnum);
	    }
	    df->rec_p = get_recnum;

	    slowPutRec = df->slowPutRec;
	    df->slowPutRec = 0;    	//force to write the dbt field id
	    put1rec(df);
	    df->slowPutRec = slowPutRec;
	}
	df->rec_num--;
    }
    dflush(df);
}

    /* if memo field exist, pack the dbt file */
    if( (df->dbf_flag & 0x83) == 0x83 )         memo_pack( df );


#ifdef RAM_ENOUGH
    if( oldBsize < 16000 ) {
	dresetbuf( df, oldBsize );
    }
#endif

    df->op_flag = tvFlag;
    // if it is a view, pack the viewdbf also
    if( ( tvFlag == VIEWFLAG ) && ( ((dVIEW *)df)->view != NULL ) ) {
	modViewForPhydelRec( df, df->rec_p );
	dpack( ((dVIEW *)df)->view );
    }

dPackExit:
    df->firstDel = LONG_MAX;

    df->write_flag.file_write = 1;
    df->write_flag.buf_write = 1;

    dseek( df, 0, dSEEK_SET );

#ifdef WSToMT
    wmtDbfUnLock(df);
    return  dERROR = 0;
#else
    if( get_recnum - put_recnum < 0x7FFF )
	return( (short)(get_recnum - put_recnum) );
    return( 0x7FFF );
#endif

} // end of dpack()

/*
-]---------------------------------------------------------------------------
				   memo_pack()
---------------------------------------------------------------------------*/
short memo_pack( dFILE *df )
{
    /*unsigned char *buf;
    short  block_num, s_pointer;
    long get_seek, put_seek;
    long *spilt;
*/
    return  0;

/*    if( (df->dbf_flag & 0x83) != 0x83 )         return( 0 );

    spilt = df->dbt_head;
    qsort(&(spilt[1]), (size_t)DBTDELMEMORYSIZE-1, sizeof(long), longcomp);

    buf = df->dbt_buf;
    get_seek = put_seek = spilt[1];
    s_pointer = 2;
    while( !eof(df->dp) && s_pointer<DBTDELMEMORYSIZE && spilt[s_pointer]>0 ) {
	block_num = 0;
	while( read(df->dp, buf, DBTBLOCKSIZE) > 0 ) {
		block_num++;
		if( memchr(buf, 0x1A, DBTBLOCKSIZE) != NULL ) break;
	}
	get_seek += block_num;
	for( ; get_seek<spilt[s_pointer]; get_seek++ ) {
		lseek(df->dp, (put_seek++)*DBTBLOCKSIZE, SEEK_SET);
		write(df->dp, buf, DBTBLOCKSIZE);
		lseek(df->dp, get_seek*DBTBLOCKSIZE, SEEK_SET);
		read(df->dp, buf, DBTBLOCKSIZE);
	}
    }
    for(s_pointer=1; s_pointer<DBTDELMEMORYSIZE; s_pointer++)
	df->dbt_head[s_pointer] = 0;
    df->dbt_head[0] = put_seek;

    return( 0 );
*/
}                       //* end of function memo_pack()


/*-------------------------------------------------------------------------
			longcmp()
----------------------------------------------------------------------------*
int longcomp( const void *a, const void *b )
{
	if( *(long *)a > *(long *)b )     return( 1 );
	if( *(long *)a < *(long *)b )     return( -1 );
	return( 0 );
}                       // end of function longcomp()
*/

/*-------------------------------------------------------------------------
			drecopy()
! caution: the pointer will be moved !!!
----------------------------------------------------------------------------*/
short drecopy( dFILE *sdf, long s_recnum, dFILE *tdf, long t_recnum )
{
	unsigned short i, j;
	char PutMark = '0';

	if( tdf->dbf_flag == SQL_TABLE || tdf->dbf_flag == ODBC_TABLE ) {
		return  2;
	}

	if( s_recnum < 0 || t_recnum < 1 || s_recnum > sdf->rec_num )
		return( 1 );
	if( s_recnum > 0 )
		sdf->rec_p = s_recnum;
	else    sdf->rec_p = 1;

	if( getrec( sdf ) == NULL )     return( 1 );
	dseek( tdf, t_recnum-1, dSEEK_SET );

	if( sdf != tdf ) {
		
		memset(tdf->rec_buf, ' ', tdf->rec_len);

		for(i=0; i<tdf->field_num; i++) {
		     if( i < sdf->field_num ) {
			 if( memcmp(&(sdf->field[i]), &(tdf->field[i]), \
				sizeof(dFIELD) - sizeof(char *)*3 ) == 0 ) {
			    memcpy(tdf->field[i].fieldstart, \
					sdf->field[i].fieldstart, \
					sdf->field[i].fieldlen );

			    if( sdf->field[i].fieldtype == 'M' ) {
				dbtToFile( sdf, getVirtualId(sdf,i), dioTmpFile );
				dbtFromFile( tdf, getVirtualId(sdf,i), dioTmpFile );
			    }

			    PutMark = '1';
			    continue;
			 }
		     }
		     for(j=0; j<sdf->field_num; j++)
			 if( memcmp(&(sdf->field[j]), &(tdf->field[i]), \
				sizeof(dFIELD) - sizeof(char *)*3 ) == 0 ) {
			    memcpy(tdf->field[i].fieldstart, \
					sdf->field[j].fieldstart, \
					sdf->field[j].fieldlen );
			    PutMark = '1';
			    break;
			 }
		}
	} else  PutMark = '1';

	if( PutMark == '1' )
		if( putrec( tdf ) == NULL )     return( 1 );

	return( 0 );

}

/*-------------------------------------------------------------------------
			dinsert()
----------------------------------------------------------------------------*/

short dinsert( dFILE *df, long recnum, short insnum )
{
#ifdef RAM_ENOUGH
	unsigned short oldBsize;
#endif
	long movenum;

	if( df->dbf_flag == SQL_TABLE || df->dbf_flag == ODBC_TABLE ) {
		return  -1;
	}
	if( insnum <= 0 )       return( insnum );
	if( recnum <= 0 )       recnum = 1;

	if( recnum > df->rec_num ) {
		recnum = df->rec_num + 1;
		df->write_flag.append = 1;
		df->write_flag.file_write = 1;
	}

	movenum = df->rec_num;

	df->rec_num += insnum;
	chsize( df->fp, df->headlen + df->rec_num * df->rec_len );

#ifdef RAM_ENOUGH
	// if the old buffer is very small
	if( (oldBsize = df->buf_bsize) < 16000 ) {
		dresetbuf( df, 32000 );
	}
#endif

	while( movenum >= recnum ) {
		df->rec_p = movenum;
		get1rec( df );
		df->rec_p = movenum + insnum;
		put1rec( df );
		movenum--;
	}

	memset( df->rec_buf, ' ', df->rec_len );        /* blank them */
	movenum = recnum + insnum - 1;
	while( movenum >= recnum ) {
		df->rec_p = movenum;
		put1rec( df );
		movenum--;
	}

#ifdef RAM_ENOUGH
	if( oldBsize < 16000 ) {
		dresetbuf( df, oldBsize );
	}
#endif

	df->rec_p = recnum;
	return( insnum );
}

/*-------------------------------------------------------------------------
			field_ident()
----------------------------------------------------------------------------*/

short *field_ident( dFILE *df, char **fld_name )
{
   unsigned short i = 0, j;
   short          k;

   while ( fld_name[i] != NULL && fld_name[i][0] != '\0' ) {

       if( i >= df->field_num ) return( NULL );

       trim( fld_name[i] );

       for( j = 0;  j < df->field_num;  j++ )
	    if( stricmp( (char *)df->field[j].field, (char *)fld_name[i] ) == 0 )
		break;

       if ( j >= df->field_num ) {
	   df->fld_id[i] = -1;
	   return ( NULL ); /* so call function can ident which one make trouble*/
       }

       df->fld_id[i] = j;
       df->field[j].sdword = i;

       i++;
   }

   if( i < df->field_num ) {
	for( j = 0;  j < df->field_num && i < df->field_num;  j++ ) {
		k = iarrayShort( df->fld_id, i, j );
		if( k < 0 ) {
			df->field[j].sdword = -i-1;
			df->fld_id[i++] = j;
		}
	}
   }

   return  df->fld_id;

} // end of field_ident()


/*
------------------------------------------------------------------------------
			GetFldPosition()
----------------------------------------------------------------------------*/
unsigned short GetFldPosition( dFILE *df, char *fld_name )
{
    unsigned short i, position;

    if( df == NULL )    return( 0xFFFF );

    position = 1;

    trim( fld_name );

    for( i = 0;  i < df->field_num;  i++ )
	if( stricmp((char *)fld_name, (char *)df->field[i].field) != 0 )
				position += (short)(df->field[i].fieldlen);
	else    break;

    if( i >= df->field_num )            return( 0xFFFF );   /* no the field */
    else                                return( position );

}


/*
------------------------------------------------------------------------------
			GetFldid()
----------------------------------------------------------------------------*/
unsigned short GetFldid( dFILE *df, char *fld_name )
{
    unsigned short i, j;
    char           *s;
    char           c;

    if( df == NULL || fld_name == NULL )    return( 0xFFFF );

    //fld_name support dbfname.fieldname
    if( (s = strchr(fld_name, '.')) != NULL ) {
        *s = '\0';
        if( stricmp(fld_name, TrimFileName(df->name)) != 0 )
        { //not the same dbf
	    *s = '.';
	    return( 0xFFFF );
        }
	*s = '.';
	fld_name = s + 1;
    }

    c = *trim( fld_name );

    if( c >= 'a' && c <= 'z' ) {
	c += ('A' - 'a');
    }
    j = df->field_num;
    for( i = 0;  i < j;  i++ ) {
	s = (char *)df->field[i].field;
	if( c != *s )
		continue;

	if( stricmp((char *)fld_name, s) == 0 )        break;
    }

    if( i >= df->field_num )    return( 0xFFFF );

/* modified 95.5.4 Xilong
 *   else                        return( i );
 */
    return  iarrayShort(df->fld_id, df->field_num, i );

} // end of GetFldid()


/*
-----------------------------------------------------------------------------
			get_all_field()
---------------------------------------------------------------------------*/
char **get_all_field( dFILE *df, char **field_buf )
{
  unsigned short i;

  for( i = 0; i < df->field_num; i++) {
      memcpy( field_buf[i], df->field[i].fieldstart, df->field[i].fieldlen );
      field_buf[i][ df->field[i].fieldlen ] = '\0';
  }

  return ( field_buf );

}


/*
----------------------------------------------------------------------------
			dbt_seek()
---------------------------------------------------------------------------*/
long dbt_seek( dFILE *df, long rec_offset, dSEEK_POS from_where )
{
	switch( from_where ) {
	   case dSEEK_SET:
		if( rec_offset < 0 )   break;           //* over head
		if( df->dbt_len == 0 ) {
			if( rec_offset > (short)df->dbt_p )
				dbtlen( df );
		} else if( rec_offset > (short)df->dbt_len )
				rec_offset = df->dbt_len;
		rec_offset -= df->dbt_p;
	   case dSEEK_CUR:
DSEEK_CUR_GOTO:
		if( rec_offset <= 0 ) {
			if( -rec_offset > (short)df->dbt_p )     //* over head length
				rec_offset = -df->dbt_p;
		} else {
			if( df->dbt_len == 0 )  dbtlen( df );
			if( rec_offset > df->dbt_len - df->dbt_p ) //* over bottom
				rec_offset = df->dbt_len - df->dbt_p;
		}
		lseek( df->dp, DBTBLOCKSIZE*rec_offset, SEEK_CUR );
		break;
	   case dSEEK_END:
		if( rec_offset > 0 )    break;
		if( df->dbt_len == 0 )  dbtlen( df );
		rec_offset += df->dbt_len - df->dbt_p;
		goto DSEEK_CUR_GOTO;
	   }

	   return( df->dbt_p );

}               /* end of function */


/*
----------------------------------------------------------------------------
			DbtEof()
---------------------------------------------------------------------------*/
long DbtEof( dFILE *df )
{
	return( df->dbt_len );
}


/*
----------------------------------------------------------------------------
			DbtWrite()
the space should be alloc well
---------------------------------------------------------------------------*/
short DbtWrite( dFILE *df )
{
    int i;

    i = write(df->dp, df->dbt_buf, DBT_DATASIZE);
    lseek(df->dp, 8, SEEK_CUR);

    if( df->pdf == NULL ) {
	df->write_flag.dbt_write = 1;
	df->write_flag.file_write = 1;
    } else {
	df->pdf->write_flag.dbt_write = 1;
	df->pdf->write_flag.file_write = 1;
    }

    return  i;

} /* end of function DbtWrite() */


/*
----------------------------------------------------------------------------
			dbtlen()
---------------------------------------------------------------------------*/
long dbtlen( dFILE *df )
{
    short dbt_p;
    DBTBLOCK *dblock = (DBTBLOCK  *)df->dbt_buf;

    for(dbt_p = 0;  dblock->next > 0;  dbt_p++) {
	if( read(df->dp, dblock, DBTBLOCKSIZE ) <= 0 )
		break;
    }

    lseek(df->dp, -dbt_p*DBTBLOCKSIZE, SEEK_CUR);  // reset the pointer

    return( df->dbt_len = df->dbt_p + dbt_p );

} //end of dbtlen()


/*
----------------------------------------------------------------------------
			DbtRead()
//the file's pointershould be moved properly
---------------------------------------------------------------------------*/
unsigned char *DbtRead( dFILE *df )
{
    short     i;
    DBTBLOCK  *dblock;

    if( df->dp <= 0 || ( df->dbt_len > 0 && df->dbt_p >= df->dbt_len ) )
        return( NULL );

    dblock = (DBTBLOCK *)df->dbt_buf;
    if( ( i = read( df->dp, dblock, DBTBLOCKSIZE ) ) <= 0 )
        return( NULL );

    if( dblock->next == 0 ) {
        df->dbt_len = ++df->dbt_p;
    } else {
      df->dbt_p++;
    }
    return( df->dbt_buf );

} /* end of function DbtRead() */
//if lastBlock is 0, -->> first
/*
----------------------------------------------------------------------------
			allocDbtBlock()
---------------------------------------------------------------------------*/
long allocDbtBlock(dFILE *df, long lastBlock)
{
    long retl;
    DBTBLOCK     dbinfo;

#ifdef WSToMT
    wmtDbfLock(df);
#endif
    if( df->dbt_head[1] != 0 )
    { //alloc from free
        retl = df->dbt_head[1];
	lseek(df->dp, retl*DBTBLOCKSIZE+DBT_DATASIZE, SEEK_SET);
        read(df->dp, &df->dbt_head[1], sizeof(long));
    } else {
        retl = df->dbt_head[0]++;
	chsize(df->dp, df->dbt_head[0]*DBTBLOCKSIZE);
    }

    if( lastBlock != 0 )
    { //give the last block that the next block info
	lseek(df->dp, lastBlock*DBTBLOCKSIZE+DBT_DATASIZE+sizeof(long), SEEK_SET);
        write(df->dp, &retl, sizeof(long));
    }

    //init the return block
    lseek(df->dp, retl*DBTBLOCKSIZE, SEEK_SET);
    memset(&dbinfo, 0, sizeof(DBTBLOCK));
    dbinfo.last = lastBlock;
    dbinfo.next = 0;
    write(df->dp, &dbinfo, DBTBLOCKSIZE);
    lseek(df->dp, retl*DBTBLOCKSIZE, SEEK_SET);

    if( df->pdf == NULL ) {
	df->write_flag.dbt_write = 1;
	df->write_flag.file_write = 1;
    } else {
	df->pdf->write_flag.dbt_write = 1;
	df->pdf->write_flag.file_write = 1;
    }

#ifdef WSToMT
    wmtDbfUnLock(df);
#endif
    return  retl;

} //end of allocDbtBlock()


/*
----------------------------------------------------------------------------
			PhyDbtDelete()
---------------------------------------------------------------------------*/
short PhyDbtDelete( dFILE *df, long block )
{
    DBTBLOCKINFO dbinfo;

    if( df == NULL )
	return  0;

    if( block <= 0 )
	return  0;
    
    if( df->cdbtIsGDC ) {
	freeBtreeData((bHEAD *)(df->dp), (char *)&block);
	return  1;
    }


    if( block >= df->dbt_head[0] )
	return  0;
	

    while( block > 0 ) {
	   lseek(df->dp, block*DBTBLOCKSIZE+DBT_DATASIZE, SEEK_SET);
	   if( read(df->dp, &dbinfo, sizeof(DBTBLOCKINFO)) == -1 )
	       break;
	   dbinfo.last = df->dbt_head[1];
	   df->dbt_head[1] = block;
	   lseek(df->dp, block*DBTBLOCKSIZE+DBT_DATASIZE, SEEK_SET);
	   write(df->dp, &dbinfo, sizeof(DBTBLOCKINFO));
	   block = dbinfo.next;
    }

    return  1;

} /* end of function PhyDbtDelete()*/


/*
----------------------------------------------------------------------------
			append_blankn()
---------------------------------------------------------------------------*/
short append_n_blank( dFILE *df, short count )
{
    if( df->dbf_flag == SQL_TABLE || df->dbf_flag == ODBC_TABLE ) {
	return  -1;
    }

    memset( df->rec_buf, 0x0020, df->rec_len);
    while( count-- ) {
	dseek( df, 0, dSEEK_END );
	putrec( df );
    }
    return  (count);
}


/*
----------------------------------------------------------------------------
			dioerror()
---------------------------------------------------------------------------*/
short dioerror( dFILE *df )
{
	return( df->error );
}

/*
----------------------------------------------------------------------------
			dclearerr()
---------------------------------------------------------------------------*/
void dclearerr( dFILE *df )
{
	df->error = 0;
	dERROR = 0;
}

/*
----------------------------------------------------------------------------
			NewRec()
---------------------------------------------------------------------------*/
unsigned char *NewRec( dFILE *df )
{
    if( df == NULL )
	return  NULL;

    return( (unsigned char *)memset( df->rec_buf, ' ', df->rec_len ) );
}


/*
----------------------------------------------------------------------------
			NewBlobFlds()
---------------------------------------------------------------------------*/
unsigned char *NewBlobFlds( dFILE *df )
{
    short   i;
    dFIELD *field;

    if( df == NULL )
	return  NULL;

    field = df->field;

    for(i = 0;  i < df->field_num;  i++) {
	if( i == df->fldTimeStampId || field[i].fieldtype == 'M' || field[i].fieldtype == 'G' ) {
		memset(field[i].fieldstart, ' ', field[i].fieldlen);
	}
    }

    return  df->rec_buf;

}



/*
----------------------------------------------------------------------------
			IsMemoExist()
---------------------------------------------------------------------------*/
short   IsMemoExist( dFILE *df )
{

    return( (df->dbf_flag & 0x83) - 0x83 );

} /* end of function IsMemoExist() */


/*
----------------------------------------------------------------------------
			MemoBufInit()
---------------------------------------------------------------------------
void MemoBufInit( unsigned char *MemoBuf )
{

    if( MemoBuf == NULL )       return;

    (*(dBITMEMO *)MemoBuf).MemoMark = dBITMEMOMemoMark;
    (*(dBITMEMO *)MemoBuf).MemoLen = 0;
    time( &((*(dBITMEMO *)MemoBuf).MemoTime) );

}  end of function MemoBufInit() */

/*
----------------------------------------------------------------------------
			dEncrypt()
---------------------------------------------------------------------------*/
short dEncrypt( char *DbfName )
{
    short fh;                           /* File Handle */
    char buf[NAMELEN];

    if( strlen ( DbfName ) > NAMELEN -1 ){ /* '-1' FOR HOLD '\0'*/
	return dERROR = 1012;           /* name too long */
    }

    MK_name( strcpy( buf, DbfName ) );

    if( (fh = sopen( buf, DOPENPARA)) < 0 ) {
	return dERROR = 1003;           /* file not found */
    }

    read( fh, buf, 32 );

    if( strncmp((char *)buf, (char *)CryptStr, 16) == 0 ) {
	return -1;
    }

    memcpy(buf+16, buf, 16);
    memcpy(buf, CryptStr, 16);

    lseek(fh, 0, SEEK_SET);
    write(fh, buf, 32);

    close( fh );

    return 0;

}

/*
----------------------------------------------------------------------------
			dDecrypt()
---------------------------------------------------------------------------*/
short dDecrypt( char *DbfName )
{
    short fh;                           /* File Handle */
    char buf[NAMELEN];

    if( strlen ( DbfName ) > NAMELEN -1 ){ /* '-1' FOR HOLD '\0'*/
	return dERROR = 1012;           /* name too long */
    }

    MK_name( strcpy( buf, DbfName ) );

    if( (fh = sopen( buf, DOPENPARA)) < 0 ) {
	return dERROR = 1003;           /* file not found */
    }

    read( fh, buf, 32 );

    if( strncmp((char *)buf, (char *)CryptStr, 16) != 0 ) {
	return 2001;                    /* not a cryptic file */
    }

    memcpy(buf, buf+16, 16);
    memset(buf+16, '\0', 16);

    lseek(fh, 0, SEEK_SET);
    write(fh, buf, 32);

    close( fh );

    return 0;

}


/*
----------------------------------------------------------------------------
			ReadDbfHead()
---------------------------------------------------------------------------*/
static dFILE *ReadDbfHead( dFILE *df )
{
    unsigned char buf[36];
    char        *s, cs;
    int         i,k;
    long        l;
    int		bMemoExist;

    if( df == NULL )     return NULL;

    if( lseek( df->fp, 0, SEEK_SET ) != 0 )
	return  NULL;

    read( df->fp, buf, 32 );
#ifndef WSToMT
    if( strncmp((char *)buf, (char *)CryptStr, 16) == 0 ) {
	memcpy(buf, buf+16, 16);
	df->pmode = '1';
    } else
#endif
	df->pmode = '0';


    df->dbf_flag = buf[0];
    df->last_modi_time[0] = buf[1];
    df->last_modi_time[1] = buf[2];
    df->last_modi_time[2] = buf[3];

    df->rec_num = *(long *)&buf[4];         /* buf[4] ~ buf[7]   */
    df->rec_len = *(unsigned short *)&buf[10];     /* buf[10] ~ buf[11] */
    df->headlen = *(unsigned short *)&buf[8];     /* buf[8] ~ buf[9]   */

    df->firstDel = *(long *)&buf[12];     /* buf[12] ~ buf[15]   */

    df->field_num = df->headlen / 32 - 1;

    df->field = (dFIELD *)zeroMalloc( df->field_num*sizeof(dFIELD) );
    df->fld_id = (short *)calloc( df->field_num, sizeof( short ) );
    if( df->field == NULL || df->fld_id == NULL ){
	close( df->fp );
	df->error  = 1001; /* ENOMEM */
	free_dFILE( df );
	return  NULL;
    }

    df->rec_buf = (unsigned char *)zeroMalloc( 2 * (df->rec_len+1) ); /* '+1' for '\0' */
    if ( df->rec_buf == NULL ) {
	close( df->fp );
	df->error = 1001; /* ENOMEM */
	free_dFILE ( df );
	return ( NULL );
    }
    //df->rec_buf[ df->rec_len ] = '\0';

    //1997.12.18
    df->rec_tmp = &(df->rec_buf[df->rec_len + 1]);

    if( try_alloc( df ) != dOK ) { /* buf_sink, buf_bsize,buf_rsize, filled */
	close( df->fp );
	free_dFILE( df );
	return  NULL;
    }

    if( ( df->dbf_flag & 0x83 ) == 0x83 ) {
	bMemoExist = 1;
    } else {
	bMemoExist = 0;
    }

    k = 1;                      /* as a counter, 1 means DEL flag */
    for( i = 0; i < (int)df->field_num; i++){
	df->fld_id[i] = i;
	read( df->fp, buf, 32 );
	if( buf[0] <= '\x0D' ) {        /* end of field description */
		df->field_num = i;
		df->field = (dFIELD *)realloc( df->field, i * sizeof ( dFIELD ) );
		df->fld_id = (short *)realloc( df->fld_id, i * sizeof ( short ) );
		if( df->field == NULL || df->fld_id == NULL ){
			close( df->fp );
			df->error  = 1001; /* ENOMEM */
			free_dFILE( df );
			return( NULL );
		}
		break;
	}

	strZcpy((char *)df->field[i].field, (char *)buf, 11);

	buf[32] = '\0';

	if( isalpha(buf[18]) )
	    strcat(df->field[i].field, &buf[18]);

	if( buf[11] == 'B' || buf[11] == 'O' )
	{ //DIO treat BLOB OLE field as MemoField
	    df->field[i].fieldtype = 'M';
	} else {
	    if( buf[15] != '\0' )
		df->field[i].fieldtype = buf[15];
	    else
		df->field[i].fieldtype = buf[11];
	}

	if( !bMemoExist && (df->field[i].fieldtype == 'M' || \
					    df->field[i].fieldtype == 'G' ) )
	{ //added 1998.11.10
		close( df->fp );
		dERROR = 4001;          /* file not found */
		free_dFILE( df );
		return( NULL );
	}

	df->field[i].fieldlen = (unsigned char)buf[16];
	df->field[i].fielddec = (unsigned char)buf[17];
	df->field[i].fieldstart = df->rec_buf + k;
	k += (unsigned char)buf[16];
    }

    //check the dbf head
    l = filelength(df->fp);

    //in dBASE IV, the record len != sum of field len
    //if( (k != df->rec_len) || \
    //                  (l < (long)df->headlen+df->rec_num*df->rec_len) ) {
    if( (l < (long)df->headlen+df->rec_num*df->rec_len) ) {
	df->rec_num = (l - df->headlen) / df->rec_len;
	//take this as right and correct it in dBASE IV?.
	//close( df->fp );
	//dERROR = 1099;
	//free_dFILE( df );
	//return  NULL;
    }

    if( bMemoExist ) {     /**/
	s = strrchr( df->name, '.' );
	cs = s[3];
	s[3] = 'T';                   /* make the name to be DBT from DBF */
	df->dp = sopen( df->name, df->oflag|O_BINARY, df->shflag, df->pmode );
	if( df->dp < 0 ) {
		close( df->fp );
		dERROR = 4001;          /* file not found */
		free_dFILE( df );
		return  NULL;
	}
	if( lseek(df->dp, 0, SEEK_SET) != 0 ) {
		close( df->fp );
		dERROR = 4002;          /* file read error */
		free_dFILE( df );
		return  NULL;
	}

	read( df->dp, df->dbt_head, DBTBLOCKSIZE );

	//1998.12.9
	if( strncmp(((char *)(df->dbt_head)), "GDC_", 4) == 0 ) {
	    close( df->dp );
	    df->dp = (long)dbTreeOpen( df->name );
	    df->cdbtIsGDC = 1;
	}

	s[3] = cs;
	df->dbt_p = df->dbt_len = 0;
    }
    df->write_flag.dbt_write = 0;

    if( ( (df->oflag & 0xF) == O_RDONLY ) && ( df->buf_rsize > (unsigned short)df->rec_num ) ) {
	df->buf_bsize = (unsigned short)df->rec_num*df->rec_len;
	df->buf_sink = (unsigned char *)realloc( df->buf_sink, df->buf_bsize );
	df->buf_rsize = (unsigned short)df->rec_num;
    }

    /* 1998.8.10
    lseek( df->fp, df->headlen, SEEK_SET );
    read( df->fp, df->buf_sink, \
	  (unsigned short)min( (long)df->buf_rsize, df->rec_num) * df->rec_len);
    */
/**/
    /* 1998.8.10
       change this to the following, for multi thread, speed up the append
    if( df->rec_num )
		df->rec_end = min( df->buf_rsize, (unsigned short)df->rec_num );
    else        df->rec_end = 1;
    */
    if( df->rec_num )
		df->rec_end = 0;
    else        df->rec_end = 1;

    strcpy(buf, "DEL___LINK");
    df->del_link_id = GetFldid(df, buf);
    if( df->del_link_id != 0xFFFF )
	df->del_link_off = (long *)(df->field[df->del_link_id].fieldstart);

    strcpy(buf, "TIME_STAMP");
    df->fldTimeStampId = GetFldid(df, buf);
    if( df->fldTimeStampId != 0xFFFF ) {
	df->fldTimeStampOffset = df->field[df->fldTimeStampId].fieldstart-df->rec_buf;
	df->fldTimeStampLen = df->field[df->fldTimeStampId].fieldlen;
    }

    df->slowPutRec = ((df->fldTimeStampId != 0xFFFF) || (df->dbf_flag & 0x83) == 0x83);

    df->rec_beg = 1;
    df->rec_no = 1;
    df->rec_p = 1;
    df->buf_freshtimes = 1;
    df->buf_usetimes = IniBufUseTimes;
    df->buf_methd = 'S';
    df->write_flag.buf_write = 0;
    df->write_flag.file_write = 0;
    df->write_flag.append = 0;

    return ( df );

} //end of ReadDbfHead()


/*
----------------------------------------------------------------------------
			dInitDF()
---------------------------------------------------------------------------*/
static dFILE *dInitDF( char *filename, short oflag, short shflag, short pmode )
{
    dFILE *df;
/*    unsigned char buf[32];
    unsigned short i, k;
*/
    oflag &= ( O_RDONLY | O_RDWR | O_WRONLY );
    oflag |= O_BINARY;

    if(  ( df = alloc_dFILE () ) == NULL )
	return( NULL );
    df->write_flag.creat = 0;
    df->oflag = oflag;
    df->shflag = shflag;
    df->pmode = pmode;

    /*change this 1998.1.31 Xilong
    if( strlen ( filename ) > NAMELEN -1 ){ // '-1' FOR HOLD '\0'
	df->error = 1012;  // name too long
	free_dFILE( df );
	return( NULL );
    }

    strcpy( df->name, filename );
    */
    strZcpy(df->name, filename, NAMELEN);
    MK_name( df->name );

    df->fp = sopen( df->name, oflag|O_BINARY, shflag, pmode );
    if( df->fp < 0 ) {
	dERROR = 1003; /* file not found */
	free_dFILE( df );
	return( NULL );
    }

    //
    // set the default lock number
    //
    df->lock_num = TOTAL_LOCKS_PER_DF;

#ifdef WSToMT
    InitializeCriticalSection( &(df->dCriticalSection) );
    df->inCriticalSection = '\0';
#endif

    return ( df );
}


/*
----------------------------------------------------------------------------
			DestroydFILE()
---------------------------------------------------------------------------*/
void DestroydFILE( dFILE *df )
{
	if( df->field != NULL )         free( df->field );
	if( df->rec_buf != NULL )       free( df->rec_buf );
	if( df->buf_sink != NULL )      free( df->buf_sink );
	if( df->dbt_head != NULL )      free( df->dbt_head );
	if( df->dbt_buf != NULL )       free( df->dbt_buf );
	if( df->buf_pointer != NULL )   free( df->buf_pointer );
	if( df->fld_id != NULL )        free( df->fld_id );
}


/*
----------------------------------------------------------------------------
			ReadViewFileName()
---------------------------------------------------------------------------*/
void ReadViewFileName(int handle, char *SourceFileName, char *ViewFileName)
{
    char *buf;

    if( lseek(handle, 0, SEEK_SET) != 0 ) {
	*SourceFileName = *ViewFileName = '\0';
	return;
    }

    buf = dGetLine( handle );

    if ( buf == NULL ) {
	*SourceFileName = *ViewFileName = '\0';
	return;
    }
    buf = dGetLine( handle );
    if ( buf == NULL || strlen( buf ) > MAXPATH ) {
	*SourceFileName = *ViewFileName = '\0';
	return;
    }

    strcpy(SourceFileName, buf);

    buf = dGetLine( handle );
    if( buf == NULL || strlen(buf) > MAXPATH ) {
	*ViewFileName = '\0';
	return;
    }
    strcpy(ViewFileName, buf);

} // end of function ReadViewFileName()


/*
----------------------------------------------------------------------------
			ReadViewFieldName()
!
! when it meet an ASQL call, it will do it immediately
---------------------------------------------------------------------------*/
char **ReadViewFieldName( int handle, dVIEW *dv )
{
    short  i;
    char  *buf;
    char  *szASQL = NULL;
    int    lenASQL = 16;        //16 for space safe

    if( dv == NULL )    return NULL;

    lseek( handle, 0, SEEK_SET );
    // skip 3 lines
    buf = dGetLine( handle );
    buf = dGetLine( handle );
    buf = dGetLine( handle );

    i = 0;
    dv->field_num = 0;
    buf = dGetLine( handle );
    if( buf == NULL ) {
	return NULL;
    }

    dv->ViewField = NULL;
    dv->szfield = NULL;

    while( buf != NULL ) {

        //support:
        // #*#.....
        // ...
        // ...
        // #ASQL script, run before the view is open
        //
        //

        if( buf[0] == '#' ) {
            lenASQL += strlen(buf) + 2;		//add 2 for \r \n
            if( lenASQL > MAX_ASQL_SCRIPT_LEN ) {
                if( dv->ViewField != NULL )
                        free(dv->ViewField);
                if( dv->szfield != NULL )
                        free(dv->szfield);
                if( szASQL != NULL )
                        free( szASQL );
                return  NULL;
            }

            if( szASQL == NULL ) {
                szASQL = malloc(lenASQL);
                szASQL = strcpy(szASQL, &buf[1]);
            } else {
                szASQL = realloc(szASQL, lenASQL);
                szASQL = strcat(szASQL, &buf[1]);
            }
	    strcat(szASQL, "\r\n");

	    buf = dGetLine( handle );
	    continue;

        } //end of 

	if( *trim(buf) == '\0' )
	    break;

	dv->ViewField = (char *)realloc( dv->ViewField, strlen( buf )+16+i );
	if( dv->ViewField == NULL ) {

	    if( dv->szfield != NULL )
		free(dv->szfield);

	    return  NULL;
	}
	dv->szfield = realloc( dv->szfield, (dv->field_num++) * sizeof(char *) );
	if( dv->szfield == NULL ) {

	    if( dv->ViewField != NULL )
		free(dv->ViewField);

	    return  NULL;
	}
//      dv->szfield[dv->field_num++] = &dv->ViewField[i];
	strcpy( &dv->ViewField[i], buf );
	i += strlen(buf);
	dv->ViewField[i++] = '`';
	buf = dGetLine( handle );
    }

    if( dv->ViewField ) {
	dv->ViewField[i-1] = '\0';
	dv->szfield = realloc( dv->szfield, (dv->field_num+1) * sizeof(char *) );
	if( dv->szfield == NULL ) {
	    free(dv->ViewField);
	    return  NULL;
	}
	seperateStr(dv->ViewField, '`', dv->szfield);
    }

    dv->szASQL = szASQL;

    return dv->szfield;

} // end of function ReadViewFieldName()


/*
----------------------------------------------------------------------------
			getFieldNum()
---------------------------------------------------------------------------*/
short getFieldNum( dFILE *df )
{
/*    dVIEW *dv; */

    if( df == NULL )     return -1;

    if( df->op_flag != VIEWFLAG ) {
	return  df->field_num;
    }
    return  ((dVIEW *)df)->field_num;

} // end of function GetViewFieldNum()


/*
----------------------------------------------------------------------------
			getFieldInfo()
---------------------------------------------------------------------------*/
dFIELD *getFieldInfo( dFILE *df, unsigned short name )
{
/*    int j;
    char *s;

    if( df == NULL )     return NULL;

    if( name >= df->field_num )                 return  NULL;*/

    return  &(df->field[ df->fld_id[name] ]);

} // end of function getFieldInfo()


/*
----------------------------------------------------------------------------
			getRecNum()
---------------------------------------------------------------------------*/
long getRecNum( dFILE *df )
{
    if( df == NULL )     return 0;

    if( df->op_flag != VIEWFLAG ) {
	if( df->dbf_flag == SQL_TABLE || df->dbf_flag == ODBC_TABLE )
		return  LONG_MAX;
	else {
#ifdef WSToMT
	   if( df->pdf != NULL )
		return  (df->rec_num = df->pdf->rec_num);
	   return  df->rec_num;
#endif
	}
    } else {
	return ((dVIEW *)df)->view->rec_num;
    }

} // end of function getRecNum()


/*
----------------------------------------------------------------------------
			getRecP()
---------------------------------------------------------------------------*/
long getRecP( dFILE *df )
{
    if( df == NULL )     return 0L;

    if( df->op_flag != VIEWFLAG ) {
	return  df->rec_no;
    } else {
	return ((dVIEW *)df)->view->rec_no;
    }

} // end of function getRecP()


/*
----------------------------------------------------------------------------
			getReallyRecP()
---------------------------------------------------------------------------*/
long getReallyRecP( dFILE *df )
{
    if( df == NULL )     return 0L;

    return  df->rec_no;

} // end of function getReallyRecP()


/*
----------------------------------------------------------------------------
			dGetLine()
---------------------------------------------------------------------------*/
char *dGetLine( int handle )
{
    char  line[MAX_LINE_LENGTH+2];
    char *s;
    short i;
    long  l;

    l = tell( handle );
    i = read(handle, line, MAX_LINE_LENGTH);

    if( i == 0 || i == -1 )       return  NULL;
    line[i] = '\0';

    if( ltrim(line)[0] == '\x1A' )        return  NULL;

    s = strchr(line, '\r');
    if( s != NULL ) {
	*s = '\0';
	i = s - line;
    }

    lseek(handle, l + i + 2, SEEK_SET);

    return subcopy(line, 0, i);

} // end of function dGetLine()


/*
----------------------------------------------------------------------------
			AppendView()
---------------------------------------------------------------------------*/
short AppendView( dFILE *df )
{
    dVIEW *temp;

    if( df == NULL )         return -1;

    if( df->op_flag != VIEWFLAG )    return -1;

    append_blank( df );
    temp = (dVIEW *)df;
    dseek( temp->view, 0, dSEEK_END );

    PutField( temp->view, 0, &(df->rec_num) );

    putrec( temp->view );

    return  dOK;
}


/*
----------------------------------------------------------------------------
			bindfield()
---------------------------------------------------------------------------*/
unsigned short bindfield( dVIEW *dv )
{
    short i, j, k, ii;

    k = dv->source.field_num + dv->view->field_num;

    dv->source.field = realloc(dv->source.field, k * sizeof(dFIELD) );
    dv->source.fld_id = realloc(dv->source.fld_id, k * sizeof(short) );
    for( j = 0, ii = i = dv->source.field_num;  i < k;  i++, j++ ) {

	//not bind the relative field
	if( stricmp(dv->view->field[j].field, viewField[0].field) == 0 )
		continue;

	memcpy(&(dv->source.field[ii]), &(dv->view->field[j]), sizeof(dFIELD));
	dv->source.fld_id[ii] = ii;
	ii++;
    }

    dv->field_num = dv->source.field_num;
    return dv->source.field_num = ii;

} // end of function bindfield()


/*
----------------------------------------------------------------------------
				makeView()
---------------------------------------------------------------------------*/
short makeView(char *viewName, char *dbfName, char *recFileName, ... )
{
	char    *p;
	short   count = 0;
	char    buf[MAXPATH];
	va_list ap;
	FILE    *fp;

	if( viewName == NULL || dbfName == NULL )       return  -1;

	fp = fopen(viewName, "wt");

	fprintf( fp,  "%s\n", DBFVIEWFLAG );
	changeFilenameExt( buf, dbfName, "");
	fprintf( fp,  "%s\n", buf );

	if( recFileName == NULL ) {
		fprintf( fp,  "NULL\n" );
	} else {
		fprintf( fp,  "%s\n", recFileName );
	}

	va_start( ap, recFileName );
	while( (p = va_arg(ap, char *)) != NULL ) {
	    count++;
	    fprintf( fp,  "%s\n", p );
	}
	va_end(ap);

	fclose( fp );

	return  count;

}


/****************
*                               maintainRecP()
**************************************************************************/
long maintainRecP( dFILE *df, long recp )
{

    if( df == NULL )         return  -1;

    if( recp >= 0 )     dseek( df, recp, dSEEK_SET );

    if( deof( df ) ) {
	dseek( df, -1L, dSEEK_END );
    }
    return  df->rec_p;
}


/****************
*                               modViewForPhydelRec()
*  call sequence: set delete flag, this, pack
****************************************************************************/
static short modViewForPhydelRec( dFILE *df, long recno )
{
    long  l;
    dFILE *pdf;
    unsigned short i;

    if( df == NULL )    return  1;

    if( ( df->op_flag == VIEWFLAG ) && ( ((dVIEW *)df)->view != NULL ) ) {

	pdf = ((dVIEW *)df)->view;
	i = ((dVIEW *)df)->RecnoFldid;

	dseek( pdf, 0, dSEEK_SET );
	while( !deof( pdf ) ) {
		getrec( pdf );
		GetField( pdf, i, &l );
		if( l > recno )         l--;
		PutField( pdf, i, &l );
		dseek( pdf, -1, dSEEK_CUR );
		putrec( pdf );
	}
    }

    return  0;

} // end of function modViewForPhydelRec()


/****************
*                               getVirtualId()
****************************************************************************/
static short getVirtualId(dFILE *df, short i)
{
    return iarrayShort( df->fld_id, df->field_num, i );
}



/****************
*                               dSyncView()
****************************************************************************/
void dSyncView( dVIEW *dv )
{
    long        recno, l, ll, lno, recp;
    dFILE       *df;
    unsigned short i;
    char        packFlag = 0;

    if( dv->view != NULL ) {

	if( dv->szASQL ) {
	    dzap( dv->view );
	    return;
	}

	lno = (df = dv->view)->rec_num;
	recno = ((dFILE *)dv)->rec_num;
	i = dv->RecnoFldid;
	recp = df->rec_p;

	//check the view dbf
	l = 1;
	while( l <= lno ) {
		df->rec_p = l++;
		get1rec( df );
		GetField( df, i, &ll );

		if( ll > recno ) {
			packFlag = 1;
			RecDelete(df);
		} else {
			putrec( df );
		}
	}
	if( packFlag ) {
		dpack(df);
		recp = 1;
	}

	if( df->rec_num < 1 ) {
		l = 1;
		while( l <= recno ) {
			PutField( df, i, &l );
			putrec( df );
			l++;
		}
	}

	df->rec_p = df->rec_no = recp;
    }

} // end of function modViewForPhydelRec()



/*-------------------------------------------------------------------------
			daddrec()
----------------------------------------------------------------------------*/
long daddrec(dFILE *df)
{
#ifdef WSToMT
    dFILE *pdf;
#endif
    int  i, k;
    char buf[256];

    if( df == NULL )    return  (-1);
    if( df->dbf_flag == SQL_TABLE || df->dbf_flag == ODBC_TABLE ) {
	return  (-1);
    }

    if( (df->oflag & 0xF) == O_RDONLY ) {
	dERROR = 2000;
	return (-1);
    }

#ifdef WSToMT
    if( df->pdf != NULL ) {
	pdf = df->pdf;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( pdf->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    pdf = pdf->pdf;
	}

	df->rec_num = pdf->rec_num;
    } else
	pdf = df;

    EnterCriticalSection( &(pdf->dCriticalSection) );
#endif

    if( pdf->del_link_id == 0xFFFF || pdf->firstDel <= 0 ) {
	//consider that the compitable to Delphi_BDE
	//pdf->firstDel = LONG_MAX;
	pdf->rec_p = pdf->rec_num + 1;
    } else {

	/*//save current record 1998.6.12 needn't save it
	memcpy(pdf->rec_tmp, pdf->rec_buf, pdf->rec_len);
	*/
	pdf->rec_p = pdf->firstDel;
	get1rec(pdf);
	pdf->firstDel = *(long *)(pdf->field[pdf->del_link_id].fieldstart);

	//delete current record         //delete memo
	if( (pdf->dbf_flag & 0x83) == 0x83 ) {
	   for(i = 0;  i < pdf->field_num;  i++) {
	      if( pdf->field[i].fieldtype == 'M' ) {
		  PhyDbtDelete(pdf, atol(subcopy(pdf->field[i].fieldstart, 0, 10)));
	      }
	   }
	}
	for(k = 0;   k < pdf->syncBhNum;   k++) {     //delete index key
	    IndexRecDel((bHEAD *)(pdf->bhs[k]), \
			 genDbfBtreeKey((bHEAD *)(pdf->bhs[k]), buf), pdf->rec_p);
	}
    }
    memcpy(pdf->rec_buf, df->rec_buf, pdf->rec_len);

    put1rec(pdf);

    LeaveCriticalSection( &(pdf->dCriticalSection) );

    return  df->rec_p;

} //end of daddrec()

/****************
*                               wsmDf()
* alloc a self rec_buf, field, fld_id
* the information about dbt use the same, and in multi thread the dbt_buf
* should use less, and the dbt information has no cache
****************************************************************************/
#ifdef WSToMT
dFILE *wsmDf(dFILE *df, short oflag, short shflag, short pmode)
{
    dFILE *tdf, *mdf;
    int   i, k;

    if( (df->dbf_flag == ODBC_TABLE) || (df->dbf_flag == SQL_TABLE) ) 
    {   //needn't build the shadow handle
	//ODBC_TABLE and SQL_TABLE isn't managed by DIO
	return  df;
    }

    if( df->op_flag == VIEWFLAG ) {

	dVIEW *dv;

	dv = (dVIEW *)zeroMalloc( sizeof(dVIEW) );
	if( dv == NULL )
		return  NULL;
	memcpy(dv, df, sizeof(dVIEW));
	tdf = (dFILE *)dv;

	if( dv->view != NULL ) {
	    mdf = dv->view;
	    dv->view = zeroMalloc( sizeof(dFILE) );
	    if( dv->view == NULL )
		return  NULL;
	    memcpy(dv->view, mdf, sizeof(dFILE));

	    dv->view->rec_buf = (unsigned char *)malloc(2*(mdf->rec_len+1));
	    if ( dv->view->rec_buf == NULL ) {
		free( tdf );
		return  NULL;
	    }
	    dv->view->rec_buf[ mdf->rec_len ] = '\0';
	    dv->view->rec_tmp = &(mdf->rec_buf[mdf->rec_len+1]);

	    dv->view->field = malloc(sizeof(dFIELD) * mdf->field_num);
	    dv->view->fld_id = malloc(sizeof(short) * mdf->field_num);
	    if ( dv->view->field == NULL || dv->view->fld_id == NULL ) {
		if( dv->view->field == NULL )
			free( dv->view->field );
		if( dv->view->fld_id == NULL )
			free( dv->view->fld_id );
		free( dv->view->rec_buf );
		free( dv->view );
		return  NULL;
	    }

	    memcpy(dv->view->field, mdf->field, (sizeof(dFIELD)*mdf->field_num));
	    memcpy(dv->view->fld_id, mdf->fld_id, (sizeof(short)*mdf->field_num));

	    k = 1;                      /* as a counter, 1 means DEL flag */
	    for( i = 0; i < (int)dv->view->field_num; i++) {
		dv->view->field[i].fieldstart = dv->view->rec_buf + k;
		k += df->field[i].fieldlen;
	    }

	    dv->view->pdf = mdf;
	    dv->view->rec_p = 1;
	    dv->view->rec_no = 1;
	}

    } else {
	tdf = malloc( sizeof(dFILE) );
	if( tdf == NULL )
		return  NULL;
	memcpy(tdf, df, sizeof(dFILE));
    }


    tdf->rec_buf = (unsigned char *)malloc(2*(df->rec_len+1)); //'+1' for '\0'
    if ( df->rec_buf == NULL ) {
	free( tdf );
	return  NULL;
    }
    tdf->rec_buf[ df->rec_len ] = '\0';
    tdf->rec_tmp = &(tdf->rec_buf[df->rec_len+1]);

    tdf->field = malloc(sizeof(dFIELD) * (int)tdf->field_num);
    tdf->fld_id = malloc(sizeof(short) * (int)tdf->field_num);
    if ( tdf->field == NULL || tdf->fld_id == NULL ) {
	if( tdf->field == NULL )
		free( tdf->field );
	if( tdf->fld_id == NULL )
		free( tdf->fld_id );
	free( tdf->rec_buf );
	free( tdf );
	return  NULL;
    }

    memcpy(tdf->field, df->field, (sizeof(dFIELD)*(int)tdf->field_num));
    memcpy(tdf->fld_id, df->fld_id, (sizeof(short)*(int)tdf->field_num));

    //1998.4.1
    //point to the original address
    if( df->dbf_flag != SQL_TABLE && df->dbf_flag != ODBC_TABLE ) {

	register char *sp = tdf->rec_buf;
	register dFIELD *field = tdf->field;

	k = 1;                      /* as a counter, 1 means DEL flag */
	for( i = 0;   i < tdf->field_num;   i++) {
	    field[i].fieldstart = sp + k;
	    k += field[i].fieldlen;
	}
    }

    tdf->pdf = df;

    oflag &= ( O_RDONLY | O_RDWR | O_WRONLY );
    oflag |= O_BINARY;
    tdf->oflag = oflag;

    tdf->rec_p = 1;
    tdf->rec_no = 1;

    df->shflag = shflag;
    df->pmode = pmode;

    if( df->isRecCheck ) {
	MidCodeType *px;
	//dFIELDWHENACTION *dfa;

	for(i = 0;  i < df->field_num;  i++) {
	    if( df->field[i].px != NULL ) {

		px = WordAnalyse( df->field[i].px );
		if( SymbolRegister(px, tdf, NULL, 0, NULL, 0) != NULL ) {
		    FreeCode(px);
		    tdf->field[i].px = NULL;
		} else {
		    tdf->field[i].px = px;
		}

		/*1998.12.9
		if( px->type >= FIELD_IDEN && px->type <= SFIELD_IDEN ) {
		    dfa = (dFIELDWHENACTION *)px->values;

		    //re_register them
		    dfa->pTargetStart = tdf->field[dfa->wTargetid].fieldstart;
		    dfa->pTargetDfile = tdf;
		}
		px = px->next;
		*/
	    } //end of if
	} //end of for
    } //if

    return  tdf;

} //end of wsmDf()

#endif


/*
=======================================================================
		uncatchTable()
=======================================================================*/
int uncatchTable(char *FileName)
{
    int   i;
    char  temp[NAMELEN];
    char  szDataBase[32];
    char *sz, *sp;
    dFILE *df;

    if( strnicmp(FileName, "SQL:", 4) != 0 && \
				      strnicmp(FileName, "ODBC:", 5) != 0 ) {

	if( FileName[1] == ':' || FileName[0] == '\\' ) {
	    strZcpy(temp, FileName, NAMELEN);
	    MakeName(temp, "DBF", NAMELEN);
	} else {
	    sp = strchr(FileName, '*');
	    if( sp == NULL ) {
		strcpy(szDataBase, "DBROOT");
		strZcpy(temp, FileName, NAMELEN);
	    } else {
		*sp++ = '\0';
		strZcpy(szDataBase, FileName, 32);
	    }

	    sz = GetCfgKey(csuDataDictionary, "DATABASE", szDataBase, "PATH");

	    if( sz != NULL ) {
		strZcpy(temp, sz, NAMELEN);
		beSurePath(temp);
	    } else {
		//database write error
		return  -1;
	    }
	    strncat(temp, sp, NAMELEN-1-strlen(temp));
	    MakeName(temp, "DBF", NAMELEN);
	}
    } else {
	return  -2;
    }

#ifdef WSToMT
    EnterCriticalSection( &dofnCE );
#endif
    for(i = 0;  i < _DioOpenFileNum_;  i++ ) {
	if( (/*_DioOpenFile_[i].op_flag == TABLEFLAG && */ \
	     stricmp(_DioOpenFile_[i].df->name, temp) == 0 ) ||
	     (_DioOpenFile_[i].op_flag == VIEWFLAG && \
	     stricmp(((dVIEW *)_DioOpenFile_[i].df)->name, temp) == 0 ) ) {
		df = _DioOpenFile_[i].df;

		if( df->write_flag.SleepDBF < 1 ) {
		    dRelease(df);
		} else {
		    _DioOpenFile_[i].FileManOpable = '\0';
		}
#ifdef WSToMT
		LeaveCriticalSection( &dofnCE );
#endif
		return  0;
	}
    }

#ifdef WSToMT
    LeaveCriticalSection( &dofnCE );
#endif
    return  _DioOpenFileNum_;

} //end of uncatchTable()


#ifdef WSToMT
int wmtDbfTryLock(dFILE *df)
{
    if( df == NULL )
	return  1;

    if( df->pdf != NULL ) {
       df = df->pdf;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( df->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    df = df->pdf;
	}

    }

    if( df->inCriticalSection == '\1' )
	return  2;
    EnterCriticalSection( &(df->dCriticalSection) );
    df->inCriticalSection = '\1';
    return  0;
}

void wmtDbfLock(dFILE *df)
{
    if( df == NULL )
	return;

    if( df->pdf != NULL ) {
       df = df->pdf;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( df->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    df = df->pdf;
	}

    }

    EnterCriticalSection( &(df->dCriticalSection) );
    df->inCriticalSection = '\1';
}

void wmtDbfUnLock(dFILE *df)
{
    if( df == NULL )
        return;

    if( df->pdf != NULL ) {
       df = df->pdf;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( df->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    df = df->pdf;
	}

    }

    LeaveCriticalSection( &(df->dCriticalSection) );
    df->inCriticalSection = '\0';
}

int wmtDbfIsLock(dFILE *df)
{
    if( df == NULL )
        return  2;

    if( df->pdf != NULL ) {
       df = df->pdf;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( df->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    df = df->pdf;
	}

    }

    if( df->inCriticalSection != '\1' )
    //if( df->dCriticalSection.LockCount == 0xFFFFFFFF )
        return  0;

    return  1;

}
#endif



void clrTempTableList( void )
{
    tempTableNum = 0;
}


void closeTempTableList( void )
{
    dFILE  *df;
    int     i;
    char    buf[MAXPATH];

#ifdef WSToMT
    EnterCriticalSection( &dofnCE );
#endif

    while( tempTableNum > 0 ) {
	df = tempTableList[--tempTableNum];

	for(i = 0;   i < _DioOpenFileNum_ && _DioOpenFile_[i].df != df;  i++ );

	if( i < _DioOpenFileNum_ ) {
		df->buf_methd = 'S';
		df->write_flag.buf_write = 0;
		df->write_flag.file_write = 0;
		df->rec_num = 0;

		strcpy(buf, df->name);
		dRelease( df );
		unlink( buf );
	}
	// else is an error
	
    }

#ifdef WSToMT
    LeaveCriticalSection( &dofnCE );
#endif

}

//
//calASQLforView()
///////////////////////////////////////////////////////////////////////////////
int calASQLforView(char *szDataBase, dVIEW *dv )
{
	
        int   j;
        MSG   msg1;
extern WSToMT FromToStru fFrTo;
extern int asqlTaskThreadNum;	//in wmtasql.c
	char  *szASQL;
	
	/*
	AskQS(szASQL, AsqlExprInMemory|Asql_USEENV,
					NULL, //lpAskQS_data->FromTable,
					NULL, //lpAskQS_data->ToTable,
					NULL, //lpAskQS_data->VariableRegister,
					NULL);//lpAskQS_data->VariableNum);
	*/ 

        //callasql, inside task
	szASQL = malloc(strlen(szDataBase) + strlen(dv->szASQL) + 16);
	if( szASQL == NULL ) {
	    return  asqlTaskThreadNum + 1;
	}
	sprintf(szASQL, "DATABASE %s\n%s", szDataBase, dv->szASQL);

        j = wmtAskQS(-1, szASQL, AsqlExprInMemory|Asql_USEENV, \
                                 &asqlEnv, \
                                 &(fFrTo.sHead),
                                 &(fFrTo.nSybNum),
                                 (EXCHNG_BUF_INFO *)wmtExbuf);
        if( j >= asqlTaskThreadNum ) {
		free( szASQL );
                return  j;
        }

        { //wait the end of this thread
            GetMessage(&msg1, NULL, ASQL_ENDTASK_MSG, ASQL_ENDTASK_MSG);
        }

	free( szASQL );

	if( dv->view->pdf != NULL )
	{   //the ASQL script has changed the rec_num
	    //
	    dv->view->rec_num = dv->view->pdf->rec_num;
	}

	return  0;
}



/******************* End Of dBASE Development Software **********************/
