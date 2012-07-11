/*********
 *  DIO.C   MIS lower supportment ( I ) -- Data I/O
 *
 *              Main modular.
 *
 * Copyright: Shanghai Institute of Railway Technology  1991, 1992
 *            EastUnion Computer Service Co., Ltd.
 *            CRSC 1997,1998
 *	      Shanghai Withub Vision Computer Software Co., Ltd. 1999-2000
 *
 * DIO Module components:
 *     (1) dio.c dio.h
 *     (2) diofile.c
 *     (3) diodbt.c diodbt.h
 *     (others) mistring arrayutl
 *
 * Caution:
 *    0xFFFF(unsigned short) == -1(short)       1994.12.29 Xilong
 *
 * 1.divide dio.c into 2 part 1995.4.24
 * 2.add multithread support 1997.10
 * 3.change put_fld, PutField, don't add tail space 1998.1.18
 * 4. save and restore XEXP_ENV when call CalExpr() 1998.12.10
 * 5. support time_stamp field 1999.1.2
 * 6. 1999.5.19
 *    correct an faltal error:
 *    BufFlush( dFILE *df ): algorithm: 'P':
 *    df->write_flag.buf_write = 1;
 * 7. 2000.4.14  if( pdf->slowPutRec ) added in put1rec and putrec
 *    for we will force to write the record and use this flag
 *
 *
 * THIS MUST USE WITH DIOFILE.C
 ***************************************************************************/

#define RJ_SUPPORT

#ifdef RJ_SUPPORT
    #include "rjio.h"
    RJFILE   *rjFile = NULL;
#endif


#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <search.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <share.h>
#include <STDARG.H>

#ifdef __TURBOC__
#include <dir.h>
#include <alloc.h>
#else
#define MAXPATH         80
#include <malloc.h>
#include <sys\locking.h>
#endif

#define STD_EXTNAME     ".dbf"
#ifndef FLOAT_PRECISION
	#define FLOAT_PRECISION		2.2204460492503131e-016
#endif

#include "mistring.h"
#include "dio.h"
#include "wst2mt.h"
#include "btree.h"
#include "sql_dio.h"
#include "odbc_dio.h"
#include "xexp.h"
#include "dbtree.h"
#include "t_int64.h"

static double zero = 0.0;

char *GetDbtId(dFILE *df, char *start);
char *PutDbtId(dFILE *df, char *start);

/*
---------------------------------------------------------------------------
!                       BufFlush()
! return:
!     success: 0, fail not 0
---------------------------------------------------------------------------*/
unsigned short BufFlush( dFILE *df )
{
    long   read_start;
    int    read_len, i, rsize;
    long   p, beg, end, Lq;
    int    MinUsePointer;
    short  MinUseTimes;
    dBUFPOINTER *buf_pointer;

    p = df->rec_p;
    rsize = df->buf_rsize;
    buf_pointer = df->buf_pointer;

    if( df->buf_methd == 'S' ) {

	beg = df->rec_beg;
	end = df->rec_end;

	//old version:
	//if( ++( df->buf_usetimes ) > rsize + IniBufUseTimes ) {

	if( ++( df->buf_usetimes ) > dIOMaxUseRecord ) {
		df->buf_usetimes = IniBufUseTimes;
		df->buf_freshtimes = 1;
	}

	if( p >= beg && p <= end ) /*  part  C */
		return  0;

	if( (df->buf_usetimes / df->buf_freshtimes < 5) && (df->write_flag.append == 0) ) {
	    for( i = 0; i < rsize; i++ ) {
		    //if( beg + i <= df->rec_num )
		    if( beg + i <= end ) {
			df->buf_pointer[i].buf_write = 0;
			df->buf_pointer[i].times = 1;
			df->buf_pointer[i].rec_num = beg + i;
		    } else {
			df->buf_pointer[i].buf_write = 0;
			df->buf_pointer[i].times = 0;
			df->buf_pointer[i].rec_num = 0;
		    }
	    }
	    if( df->write_flag.buf_write == 1 ) {
		long  ll;

		ll = df->headlen + ( beg - 1 ) * df->rec_len;
		if( lseek ( df->fp, ll, SEEK_SET ) != ll ) {
		    return  0xFFFF;
		}

		if( ( dERROR = write ( df->fp, df->buf_sink, (end - beg + 1) * \
							df->rec_len ) ) == -1 )
			return  0xFFFF;
	    }
	    df->write_flag.buf_write = 0;
	    df->buf_methd = 'P';
	    df->min_recnum = beg;
	    df->max_recnum = end;

	    goto dIO_Pei;
	}

	//this time we want to append a record so we can move rec_end directly
	if( p >= beg && p - beg < (long)rsize && df->write_flag.append == 1 ) {
		if( end < beg )
		{ //the records between beg and p is not in memory
		   df->rec_beg = p;
		}
		df->rec_end = p;
		return  0;    // ( (short)(p - beg) );
	}
	read_start = p; /* if nether part B nor part D, read_start hold this values */

	/* when the buffer size is less than rsize/2, we needn't pay
	   attention to the Pointer-In-Buffer-Position. */
	if( p < beg && p >= beg - rsize/2 ) /*  part  B */
	    if ( ( read_start = beg - rsize/2 ) < 1 )
		read_start = 1;                 /* less than header */

	if ( p > end && p < end + rsize )           /*  part  D */
	    read_start = end + 1;

	if( df->write_flag.buf_write == 1 ) {         /* swap the buf */
		long  ll;

		if( ++( df->buf_freshtimes ) > rsize )
			df->buf_freshtimes = 1;

		ll = df->headlen + ( beg - 1 ) * df->rec_len;
		if( lseek( df->fp, ll, SEEK_SET ) != ll )
		{	return  0xFFFF;
		}

		if( ( dERROR = write ( df->fp, df->buf_sink, (end - beg + 1) * \
							df->rec_len ) ) == -1 )
		{       return  0xFFFF;
		}
		df -> write_flag.buf_write = 0;
	}

dIO_Shi:

	read_len = ( df->rec_num + 1 - read_start < (long)rsize ) ?     \
/*YES*/            ( df->rec_num + 1 - read_start ) :   \
/*NO*/             rsize;

	if( read_len == 0 ) {
		if( df->dIOReadWriteFlag == 1 ) {           /* now is write */
			df->rec_beg = df->rec_end = read_start;
			return  0;
		} /* ELSE now is read */
		return  0xFFFF;
	}
	if( ++( df->buf_freshtimes ) > rsize )  df->buf_freshtimes = 1;

	{
	    long  ll;

	    ll = df->headlen + ( read_start - 1 ) * df->rec_len;
	    if( lseek(df->fp, ll, SEEK_SET ) != ll )
		return  0xFFFF;
	}
	if( ( dERROR = read( df->fp, df->buf_sink, \
					  read_len * df->rec_len ) ) == -1 )
	{
		return  0xFFFF;
	}
	if( df->write_flag.append == 1 )
		df->rec_end = read_start + read_len;
	else
		df->rec_end = read_start + read_len - 1;
	df->rec_beg = read_start;

	return  0;

    }
//    else {
/* buf_methd='P' */

    if( (p<=df->max_recnum && p>=df->min_recnum && (df->max_recnum-df->min_recnum)<=rsize) || \
		df->write_flag.append ) {
			/* this algorithm is difficult to append record */

	if( df->write_flag.buf_write == 1 )
	{
	      for( i = 0; i < rsize; i++ )
		if( buf_pointer[i].times > 0 && buf_pointer[i].buf_write == 1 ) {
		    long  ll;

		    ll = df->headlen + (buf_pointer[i].rec_num\
						- 1 ) * df->rec_len;
		    if( lseek( df->fp, ll, SEEK_SET ) != ll )
			return  0xFFFF;

		    if( ( dERROR = write(df->fp, &(df->buf_sink[ i * df->rec_len ]), \
			df->rec_len ) ) == -1 )
		    {
			return  0xFFFF;
		    }
		}
	}
	df->write_flag.buf_write = 0;
	read_start = p;
	df->buf_methd = 'S';
	df->buf_freshtimes = 1;
	df->buf_usetimes = IniBufUseTimes;
	goto dIO_Shi;
    }

dIO_Pei:
    if( p <= df->max_recnum && p >= df->min_recnum )
    {
	    for( i = 0; i < rsize; i++ ) {
		if( buf_pointer[i].rec_num == p && \
				buf_pointer[i].times > 0 ) {

			df->rec_beg = p - i;

			if( buf_pointer[i].times >= dIOMaxUseRecord )
			{
			   int  j;
			   for( j = 0; j < rsize; j++ )
			       buf_pointer[j].times = buf_pointer[j].times / 3 + 1;
			}
			buf_pointer[i].times++;

 /* we shouldn't change the buf_flag from write to read, but we should
  * change it from read to write. */
			if( df->dIOReadWriteFlag == 1 ) {

				//1999.5.19
				df->write_flag.buf_write = 1;

				buf_pointer[i].buf_write = 1;
			}

			return  0;              //( (short)df->rec_beg );
		}
	    }
   }
/*ELSE
*/
    MinUseTimes = dIOMaxUseRecord;
    MinUsePointer = 0;
    for( i = 0; i < rsize; i++ ) {
	if( MinUseTimes > buf_pointer[i].times ) {
		MinUseTimes = buf_pointer[i].times;
		MinUsePointer = i;
	}
    }

    if( MinUseTimes >= dIOMaxUseRecord )
    {
	for( i = 0; i < rsize; i++ )
		buf_pointer[i].times = buf_pointer[i].times / 3 + 1;
    }
    buf_pointer[MinUsePointer].times++;

    if( buf_pointer[MinUsePointer].buf_write == 1 ) {
	long  ll;

	ll = df->headlen+(buf_pointer[MinUsePointer].rec_num \
			- 1 ) * df->rec_len;
	if( lseek(df->fp, ll, SEEK_SET ) != ll )
	    return  0xFFFF;

	if( ( dERROR = write(df->fp,&(df->buf_sink[ MinUsePointer * df->rec_len ]), \
			df->rec_len ) ) == -1 )
	{
		return  0xFFFF;
	}
    }

 /* we shouldn't change the buf_flag from write to read, but we should
  * change it from read to write. */
    if( df->dIOReadWriteFlag == 1 ) {

	//1999.5.19
	df->write_flag.buf_write = 1;

	buf_pointer[MinUsePointer].buf_write = 1;
    }

    //the old record
    Lq = buf_pointer[MinUsePointer].rec_num;
    if(  Lq >= df->max_recnum || Lq <= df->min_recnum || \
				p >= df->max_recnum || p <= df->min_recnum ) {
			/* Now I have to find new Max or Min */
		buf_pointer[MinUsePointer].rec_num = p;

		df->max_recnum = buf_pointer[0].rec_num;
		df->min_recnum = buf_pointer[0].rec_num;

		for( i = 1; i < rsize; i++ ) {

			if( buf_pointer[i].times > 0 ) {
			    if( buf_pointer[i].rec_num > df->max_recnum )
				df->max_recnum = buf_pointer[i].rec_num;
			    if( buf_pointer[i].rec_num < df->min_recnum )
				df->min_recnum = buf_pointer[i].rec_num;
			}
		}
    } else {
	buf_pointer[MinUsePointer].rec_num = p;
    }

    /*if( dIOReadWriteFlag == 0 )*/    /* if it wants to read */
    {
	long  ll;

	ll = df->headlen + ( p - 1 ) * df->rec_len;
	if( lseek(df->fp, ll, SEEK_SET) != ll )
	    return  0xFFFF;
    }

    //remember that the p points to the next record always
    //read the record, the record is exist, for 'P' dosn't support
    //append of record
    dERROR = read(df->fp, &(df->buf_sink[MinUsePointer * df->rec_len] ), \
								df->rec_len);

    df->rec_beg = p - MinUsePointer;


    if( dERROR == -1 )   return  0xFFFF;
    return  0;       //( (short)df->rec_beg );

} /* end of function BufFlush() */



