//=====================================================================\\
//==     RJIO.C --Read & Write Journal Tools                         ==\\
//==                                                                 ==\\
//==             main moudle                                         ==\\
//==                                                                 ==\\
//==   Writen by MengHongWen, 1994/11                                ==\\
//==   Copyright MIS Research, all right reserved                    ==\\
//=====================================================================//


#include <windows.h>
#include <io.h>
//#include <dos.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "rjio.h"
#include "mxccfg.h"
#include "mistring.h"

//#define RJFILEMARK       "TGMIS Journal.\r\n"
#define RJFILEMARK       "ASQL_LOG_v3.0.\r\n"
#define marlLen          14

RJFieldType FieldName[keynum]= {
    {RJ_TYPE,    "TYPE",     "T"},
    {RJ_OPERATOR,"OPERATOR", "O"},
    {RJ_TIME,    "TIME",     "H"},
    {RJ_BASE,    "BASE",     "B"},
    {RJ_FIELD,   "FIELD",    "F"},
    {RJ_GRADE,   "GRADE",    "G"},
    {RJ_DATA,    "DATA",     "D"}
};

static char *szSummary = "SUMMARY";

/*
 * ------------ local function prototype --------------------
 */

RJDATA *AllocDS( void );
void  FreeDS( RJDATA *ds );




/*
 * ----------------------------------------------------------
 *            createRJ()
 * ----------------------------------------------------------
 */
RJFILE *createRJ( char *RJname )
// create a empty diary-sheet file, system'date was used.
{
    RJFILE *rf;
    int    i, iy;
    char   buf[30];
    char   buf2[30];
    SYSTEMTIME SystemTime;
    

// address of system time structure 
/* I am sure to create the file
    if( access( RJname, 0 ) == 0 )
    {
       // warning...
       if( ( rf = openRJ( RJname ) ) != NULL ) {
		return  rf;
       }
    }
*/
    rf = (RJFILE *)malloc( sizeof(RJFILE) );
    if( rf == NULL ) 	return NULL;
    memset( rf, '\0', sizeof(RJFILE) );

    if ( !(rf->handle = cfg_create(RJname, rjHEAD_SIZE) ) )
       {
       free (rf);
       return NULL;
       }

    if ( !(rf->ds = AllocDS()) )
    {
       cfg_close( rf->handle );
       free (rf);
       return NULL;
    }

    cfg_writeLine( rf->handle, RJFILEMARK );

    cfg_make(rf->handle, szSummary, 0);

    GetLocalTime( &SystemTime );
    for(iy = 0;  iy < MAXYEAR_RJIO;  iy++) {
       for( i=1; i<=12; i++ )
       {
		sprintf( buf, "%4d%02d", SystemTime.wYear+iy, i); // make keyword
		// enough to content 999,999,999,999
		sprintf( buf2,"%014ld",rf->segPos[i-1]);
		cfg_write( rf->handle, buf, buf2, 1 );
       }
    }

    sprintf(buf, "%4d%02d%02d", SystemTime.wYear,SystemTime.wMonth,SystemTime.wDay);
    cfg_write( rf->handle,"OPTIME", buf, 1 );
    cfg_write( rf->handle,"ETIME", buf, 1 );
    cfg_write( rf->handle,"STIME", buf, 1 );

    rf->curdate = atol( buf );
    rf->startdate = rf->curdate;
    rf->endate = rf->curdate;

    for( i=1; i<=12; i++ )
       {
       rf->segPos[i-1] = 0L;
       }

    InitializeCriticalSection( &(rf->dCriticalSection) );

    return  rf;
}

/*
 * ----------------------------------------------------------
 *            openRJ()
 * ----------------------------------------------------------
 */
