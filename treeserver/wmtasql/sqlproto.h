/***************************************************************************
 *
 *                         INFORMIX SOFTWARE, INC.
 *
 *                            PROPRIETARY DATA
 *
 *      THIS DOCUMENT CONTAINS TRADE SECRET DATA WHICH IS THE PROPERTY OF 
 *      INFORMIX SOFTWARE, INC.  THIS DOCUMENT IS SUBMITTED TO RECIPIENT IN
 *      CONFIDENCE.  INFORMATION CONTAINED HEREIN MAY NOT BE USED, COPIED OR 
 *      DISCLOSED IN WHOLE OR IN PART EXCEPT AS PERMITTED BY WRITTEN 
 *      AGREEMENT  SIGNED BY AN OFFICER OF INFORMIX SOFTWARE, INC.
 *
 *      THIS MATERIAL IS ALSO COPYRIGHTED AS AN UNPUBLISHED WORK UNDER
 *      SECTIONS 104 AND 408 OF TITLE 17 OF THE UNITED STATES CODE. 
 *      UNAUTHORIZED USE, COPYING OR OTHER REPRODUCTION IS PROHIBITED BY LAW.
 *
 *
 *  Title:          sqlproto.h (same as esqlcwin.h)
 *
 *  Description:    Definition/Declarations for ESQL/C  for Windows library routines.  
 *
 *  Date:       9th Oct 1992.
 ***************************************************************************
 */

#pragma pack(push, 4)

#ifndef _INC_ESQLCWIN
#define _INC_ESQLCWIN


/***** Common definnitons and typedefs *************************************/

#define SQLRET      long 


/***** Pointer types   ***************************************************/

typedef _SQCURSOR far *_LPSQCURSOR;
typedef _SQSTMT far *       _LPSQSTMT;
typedef _FetchSpec far *        _LPFETCHSPEC;
typedef struct sqlda far *      _LPSQLDA;
typedef _LPSQLDA far *      _LPLPSQLDA;
typedef LPSTR far *     _LPLPSTR;
typedef double far *        _LPDOUBLE;
typedef short far *     _LPSHORT;
typedef int far *           _LPINT;
typedef long far *      _LPLONG;
typedef struct value far *      _LPVALUE;

typedef struct sqlvar_struct far *  _LPSQLVAR;
typedef struct hostvar_struct far *     _LPHOSTVAR;
typedef struct decimal far *    _LPDECIMAL;
typedef struct intrvl far  *        _LPINTRVL;
typedef struct dtime far  *     _LPDTIME;



/***** Function prototype for SQL functions ********************************/