/*
-----------------------------------------------------------------------------
	    deof()
----------------------------------------------------------------------------*/
short deof( dFILE *df )
{
    if( df->dbf_flag == SQL_TABLE ) {
	return  TsqlEOF(df);
    }

    if( df->op_flag == VIEWFLAG ) {
	dVIEW *dv = (dVIEW *)df;
	if(  dv->view != NULL  ) {
		return ( ( ( dv->view->rec_p ) - ( dv->view->rec_num ) >= 1 ) ? 1 : 0 );
	}
    }

    //1999.9.11
    if( df->pdf != NULL ) {
	df->rec_num = df->pdf->rec_num;
    }

    return ( ( ( df->rec_p ) - ( df->rec_num ) >= 1 ) ? 1 : 0 );

} // end of function deof()


/*
----------------------------------------------------------------------------
	    dtell()
----------------------------------------------------------------------------*/
long dtell( dFILE *df )
{
    if( df->op_flag == VIEWFLAG ) {
	dVIEW *dv = (dVIEW *)df;
	if(  dv->view != NULL  ) {
		return dv->view->rec_p;
	}
    }
    return  df->rec_p;
}

/*
-----------------------------------------------------------------------------
	    dseek()
----------------------------------------------------------------------------*/
long dseek( dFILE *df, long rec_offset, dSEEK_POS from_where )
{
    dVIEW *dv;

    if( df == NULL ) return  0;

    if( df->dbf_flag == ODBC_TABLE ) {
	return  0;
    }
    
    if( df->dbf_flag == SQL_TABLE ) {
	return  TsqlSeek(df, rec_offset, from_where);
    }


#ifdef WSToMT
    if( df->pdf != NULL ) {

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( df->pdf->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    df->rec_num = df->pdf->pdf->rec_num;
	} else {
	    df->rec_num = df->pdf->rec_num;
	}
    }
#endif

    dv = (dVIEW * )df;
    if( ( df->op_flag == VIEWFLAG ) && ( dv->view != NULL ) ) {

	dseek( dv->view, rec_offset, from_where );

	if( dv->view->rec_p <= dv->view->rec_num ) {
	    get1rec( dv->view );
	    GetField( dv->view, dv->RecnoFldid, &(df->rec_p) );
	} else {
	    df->rec_p = df->rec_num + 1;
	}
    } else {
	switch ( from_where ) {
	    case ( dSEEK_SET ): df->rec_p = rec_offset + 1;
			break;
	    case ( dSEEK_CUR ): df->rec_p += rec_offset;
			break;
	    case ( dSEEK_END ): df->rec_p = df->rec_num + rec_offset + 1;
			break;
	    default           : return  0L;
	}
    }

    if ( df->rec_p < 1 ) df->rec_p = 1;
    else {
	if ( df->rec_p > df->rec_num ) {
// Here we should not change the append flag. Xilong 1994.08.29
//              df->write_flag.append = 1;
		df->rec_p = df->rec_num + 1;
	}
// Here we should not change the append flag. Xilong 1994.08.29
//      else  df->write_flag.append = 0;
    }

    return( df->rec_p );

}

/*
----------------------------------------------------------------------------
!!                      getrec()
----------------------------------------------------------------------------*/
unsigned char *getrec( dFILE *df )
{
    dVIEW *dv;
    int   i;
#ifdef WSToMT
    dFILE *pdf;
#endif
    long l;

    if( df == NULL )    return  NULL;

    if( df->rec_p <= 0 ) {
	df->error = 2000;
	return  NULL;
    }

    if( df->dbf_flag == ODBC_TABLE ) {

	unsigned char *s;

	s = odbcGetRec(df);
	df->rec_p++;
	return  s;
    }

    if( df->dbf_flag == SQL_TABLE ) {

	unsigned char *s;

	s = TsqlGetRow(df);
	df->rec_p++;
	return  s;
    }

    if( deof( df ) )    return  NULL;

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
    } else
	pdf = df;

    EnterCriticalSection( &(pdf->dCriticalSection) );

    df->rec_num = pdf->rec_num;

    pdf->dIOReadWriteFlag = 0;       /* set read flag */

    pdf->rec_p = df->rec_p;
    if( BufFlush( pdf ) == 0xFFFF ) {
	 LeaveCriticalSection( &(pdf->dCriticalSection) );
	 return( NULL );
    }

    memcpy( df->rec_buf, \
	 &pdf->buf_sink[ (unsigned short)( pdf->rec_p - pdf->rec_beg ) * pdf->rec_len ], \
	 df->rec_len );
    LeaveCriticalSection( &(pdf->dCriticalSection) );
#else
    if( BufFlush( df ) == 0xFFFF )      return( NULL );

    memcpy( df->rec_buf, \
	 &df->buf_sink[ (unsigned short)( df->rec_p - df->rec_beg ) * df->rec_len ], \
	 df->rec_len );
#endif

    // reserved the record no
    df->rec_no = df->rec_p;