RJFILE *openRJ( char *RJname )
// open diary-sheet file, system'date was used.
{
    RJFILE *rf;
    int    i;
//    struct dosdate_t d;
    char   buf[30];
    char   buf2[30];
    int    year;

    if ( access( RJname, 0 ) != 0 ) 	return NULL;

    rf = (RJFILE *)malloc( sizeof(RJFILE) );
    if ( rf == NULL ) return NULL;
    memset( rf, '\0', sizeof(RJFILE) );

    if ( !(rf->handle = cfg_open(RJname, rjHEAD_SIZE) ) )
       {
       free(rf);
       return NULL;
       }

    if ( !(rf->ds = AllocDS()) )
       {
       cfg_close( rf->handle );
       free (rf);
       return NULL;
       }

    cfg_readLine( rf->handle, (char *)&buf, 30 );

    if ( strncmp( buf, RJFILEMARK, marlLen ) != 0 )
    {
       // inVaild journal file.
       cfg_close(rf->handle);
       free(rf);
       return NULL;
    }

    // locate the segment
    if( cfg_segment( rf->handle, szSummary, NULL) < 0 )
    {
       // inVaild journal file.
       cfg_close(rf->handle);
       free(rf);
       return NULL;
    }

    cfg_read( rf->handle, "OPTIME", buf );
    rf->curdate = atol( buf );

    cfg_read( rf->handle, "ETIME", buf );
    rf->endate = atol( buf );

    cfg_read( rf->handle, "STIME", buf );
    rf->startdate = atol( buf );

    buf[4]='\0';
    year = atoi( buf );

    for( i = 1;   i <= 12;   i++ )
       {
       sprintf( buf, "%4d%02d", year, i); // make keyword
       cfg_read( rf->handle, buf, (char *)&buf2 );
       rf->segPos[i-1] = atol( buf2 );
       rf->exist[i-1] = ( rf->segPos[i-1] ) ? 1 : 0;
       }

    InitializeCriticalSection( &(rf->dCriticalSection) );

    return  rf;

}


/*
 * ----------------------------------------------------------
 *            closeRJ()
 * ----------------------------------------------------------
 */
void closeRJ( RJFILE *rf )
{
    int    i,year;
    char   buf[30];
    char   buf2[30];

    if ( !rf ) return;

    cfg_segment( rf->handle, szSummary, NULL);

    sprintf( buf, "%ld", rf->curdate );
    cfg_write( rf->handle, "OPTIME", buf, 1 );

    sprintf( buf, "%ld", rf->endate );
    cfg_write( rf->handle, "ETIME", buf, 1 );

    sprintf( buf, "%ld", rf->startdate );
    cfg_write( rf->handle, "STIME", buf, 1 );

    buf[4]='\0';
    year = atoi( buf );

    for( i=1; i<=12; i++ )
       {
       sprintf( buf, "%4d%02d", year, i); // make keyword
       sprintf( buf2, "%014ld", rf->segPos[i-1] );
       cfg_write( rf->handle, buf, buf2, 1 );
       }

    if ( rf->handle )
	cfg_close(rf->handle);

    rf->handle = 0;

    FreeDS( rf->ds );
    free(rf);
    rf = 0;
}

/*
 * ----------------------------------------------------------
 *            vaildDate()
 * ----------------------------------------------------------
 */
int vaildDate( RJFILE *rf, char *date )
{
    long  date_l;
    /*for speed
    if( rf == NULL || date == NULL ) return 0;
    if( strlen(date) < 8 )	return 0;
    */
    date_l = atol( date );
    if ( rf->curdate < rf->startdate ) return 0;
    if ( date_l >= rf->curdate  )
       {
       rf->endate = date_l;
       rf->curdate = date_l;
       return  1;
       }
    return 0;
}

/*
 * ----------------------------------------------------------
 *            AllocDS()
 * ----------------------------------------------------------
 */
RJDATA *AllocDS( void )
// alloc a data struct space
{
    int  i;
    RJDATA *ds=(RJDATA *)malloc(sizeof(RJDATA));
    if ( ds )
       {
       memset( ds, 0, sizeof(RJDATA) );
       ds->buf_len = rec_buf_size;
       ds->rec_buf =(char *)malloc( ds->buf_len );
       if ( ds->rec_buf == NULL )
	  {
	  free( ds );
	  return NULL;
	  }
       ds->rec_len = 0;
       memset( ds->rec_buf, 0, ds->buf_len);
       for( i=0; i<keynum; i++ )  ds->p[i] = ds->rec_buf;
       }
    return ds;
}

/*
 * ----------------------------------------------------------
 *            FreeDS()
 * ----------------------------------------------------------
 */
void  FreeDS( RJDATA *ds )
// free data struct space
{
    if ( ds )
       {
       if ( ds->rec_buf ) free( ds->rec_buf );
       free( ds );
       }
}

/*
 * ----------------------------------------------------------
 *            RJSeek()
 * ----------------------------------------------------------
 */
