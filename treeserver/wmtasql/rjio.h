/*//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\\
//%%     RJIO.H  --Read & Write Journal Tools                        %%\\
//%%                                                                 %%\\
//%%   Writen by MengHongWen, 1994/11                                %%\\
//%%   Copyright MIS Research, all right reserved                    %%\\
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\\*/

#ifndef __RWRJ_H_
#define __RWRJ_H_

#include <windows.h>
#include "mxccfg.h"

#define MAXYEAR_RJIO	10
#define  Break_Char  	'\xFF'

typedef int     nameType;

#define  rec_buf_size    4096
#define  line_buf_size   4096
#define  rjHEAD_SIZE	 16

// define TYPE field's type
#define       REC_ADD       0x001
#define       REC_DEL       0x002
#define       REC_MODI      0x003
#define       ORDER_MODI    0x004

// define TYPE field's string type
#define       szREC_ADD       "A"
#define       szREC_DEL       "D"
#define       szREC_MODI      "M"
#define       szORDER_MODI    "K"

// define  RJIO's field name
#define       keynum        7
#define       RJ_TYPE       100       // don't change, this is head
#define       RJ_OPERATOR   101       // don't change
#define       RJ_TIME       102       // don't change
#define       RJ_BASE       103       // don't change
#define       RJ_FIELD      104       // don't change
#define       RJ_GRADE	    105	      // don't change
#define       RJ_DATA       106       // don't change, this is end

typedef struct
{
    int     id;
    char   *key;
    char   *qkey;
} RJFieldType;

extern RJFieldType FieldName[keynum];

typedef  struct
{
    char  *p[keynum];
    char  *rec_buf;
    long   rec_len;
    long   buf_len;
} RJDATA;

typedef struct
{
    MXCCFG *handle;
    RJDATA *ds;
    hPOS   segPos[12];
    short  exist[12];
    long   curdate;
    long   startdate;
    long   endate;
    char   buffer[line_buf_size];

    CRITICAL_SECTION 	dCriticalSection;

} RJFILE;


/*
 * -----------------------------------------------------
 *         function prototype
 * -----------------------------------------------------
 */

char  *getRJName( char *filename );
      // get current used rj file name into filename

RJFILE *createRJ( char *RJname );
	//
RJFILE *openRJ( char *RJname );
	//
void    closeRJ( RJFILE *rf );
	//
int     validDate( RJFILE *rf, char *date );
	// date format: year+month+day
int     RJBufCopy( RJFILE *dest, RJFILE *src );
	// copy src's rec_buf to dest' rec_buf

int     RJSeek( RJFILE *rf, char *date );
	// start a read procedure
int     RJRead( RJFILE *rf);
	// untill empty
char    *readContent( RJFILE *rf, nameType name );
	// read RJ field content


void    newRecord( RJFILE *rf );
	// start a write procedure
int     writeContent( RJFILE *rf, nameType name, char *buf );
	// read RJ field content, failure return
int     RJWrite( RJFILE *rf, char *date );
	// append method

int     RJ_write( RJFILE *rf );

void getCompressClock(char *buf, LPSYSTEMTIME lpSystemTime);
void getReallyClock(char *buf, LPSYSTEMTIME lpSystemTime);

short RJDateEnd(RJFILE *rf, char *date);

int allocRjRes(RJFILE *rf);
int freeRjRes(RJFILE *rf);

#endif