/*    df->rec_buf[df->rec_len] = '\0';
    When we alloc this buffer, we have already set the end 0, and we
    will never overlay write the end byte.                              */

    dv = (dVIEW *)df;
    if( (df->op_flag == VIEWFLAG) && dv->view != NULL ) {
	if( dv->view->rec_p > dv->view->rec_num ) {
		df->rec_p = df->rec_num + 1;
	} else {
		if( dv->view->field_num > 1 ) {
			getrec( dv->view );
			memcpy( dv->rec_buf, dv->view->rec_buf, dv->view->rec_len);
			l = dv->view->rec_p;

			if( l <= dv->view->rec_num ) {
				getrec( dv->view );
				GetField( dv->view, dv->RecnoFldid, &(df->rec_p) );
				memcpy( dv->view->rec_buf, dv->rec_buf, dv->view->rec_len);
			} else          df->rec_p = df->rec_num+1;
		} else {
			l = ++(dv->view->rec_p);
			if( l <= dv->view->rec_num ) {
				getrec( dv->view );
				GetField( dv->view, dv->RecnoFldid, &(df->rec_p) );
			} else  df->rec_p = df->rec_num+1;
		}
		dv->view->rec_p = l;      // mentation it
	}

	for(i = 0;  i < df->field_num;  i++)
	{ //clear the field not in use table
	    if( df->field[i].sdword < 0 )
		memset(df->field[i].fieldstart, 0, df->field[i].fieldlen);
	}

    } else      df->rec_p++;

    return( df->rec_buf );
}

/*
-----------------------------------------------------------------------------
!!                      putrec()
----------------------------------------------------------------------------*/
unsigned char *putrec( dFILE *df )
{
    dVIEW *dv;
#ifdef WSToMT
    dFILE *pdf;
#endif
    long l;
    char *sp;
    int  myOffset;
    int  overWrite;

    if( df->rec_p <= 0 ) {
	df->error = 2000;
	return  NULL;
    }

    if( df == NULL ) return  NULL;
    if( df->dbf_flag == SQL_TABLE )
    {
	if( df->rec_p > df->rec_num ) {
		df->rec_p = df->rec_num + 1;
		df->write_flag.append = 1;
		//return  sqlInsert(df);
	} else {
		df->write_flag.append = 0;
		//return  sqlUpdate(df);
	}
	return  NULL;
    }

    if( (df->oflag & 0xF) == O_RDONLY ) {
	df->error = 2000;
	return( NULL );
    }

    ///////////
    //check record
    if( df->isRecCheck ) {
	int  i;
	XEXP_ENV xenv;

	saveXexpEnv( &xenv );
	for(i = 0;  i < df->field_num;  i++) {
	    if( df->field[i].px != NULL ) {
		if( (short)CalExpr((MidCodeType *)df->field[i].px) == 0 ) {
			df->error = i+1;
			restoreXexpEnv( &xenv);
			return  NULL;
		}
	    }
	}
	restoreXexpEnv( &xenv);
    }

#ifdef WSToMT /*{*/
    if( df->pdf != NULL ) {
	pdf = df->pdf;
	//this will cause an error 1998.2.1 Xilong
	//
	//dseek(df, 0, dSEEK_END);
	//......
	//putrec()
	//
	//during this action, the rec_num, has been changed
	//df->rec_num = pdf->rec_num;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( pdf->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    pdf = pdf->pdf;
	}
    } else
	pdf = df;

    EnterCriticalSection( &(pdf->dCriticalSection) );

    pdf->dIOReadWriteFlag = 1;       /* set write flag */

    if( df->rec_p > df->rec_num ) {
	df->rec_p = pdf->rec_p = pdf->rec_num + 1;
	df->write_flag.append = 1;
	pdf->write_flag.append = 1;
    } else {
	df->rec_num = pdf->rec_num;
	pdf->rec_p = df->rec_p;
	df->write_flag.append = 0;
	pdf->write_flag.append = 0;
    }

    //1999/12/22
    // reserved the record no
    pdf->rec_no = df->rec_no = df->rec_p;

    if( BufFlush( pdf ) == 0xFFFF ) {
	 pdf->write_flag.append = 0;
	 LeaveCriticalSection( &(pdf->dCriticalSection) );
	 return( NULL );        // write error
    }

    /*this cause an error: 1998.2.28
      the rec_buf was kept by client users
    if( df->timeStamp != pdf->timeStamp && !(pdf->write_flag.append) && \
					    (df->dbf_flag & 0x83) == 0x83 )
    */

    myOffset = (pdf->rec_p-pdf->rec_beg) * pdf->rec_len;

    //1999/12/22
    //keep the old value in buf_sink into pdf->rec_buf
    sp = &pdf->buf_sink[myOffset];

    if( df != pdf && pdf->slowPutRec && !(pdf->write_flag.append) )
    { //2000.4.14 Xilong Pei
	memcpy(pdf->rec_buf, sp, pdf->rec_len);
    }

    //2000.10.14
    //if( pdf->buf_sink[myOffset] == '*' )
    if( *sp == '*' )
	overWrite = 1;
    else {
	overWrite = 0;
	if( pdf->write_flag.append == 0 ) {
	    if( (df->op_flag == VIEWFLAG) && ((dVIEW *)df)->view != NULL )
	    { //now operating a view

		int i;
		long    li;
		dFIELD *field;
		char *sp1 = df->rec_buf;

		field = df->field;
		for(i = 0;  i < df->field_num;  i++)
		{ //
		  //recall the field not in view with the table original
		  //content
		  //
		    if( df->field[i].sdword < 0 ) {
			li = field[i].fieldstart - sp1;
			memcpy(&sp1[li], &(sp[li]), field[i].fieldlen);
		    }
		}
	    }
	}
    }


#ifdef RJ_SUPPORT /*{*/
    if( df->wlog )
    { //write log--------------------

	short i;
	char buf[256];
	dFIELD *field;

	//lock the resource
	allocRjRes(rjFile);

	newRecord(rjFile);

	sprintf(buf, "%d", df->tbid);
	writeContent(rjFile, RJ_BASE, buf);

	sprintf(buf, "%d", df->mainkey_num);
	writeContent(rjFile, RJ_GRADE, buf);

	if( pdf->write_flag.append || pdf->buf_sink[myOffset] == '*' ) {
	    writeContent(rjFile, RJ_TYPE, szREC_ADD);
	} else {
	    writeContent(rjFile, RJ_TYPE, szREC_MODI);
	    for(i = 0;  i < df->mainkey_num;  i++) {

		//2000.2.12 XilongPei
		//field = &(df->field[i]);
		field = &(df->field[ df->mainkey_fields[i] ]);

		writeContent(rjFile, RJ_FIELD, field->field);
		writeContent(rjFile, RJ_DATA, rtrim(subcopy(field->fieldstart,0,field->fieldlen)));
	    }
	}
	writeContent(rjFile, RJ_OPERATOR, "U");

    } else { //don't write log--------------------

      //
      //////////////////////////
      //
      if( pdf->fldTimeStampId != 0xFFFF )
      { //write time stamp
	char *sz;
	char  buf[256];
	int   len;

	sz = &pdf->buf_sink[myOffset + pdf->fldTimeStampOffset];
	for( len = pdf->fldTimeStampLen, buf[len--] = '\0';  \
							len >= 0;  len-- ) {
	    buf[len] = sz[len];
	    sz[len] = ' ';
	}
	len = strlen(itoa(atoi(buf)+1, buf, 10));
	if( len > pdf->fldTimeStampLen ) {
	    *sz = '1';
	} else {
	    memcpy(sz, buf, len);
	}

	//t_u64toa(t_atoi64(sz)+1, &sp[pdf->fldTimeStampOffset]);
      }

      //if( pdf->fldTimeStampId != 0xFFFF || (df->dbf_flag & 0x83) == 0x83 )
      if( df->slowPutRec ) {
	short   i;
	long    li;
	dFIELD *field;
	//char   *sp = &pdf->buf_sink[myOffset];
	char   *sp1 = df->rec_buf;

	if( pdf->write_flag.append || overWrite ) {
	    memset(sp, ' ', pdf->rec_len );
	}

	field = df->field;
	for(i = 0;  i < df->field_num;  i++) {
	    if( i != df->fldTimeStampId && field[i].fieldtype != 'M' && field[i].fieldtype != 'G' ) {
		li = field[i].fieldstart - sp1;
		memcpy(&sp[li], &(sp1[li]), field[i].fieldlen);
	    }
	}
      } else {
	memcpy(&pdf->buf_sink[myOffset], df->rec_buf, pdf->rec_len);
	if( pdf->write_flag.append || overWrite ) {
	    pdf->buf_sink[myOffset] = ' ';
	}
      }

      goto   jmpOfPUTREC;

    }

    { //write the really data
	short   i;
	long    li;
	dFIELD *field;
	//char *sp = &pdf->buf_sink[myOffset];
	char *sp1 = df->rec_buf;

	if( pdf->write_flag.append || overWrite ) {
	    memset(sp, ' ', pdf->rec_len );
	}

	field = df->field;
	for(i = 0;  i < df->field_num;  i++) {
	    if( i != df->fldTimeStampId && field[i].fieldtype != 'M' && field[i].fieldtype != 'G' ) {
		li = field[i].fieldstart - sp1;
		if( strncmp(&sp[li], &(sp1[li]), field[i].fieldlen) != 0 )
		{ //write log
		    if( df->wlog ) {
			if( pdf->write_flag.append || pdf->buf_sink[(pdf->rec_p-pdf->rec_beg)*pdf->rec_len] == '*' ) {
			    writeContent(rjFile, RJ_FIELD, field[i].field );

			    //new value
			    writeContent(rjFile, RJ_DATA, rtrim(subcopy(&(sp1[li]),0,field[i].fieldlen)));
			} else {
			    writeContent(rjFile, RJ_FIELD, field[i].field );

			    //old value
			    writeContent(rjFile, RJ_DATA, rtrim(subcopy(&(sp[li]),0,field[i].fieldlen)));

			    //new value
			    writeContent(rjFile, RJ_DATA, rtrim(subcopy(&(sp1[li]),0,field[i].fieldlen)));
			}
		    }

		    memcpy(&sp[li], &(sp1[li]), field[i].fieldlen);
		}
	    }
	}
    } ////////////////////////////////////

    if( df->wlog ) {
	RJ_write(rjFile);
	freeRjRes(rjFile);
    }