int  RJSeek( RJFILE *rf, char *date )
// start a new read procedure
{
    int   d;
    int   retVal;

    if( date == NULL ) {
	d = 0;
    } else {

	if( strlen(date) < 8 )	return  0;

	d = (date[4] - '0') * 10 + (date[5] - '0') - 1;
	if ( d < 0 || d >= 12 ) 	return 0;
    }

    // skip to a really position
    if( rf->segPos[d] == 0 )	retVal = 2;
    else			retVal = 1;
    for(  ;  d < 11 && rf->segPos[d] == 0;  d++);

    cfg_setSeg( rf->handle, rf->segPos[d] );
    //****
    fseek( rf->handle->hCFG, rf->handle->seg, SEEK_SET );

    return retVal;
}

/*
 * ----------------------------------------------------------
 *            RJRead()
 * ----------------------------------------------------------
 */
int  RJRead( RJFILE *rf )
// untill empty
{
    int   i;
    char  *p;

    newRecord( rf );

    for( i=0; i<keynum; i++ )
       {
       if (!(cfg_readLine( rf->handle, rf->buffer, line_buf_size ))) return 0; // end
       if ( strnicmp( rf->buffer, FieldName[i].qkey, strlen(FieldName[i].qkey)) != 0 )
	   return 0;  // error journal record
       p = strchr( rf->buffer, '=' );
       if ( p == NULL )   return 0;
       p++;
       trim(p);
       writeContent( rf, 100+i, p );
       }
    return 1;
}

/*
 * ----------------------------------------------------------
 *            readContent()
 * ----------------------------------------------------------
 */
char  *readContent( RJFILE *rf, nameType name )
// read RJ field content
{
    return rf->ds->p[name-100];
}


/*
 * ----------------------------------------------------------
 *            newRecord()
 * ----------------------------------------------------------
 */
void  newRecord( RJFILE *rf )
{
    int i;
    for( i=0; i<keynum; i++ )  rf->ds->p[i] = rf->ds->rec_buf;
    rf->ds->rec_len = 0;

    memset( rf->ds->rec_buf, '\0', rf->ds->buf_len );
}

/*
 * ----------------------------------------------------------
 *            writeContent()
 * ----------------------------------------------------------
 */
int  writeContent( RJFILE *rf,nameType name, char *buf )
//  rf->rec_buf bytes map:
//  =====================
//
//    'U'  meaning used byte
//    '.'  meaning free byte
//
//  UUUUUUUUUUUUUUUUUUUUUUUUUUUUUU...................
//     ³            ³            ³
//     ³<-blocklen->³<-interval->³
//     ³            ³            À> rec_len
//     ³            À>insertp
//     À>ds->p[tp]
//
{
    int   tp = name-100;
    int   hasBreak = 0,i;
    int   insertp, blocklen, interval, addlen;
    char  *p;

    if ( tp<0 || tp>= keynum ) return 0;
    if ( !rf->ds || !buf ) return 0;

    if ( rf->ds->p[tp] != (rf->ds->rec_buf+rf->ds->rec_len) ) // is end ?
	 hasBreak = 1;

    addlen = strlen(buf);
    if ( hasBreak == 0 ) addlen++; // for '\0'

    if ( (rf->ds->rec_len + addlen + hasBreak) >= rf->ds->buf_len)
       {
       int   newsize = rf->ds->buf_len + rf->ds->buf_len /2;
       char *p =
	    (char *)realloc( rf->ds->rec_buf, newsize );

	    if ( p == NULL /*|| p == rf->ds->rec_buf*/ ) // alloc failure
		return 0;
       // realloced success
       rf->ds->rec_buf = p;
       memset( rf->ds->rec_buf + rf->ds->buf_len, '\0', rf->ds->buf_len /2 );
       rf->ds->rec_len = newsize;
       }

    // search next pointer
    p = rf->ds->rec_buf + rf->ds->rec_len;
    for( i=0; i<keynum; i++ )
       {
       if ( i == tp )  continue;
       if ( rf->ds->p[i] <= rf->ds->p[tp] ) continue;
       if ( (rf->ds->p[i] - rf->ds->p[tp]) < (p - rf->ds->p[tp]) )
	  p = rf->ds->p[i];
       }

    // get blocklen
    blocklen = p - rf->ds->p[tp];

    insertp = (rf->ds->p[tp]-rf->ds->rec_buf) + blocklen;
    interval = rf->ds->rec_len - insertp;

    // move data backward
    memmove( rf->ds->rec_buf + insertp + addlen + hasBreak, \
	     rf->ds->rec_buf + insertp, interval + addlen + hasBreak );

    // add break char
    if ( hasBreak )
       *(rf->ds->rec_buf + insertp-1) = Break_Char; // -1 for overwrire '\0'

    // add field content
    memmove( rf->ds->rec_buf + insertp, buf, addlen + hasBreak );

    // update ds->p;
    for( i=0; i<keynum; i++ )
       {
       if ( i == tp )   continue;
       if ( rf->ds->p[i] >= rf->ds->p[tp] )
	    rf->ds->p[i] += addlen+hasBreak;
       }

    // increase rec_len
    rf->ds->rec_len += addlen+hasBreak;
    return 1;
}