SQLRET   WINAPI    _iqalloc(LPSTR, int);
SQLRET   WINAPI    _iqbeginwork(void);
SQLRET   WINAPI    _iqcdcl(_LPSQCURSOR, LPSTR, _LPLPSTR, _LPSQLDA, _LPSQLDA, int);
SQLRET   WINAPI    _iqcddcl(_LPSQCURSOR, LPSTR, _LPSQCURSOR, int);
SQLRET   WINAPI    _iqcftch(_LPSQCURSOR, _LPSQLDA, _LPSQLDA,  LPSTR,_LPFETCHSPEC); 
SQLRET   WINAPI    _iqclose(_LPSQCURSOR);
SQLRET   WINAPI    _iqcommit(void);
SQLRET   WINAPI    _iqcput(_LPSQCURSOR,_LPSQLDA, LPSTR);
SQLRET   WINAPI    _iqcrproc(LPSTR);
SQLRET   WINAPI    _iqdbase(LPSTR,int);
SQLRET   WINAPI    _iqdbclose(void);
SQLRET   WINAPI    _iqdcopen(_LPSQCURSOR,_LPSQLDA, LPSTR, _LPVALUE, int);
SQLRET   WINAPI    _iqdealloc(LPSTR);
SQLRET   WINAPI    _iqdescribe(_LPSQCURSOR, _LPLPSQLDA,  LPSTR);
SQLRET   WINAPI    _iqexecute(_LPSQCURSOR, _LPSQLDA , LPSTR, _LPVALUE);
SQLRET   WINAPI    _iqeximm(LPSTR);
SQLRET   WINAPI    _iqexproc(_LPSQCURSOR,_LPLPSTR, int,_LPSQLVAR,int,_LPSQLVAR, int);
SQLRET   WINAPI    _iqflush(_LPSQCURSOR);
SQLRET   WINAPI    _iqfree(_LPSQCURSOR);
SQLRET   WINAPI    _iqgetdesc(LPSTR, int, _LPHOSTVAR, int);
_LPSQCURSOR  FAR PASCAL _iqlocate_cursor(LPSTR,int,int,short);
_LPSQCURSOR  WINAPI _iqnprep(LPSTR, LPSTR, short); 
SQLRET   WINAPI    _iqrollback(void);
SQLRET   WINAPI    _iqsetdesc(LPSTR, int, _LPHOSTVAR, int);
SQLRET   WINAPI    _iqslct(_LPSQCURSOR, _LPLPSTR, int, _LPSQLVAR, int, _LPSQLVAR, int);
SQLRET   WINAPI    _iqstmnt(_LPSQSTMT, _LPLPSTR, int, _LPSQLVAR, _LPVALUE);
SQLRET   WINAPI    _iqstop(void);
void far * WINAPI GetConnect( void );
void far * WINAPI SetConnect( void far *NewConInfo );
void far * WINAPI ReleaseConnect( void far *NewConInfo );
SQLRET	 WINAPI    sqgetdbs(_LPINT ret_fcnt, char far * far *fnames,
			    int fnsize, LPSTR farea, int  fasize);
void FAR PASCAL    SqlFreeMem( void far * MemAddr, int FreeType );



/***** Function prototype for Library  ***********************************/

/***** Data Type Functions *******************************************/

BOOL        WINAPI    risnull(int, LPSTR);   
SQLRET   WINAPI    rsetnull(int, LPSTR);
int              WINAPI    rtypalign(int, int);
int              WINAPI    rtypmsize(int, int);
LPSTR       WINAPI    rtypname(int);
int              WINAPI    rtypwidth(int, int);


/***** Numeric Formatting Routines ***********************************/

SQLRET   WINAPI    rfmtdouble(double, LPSTR, LPSTR);
SQLRET   WINAPI     rfmtlong(long,  LPSTR,  LPSTR);


/***** Character and String functions *********************************/

int              WINAPI    bycmpr(LPSTR, LPSTR, int);
SQLRET   WINAPI    bycopy(LPSTR, LPSTR, int);
SQLRET   WINAPI    byfill(LPSTR, int, char);
int              WINAPI    byleng(LPSTR, int);
SQLRET   WINAPI    ldchar(LPSTR, int, LPSTR);  
SQLRET   WINAPI    rdownshift(LPSTR);
SQLRET   WINAPI    rstod(LPSTR, _LPDOUBLE);
SQLRET   WINAPI    rstoi(LPSTR, _LPINT);
SQLRET   WINAPI    rstol(LPSTR, _LPLONG);
SQLRET   WINAPI    rupshift(LPSTR);
SQLRET   WINAPI    stcat(LPSTR, LPSTR);
SQLRET   WINAPI    stchar(LPSTR, LPSTR, int);
int              WINAPI    stcmpr(LPSTR, LPSTR);
SQLRET   WINAPI    stcopy(LPSTR, LPSTR);
int              WINAPI    stleng(LPSTR);


/***** Decimal Type Functions *****************************************/