jmpOfPUTREC:
#else

    if( (df->dbf_flag & 0x83) == 0x83 ) {
	short   i;
	long    li;
	dFIELD *field;
	//char *sp = &pdf->buf_sink[myOffset];
	char *sp1 = df->rec_buf;

	if( pdf->write_flag.append || overWrite ) {
	    memset(&pdf->buf_sink[myOffset], ' ', pdf->rec_len );
	}

	field = df->field;
	for(i = 0;  i < df->field_num;  i++) {
	    if( field[i].fieldtype != 'M' && field[i].fieldtype != 'G' ) {
		li = field[i].fieldstart - sp1;
		memcpy(&sp[li], &(sp1[li]), field[i].fieldlen);
	    }
	}
    } else {
	memcpy(&pdf->buf_sink[(pdf->rec_p-pdf->rec_beg) * pdf->rec_len],\
					       df->rec_buf, pdf->rec_len );
	if( pdf->write_flag.append || overWrite ) {
	    pdf->buf_sink[(pdf->rec_p-pdf->rec_beg) * pdf->rec_len] = ' ';
	}
    }

#endif /*}*/

    pdf->write_flag.buf_write = 1;
    pdf->write_flag.file_write = 1;

    if( pdf->syncBhNum > 0 || pdf->write_flag.append ) {
	int k;

	for(k = 0;  k < pdf->syncBhNum;  k++) {
		if( df->syncBhNeed[k] || pdf->write_flag.append ) {
			IndexSyncDbf(pdf->bhs[k], df->rec_buf);
			df->syncBhNeed[k] = '\0';
		}
		/* 1999/12/21
		if( pdf->syncBhNeed[k] || pdf->write_flag.append ) {
			IndexSyncDbf(pdf->bhs[k], df->rec_buf);
			pdf->syncBhNeed[k] = '\0';
		}*/
	}
    }

    //1998.2.20 Xilong
    if( pdf != df ) {
	if( pdf->write_flag.append /*== 1*/ ) {
	    pdf->rec_num++;

	    //2000.6.24
	    if( df->pdf != pdf )
		df->pdf->rec_num++;
	}
	pdf->write_flag.append = 0;
    }

    LeaveCriticalSection( &(pdf->dCriticalSection) );

#else
    df->dIOReadWriteFlag = 1;       /* set write flag */

    if( df->rec_p > df->rec_num ) {
	df->rec_p = df->rec_num + 1;
	df->write_flag.append = 1;
//      append_flag = 1;
    } else {
	df->write_flag.append = 0;
//      append_flag = 0;
    }


    if( BufFlush( df ) == 0xFFFF ) {
	 df->write_flag.append = 0;
	 return( NULL );        // write error
    }

    memcpy( &df->buf_sink[( df->rec_p - df->rec_beg ) * df->rec_len ],\
	 df->rec_buf, df->rec_len );
#endif /*}*/

    df->write_flag.buf_write = df->write_flag.file_write = 1;
    /* this buf_write flag is for all the buffer */

    //1999/12/22
    /*
    // reserved the record no
    df->rec_no = df->rec_p;
    */

    dv = (dVIEW *)df;
    if( ( df->op_flag == VIEWFLAG ) && dv->view != NULL ) {
	if( df->write_flag.append == 1 ) {

	    long p;

	    p = df->rec_num + 1;
	    dseek( dv->view, 0L, dSEEK_END );
	    PutField(dv->view, dv->RecnoFldid, &(p) );
	    putrec( dv->view );
	}

// change the line with following. 1994.12.23 Xilong
//      if( dv->view->rec_p <= dv->view->rec_num ) {
	if( df->write_flag.append == 0 ) {
	    if( dv->view->field_num > 1 ) {
		getrec( dv->view );
		memcpy( dv->rec_buf, dv->view->rec_buf, dv->view->rec_len);
		l = dv->view->rec_p;

		getrec( dv->view );
		GetField( dv->view, dv->RecnoFldid, &(df->rec_p) );

		memcpy( dv->view->rec_buf, dv->rec_buf, dv->view->rec_len);
	    } else {
		l = ++(dv->view->rec_p);
		getrec( dv->view );
		GetField( dv->view, dv->RecnoFldid, &(df->rec_p) );
	    }
	    dv->view->rec_p = l;      // mentation it
	} else  {
	    df->rec_p = df->rec_num + 2;
	}
    } else      df->rec_p++;

    if( df->write_flag.append == 1 ) {
	df->rec_num++;
    }

    df->write_flag.append = 0;
    return  df->rec_buf;

} // end of function putrec()