/*
 * ----------------------------------------------------------
 *            RJWrite()
 * ----------------------------------------------------------
 */
int  RJWrite( RJFILE *rf, char *date )
// append method
{
    int   d, i;
    char  datetmp[7];
    long  inc_size=0L;
    int   retval;

    retval = vaildDate( rf, date );

    d = (date[4] - '0') * 10 + (date[5] - '0') - 1;
    if ( d < 0 || d >= 12 ) 	return 0;

    if ( !retval ) return 0;
    if ( !rf->exist[d] ) {
	rf->exist[d] = 1;
	strncpy( datetmp, date, 6 );
	datetmp[6] = '\0';
	// rj  make group
	cfg_make(rf->handle,datetmp, 1);
	rf->segPos[d] = cfg_getPos(rf->handle);
	cfg_setSeg( rf->handle, rf->segPos[d] );
    }

    for( i=0; i< keynum; i++ )
       {
       cfg_write( rf->handle, FieldName[i].qkey, rf->ds->p[i], 2 );
       inc_size+=strlen(FieldName[i].qkey);
       inc_size++; // '='
       inc_size+=strlen(rf->ds->p[i]);
       inc_size+=2; // "\r\n"
       }
/* Modi 1995.6.19
    cfg_writeLine( rf->handle, "\r\n" );
    inc_size+=2;*/

    // update ds->segpos
    for( i=d+1; i<12; i++ ) {
       if( rf->segPos[i] != 0L )
	     rf->segPos[i] +=inc_size;
    }  // end of for

    return 1;
}

int  RJBufCopy( RJFILE *dest, RJFILE *src )
// copy src's rec_buf to dest' rec_buf
{
    int  i;
    for( i=RJ_TYPE; i<=RJ_DATA; i++ )
       if ( !writeContent(dest,i,readContent(src,i) )) return 0;
    return 1;
}


int  RJ_write( RJFILE *rf )
{
    char    dt[20];
    SYSTEMTIME SystemTime;

    GetLocalTime( &SystemTime );
    getCompressClock(dt, &SystemTime);

    writeContent( rf, RJ_TIME, dt );
    sprintf(dt, "%4d%02d%02d", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay);

    return  RJWrite( rf, dt );

} // end of RJ_write()



void getCompressClock(char *buf, LPSYSTEMTIME lpSystemTime)
{
    sprintf( buf, "%4d%c%c%c%c%c", lpSystemTime->wYear, '0'+lpSystemTime->wMonth, '0'+lpSystemTime->wDay, \
				'0'+lpSystemTime->wHour, '0'+lpSystemTime->wMinute, '0'+lpSystemTime->wSecond);
}

void getReallyClock(char *buf, LPSYSTEMTIME lpSystemTime)
{
    lpSystemTime->wMonth = buf[4] - '0';
    lpSystemTime->wDay = buf[5] - '0';
    lpSystemTime->wHour= buf[6] - '0';
    lpSystemTime->wMinute = buf[7] - '0';
    lpSystemTime->wSecond = buf[8] - '0';;
    buf[4] = '\0';
    lpSystemTime->wYear = atoi(buf);
} // end of getReallyClock()


short RJDateEnd(RJFILE *rf, char *date)
{
    char  	*s;
    char  	buf[256];
    SYSTEMTIME SystemTime;

    s = readContent(rf, RJ_TIME);
    strcpy(buf, s);
    getReallyClock(buf, &SystemTime);
    sprintf(buf, "%4d%02d%02d", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay);

    if( strcmp(buf, date) > 0 )		return  1;
    return  0;

} // end of RJDateEnd()


int allocRjRes(RJFILE *rf)
{
    EnterCriticalSection( &(rf->dCriticalSection) );
    return  0;
}

int freeRjRes(RJFILE *rf)
{
    LeaveCriticalSection( &(rf->dCriticalSection) );
    return  0;
}


			//=========================================\\
			//==   Read-Write Journal Tool kits      ==\\
			//==   version 1.21                      ==\\
			//==                                     ==\\
			//==   copyright(c) MIS Research 1994/11 ==\\
			//=========================================\\