SQLRET   WINAPI    deccvasc(LPSTR, int, _LPDECIMAL);
SQLRET   WINAPI    dectoasc(_LPDECIMAL, LPSTR, int, int);
SQLRET   WINAPI    deccvint (int, _LPDECIMAL);
SQLRET   WINAPI    dectoint(_LPDECIMAL, _LPINT);
SQLRET   WINAPI    deccvlong(long, _LPDECIMAL);
SQLRET   WINAPI    dectolong(_LPDECIMAL, _LPLONG);
SQLRET   WINAPI    deccvdbl(double, _LPDECIMAL);
SQLRET   WINAPI    dectodbl(_LPDECIMAL, _LPDOUBLE);
SQLRET   WINAPI    decadd(_LPDECIMAL,  _LPDECIMAL, _LPDECIMAL);
SQLRET   WINAPI    decsub(_LPDECIMAL, _LPDECIMAL, _LPDECIMAL);
SQLRET   WINAPI    decmul(_LPDECIMAL, _LPDECIMAL, _LPDECIMAL);
SQLRET   WINAPI    decdiv(_LPDECIMAL, _LPDECIMAL, _LPDECIMAL);
int              WINAPI    deccmp(_LPDECIMAL, _LPDECIMAL);
SQLRET   WINAPI    deccopy(_LPDECIMAL, _LPDECIMAL);
LPSTR      WINAPI    dececvt(_LPDECIMAL, int, _LPINT, _LPINT);
LPSTR      WINAPI    decfcvt(_LPDECIMAL, int, _LPINT, _LPINT);
SQLRET   WINAPI    decround(_LPDECIMAL ,int);
SQLRET   WINAPI    dectrunc(_LPDECIMAL, int);
SQLRET   WINAPI    rfmtdec(_LPDECIMAL, LPSTR, LPSTR);


/***** DATE Functions ***********************************************/

SQLRET   WINAPI     rdatestr(long, LPSTR);
SQLRET   WINAPI     rdayofweek(long);
SQLRET   WINAPI     rdefmtdate(_LPLONG, LPSTR, LPSTR);
SQLRET   WINAPI     rfmtdate(long, LPSTR, LPSTR);
SQLRET   WINAPI     rjulmdy(long, _LPSHORT);
BOOL       WINAPI      rleapyear(int);
SQLRET   WINAPI     rmdyjul(_LPSHORT, _LPLONG);
SQLRET   WINAPI     rstrdate(LPSTR,_LPLONG);
SQLRET   WINAPI     rtoday(_LPLONG);




/***** DATE TIME and INTERVAL  Data Type Functions  ******************/

SQLRET   WINAPI    dtcurrent(_LPDTIME);
SQLRET   WINAPI    dtcvasc(LPSTR, _LPDTIME);
SQLRET   WINAPI    dtcvfmtasc(LPSTR, LPSTR, _LPDTIME);
SQLRET   WINAPI    dtextend(_LPDTIME, _LPDTIME);
SQLRET   WINAPI    dttoasc(_LPDTIME ,LPSTR);
SQLRET   WINAPI    dttofmtasc(_LPDTIME, LPSTR, int, LPSTR);
SQLRET   WINAPI    incvasc(LPSTR, _LPINTRVL);
SQLRET   WINAPI    incvfmtasc(LPSTR, LPSTR, _LPINTRVL);
SQLRET   WINAPI    intoasc(_LPINTRVL, LPSTR);
SQLRET   WINAPI    intofmtasc(_LPINTRVL, LPSTR, int, LPSTR);


/***** Error Message Function *************************************/

SQLRET   WINAPI    rgetmsg(long, LPSTR, short);

/******************************************************************/

SQLRET   WINAPI    sqlstart(void);
SQLRET   WINAPI    sqlbreak(void);
SQLRET   WINAPI    sqldetach(void);
SQLRET   WINAPI    sqlexit(void);
SQLRET   WINAPI    sqlbreakcallback( FARPROC );
SQLRET   WINAPI    sqldone( void );


#endif       /* _INC_ESQLCWIN */

#pragma pack(pop, 4)