/*
----------------------------------------------------------------------------
!!                      GetDelChar()
----------------------------------------------------------------------------*/
unsigned char GetDelChar( dFILE *df )
{
#ifdef WSToMT
    dFILE 	   *pdf;
#endif
    unsigned char   c;

    if( df == NULL ) return  dNOT_OK;
    if( df->dbf_flag == SQL_TABLE ) {
	return  '\0';
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

    pdf->dIOReadWriteFlag = 0;       /* set write flag */
    BufFlush( pdf );

    c = pdf->buf_sink[(unsigned short)( pdf->rec_p - pdf->rec_beg ) * pdf->rec_len];
    LeaveCriticalSection( &(pdf->dCriticalSection) );
#else
    df->dIOReadWriteFlag = 0;       /* set write flag */
    BufFlush( df );
    c = pdf->buf_sink[(unsigned short)( pdf->rec_p - pdf->rec_beg ) * pdf->rec_len];
#endif
    return  c;
} //end of GetDelChar()

/*
----------------------------------------------------------------------------
!!                      PutDelChar()
----------------------------------------------------------------------------*/
short PutDelChar( dFILE *df, unsigned char DelChar )
{
    short  isRecCheck, wlog, syncBhNum;
#ifdef WSToMT
    dFILE *pdf;
#endif

    if( df == NULL )    return  1;
    if( df->dbf_flag == SQL_TABLE ) {
	return  '\0';
    }

    if( (df->oflag & 0xF) == O_RDONLY ) {
	dERROR = 2000;
	return( 2000 );
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

    pdf->rec_p = df->rec_p;

    if( get1rec(pdf) == NULL ) {
	LeaveCriticalSection( &(pdf->dCriticalSection) );
	dERROR = 2000;
	return( 2000 );
    }

#ifdef RJ_SUPPORT
    if( df->wlog ) {

	short i;
	char buf[256];
	dFIELD *field;

	//lock the resource
	allocRjRes(rjFile);

	newRecord(rjFile);

	sprintf(buf, "%d", df->tbid);
	writeContent(rjFile, RJ_BASE, buf);

	sprintf(buf, "%d", df->mainkey_num);
	writeContent(rjFile, RJ_GRADE, buf);

	writeContent(rjFile, RJ_TYPE, szREC_DEL);

	writeContent(rjFile, RJ_OPERATOR, "U");

	for(i = 0;  i < df->field_num;  i++) {
	    field = &(df->field[i]);
	    writeContent(rjFile, RJ_FIELD, field->field);
	    writeContent(rjFile, RJ_DATA, rtrim(subcopy(field->fieldstart,0,field->fieldlen)));
	}
    }
#endif

    if( pdf->del_link_id != 0xFFFF ) {
	*(long *)(pdf->field[pdf->del_link_id].fieldstart) = pdf->firstDel;
	pdf->firstDel = pdf->rec_p;
    } else {
	if( pdf->rec_p < pdf->firstDel )
		pdf->firstDel = pdf->rec_p;
    }

    //{ BEGIN//
    isRecCheck = pdf->isRecCheck;
    wlog = pdf->wlog;
    syncBhNum = pdf->syncBhNum;

    pdf->isRecCheck = 0;
    pdf->wlog = 0;
    pdf->syncBhNum = 0;

    if( put1rec(pdf) == NULL ) {
	LeaveCriticalSection( &(pdf->dCriticalSection) );
	dERROR = 2000;
	return( 2000 );
    }

    pdf->isRecCheck = isRecCheck;
    pdf->wlog = wlog;
    pdf->syncBhNum = syncBhNum;
    //} END//

    //the delchar won't be writen by put1rec
    pdf->buf_sink[(pdf->rec_p-pdf->rec_beg) * pdf->rec_len] = DelChar;

    /*this algorithm haven't deal with buf_swap_method='P' 1998.2.1
    pdf->dIOReadWriteFlag = 1;
    BufFlush( pdf );
    pdf->write_flag.buf_write = 1;
    pdf->write_flag.file_write = 1;
    df->write_flag.buf_write = 1;
    df->write_flag.file_write = 1;


    pdf->buf_sink[ (unsigned short)( pdf->rec_p - pdf->rec_beg ) * pdf->rec_len ] = DelChar;
   */
    LeaveCriticalSection( &(pdf->dCriticalSection) );
#else
    df->dIOReadWriteFlag = 1;       /* set write flag */
    BufFlush( df );
    get1rec(df);

    put1rec(df);
    //the delchar won't be writen by put1rec sometime
    pdf->buf_sink[(pdf->rec_p-pdf->rec_beg) * pdf->rec_len] = DelChar;

    df->write_flag.buf_write = df->write_flag.file_write = 1;

    if( df->rec_p < df->firstDel || df->firstDel == LONG_MAX )
	df->firstDel = df->rec_p;

#endif

    // if it is a view write it to the dbf alos
    if( ( df->op_flag == VIEWFLAG ) && ( ((dVIEW *)df)->view != NULL ) ) {
	PutDelChar( ((dVIEW *)df)->view, DelChar );
    }

    return( 0 );
}


/*
------------------------------------------------------------------------------
			get_fld()
----------------------------------------------------------------------------*/

char *get_fld( dFILE *df, unsigned short name, char *dest )
{

    dFIELD *field;

    if( name >= df->field_num ) {
	df->error = 3001;
	return( NULL );
    }

    field = &(df->field[df->fld_id[name]]);
    memcpy( dest, field->fieldstart, field->fieldlen );
    dest[ field->fieldlen ] = '\0';
    if( field->fieldtype != 'N' && field->fieldtype != 'D' ) {
	rtrim( dest );
    }

    return ( dest );
} /* End of Function get_fld() */

/*
------------------------------------------------------------------------------
			put_fld()
----------------------------------------------------------------------------*/

short put_fld( dFILE *df, unsigned short name, char *src )
{
    int           len, len_temp;
    unsigned char *start;
    short          fld_id;

    if( name >= df->field_num ) {
	df->error = 3001;
	return( 1 );
    }

    fld_id = df->fld_id[name];
    len = df->field[fld_id].fieldlen;
    start = df->field[fld_id].fieldstart;
    if( df->syncBhNum > 0 ) {
      int i, k;

      for(k = 0;   k < df->syncBhNum;   k++) {
	bHEAD *bh = df->bhs[k];
	for(i = 0;  i < bh->keyFieldNum;  i++) {
            if( fld_id == bh->KeyFldid[i] ) {
                df->syncBhNeed[k] = '\1';
                break;
            }
	}
      }
    }

    if( df->field[fld_id].fieldtype != 'N' ) {
	/*change this design 1998.1.18 Xilong
	// left adjust
	if( len > len_temp ) {
		memset( &(start[ len_temp ]), ' ', len-len_temp );
		len = len_temp;
	}
	memcpy( start, (unsigned char *)src, len );
	*/
	strncpy(start, (unsigned char *)src, len);
    } else {
	// right adjust
	len_temp = strlen( (char *)src );
	if( len > len_temp ) {
		memset(start , ' ', len-len_temp );
		memcpy( &(start[ len - len_temp ]), (unsigned char *)src, len_temp );
	} else {
		memcpy( start, (unsigned char *)src, len );
	}
    }

    return( 0 );
}


/*
------------------------------------------------------------------------------
			GetField()
//?if value<=30000 return the short pointer else long pointer
Caution:
    dest should has a byte additional space to  hold the '\0' when the field
    type is 'M'
----------------------------------------------------------------------------*/

void *GetField( dFILE *df, unsigned short name, void *dest )
{
    unsigned char *start;
    long  	   len;
    long 	   dbt_block;
    dFIELD 	  *field;

    if( name >= df->field_num ) {
	df->error = 3001;
	return( NULL );
    }

    field = &(df->field[df->fld_id[name]]);
    start = field->fieldstart;
    len = field->fieldlen;

    switch( field->fieldtype ) {
	case 'C':
	case 'L':
		memcpy( (char *)dest, start, len );
		((char *)dest)[ len ]   = '\0';
		rtrim( dest );
		break;
	case 'N':
		if( field->fielddec == 0 ) {
#ifndef WSToMT
		    if( field->fieldlen < 5 )
			*(short *)dest = atoi( subcopy((char *)start, 0, len) );
		    else
#endif
			*(long *)dest = atol( subcopy((char *)start, 0, len) );
		} else
			*(double *)dest = atof(subcopy((char *)start,0, len));
		break;
	case 'D':
		memcpy( (char *)dest, start, len );
		((char *)dest)[ len ]   = '\0';
		break;
	case 'V':                       /* void type */
	case 'O':                       /* Overlay DBF type */
		memcpy( (char *)dest, start, len );
		break;
	case 'M':
	case 'G':
#ifdef WSToMT
		wmtDbfLock(df);
		GetDbtId(df, start);
#endif
		dbt_block = atol( subcopy((char *)start, 0, len) );
		df->dbt_len = 0;
		df->dbt_p = 1;
		if( dbt_block <= 0 ) {
#ifdef WSToMT
		    wmtDbfUnLock(df);
#endif
		    //1999.5.29
		    if( (void *)dest == NULL ) {
			*(unsigned char *)df->dbt_buf = '\0';
		    } else {
			*(unsigned char *)dest = '\0';
		    }

		    return  NULL;
		}

		if( (void *)dest == NULL ) {

			if( df->cdbtIsGDC )
			{ //DBT in G_D_C

			    readBtreeData((bHEAD *)(df->dp), (char *)&dbt_block, df->dbt_buf, DBTBLOCKSIZE);

			} else {
			    lseek( df->dp, dbt_block*DBTBLOCKSIZE, SEEK_SET );
			    DbtRead( df );
			}

			/* if the first two char is "N\0", the memo is not ASCII
			 * string */
			/*if( (*(dBITMEMO *)df->dbt_buf).MemoMark == dBITMEMOMemoMark ) {
				df->dbt_len = (unsigned char)(*(dBITMEMO *)df->dbt_buf).MemoLen;
			}
			*/dest = (void *)df->dbt_buf;

		} else {
			if( df->cdbtIsGDC )
			{ //DBT in G_D_C

			    readBtreeData((bHEAD *)(df->dp), (char *)&dbt_block, dest, \
					  (int)*(unsigned char *)dest * \
					  DBTBLOCKSIZE);

			} else {
			    DBTBLOCK *dblock = (DBTBLOCK *)df->dbt_buf;

			    start = (unsigned char *)dest;
			    lseek( df->dp, dbt_block*DBTBLOCKSIZE, SEEK_SET );
			    for(len = *(unsigned char *)dest;  len > 0;  len--) {
				if( DbtRead( df ) == NULL ) {
					break;
				}
				
				if( (len == *(unsigned char *)dest) && \
				    (*(dBITMEMO *)(df->dbt_buf)).MemoMark == dBITMEMOMemoMark ) {
				    memcpy(start, df->dbt_buf+sizeof(dBITMEMO), DBT_DATASIZE);
				    start += (DBT_DATASIZE - sizeof(dBITMEMO));
				} else {
				    memcpy(start, df->dbt_buf, DBT_DATASIZE);
				    start += DBT_DATASIZE;
				}

				if( dblock->next <= 0 ) {
				    //start += DBT_DATASIZE;
				    break;
				} else {
				    lseek(df->dp, dblock->next*DBTBLOCKSIZE, SEEK_SET);
				}
				//start += DBT_DATASIZE;
			    }
			    *start = '\0';
			}
		} /* end of else */
#ifdef WSToMT
		wmtDbfUnLock(df);
#endif

    }

    return( dest );

} /* End of Function */

/*
-----------------------------------------------------------------------------
			PutField()
----------------------------------------------------------------------------*/

short PutField( dFILE *df, unsigned short name, void *src )
{
  unsigned char *start;
  long     	len, len_temp;
  unsigned char *buf, float_buf[dMAXDECLEN+1];
  int   	decimal, sign;
  short 	fld_id;

  if( name >= df->field_num ) {
	df->error = 3001;
	return  1;
  }

  fld_id = df->fld_id[name];
  start = df->field[fld_id].fieldstart;
  len = df->field[fld_id].fieldlen;

  if( df->syncBhNum > 0 ) {
      int i, k;
      for(k = 0;   k < df->syncBhNum;   k++) {
	bHEAD *bh = df->bhs[k];
	for(i = 0;  i < bh->keyFieldNum;  i++) {
            if( fld_id == bh->KeyFldid[i] ) {
                df->syncBhNeed[k] = '\1';
                break;
	    }
	}
      }
  }

  switch( df->field[fld_id].fieldtype ) {
	case 'C':
	case 'D':
	case 'L':
		strncpy(start, (unsigned char *)src, len);
		/*change this design 1998.1.18 Xilong
		if( src == NULL )       len_temp = 0;
		else                    len_temp = strlen( (char *)src );
		if( len > len_temp ) {
			memset( &(start[ len_temp ]), ' ', len-len_temp );
			len = len_temp;
		}
		memcpy( start, (unsigned char *)src, len );
		*/
		break;
	case 'V':                       /* void type */
	case 'O':                       /* Overlay DBF type */
		memcpy( start, (void *)src, len );
		break;
	case 'N':
		if( src == NULL )       src = &zero;
		if( df->field[fld_id].fielddec == 0 ) {
#ifndef WSToMT
		    if( df->field[fld_id].fieldlen < 5 )
			decimal = strlen( itoa(*(short *)src, (char *)float_buf, 10) );
		    else
#endif
			decimal = strlen( ltoa(*(long *)src, (char *)float_buf, 10) );
		} else {

			unsigned char float_buf2[dMAXDECLEN+1];

			buf = (unsigned char *)fcvt( *(double *)src, \
				(short)df->field[fld_id].fielddec, \
				&decimal, &sign );

			//1999.11.10 Xilong Pei
			if( decimal <= -19 )
			{
				src = &zero;
				decimal = 0;
				sign = 0;
			}


			/* insert the decimal point */
			if( decimal <= 0 ) {
			      decimal = 1-decimal;
			      memset(float_buf2, '0', decimal);
			      float_buf2[decimal] = '\0';
			      strcat(float_buf2, buf);
			      decimal = 1;
			      buf = float_buf2;
			}
			if( sign != 0 ) {
				float_buf[0] = '-';
				memcpy( &float_buf[1], buf, decimal);
				decimal++;
				float_buf[decimal] = '.';
				decimal++;
				memcpy( &float_buf[decimal], &buf[decimal-2], \
					(short)df->field[fld_id].fielddec );
				float_buf[ decimal + \
					(unsigned char)df->field[fld_id].fielddec ] = '\0';
			} else {
				memcpy( float_buf, buf, decimal);
				float_buf[decimal] = '.';
				decimal++;
				memmove( &float_buf[decimal], &buf[decimal-1], \
					(short)df->field[fld_id].fielddec );
				float_buf[ decimal + \
					(unsigned char)df->field[fld_id].fielddec ] = '\0';
			}
		}
		memset( start, ' ', len );

		if( decimal + df->field[fld_id].fielddec > \
				     (int)df->field[fld_id].fieldlen ) {
			memcpy( start, float_buf, \
				      df->field[fld_id].fieldlen );
		} else {
			memcpy( &(start[ len - decimal - \
			     df->field[fld_id].fielddec]), float_buf, \
			     decimal + df->field[fld_id].fielddec);
		}
		break;
	case 'M':
	case 'G':
	{

		long memo_start;

		df->dbt_p = 1;

#ifdef WSToMT
		wmtDbfLock(df);
#endif
		//treat every memo as new ? why? jut a test
		//memo_start = 0;

		/* if the write buffer isn't the default buffer
		 * user can exterened the memo record size
		 */
		 if( (*(dBITMEMO *)src).MemoMark == dBITMEMOMemoMark ) {
		     if( (*(dBITMEMO *)src).MemoLen <= 0 )
			len = 0;
		     else
			len = (*(dBITMEMO *)src).MemoLen + sizeof(dBITMEMO);

		 } else {
		     if( *(char *)src == '\0' )
			len = 0;
		     else
			len = strlen( src )+1;
		 }
		 /*else {
		     // set the end mark: EOF if necessary
		     ((char *)src)[len = strlen( src )] = '\x1A';
		 }*/

		 memo_start = atol(subcopy(start, 0, 10));

		 //1999.5.29
		 if( len <= 0 )
		 {
		    if( memo_start > 0 ) {
			PhyDbtDelete(df, memo_start);
		    }
		    goto SKIP_DIO_PUTFIELD;
		 }

		 if( df->cdbtIsGDC ) {

		    /* if the memo field has body delete it first, the GDC will do it itself
		    */

		    long  id, idKey;

		    if( (*(dBITMEMO *)src).MemoMark == dBITMEMOMemoMark ) {
			src = (char *)src + sizeof(dBITMEMO);
		    }

		    if( (id = atol(subcopy(start,0,10))) <= 0 ) {
			idKey = LONG_MAX;
			readBtreeData((bHEAD *)(df->dp), (char *)&idKey, \
						(char *)&id, sizeof(long));
			id++;
			writeBtreeData((bHEAD *)(df->dp), (char *)&idKey, \
						(char *)&id, sizeof(long));
		    }

		    writeBtreeData((bHEAD *)(df->dp), (char *)&id, src, len);
		    sprintf(start, "%9ld", id);
		    if( PutDbtId(df, start) == NULL ) {
			PhyDbtDelete(df, memo_start);
			return  1;
		    }

		 } else {

		     
		    /* if the memo field has body delete it first
		    */
		    if( memo_start > 0 ) {
			PhyDbtDelete(df, memo_start);
			memo_start = 0;
		    }

		    df->dbt_len = 0;

		    for(len_temp = 0;   len_temp < len;  len_temp += DBT_DATASIZE) {
		      memo_start = allocDbtBlock(df, memo_start);
		      if( df->dbt_len == 0 ) {
			  /* write them to the record buffer
			     this algorithm does not suit for Foxpro
			     ltoa( memo_start, (char *)float_buf, 10 );
			     strncpy( (char *)start, (char *)float_buf, 10 );
			  */
			  sprintf(start, "%9ld", memo_start);
			  if( PutDbtId(df, start) == NULL ) {
				PhyDbtDelete(df, memo_start);
				return  1;
			  }
		      }
		      write( df->dp, (char *)src+len_temp, \
				       min(DBT_DATASIZE, len-len_temp) );
		      (df->dbt_len)++;
		    }
		 }

		 //((char *)src)[len] = '\0';

/* the record buffer needn't write here, for user have to write the record
   buffer after write field buffer
		df->rec_p--;
		putrec( df );
*/

SKIP_DIO_PUTFIELD:

#ifdef WSToMT
		wmtDbfUnLock(df);
#endif
		if( df->pdf == NULL )
		    df->write_flag.dbt_write = 1;
		else
		    df->pdf->write_flag.dbt_write = 1;
	}
  }

  return  0;

}


/*
----------------------------------------------------------------------------
!!                      get1rec()
!! dont increace rec_p
----------------------------------------------------------------------------*/
unsigned char *get1rec( dFILE *df )
{
    int    i;
#ifdef WSToMT
    dFILE *pdf;
#endif

    if( df == NULL )    return  NULL;

    if( df->rec_p <= 0 ) {
	df->error = 2000;
	return  NULL;
    }

    if( df->dbf_flag == ODBC_TABLE ) {
	return  odbcGetRec(df);
    }
    if( df->dbf_flag == SQL_TABLE ) {
	return  TsqlGetRow(df);
    }

    if( deof( df ) )    return  NULL;


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

    pdf->dIOReadWriteFlag = 0;       // set read flag

    pdf->rec_p = df->rec_p;

    if( BufFlush( pdf ) == 0xFFFF ) {
	 LeaveCriticalSection( &(pdf->dCriticalSection) );
	 return( NULL );
    }

    memcpy( df->rec_buf, \
	     &pdf->buf_sink[( pdf->rec_p - pdf->rec_beg ) * pdf->rec_len ], \
	     df->rec_len );

    LeaveCriticalSection( &(pdf->dCriticalSection) );
#else
    if( BufFlush( df ) == 0xFFFF )      return  NULL;

    memcpy( df->rec_buf, \
	 &df->buf_sink[ ( df->rec_p - df->rec_beg ) * df->rec_len ], \
	 df->rec_len );
#endif
    // reserved the record no
    df->rec_no = df->rec_p;
    /*
    df->rec_no = df->rec_p;
    lseek(df->fp, df->headlen+(df->rec_p - 1)*df->rec_len, SEEK_SET );
    if( ( dERROR = read( df->fp, df->rec_buf, df->rec_len ) ) == -1 )
    {
	return  NULL;
    }
    */

    //1998.7.31
    if( (df->op_flag == VIEWFLAG) && ((dVIEW *)df)->view != NULL ) {
	for(i = 0;  i < df->field_num;  i++)
	{ //clear the field not in use table
	    if( df->field[i].sdword < 0 )
		memset(df->field[i].fieldstart, 0, df->field[i].fieldlen);
	}
    }

    return  df->rec_buf;

} // end of get1rec()


/*
-----------------------------------------------------------------------------
!!                      put1rec()
!!  check
!!  put data into buf_sink
!!  sync the index
!!
!!
----------------------------------------------------------------------------*/
unsigned char *put1rec( dFILE *df )
{
//    char append_flag;
    dVIEW *dv;
#ifdef WSToMT
    dFILE *pdf;
#endif
    char *sp;
    int myOffset;
    int overWrite;

    if( df == NULL )    return  NULL;

    if( df->rec_p <= 0 ) {
	df->error = 2000;
	return  NULL;
    }

    if( df->dbf_flag == SQL_TABLE ) {
	return  NULL;
    }

    if( (df->oflag & 0xF) == O_RDONLY ) {
	df->error = 2000;
	return  NULL;
    }

    ///////////
    //check record
    if( df->isRecCheck ) {
	int  i;
	XEXP_ENV xenv;

	saveXexpEnv( &xenv );
	for(i = 0;  i < df->field_num;  i++) {
	    if( df->field[i].px != NULL ) {
		if( (short)CalExpr((MidCodeType *)df->field[i].px) == 0 ) {
			df->error = i+1;
			restoreXexpEnv( &xenv);
			return  NULL;
		}
	    }
	}
	restoreXexpEnv( &xenv);
    }

#ifdef WSToMT
    if( df->pdf != NULL ) {
	pdf = df->pdf;
	//this will cause an error 1998.2.1 Xilong
	//
	//dseek(df, 0, dSEEK_END);
	//......
	//putrec()
	//
	//during this action, the rec_num, has been changed
	//df->rec_num = pdf->rec_num;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( pdf->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    pdf = pdf->pdf;
	}
    } else
	pdf = df;

    EnterCriticalSection( &(pdf->dCriticalSection) );

    pdf->dIOReadWriteFlag = 1;       /* set write flag */

    if( df->rec_p > df->rec_num ) {
	df->rec_p = pdf->rec_p = pdf->rec_num + 1;
	df->write_flag.append = 1;
	pdf->write_flag.append = 1;
    } else {
	df->rec_num = pdf->rec_num;
	pdf->rec_p = df->rec_p;
	df->write_flag.append = 0;
	pdf->write_flag.append = 0;
    }

    //1999/12/22
    // reserved the record no
    pdf->rec_no = df->rec_no = df->rec_p;

    if( BufFlush( pdf ) == 0xFFFF ) {
	 pdf->write_flag.append = 0;
	 LeaveCriticalSection( &(pdf->dCriticalSection) );
	 return( NULL );        // write error
    }

    /*this cause an error: 1998.2.28
      the rec_buf us kept by client users
    if( df->timeStamp != pdf->timeStamp && !(pdf->write_flag.append) && \
					    (df->dbf_flag & 0x83) == 0x83 ) {
    */

    myOffset = (pdf->rec_p-pdf->rec_beg)*pdf->rec_len;

    //1999/12/22
    //keep the old value in buf_sink into pdf->rec_buf
    sp = &pdf->buf_sink[myOffset];
    
    if( df != pdf && pdf->slowPutRec && !(pdf->write_flag.append) )
    { //2000.4.14 Xilong Pei
	memcpy(pdf->rec_buf, sp, pdf->rec_len);
    }

    //2000.10.14
    //if( pdf->buf_sink[myOffset] == '*' )
    if( *sp == '*' )
	overWrite = 1;
    else {
	overWrite = 0;
	if( pdf->write_flag.append == 0 ) {
	    if( (df->op_flag == VIEWFLAG) && ((dVIEW *)df)->view != NULL )
	    { //now operating a view

		int i;
		long    li;
		dFIELD *field;
		char *sp1 = df->rec_buf;

		field = df->field;
		for(i = 0;  i < df->field_num;  i++)
		{ //
		  //recall the field not in view with the table original
		  //content
		  //
		    if( df->field[i].sdword < 0 ) {
			li = field[i].fieldstart - sp1;
			memcpy(&sp1[li], &(sp[li]), field[i].fieldlen);
		    }
		}
	    }
	}
    }


#ifdef RJ_SUPPORT
    if( df->wlog )
    { //write log---------------

	short i;
	char buf[256];
	dFIELD *field;

	//lock the resource
	allocRjRes(rjFile);

	newRecord(rjFile);

	sprintf(buf, "%d", df->tbid);
	writeContent(rjFile, RJ_BASE, buf);

	sprintf(buf, "%d", df->mainkey_num);
	writeContent(rjFile, RJ_GRADE, buf);

	if( pdf->write_flag.append || overWrite ) {
	    writeContent(rjFile, RJ_TYPE, szREC_ADD);
	} else {
	    writeContent(rjFile, RJ_TYPE, szREC_MODI);
	    for(i = 0;  i < df->mainkey_num;  i++) {

		//2000.2.12 XilongPei
		//field = &(df->field[i]);
		field = &(df->field[ df->mainkey_fields[i] ]);

		writeContent(rjFile, RJ_FIELD, field->field);
		writeContent(rjFile, RJ_DATA, rtrim(subcopy(field->fieldstart,0,field->fieldlen)));
	    }
	}
	writeContent(rjFile, RJ_OPERATOR, "U");

    } else { //don't write log-------------


      //
      //////////////////////////
      //
      if( pdf->fldTimeStampId != 0xFFFF )
      { //write time stamp
	char *sz;
	char  buf[256];
	int   len;

	sz = &pdf->buf_sink[myOffset + pdf->fldTimeStampOffset];
	for( len = pdf->fldTimeStampLen, buf[len--] = '\0';  \
							len >= 0;  len-- ) {
	    buf[len] = sz[len];
	    sz[len] = ' ';
	}
	len = strlen(itoa(atoi(buf)+1, buf, 10));
	if( len > pdf->fldTimeStampLen ) {
	    *sz = '1';
	} else {
	    memcpy(sz, buf, len);
	}

	//t_u64toa(t_atoi64(sz)+1, &sp[pdf->fldTimeStampOffset]);
      }

      if( df->slowPutRec ) {
	short   i;
	long    li;
	dFIELD *field;
	//char *sp = &pdf->buf_sink[myOffset];
	char *sp1 = df->rec_buf;

	if( pdf->write_flag.append || overWrite ) {
	    memset(sp, ' ', pdf->rec_len );
	}

	field = df->field;
	for(i = 0;  i < df->field_num;  i++) {
	    if( i != df->fldTimeStampId && field[i].fieldtype != 'M' && field[i].fieldtype != 'G' ) {
		li = field[i].fieldstart - sp1;
		memcpy(&sp[li], &(sp1[li]), field[i].fieldlen);
	    }
	}
      } else {
	memcpy(&pdf->buf_sink[myOffset], df->rec_buf, pdf->rec_len );
	if( pdf->write_flag.append || overWrite ) {
	    pdf->buf_sink[myOffset] = ' ';
	}
      }

      goto   jmpOfPUTREC;

    }

    { //write the log of this record
      //______________________________________________________________
	short   i;
	long    li;
	dFIELD *field;
	//char *sp = &pdf->buf_sink[myOffset];
	char *sp1 = df->rec_buf;

	if( pdf->write_flag.append || overWrite ) {
	    memset(sp, ' ', pdf->rec_len );
	}

	field = df->field;
	for(i = 0;  i < df->field_num;  i++) {
	    if( i != df->fldTimeStampId && field[i].fieldtype != 'M' && field[i].fieldtype != 'G' ) {
		li = field[i].fieldstart - sp1;
		if( strncmp(&sp[li], &(sp1[li]), field[i].fieldlen) != 0 )
		{ //write log
		    if( df->wlog ) {
			if( pdf->write_flag.append || overWrite ) {
			    writeContent(rjFile, RJ_FIELD, field[i].field );

			    //new value
			    writeContent(rjFile, RJ_DATA, rtrim(subcopy(&(sp1[li]),0,field[i].fieldlen)));
			} else {
			    writeContent(rjFile, RJ_FIELD, field[i].field );

			    //old value
			    writeContent(rjFile, RJ_DATA, rtrim(subcopy(&(sp[li]),0,field[i].fieldlen)));

			    //new value
			    writeContent(rjFile, RJ_DATA, rtrim(subcopy(&(sp1[li]),0,field[i].fieldlen)));
			}
		    }

		    memcpy(&sp[li], &(sp1[li]), field[i].fieldlen);
		}
	    }
	}
    } ////////////////////////////////////

    if( df->wlog ) {
	RJ_write(rjFile);
	freeRjRes(rjFile);
    }

jmpOfPUTREC:
#else
    if( (df->dbf_flag & 0x83) == 0x83 ) {
	short   i;
	long    li;
	dFIELD *field;
	//char *sp = &pdf->buf_sink[myOffset];
	char *sp1 = df->rec_buf;

	if( pdf->write_flag.append || overWrite ) {
	    memset(sp, ' ', pdf->rec_len);
	}

	field = df->field;
	for(i = 0;  i < df->field_num;  i++) {
	    if( field[i].fieldtype != 'M' && field[i].fieldtype != 'G' ) {
		li = field[i].fieldstart - sp1;
		memcpy(&sp[li], &(sp1[li]), field[i].fieldlen);
	    }
	}
    } else {
	memcpy(&pdf->buf_sink[sp], df->rec_buf, pdf->rec_len);
    }
#endif

    pdf->write_flag.buf_write = 1;
    pdf->write_flag.file_write = 1;

    if( pdf->syncBhNum > 0 || pdf->write_flag.append ) {
	int k;

	for(k = 0;  k < pdf->syncBhNum;  k++) {
		if( df->syncBhNeed[k] || pdf->write_flag.append ) {
			IndexSyncDbf(pdf->bhs[k], df->rec_buf);
			df->syncBhNeed[k] = '\0';
		}
		/*1999/12/21
		if( pdf->syncBhNeed[k] || pdf->write_flag.append ) {
			IndexSyncDbf(pdf->bhs[k], df->rec_buf);
			pdf->syncBhNeed[k] = '\0';
		}*/
	}
    }

    //1998.2.20 Xilong
    if( pdf != df ) {
	if( pdf->write_flag.append /*== 1*/ ) {
	    pdf->rec_num++;
	    
	    //2000.6.24
	    if( df->pdf != pdf )
		df->pdf->rec_num++;
	}
	pdf->write_flag.append = 0;
    }

    LeaveCriticalSection( &(pdf->dCriticalSection) );

#else
    df->dIOReadWriteFlag = 1;       /* set write flag */

    if( df->rec_p > df->rec_num ) {
	df->rec_p = df->rec_num + 1;
	df->write_flag.append = 1;
//      append_flag = 1;
    } else {
	df->write_flag.append = 0;
//      append_flag = 0;
    }

    if( BufFlush(df) == 0xFFFF ) {
	 df->write_flag.append = 0;
	 return  NULL;  //* write error
    }

    memcpy( &df->buf_sink[myOffset ], df->rec_buf, df->rec_len );
#endif

    df->write_flag.buf_write = df->write_flag.file_write = 1;
    //* this buf_write flag is for all the buffer

    /*1999/12/22
    // reserved the record no
    df->rec_no = df->rec_p;
    */

    dv = (dVIEW *)df;
    if( ( df->op_flag == VIEWFLAG ) && dv->view != NULL ) {
	if( df->write_flag.append == 1 ) {

	    long p;

	    p = df->rec_num + 1;
	    dseek( dv->view, 0L, dSEEK_END );
	    PutField(dv->view, dv->RecnoFldid, &(p) );
	    putrec( dv->view );
	}
    }

    if( df->write_flag.append /*== 1*/ ) {
	df->rec_num++;
    }

    df->write_flag.append = 0;

    return  df->rec_buf;

} // end of function put1rec()


/*
----------------------------------------------------------------------------
!!                      GetDbtId()
----------------------------------------------------------------------------*/
char *GetDbtId(dFILE *df, char *start)
{
#ifdef WSToMT
    dFILE 	*pdf;
#endif

    //inside use, needn't check
    //if( df == NULL ) return  dNOT_OK;

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

    if( pdf->timeStamp == df->timeStamp )
	return  start;
#endif

    if( df->dbf_flag == SQL_TABLE ) {
	return  '\0';
    }

#ifdef WSToMT
    EnterCriticalSection( &(pdf->dCriticalSection) );

    pdf->dIOReadWriteFlag = 0;       /* set write flag */
    pdf->rec_p = df->rec_no;
    BufFlush( pdf );

    memcpy(start, &(pdf->buf_sink[(pdf->rec_p-pdf->rec_beg) * pdf->rec_len + \
						(start-df->rec_buf)]), 10);
    LeaveCriticalSection( &(pdf->dCriticalSection) );
#else
    df->dIOReadWriteFlag = 0;       /* set write flag */
    df->rec_p = df->rec_no;
    BufFlush( df );
    memcpy(start, &(pdf->buf_sink[(pdf->rec_p-pdf->rec_beg) * pdf->rec_len + \
						(start-df->rec_buf)]), 10);
#endif

    return  start;

} //end of GetDbtId()


/*
----------------------------------------------------------------------------
!!                      PutDbtId()
----------------------------------------------------------------------------*/
char *PutDbtId(dFILE *df, char *start)
{
#ifdef WSToMT
    dFILE *pdf;
#endif
    char  buf[16];
    long  l;
    char  *start2;
    int	  slowPutRec;

    if( df->dbf_flag == SQL_TABLE ) {
	return  NULL;
    }

    if( (df->oflag & 0xF) == O_RDONLY ) {
	df->error = 2000;
	return  NULL;
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

	//1998.2.27
	//df->rec_num = pdf->rec_num;
    } else
	pdf = df;

    EnterCriticalSection( &(pdf->dCriticalSection) );

    df->timeStamp = ++(pdf->timeStamp);

    //keep the start[] in pdf->rec_buf
    memcpy(buf, start, 10);

    //pdf->rec_p = df->rec_no;
    pdf->rec_p = df->rec_p;

    memcpy(pdf->rec_tmp, pdf->rec_buf, pdf->rec_len);
    if( get1rec(pdf) == NULL )
	return  NULL;

    start2 = &(pdf->rec_buf[start-df->rec_buf]);
    
    //2000.8.2
    /*if( (l = atol(subcopy(start2, 0, 10))) > 0 && \
						pdf->cdbtIsGDC == 0 ) {
    */
    if( (l = atol(subcopy(start2, 0, 10))) > 0 && l != atoi(start) ) {
	PhyDbtDelete(pdf, l);
    }
    memcpy(start2, buf, 10);

    slowPutRec = pdf->slowPutRec;
    pdf->slowPutRec = 0;    	//force to write the dbt field id
    put1rec(pdf);
    pdf->slowPutRec = slowPutRec;

    memcpy(pdf->rec_buf, pdf->rec_tmp, pdf->rec_len);

    pdf->write_flag.dbt_write = 1;

    LeaveCriticalSection( &(pdf->dCriticalSection) );
#else
    df->dIOReadWriteFlag = 1;       /* set write flag */
    df->rec_p = df->rec_no;
    BufFlush( df );
    memcpy(&(pdf->buf_sink[(pdf->rec_p-pdf->rec_beg) * pdf->rec_len + \
					(start-df->rec_buf)]), start, 10);
    df->write_flag.buf_write = 1;
    df->write_flag.file_write = 1;
#endif

    return  start;

} //end of PutDbtId()


int absSyncDfBh(dFILE *df)
{
    int k;

    for(k = 0;  k < df->syncBhNum;  k++) {
	df->syncBhNeed[k] = '\1';
    }

    return  k;

} //absSyncDfBh()




/***************** End Of dBASE Development Software **********************/
