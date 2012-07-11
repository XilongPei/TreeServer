/***************************************************************************\
 * ASQLANA.C
 * ^-_-^
 *
 * Author: Lianchun Song    1992.4
 * Rewrite by Xilong Pei    1993.6 1994.5
 *            Xilong Pei    1994.6.16 v2.01 add more dbf stat
 *            1995.11.15 add groupaction which is execute with a group data
 *	      1995.12.11 make the multi querry can be self related
 *       -------------------------
 *	      v 3.00
 *	      1996.12.20-31 support LET, IF-ELSE-ENDIF, WHILE-ENDWHILE
 *	      1997.1.2	    support FUNCTION-CALL
 *	      1997.1.4.     add wWord->keyWordPossible: 2.keyword next;
 *                          1.possible keyword next; 0.not keyword
 *			    3. impossible to be keyword
 *			    ENHANCE KEYWORD RECOGNIZE
 *	      1997.3.25.    correct GroupExec for the last record run
 *            1997.10.23.   support multi thread
 *	      1998.8.4.     allow more dbf run with scope (=>scope_more_dbf)
 *
 * Copyright: SRIT MIS Research.
 *            (c) East-Union Computer Service Co., Ltd. 1994 1995
 *	      (c) Shanghai Zhongtie Software Company 1996-1998
 *	      (c) Shanghai Withub Vision Software Co., Ltd. 1999-2000
 * Notice:
 *    1. when error occured, the following is used:
 *        szAsqlErrBuf
 *    2. fFrTo.exHead.nLevel is defined as:
 *        -1 run before condition
 *         1 run after condition
 *         2 groupaction
 *    3. nLevel: 'B': Before, 'A': Ahead, 'G': Group,
 *               'b': program before, 'a': program ahead
 *    4. use recpack() when GROUPBY or MORE DBFs will difficult to calculate
 *       ??? 1997.11.4
 *    5. About the environment:
 *           (1) if use DATABASE appeares, use the default.
 *           (2) when NET system, the ConditionType should be
 *               AsqlExprInFile|Asql_USEENV
 *    6. ExecActTable * ->regEd: 0, gram, regster; 1, gram, not regster
 *    7. 1998.4.9
 *       add ONE_MODALITY_ACTION support:
 *       act webprint("hello world!")
 *       from ...
 *       condition
 *	 begin
 *          webprint("hello world!")
 *       end
 *     8.correct an error, 1998.4.18
 *       condition
 *       begin
 *       //no end
 *     9.1998.5.20
 *       don't commit the transaction when the task fail.
 * 	 asqlCloseFiles( int commitOrRollback );
 *     10.1998.6.13
 *       support the blank condition segment, for the action cann't be run
 *	 without the cindition segment
 *       CONDITION
 *	 BEGIN
 *	 END
 *     11.1998.10.19
 *       donnot wmtDbfLock() when AsqlMoreDbfExecute() want to append:
 *       TO (append)
 *     12.1998.11.12
 *	 add fFrTo.iScopeSkipAdjust to support SCOPEBY decending
 *     13.1998.11.25
 *	 change this:
 *       if( fFrTo.eHead == NULL )
 *	 into:
 *       if( ConditionAppeared == 0 )
 *       considered that:
 *       condition
 *       begin
 *       end
 *       //there is no statements in this condition segment
 *
 *       14.1998.12.30
 *       support keyword:COMMIT
 *       ACTION ..
 *       CONDITION
 *          ...
 *       ACTION ...(1)  -+   is the ACTION(2) is action before next condition
 *                       +-- or action after the first condition
 *       ACTION ...(2)  -+   seperate them with keyword: COMMIT
 *       CONDITION
 *           ...
 *
 *       15.1999.1.31
 *          speed up the statistics algorithm
 *          when calculate the X statement first check the Y result
 *
 *       16.1999.5.16
 *	    add except method
 *
 *          TRY abc
 *          FROM .. To .. COND
 *          EXCEPT abc
 *          ENDEXCEPT
 *
 *       17.1999.5.17
 *          FROM a,b,*key
 *          COND
 *          BEGIN
 *              1 $ calrec(2,...)
 *          END
 *          wether b.key is NULL (no relation exists) the action will be
 *	    run, CORRECT this,
 *	    when b.key is NULL, the dFILE->rec_p of b will be set to -1
 *          in Btree.c
 *
 *       18.2000/2/28
 *	    wGroupbyAction support "groupaction state of acion"
 *
 *	 19.2000/3/3
 *	    fFrTo.auditStr is this structure:
 *                rwe\0rwe\0...\0\0
 *	    if one table has no audit information, the 3 bytes will be set \0
 *	    the work of `set to 0' is done by
 *	    memset(&fFrTo, 0, sizeof(FromToStru));
 *
 *       20.2001/7/6
 *          enlarge the size of szAsqlErrBuf, for holding more error informations
 *
 *       21.2001/8/5
 *          add TextPro() to support large string defination:
 *          TEXT abc
 *             abc string ...... , this string will not be escaped by '\'
 *          ENDTEXT
 *          ______________________
 *          the string abc="   abc string ........"
\***************************************************************************/

#define _InTJCXProgramBody_     "TJCX v3.00"

//#define DEBUG

//#define RUNING_ERR_MESSAGE_ON
//#define _AsqlRuningMessage_Assigned

//I found that the memcmp is much faster than my function
#define bStrCmpOptimized

/** if define _DEF_groupWassymbol, the ASQLANA.c will write an varitable
 ** table to outside when the TO is defined but GROUPACTION is not assigned
**/
//#define _DEF_groupWassymbol

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <errno.h>
#include <share.h>
#include <errno.h>
#include <ctype.h>
#include <LIMITS.H>

#include "dir.h"
#include "dio.h"
#include "dioext.h"
#include "idbase.h"
#include "xexp.h"
#include "ASQLANA.H"
#include "ASQLERR.H"
#include "asqlutl.h"
#include "btree.h"
#include "mistring.h"
#include "btreeext.h"
#include "strutl.h"
#include "memutl.h"
#include "wst2mt.h"
#include "cfg_fast.h"
#include "exchbuff.h"
#include "ts_const.h"
#include "odbc_dio.h"
#include "ts_dict.h"
#include "dbtree.h"
#include "ascii_s.h"
#include "db_s_ndx.h"
#include "t_lock_r.h"
#include "str_gram.h"


#ifdef RUNING_ERR_MESSAGE_ON
#ifdef __N_C_S
	#include "msgbox.h"
	#include "pubvar.h"
#endif
#endif

#ifdef _AsqlRuningMessage_Assigned
#ifdef __N_C_S
	#ifndef __PUBVAR_H_
		#include "pubvar.h"
	#endif
	#include "view.h"
	#include "cxshow.h"
#else
#include <windows.h>
extern BOOL csRuningMessage( short item, char *str );

#endif
WSToMT	char    insideTask = 0;
#endif

/*****
#include "bugstat.h"
*****/
static void asqlCloseFiles( int commitOrRollback );
static Word *   pWordAnaPre( char *, char * );
static void    IgnoreUselessChar( void );
static short   wFlush( unsigned short nLength );
static short   wEof( void );
static short   wEofTest( void );
static unsigned short   SearchWordEnd( void );
static short   IdenRecognize( void );
static short   GetWord( void );
static short   FillActTable( MidCodeType *xPointer );
static ExprTable *   CreatExprTableItem( void );
static short   QueryPro( void );
static unsigned short   SearchVarChar( char *send );
static short   SearchAssistChar( char * );
static short   ToPro( void );
static short   ModiStruPro( void );
static short   FromPro( void );
static short   TitlePro( void );
static short   ActPro( void );
static short   GroupActPro( void );
static short   GroupPro( void );
static short   OrderbyPro(void);
static short   StatPro( void );
static short   SumPro( void );
static short   LabelPro( void );
static short   DatabasePro( void );
static short   ExclusivePro( void );
static short   AsqlSetPro( void );
static short   RemarkPro( void );
static short   Remark2Pro( void );
static short   AsqlUpdatePro( void );
static short   AveragePro( void );
static short   MaxPro( void );
static short   MinPro( void );
static short   TryPro( void );
static short   ExceptPro( void );
static short   EndExceptPro( void );
static short   PredicatesPro( void );
static short   TypePro( short type );
static short   ArrayPro( short artype );
static short   DefinePro( void );
static SysVarOFunType * SymSearch( char *Key, SysVarOFunType *Base, short BaseKeyNum );
static short   ActionRecognize( void );
static unsigned short   SearchExprEnd( void );
static short   GroupNotExec( dFILE *dsp, dFILE *dtp );
static short   GroupExec( dFILE *dsp, dFILE *dtp );
static short   SortedGroupExec( dFILE *dsp, dFILE *dtp );
static short   GroupNotScopeExec( dFILE *dsp, dFILE *dtp );
static short   MidIden( dFILE *df, dFILE *dt );
static short   ExecRec( dFILE *dsp, dFILE *dtp );
static void    bhClose( bHEAD *bh[], short num );
static ExprTable * StatExec( ExprTable *etPoint );
void   wWordFree( void );
void   qsFree( void );
void   qsTableFree( void );
dFIELD * GenerateDfield( char **DbfName, dFILE *DfileHandle[], \
				unsigned short DfileNum, char *Description);
static short  AsqlMoreDbfExecute( void );

#ifdef _DEF_groupWassymbol
static short groupWassymbol(dFILE *tdf, dFIELD *keyFld, char *keyBuf, short CurState);
#endif

static ExecActTable *genGramMan( void );
static short  execGramMan(ExecActTable *exExAct, dFILE *dsp, dFILE *dtp, char abg);
static short programPro( void );
unsigned short getAtoken(char *seed, int tokenBufLen, char *tokenBuf);
signed char  bExactStrCmp(const char *s1, const char *s2, short maxlen);
signed char  bFuzzyStrCmp(const char *s1, const char *s2, short maxlen);
unsigned short getAtokenFromBuf(char *sz, int tokenBufLen, char *tokenBuf);

static short   TextPro( void );
static short addOneVarToSybTab(char *szTextName, char *sp, int iSzLen);


#ifndef bStrCmpOptimized
short bStrCmp(const char *s1, const char *s2, short maxlen);
#else
#define	bStrCmp memcmp
#endif


WSToMT static short nActSerialCode;     // indicate the position of action table

//NORMAL_ACTION and GROUPBY_ACTION are defined in xexp.h
static short one = NORMAL_ACTION;
static short wGroupbyAction = GROUPBY_ACTION;

WSToMT static ExecActTable *apExecActTable = NULL;
WSToMT static CalActOrdTable *apCalActOrdTable = NULL;
WSToMT static ExprTable *eEndPoint = NULL;
WSToMT static char   AsqlStatTarget[4096] = "\0";

WSToMT static int    iAsqlStatAction;
// 1: statistics
// 2: summer
// 3: average
// 4: max
// 5: min


//WSToMT static char   AsqlSumTarget[4096] = "\0";
//WSToMT static char   AsqlAvgTarget[4096] = "\0";
WSToMT static char   StatementPreAction[4096] = "\0";

//this value has no use for we change the algorithm of statistics 1995.10.23
//WSToMT static char   AsqlStatTailAction[8] = "\0";
//WSToMT static MidCodeType *m_c_AsqlStatTailAction = NULL;
WSToMT static short ConditionAppeared = 0;
WSToMT SysVarOFunType  **XexpUserDefinedVar;
WSToMT short *XexpUserDefinedVarNum;

// if others program want askQS stop, they can post a stamp to askQS;
WSToMT char           errorStamp;
WSToMT char           *asqlStatFlagY = NULL;
WSToMT int            asqlStatMaxY = -1;
WSToMT char	      szExcept[32] = "";


WSToMT FromToStru fFrTo = { "", NULL, 0, "", ""};
WSToMT Word *wWord = NULL;


char  tmpPath[MAXPATH] = "";


/*--------------------------------------------------------------------------
 *  Function Name:  pWordAnaPre( )
 *  arguments:  char *filename
 *              query and statistics data file name
 *  return: sucess : Word *
 *          unsucess: NULL
 *  qsError:
 *------------------------------------------------------------------------*/
static Word *   pWordAnaPre( char *filename, char *buf )
{
    char *ph, *pe;

    // alloc a block of memory for word
    if( (wWord = (Word *)realloc( wWord, sizeof( Word ) ) ) == NULL ) {
	qsError = 1001;
	return NULL;
    }

    if( filename == NULL || *filename == '\0' ) {
	if( buf == NULL ) {
	      free( wWord );
	      qsError = 2001;
	      return  NULL;
	}
	wWord->name[0] = '\0';
	wWord->nFlush = AsqlExprInMemory;
	wWord->pcWordAnaBuf =  buf;
	wWord->pcUsingPointer  = buf;
	wWord->nCharNum = strlen( buf );
    } else {
	strcpy( wWord->name, filename );
	if( (wWord->fp = fopen( filename, "rb")) == NULL ) {
	      free( wWord );
	      qsError = 2001;
	      return  NULL;
	}
	wWord->nFlush = AsqlExprInFile;
	wWord->nStopRead = 0;
	if( (wWord->pcWordAnaBuf = (char *)zeroMalloc( Asql_CHARBUFFSIZE + 1 )) \
								== NULL ) {
	      free( wWord );
	      qsError = 1002;
	      return  NULL;
	}
	wWord->pcUsingPointer = wWord->pcWordAnaBuf;
	if( (wWord->nCharNum = fread(wWord->pcWordAnaBuf, 1, \
				     Asql_CHARBUFFSIZE,  wWord->fp) ) < 0 ) {
	      free( wWord );
	      qsError = 2002;
	      return  NULL;
	}
	//wWord->pcWordAnaBuf[ wWord->nCharNum ] = '\0';
    } // end of else

    wWord->unget = 0;
    wWord->keyWordPossible = 2;

    // deal the comment
    //
    //  from sub0.dbf /* remark */
    //  when recognize the FromPro(), willn't deal with the remark
    //  so do it here
    //
    ph = wWord->pcUsingPointer;
    while( TRUE ) {
	if( (ph = strstr(ph, "/*")) == NULL )
		break;
	if( (pe = strstr(ph, "*/")) == NULL ) {
		if( (ph-wWord->pcUsingPointer) > Asql_CHARBUFFSIZE / 2 )
		{
			break;
		}
		qsError = 3001;         // comment too long
		return  NULL;
	} else {
		// set the comment into blank
		pe += 2;
		while( ph != pe )       *ph++ = ' ';
	}
    }

#ifdef OLD_REMARK_DEAL_ALGORITHM
    // deal the comment as \x0D "\x0A//"
    ph = wWord->pcUsingPointer;
    while( TRUE ) {
	if( *ph != '/' || *(ph+1) != '/' )
	{
	    if( (ph = strstr(ph, "\x0A//")) == NULL )
		break;
	    pe = strchr(ph+3, '\x0A');
	} else {
	    pe = strchr(ph+2, '\x0A');
	}

	if( pe == NULL ) {
		if( (ph-wWord->pcUsingPointer) > Asql_CHARBUFFSIZE / 2 )
		{
			break;
		}
		qsError = 3001;         // comment too long
		return  NULL;
	} else {
		// set the comment into blank
		pe++;
		while( ph != pe )       *ph++ = ' ';
	}
    }
#endif

    // deal the line continue char
    pe = wWord->pcUsingPointer;
    while( TRUE ) {
	if( (ph = strchr( pe, '\\' )) == NULL )
		break;
	pe = ph + 1;
	while( *pe != '\0' ) {
		if( *pe == '\t' || *pe == ' ' )         pe++;
		else                                    break;
	}
	if( *pe == '\0' )
		break;
	if( *pe == '\r' || *pe == '\n' ) {
		*ph++ = ' ';                            // overwrite the '\'
		while( *ph != '\0' ) {
			if( *ph == '\r' || *ph == '\n' )  *ph++ = ' ';
			else                              break;
		}
		pe = ph;
	} // end of if
    } // end of while

    return  wWord;

} //* end of function WordAnapre()


/*---------------
 *  Function Name:  wFlush()
 *  Arguments:  Word *
 *              short  length in buffer
 *  Unility:    flush word buffer
 *  return: change: TRUE
 *          unchange: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   wFlush( unsigned short nLength )
{
    char *p, *ph, *pe;
    short ReservedLength, len;

    if( wWord->nFlush == AsqlExprInMemory )
	return  FALSE;

    if( wWord->nStopRead == 1 )
	return  FALSE;

    // current expression in buffer is not just in char set " \t\n\r"
    if( (ReservedLength = strlen( wWord->pcUsingPointer )) - nLength < \
			Asql_CHARBUFFSIZE / 2 && wWord->nStopRead == 0 ) {
	return  FALSE;
    }

    // move the left expression forward
    if( wWord->pcUsingPointer != wWord->pcWordAnaBuf )
	strncpy( wWord->pcWordAnaBuf, wWord->pcUsingPointer, Asql_CHARBUFFSIZE);

    // fill the end buffer by read file
    wWord->pcUsingPointer = wWord->pcWordAnaBuf;
    p = wWord->pcWordAnaBuf + ReservedLength;
    len = Asql_CHARBUFFSIZE-ReservedLength;
    if( len == 0 ) {
	  wWord->nCharNum = ReservedLength;
	  return  FALSE;
    }

    if( (wWord->nCharNum = fread(p, 1, len, wWord->fp)) < 0 ) {
	  qsError = 2002;
	  return  FALSE;
    }
    if( wWord->nCharNum < len ) {

	//add a end '\n' for assign the end of file,
	//there is no end mark in some editor!  1996.01.24 Xilong
	p[ (wWord->nCharNum)++ ] = '\n';

	wWord->nStopRead = 1;
	if( wWord->nCharNum == 0 ) {
		wWord->nCharNum = ReservedLength;
		return  FALSE;
	}
    }
    wWord->nCharNum += ReservedLength;
    wWord->pcWordAnaBuf[ wWord->nCharNum ] = '\0';

    // deal the comment
    //
    //  from sub0.dbf /* remark */
    //  when recognize the FromPro(), willn't deal with the remark
    //  so do it here
    //
    ph = wWord->pcUsingPointer;
    while( TRUE ) {
	if( (ph = strstr(ph, "/*")) == NULL )
		break;
	if( (pe = strstr(ph, "*/")) == NULL ) {
		if( (ph-wWord->pcUsingPointer) > Asql_CHARBUFFSIZE / 2 )
		{
			break;
		}
		qsError = 3001;         //comment too long
		return  FALSE;
	} else {
		// set the comment into blank
		pe += 2;
		while( ph != pe )       *ph++ = ' ';
	}
    }

#ifdef OLD_REMARK_DEAL_ALGORITHM
    // deal the comment as \x0D "\x0A//"
    ph = wWord->pcUsingPointer;
    while( TRUE ) {
	if( *ph != '/' || *(ph+1) != '/' )
	{
	    if( (ph = strstr(ph, "\x0A//")) == NULL )
		break;
	    pe = strchr(ph+3, '\x0A');
	} else {
	    pe = strchr(ph+2, '\x0A');
	}

	if( pe == NULL ) {
		if( (ph-wWord->pcUsingPointer) > Asql_CHARBUFFSIZE / 2 )
		{
			break;
		}
		qsError = 3001;         //comment too long
		return  FALSE;
	} else {
		// set the comment into blank
		pe++;
		while( ph != pe )       *ph++ = ' ';
	}
    }
#endif

    // deal the line continue char
    pe = wWord->pcUsingPointer;
    while( TRUE ) {
	if( (ph = strchr( pe, '\\' )) == NULL )
		break;
	pe = ph + 1;
	while( *pe != '\0' ) {
		if( *pe == '\t' || *pe == ' ' )         pe++;
		else                                    break;
	}
	if( *pe == '\0' )
		break;
	if( *pe == '\r' || *pe == '\n' ) {
		*ph++ = ' ';                            // overwrite the '\'
		while( *ph != '\0' ) {
			if( *ph == '\r' || *ph == '\n' )  *ph++ = ' ';
			else                              break;
		}
		pe = ph;
	} // end of if
    } // end of while

    return  TRUE;

} /* end of wflush */

/*---------------
 *  Function Name:  IgnoreUselessChar()
 *  arguments:  Word buffer *
 *  unility: search a assume string in word buffer
 *  return:  length
 *  qsError:
 *-------------------------------------------------------------------------*/
static void   IgnoreUselessChar( void )
{
    /*
    while( TRUE )  {
	wWord->pcUsingPointer += \
			strspn( wWord->pcUsingPointer, " \t\n\r" );
	if( !wFlush(0) )   return;
    }
    */

    unsigned char  *sz;

    while( TRUE )  {

	sz = wWord->pcUsingPointer;

	while( asc_asqlBk[*sz] )	sz++;

	wWord->pcUsingPointer = sz;

	if( !wFlush(0) )   return;
    }


} /* end of IgnoreUselessChar() */



/*---------------
 *  Function Name:  wEof()
 *  arguments:  Word buffer *
 *  unility: search a word or expression or action end sign
 *  return:  length
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   wEof( void )
{
    if( wWord->pcUsingPointer == NULL )
	return  TRUE;

    IgnoreUselessChar();

    if( wWord->pcUsingPointer == (wWord->pcWordAnaBuf + wWord->nCharNum) )
	return  TRUE;

    if( wWord->pcUsingPointer[0] == '\x1A' )
	return  TRUE;

    return  FALSE;

} //end of wEof()


/*---------------
 *  Function Name:  wEofTest()
 *  arguments:  Word buffer *
 *  unility: search a word or expression or action end sign
 *  return:  length
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   wEofTest( void )
{
    char *sz = wWord->pcUsingPointer;

    if( sz == NULL )
	return  TRUE;

    if( sz == (wWord->pcWordAnaBuf + wWord->nCharNum) )
	return  TRUE;

    if( sz[0] == '\x0' || sz[0] == '\x1A' )
	return  TRUE;

    return  FALSE;

} //end of wEofTest()


/*--------------
 *  Function Name:  SearchWordEnd()
 *  Arguments:  Word buffer *
 *  Unility: search a assume string in word buffer
 *  Return:  length
 *  qsError:
 *-------------------------------------------------------------------------*/
unsigned short   SearchWordEnd( void )
{
    /*short nLen;

    static char WordEnd[7] = { ' ', Asql_SEPSYMB, ',', '\t', '\n', '\r', '\0' };

    IgnoreUselessChar();

    while( TRUE )  {
	nLen = strcspn(wWord->pcUsingPointer, WordEnd);
	if( !wFlush(nLen) )      break;
    }

    return  nLen;
    */

    short 	   nLen;
    unsigned char  *sz;

    IgnoreUselessChar();

    while( TRUE )  {

	sz = wWord->pcUsingPointer;
	while( !asc_asqlSp[*sz] )	sz++;
	nLen = sz - wWord->pcUsingPointer;

	if( !wFlush(nLen) )      break;

    }

    return  nLen;

} /* end of SearchWordEnd() */



/*---------------
 *  Function Name:  SearchExprEnd()
 *  arguments:  Word buffer *
 *  unility: search a word or expression or action end sign
 *  return:  length
 *  qsError:
 *-------------------------------------------------------------------------*/
static unsigned short   SearchExprEnd( void )
{
    unsigned char *pcPoint;

    // the wFlush is called hintly,
    // so we need not pay attention to the buffer manager here

    pcPoint = wWord->pcUsingPointer;
    while( *pcPoint ) {
	   if( *pcPoint == '"' ) {  /* string process */
		pcPoint++;
		while( *pcPoint != '"' && *pcPoint != '\0' ) {
			if( *pcPoint == '\\' )  pcPoint++;
			pcPoint++;
		}
		if( *pcPoint++ == '\0' ) {
			qsError = 3002;
			return  FALSE;
		}
		if( *pcPoint == '"' ) {
			qsError = 3008;
			return  FALSE;
		}
	   }
	   /* take this kind gram as error
	      expr begin
	      end

	      it should be writen as:
	      expr
	      begin
	      end

	   if( (*pcPoint == '$') || (*pcPoint == '#') || \
	       ( toupper(*pcPoint) == 'E' && !strnicmp(pcPoint,"END",3) ) || \
	       ( toupper(*pcPoint) == 'B' && !strnicmp(pcPoint,"BEGIN",5))||\
	       ( *pcPoint == '\r' ) || \
	       ( *pcPoint == '\n' ) ) {
			return  pcPoint-wWord->pcUsingPointer;
	   }  // end of if
	   */

	   /*
	   if( (*pcPoint == '$') || (*pcPoint == '#') || \
	       ( *pcPoint == '\r' ) || \
	       ( *pcPoint == '\n' ) ) {
			return  pcPoint-wWord->pcUsingPointer;
	   }  // end of if
	   */

	   if( asc_dcnr[*pcPoint] ) {
		return  pcPoint-wWord->pcUsingPointer;
	   }

	   pcPoint++;

    } // end of while

    if( pcPoint != wWord->pcUsingPointer )
	return  pcPoint-wWord->pcUsingPointer;

    qsError = 3003;
    return  0;

} //* end of search exprssion end function



/*---------------
 *  Function Name:  IdenRecognize( )
 *  Arguments:  Word *
 *  Unility:    recognize a iden or retain word
 *  Return: sucess : a word ( type, inter numberal ... )
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   IdenRecognize( void )
{
    unsigned short i;
    short          j, k;
    char 	   Temp[8], c;
    char 	  *sz;

    if( wWord->keyWordPossible )
    {

	if( (i = SearchWordEnd() ) == 0 ) {
	    qsError = 3006;
	    return  FALSE;
	}

	sz = wWord->pcUsingPointer;

	if( i <= 1 )
	    goto IDEN_JMP1;

	if( sz[0] == '/' )
	{
	    if( sz[1] == '/' )
	    { //remark
		wWord->nWordLength = 2;
		wWord->nType = Asql_RETAINTYPE;
		wWord->nInterNumberal = Asql_REM;
		wWord->pcUsingPointer += 2;
		wWord->xPointer = NULL;
		return  TRUE;
	    } else if( sz[1] == '*' ) {		//  /*
		wWord->nWordLength = 2;
		wWord->nType = Asql_RETAINTYPE;
		wWord->nInterNumberal = Asql_REM2;
		wWord->pcUsingPointer += 2;
		wWord->xPointer = NULL;
		return  TRUE;
	    }
	}

	//not able to be a keyword
	for(j = 0;  j < i;  j++) {
	    if( !isalpha(sz[j]) )
		goto IDEN_JMP1;
	}

	k = toupper(sz[0]) - 'A';
	j = asqlKeyHash[k];
	k = asqlKeyHash[k+1];
	for( ;  j < k ;  j++ ) {
	    if( !strnicmp(sz, retainarry[j], i) ) {

	/*old version
	for( j = 0; j < TYPERETAINWORDBEGIN; j++ ) {
	    if( !strnicmp(sz, retainarry[j], i ) ) {
	*/
		wWord->nWordLength = i;
		wWord->nType = Asql_RETAINTYPE;
		wWord->nInterNumberal = 1101+j;
		wWord->pcUsingPointer += i;
		wWord->xPointer = NULL;
		return  TRUE;
	    }
	}
    }

IDEN_JMP1:

    if( wWord->keyWordPossible == 2  )
    {	//there should be an keyword, but not
	qsError = 3018;
	return  FALSE;
    }

    IgnoreUselessChar();

    if( *(wWord->pcUsingPointer) == Asql_STATSYMB ) {
/*allow the stat and sum appeares at the same time.
	if( *AsqlStatTarget == '\0' ) {
		qsError = 3004;
		return  FALSE;
	}*/
	switch( toupper( *(++(wWord->pcUsingPointer)) ) ) {
	    case 'A':
	    case 'Y':
		i = 0;
		(wWord->pcUsingPointer)++;
		while( (c = *((wWord->pcUsingPointer)++)) != ':' ) {
			Temp[i++] = c;
			if( i >= 6 ) {
				qsError = 3005;
				return FALSE;
			}
		}
		Temp[i] = '\0';
		j = atoi(Temp);
		if( j > asqlStatMaxY )
			asqlStatMaxY = j;
		sprintf(StatementPreAction, "staty(%d)", j-1 );
		break;

	    case 'B':
	    case 'X':
		i = 0;
		(wWord->pcUsingPointer)++;
		while( (c = *((wWord->pcUsingPointer)++)) != ':' ) {
			Temp[i++] = c;
			if( i >= 6 ) {
				qsError = 3005;
				return FALSE;
			}
		}
		Temp[i] = '\0';
		i = atoi(Temp)-1;

		switch( iAsqlStatAction ) {
		    case 1:
			 sprintf(StatementPreAction, "statx(%d,%s)", \
							i, AsqlStatTarget);
			 break;
		    case 2:
			sprintf(StatementPreAction, "sumx(%d,%s)", \
							  i, AsqlStatTarget);
			break;
		    case 3:
			sprintf(StatementPreAction, "xavg(%d,%s)", \
							  i, AsqlStatTarget);
			break;
		    case 4:
			sprintf(StatementPreAction, "xmax(%d,%s)", \
							  i, AsqlStatTarget);
			break;
		    case 5:
			sprintf(StatementPreAction, "xmin(%d,%s)", \
							  i, AsqlStatTarget);
			break;
		    default:
			qsError = 4005;		//not define stat action
			return  FALSE;
		}

	} // end of switch
    } // end of if

    wWord->nType = Asql_EXPRTYPE;

    // when we meet a statistics statement here, we should mention wehther
    // we will get a not 0 length expression for CalExpr will return a
    // TRUE when it meet expression such as: ""
    if( (wWord->nWordLength = SearchExprEnd()) <= 0 ) {
	if( *StatementPreAction == '\0' )       return  FALSE;
	if( (wWord->xPointer = WordAnalyse( "" )) == NULL ) {
		qsError = 4001;
		return  FALSE;
	}
    } else {
	sz = wWord->pcUsingPointer+wWord->nWordLength;
	c = *sz;
	*sz = '\0';
	if( (wWord->xPointer = WordAnalyse( wWord->pcUsingPointer )) == NULL ) {
		qsError = 4002;
		return  FALSE;
	}
	*sz = c;
    }

    wWord->pcUsingPointer += wWord->nWordLength;
    wWord->nInterNumberal = Asql_EXPRESSION;

    return  TRUE;

} // end of function IdenRecognize()


/*---------------
 *  Function Name:  GetWord( )
 *  Arguments:  Word *
 *  Return: sucess : a word ( type, inter numberal ... )
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   GetWord( void )
{
    unsigned short i;

    if( wWord->unget ) {
	wWord->unget = 0;
	return  TRUE;
    }

    wWord->nType = wWord->nInterNumberal = wWord->nWordLength = \
					      wWord->nCol = wWord->nLine = 0;
    wWord->xPointer = NULL;

// this sentence is called in wEof, so we don't call this function here
//    IgnoreUselessChar( wWord );

    /*if( wWord->keyWordPossible == 2 ) {
	wWord->keyWordPossible = 1;
    } else if( wWord->keyWordPossible == 3 ) {
	wWord->keyWordPossible = 0;
    } else {*/
    if( wWord->keyWordPossible == 3 ) {
	wWord->keyWordPossible = 0;
    } else if( wWord->keyWordPossible != 2  ) {
	if( wEofTest() ) {
	    wWord->nWordLength = 0;
	    wWord->nType = Asql_FILEEND;
	    wWord->nInterNumberal = Asql_RETAINTYPE;   //OLD: Asql_ACTIONEXPR;
	    wWord->pcUsingPointer = NULL;
	    return  TRUE;           // treat this as normal
        }

	i = 0;
	while( i == 0 ) {
	   switch( *(wWord->pcUsingPointer) ) {
		case ' ':
		case '\t':
		case '\xA1':
			wWord->pcUsingPointer++;
			break;
		case '\r':
		case '\n':
			i = 2;
			break;
		default:
			i = 1;
	   }
	}
	if( i == 2 )
		wWord->keyWordPossible = 1;
	else
		wWord->keyWordPossible = 0;
    }

    if( wEof() ) {
	wWord->nWordLength = 0;
	wWord->nType = Asql_FILEEND;
	wWord->nInterNumberal = Asql_RETAINTYPE;   //OLD: Asql_ACTIONEXPR;
	wWord->pcUsingPointer = NULL;
	return  TRUE;           // treat this as normal
    }

    switch( wWord->nReadMethod )  {
	case Asql_STANDARDREAD:
	      if( *wWord->pcUsingPointer != Asql_SEPSYMB ) {
		    if( IdenRecognize() == FALSE ) {
			 return  FALSE;
		    }
		    return  TRUE;
	      }
	      wWord->pcUsingPointer++;   /* $,# */
	      if( ActionRecognize() == FALSE ) {
		  qsError = 3007;
		  return  FALSE;
	      }
	      return  TRUE;

	case Asql_STRINGREAD:
	      if( (i = SearchWordEnd()) == 0 )
		  return  FALSE;
	      wWord->nType = Asql_STRINGTYPE;
	      wWord->nWordLength = i;
	      wWord->nInterNumberal = 0;
	      // trans the define value by xPointer
	      wWord->xPointer = (MidCodeType *)wWord->pcUsingPointer;
	      wWord->pcUsingPointer += i;
	      return  TRUE;
    }

    return FALSE;

} //* end of function GetWord()


/*==========================================================================
 *  Function Name:      AsqlAnalyse()
 *  arguments:
 *  input:  ASK data describ
 *  output: middle result
 *  return: sucess: TRUE
 *          unsucess: FALSE
 *  call:
 *        all retain word process module
 *  qsError:
 ==========================================================================*/
short  AsqlAnalyse( char *pFilename, char *buf )
{

    // text in file, but never prepared
    if( wWord == NULL ) {
	if( (wWord = pWordAnaPre( pFilename, buf )) == NULL ) {
		return FALSE;
	}

	wWord->nReadMethod = Asql_STANDARDREAD;
	wWord->keyWordPossible = 2;
	if( GetWord() == FALSE ) {
		wWordFree();
		qsFree();
//              qsError = 3008;
		return  FALSE;
	}
    } // end of if wWord==NULL


    // uphold the action table
    nActSerialCode = 0;

    //$$$$$$$$here is a test
    //programPro();

    while( wWord->nType == Asql_RETAINTYPE ) {
	 switch( wWord->nInterNumberal ) {
	    //case Asql_ACTION:
	    case Asql_LET:
	    case Asql_IF:
	    case Asql_WHILE:
	    case Asql_CALL:
		 if( programPro() == FALSE ) {
			wWordFree();
			qsFree();
			return  FALSE;
		 }
		 ConditionAppeared = 1;
		 break;
	    case Asql_ACTION:
		 if( ActPro() == FALSE ) {
			wWordFree();
			qsFree();
//                      qsError = 3009;
			return  FALSE;
		 }
		 break;

	    case Asql_GROUPACTION:
		 if( GroupActPro() == FALSE ) {
			wWordFree();
			qsFree();
//                      qsError = 3009;
			return  FALSE;
		 }
		 break;

	    case Asql_CONDITION_PRA:

		 if( ConditionAppeared == 1 ) {
			if( asqlStatMaxY > 0 )
			{
			    if( (asqlStatFlagY = malloc(asqlStatMaxY+32)) == NULL ) {
				qsError = 1001;
				return  FALSE;
			    }
			    memset(asqlStatFlagY, 0, asqlStatMaxY);
			}
			return  TRUE;
		 }

		 if( QueryPro() == FALSE ) {
			wWordFree();
			qsFree();
//                      qsError = 3010;
			return  FALSE;
		 }
		 ConditionAppeared = 1;
		 break;

	    case Asql_DEFINE:
		 if( DefinePro() == FALSE ) {
			wWordFree();
			qsFree();
//                      qsError = 3011;
			return  FALSE;
		 }
		 break;

	    case Asql_FROM:
		 if( ConditionAppeared == 1 ) {
			if( asqlStatMaxY > 0 )
			{
			    if( (asqlStatFlagY = malloc(asqlStatMaxY+32)) == NULL ) {
				qsError = 1001;
				return  FALSE;
			    }
			    memset(asqlStatFlagY, 0, asqlStatMaxY);
			}
			return  TRUE;
		 }

		 if( FromPro() == FALSE ) {
			wWordFree();
			qsFree();
//                      qsError = 3012;
			return  FALSE;
		 }
		 break;

	    case Asql_GROUPBY:
		 if( GroupPro() == FALSE ) {
			wWordFree();
			qsFree();
//                      qsError = 3013;
			return  FALSE;
		 }
		 break;

	    case Asql_ORDERBY:
		 if( OrderbyPro() == FALSE ) {
			wWordFree();
			qsFree();
//                      qsError = 3013;
			return  FALSE;
		 }
		 break;

	    case Asql_PREDICATES:

		 if( ConditionAppeared == 1 ) {
			if( asqlStatMaxY > 0 )
			{
			    if( (asqlStatFlagY = malloc(asqlStatMaxY+32)) == NULL ) {
				qsError = 1001;
				return  FALSE;
			    }
			    memset(asqlStatFlagY, 0, asqlStatMaxY);
			}
			return  TRUE;
		 }

		 if( PredicatesPro() == FALSE ) {
			wWordFree();
			qsFree();

			if( qsError == 0 )
				qsError = 3014;

			return  FALSE;
		 }
		 break;

	    case Asql_TITLE:
		 if( TitlePro() == FALSE ) {
			wWordFree();
			qsFree();
//                      qsError = 3015;
			return  FALSE;
		 }
		 break;

	    case Asql_TEXT:
		 if( TextPro() == FALSE ) {
			wWordFree();
			qsFree();
//                      qsError = 3015;
			return  FALSE;
		 }
		 break;

	    case Asql_TO:
		 if( ConditionAppeared == 1 ) {
			if( asqlStatMaxY > 0 )
			{
			    if( (asqlStatFlagY = malloc(asqlStatMaxY+32)) == NULL ) {
				qsError = 1001;
				return  FALSE;
			    }
			    memset(asqlStatFlagY, 0, asqlStatMaxY);
			}
			return  TRUE;
		 }

		 if( ToPro() == FALSE )    {
			wWordFree();
			qsFree();
//                      qsError = 3016;         qsError comes from inside
			return  FALSE;
		 }
		 break;

            case Asql_UPDATE:
	         if( AsqlUpdatePro() == FALSE ) {
		     qsError = 3023;
		     return  FALSE;
		 }
		 break;

	    case Asql_DATABASE:
		 if( DatabasePro() == FALSE ) {
		     return  FALSE;
		 }
		 break;

	    case Asql_EXCLUSIVE:
		 ExclusivePro();
		 break;

	    case Asql_FUNCTION:
	    {
		ExecActTable *exExAct;
		short        wordlength;

		//get function Name
		if( (wordlength = SearchWordEnd()) == 0 )
			return  FALSE;

		strZcpy(fFrTo.asqlFun[fFrTo.funNum].funName, \
				wWord->pcUsingPointer, min(32,wordlength+1));
		wWord->pcUsingPointer += wordlength;

		exExAct = fFrTo.exHead;
		fFrTo.exHead = NULL;

		if( GetWord() == FALSE ) {
			wWordFree();
			qsFree();
			return  FALSE;
		}

		if( programPro() == FALSE ) {
			wWordFree();
			qsFree();
			return  FALSE;
		 }
		 fFrTo.asqlFun[fFrTo.funNum++].exExAct = fFrTo.exHead;
		 fFrTo.exHead = exExAct;

		 //eat the word "END"
		 wWord->unget = 0;
	    }
	    break;

	    case Asql_COMMIT:
		 if( asqlStatMaxY > 0 )
		 {
		     if( (asqlStatFlagY = malloc(asqlStatMaxY+32)) == NULL ) {
			qsError = 1001;
			return  FALSE;
		     }
		     memset(asqlStatFlagY, 0, asqlStatMaxY);
		 }

		 wWord->keyWordPossible = 2;
		 if( GetWord() == FALSE ) {
		     wWordFree();
		     qsFree();
		     return  FALSE;
		 }

		 if( wWord->nType == Asql_FILEEND )
		 {
		     //......
		     //commit    <- to omit this commit
		     //FILEEND(\x1A)

		     break;
		 }

		 ConditionAppeared = 1;

		 return  TRUE;
	    break;

	    case Asql_REM:
		if( RemarkPro() == FALSE )
		    return  FALSE;
	    break;

	    case Asql_REM2:
		if( Remark2Pro() == FALSE )
		    return  FALSE;
	    break;

	    case Asql_MODISTRU:
		if( ModiStruPro() == FALSE )
		    return  FALSE;
	    break;

	    case Asql_TRY:
		if( TryPro() == FALSE )
		    return  FALSE;
	    break;

	    case Asql_EXCEPT:
		if( ExceptPro() == FALSE )
		    return  FALSE;

		//commit this work
		if( asqlStatMaxY > 0 )
		{
		     if( (asqlStatFlagY = malloc(asqlStatMaxY+32)) == NULL ) {
			qsError = 1001;
			return  FALSE;
		     }
		     memset(asqlStatFlagY, 0, asqlStatMaxY);
		}
		ConditionAppeared = 1;

		//prepare for the next segment
		wWord->nType = Asql_RETAINTYPE;
		wWord->nInterNumberal = Asql_NULL;

		//ASQL shouldn't eat another atom for afraid to meet
		//Asql_FILEEND for:
		// if( wWord->nType == Asql_FILEEND ) {
		//	wWordFree();
		// }
		//TO process EXCEPTION will use the wWord

		return  TRUE;
	    break;

	    case Asql_ENDEXCEPT:
		if( EndExceptPro() == FALSE )
		    return  FALSE;
	    break;

	    case Asql_SET:
		 if( AsqlSetPro() == FALSE )
		     return  FALSE;
	    break;

	    case Asql_NULL:
		;//do nothing fot NULL statement
	    break;

	    default:
		 wWordFree();
		 qsFree();
                 qsError = 3017;
		 return  FALSE;
	 } // end of switch

	 wWord->keyWordPossible = 2;
	 if( GetWord() == FALSE ) {
		wWordFree();
		qsFree();
//              qsError = 3008;
		return  FALSE;
	 }

	 if( wWord->nType == Asql_FILEEND )     break;

    } // end of while

    if( wWord->nType == Asql_FILEEND ) {
	ConditionAppeared = -1;         // use for stop query & statistics
/*the following is deleted by Xilong 1994.11.1
  move it into function wWordFree()
	if( wWord->nFlush == AsqlExprInFile ) {
		fclose( wWord->fp );
*/              wWordFree();
//       }
	if( asqlStatMaxY > 0 )
	{
	    if( (asqlStatFlagY = malloc(asqlStatMaxY+32)) == NULL ) {
		qsError = 1001;
		return  FALSE;
	    }
	    memset(asqlStatFlagY, 0, asqlStatMaxY);
	}
	return  TRUE;
    }

    if( szAsqlErrBuf[0] == '\0' )
	 strcpy(szAsqlErrBuf,"...");
    else strcat(szAsqlErrBuf,"...");
    strcat(szAsqlErrBuf, substr(wWord->pcUsingPointer, 0, sizeof(szAsqlErrBuf)-strlen(szAsqlErrBuf)) );

    qsError = 3018;

    qsFree();
    return  FALSE;

} /* end of query and statistics analyst */


/*---------------
 *  Function Name:     DefinePro( )
 *  input:  ASK data describ from segment
 *  output: from access way
 *  return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   DefinePro( void )
{

    SYSDEFINETYPE *def;
    short nNameLength;

    if( (fFrTo.dHead = (SYSDEFINETYPE *)realloc( fFrTo.dHead, \
			(fFrTo.nDefinNum+1) * sizeof(SYSDEFINETYPE) ) ) == \
								     NULL ) {
	  return  FALSE;
    }

    def = (SYSDEFINETYPE *)fFrTo.dHead;
    wWord->nReadMethod = Asql_STRINGREAD;

    wWord->keyWordPossible = 3;
    if( GetWord() == FALSE )
	return  FALSE;
    if( wWord->nType == Asql_FILEEND )
	return  FALSE;

    strncpy( def[fFrTo.nDefinNum].DefineName, (char *)wWord->xPointer, \
							wWord->nWordLength );
    def[fFrTo.nDefinNum].DefineName[wWord->nWordLength] = '\0';

// if the define body cannot be null, the following two sentence is necessary
    if( (nNameLength = SearchVarChar("\r\n")) == 0 )
	 return  FALSE;

    // get the define body
    strncpy( def[ fFrTo.nDefinNum ].DefineValues, \
			(char *)wWord->pcUsingPointer, nNameLength );
    def[ fFrTo.nDefinNum ].DefineValues[ nNameLength ] = '\0';
    wWord->pcUsingPointer += nNameLength;
    wWord->nReadMethod = Asql_STANDARDREAD;
    fFrTo.nDefinNum++;

    return  TRUE;

} // end of DefineProc()

unsigned short getAtoken(char *seed, int tokenBufLen, char *tokenBuf)
{
    unsigned short ui = 0;
    int            tokenBufGot = 0;
    char           buf[512];
    char          *sz;

    tokenBuf[0] = '\0';
    IgnoreUselessChar();

    sz = wWord->pcUsingPointer;

    if( *sz == '"' ) {
	for(ui = 1;  sz[ui] != '\0';  ui++)
	{
	    if( sz[ui] == '"' && sz[ui-1] != '\\' ) {
		strZcpy(buf, &(sz[1]), min(510, (int)ui));
		cnStrToStr(tokenBuf, buf, '\\', (short)tokenBufLen);
		tokenBufGot = 1;
		break;
	    }
	}
	ui += strcspn( &(sz[ui+1]), seed )+1;
    } else {
	if( (ui = SearchVarChar(seed)) == 0 )
	    return  0;
    }

    if( ui == 0 ) {
	if( *sz == '\0' )
	    return  0;
    }
    ui++;

    if( tokenBufGot == 0 ) {
	strZcpy(tokenBuf, sz, min(tokenBufLen, ui+1));
    }

    return  ui;

} //end of getAtoken()




/*--------------
 *  Function Name:      FromPro()
 *  input:  ASK data describ from segment
 *  output: from access way
 *  return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   FromPro( void )
{

    unsigned short nVarLength, nDbNum, ui;
    unsigned char  *temp, *p, *pb, c;
    dFILE 	 **df = NULL;
    char 	   buf[MAXPATH];
    char           alias[10];
    char	   directManDf = 0;

    // if now we meet the next FROM, close the first one
    if( fFrTo.cSouDbfNum >= 1 ) {
	short  i;

	if( fFrTo.phuf != NULL )
	{ //commit & clear it
	    asqlDbCommit();

	    for(i = fFrTo.cSouDbfNum - 1;   i >= 0;   i--) {
		freeRecLock( fFrTo.cSouFName[i] );
	    }

	    fseek(fFrTo.phuf, 0, SEEK_SET);
	    fwrite("\0\0\0\0", sizeof(dFILE *), 1, fFrTo.phuf);
	}

	memset(&(fFrTo.syncBh), 0, sizeof(bHEAD *)*MOST_DBFNUM_ONCEQUERY);
	for(i = fFrTo.cSouDbfNum - 1;   i >= 0;   i--) {
		if( fFrTo.fromExclusive )
			wmtDbfUnLock(fFrTo.cSouFName[i]);

		dSleep(fFrTo.cSouFName[i]);
	}
	free( fFrTo.cSouFName );
	fFrTo.cSouDbfNum = 0;
    }

    if( (nVarLength = SearchVarChar("\n\r")) == 0 )
	return  FALSE;

    p = wWord->pcUsingPointer;
    pb = p;
    while( *p == '(' ) {
	 p = ++(wWord->pcUsingPointer);

	 ui = getAtoken(":,)", Asql_KEYWORDLENGTH, fFrTo.szScopeKey);
	 if( ui == 0 )
	     break;

	 if( strnicmp(fFrTo.szScopeKey, "SCOPE", 5) != 0 ) {
	     /*wWord->pcUsingPointer += ui;
	     fFrTo.szScopeKey[0] = '\0';
	     */
	     qsError = 2004;
	     return  FALSE;
	 }

	 if( fFrTo.szScopeKey[5] != ':' && fFrTo.szScopeKey[5] != ',' ) {
	     char *sz;
	     int   len;

	     for( sz = &fFrTo.szScopeKey[5];
		  *sz != '\0' && *sz == '\t' || *sz == ' ';
		  sz++);

	     len = strlen(sz)-1;

	     //when the len is 0, this is correct, len won't be neg for
	     //this is a token, and has a ':' or ',' hasn't be used
	     if( strnicmp(sz, "ASCEND", len) == 0 ) {
		fFrTo.iScopeSkipAdjust = 0;
	     } else  if( strnicmp(sz, "DECEND", len) != 0 ) {
		fFrTo.iScopeSkipAdjust = -2;
	     } else {
		qsError = 2004;
		return  FALSE;
	     }
	 } else {
	     fFrTo.iScopeSkipAdjust = 0;
	 }

	 if( wWord->pcUsingPointer[ui] == ')' ) {
	     /*wWord->pcUsingPointer += ui;
	     fFrTo.szScopeKey[0] = '\0';
	     */
	     qsError = 2004;
	     return  FALSE;
	 }
	 wWord->pcUsingPointer += ui;

	 //key
	 ui = getAtoken(":,)", Asql_KEYWORDLENGTH, fFrTo.szScopeKey);
	 if( ui == 0 || wWord->pcUsingPointer[ui-1] == ')' ) {
	     /*wWord->pcUsingPointer += ui;
	     fFrTo.szScopeKey[0] = '\0';
	     */
	     qsError = 2004;
	     return  FALSE;
	 }
	 fFrTo.szScopeKey[ui-1] = '\0';
	 wWord->pcUsingPointer += ui;

	 //scope start
	 ui = getAtoken(":,)", Asql_KEYWORDLENGTH, fFrTo.szScopeStart);
	 if( ui == 0 ) {
	     ui = getAtoken(")", Asql_KEYWORDLENGTH, fFrTo.szScopeStart);
	     wWord->pcUsingPointer += ui;
	     fFrTo.szScopeEnd[0] = '\x7F'; //give it a max value
	     break;
	 }

	 if( wWord->pcUsingPointer[ui-1] == ')' ) {
	     wWord->pcUsingPointer += ui;
	     fFrTo.szScopeEnd[0] = '\x7F'; //give it a max value
	     break;
	 }
	 fFrTo.szScopeStart[ui-1] = '\0';
	 wWord->pcUsingPointer += ui;

	 //scope end
	 ui = getAtoken(":,)", Asql_KEYWORDLENGTH, fFrTo.szScopeEnd);
	 if( ui != 0 ) {
	     fFrTo.szScopeEnd[ui-1] = '\0';
	     wWord->pcUsingPointer += ui;
	 } else {
	     ui = getAtoken(")", Asql_KEYWORDLENGTH, fFrTo.szScopeEnd);
	     wWord->pcUsingPointer += ui;
	     fFrTo.szScopeEnd[0] = '\x7F';
	 }
    }

    nVarLength -= (wWord->pcUsingPointer-pb);

    p = wWord->pcUsingPointer;

    while( *p == '\t' || *p == ' ' ) p++;

    if( strnicmp(p, "SQL:", 4) == 0 )
    { //SQL source
	if( (df = (dFILE **)realloc(df, sizeof(dFILE *)) ) == NULL ) {
		qsError = 1003;
		return  FALSE;
	}

	dsetbuf(0);
	c = p[nVarLength];
	p[nVarLength] = '\0';
	if( ( df[0] = dAwake(p, DOPENPARA) ) == NULL ) {
		p[nVarLength] = c;
		qsError = 2003;
		return  FALSE;
	}
	p[nVarLength] = c;
	fFrTo.cSouDbfNum = 1;
	fFrTo.cSouFName = df;
	wWord->pcUsingPointer += nVarLength;

	return  TRUE;
    }

    if( strnicmp(p, "ODBC:", 5) == 0 )
    { //SQL source
	if( (df = (dFILE **)realloc(df, sizeof(dFILE *)) ) == NULL ) {
		qsError = 1003;
		return  FALSE;
	}

	dsetbuf(0);
	c = p[nVarLength];
	p[nVarLength] = '\0';
	if( ( df[0] = dAwake(p, DOPENPARA) ) == NULL ) {
		p[nVarLength] = c;
		qsError = 2003;
		strZcpy(szAsqlErrBuf, szOdbcExecResult, sizeof(szAsqlErrBuf) );
		return  FALSE;
	}
	p[nVarLength] = c;
	fFrTo.cSouDbfNum = 1;
	fFrTo.cSouFName = df;
	wWord->pcUsingPointer += nVarLength;

	return  TRUE;
    }

    nDbNum = 0;
    while( TRUE ) {
	 temp = p;
	 if( *temp == '*' )
	 { //keyword expression must at the tail 1998.1.18
		int   ic;
		char *sz = fFrTo.szKeyField;

		ic = 0;
		p++;	//skip '*'
		while( *p && *p != '\n' && *p != '\r' ) {
		    if( ic < ASQL_MAX_REL_EXPR_LEN-1 )
			sz[ic++] = *p++;
		    else {
			sz[ic] = '\0';
			break;
		    }
		}
		sz[ic] = '\0';

		break;

	 } else {

		//while( *p && *p != '\n' && *p != '\r' && *p != ',' )  p++;

		while( asc_state_1[*p] )  p++;
		c = *p;
		*p = '\0';

		//2000.3.3
		if( nDbNum >= MOST_DBFNUM_ONCEQUERY ) {
			qsError = 1003;
			return  FALSE;
		}

		if( (df = (dFILE **)realloc(df, (nDbNum+1) * \
				      sizeof(dFILE *)) ) == NULL ) {
			qsError = 1003;
			return  FALSE;
		}

#ifdef WSToMT
		if( lServerAsRunning ) {
		    if( temp[0] == '"' && (temp[2] == ':' || temp[1] == '\\') ) {
			strZcpy(buf, temp, MAXPATH);
			directManDf = 1;
		    } else if( temp[1] == ':' || temp[0] == '\\' ) {
			strZcpy(buf, temp, MAXPATH);
			directManDf = 1;
		    } else if( temp[0] == '#' ) {
			sprintf(buf, "%s%s.%03X", tmpPath, trim(temp), intOfThread&0xFFF);
			directManDf = 1;
		    } else {
			directManDf = 0;
			if( *temp != '^' ) {
			    strcpy(buf, asqlEnv.szAsqlFromPath);
			    strncat(buf, temp, MAXPATH-strlen(buf));
			} else {
			    strcpy(buf, asqlEnv.szAsqlResultPath);
			    strncat(buf, temp+1, MAXPATH-strlen(buf));
			}
		    }
		} else
#endif
		{
		    if( *temp == '#' ) {
			sprintf(buf, "%s%s.%03X", tmpPath, trim(temp), intOfThread&0xFFF);
		    } else if( *temp != '^' ) {
			makefilename(buf, asqlEnv.szAsqlFromPath, temp);
		    } else {
			makefilename(buf, asqlEnv.szAsqlFromPath, temp+1);
		    }
		}

#ifdef __BORLANDC__
		if( nDbNum == 0 ) {
			dsetbuf(32000);
		} else {
			dsetbuf(8192);
		}
#else
		dsetbuf(64000);
#endif
		{ //get alias

		    char *sz;

		    lrtrim(buf);
		    if( buf[0] == '"' ) {
			sz = strchr(&buf[1], '"');
			if( sz != NULL )
			    sz = &buf[strcspn(sz+1, " \t")];
		    } else {
			sz = &buf[strcspn(buf, " \t")];
		    }

		    if( sz != NULL && *sz != '\0' ) {
			*sz = '\0';
			strZcpy(alias, trim(sz+1), 10);
			if( alias[0] != '\0' && !isalpha(alias[0]) ) {
			    qsError = 2008;
			    return  FALSE;
			}
		    } else
			alias[0] = '\0';
		}

		if( directManDf )
		{ //menaged this table by myself, donn't use the
		  //ASQL system dictionary

		    char buff[288];

		    // '$' means imposible database
		    strcpy(buff, "$*");

		    strcat(buff, buf);
		    if( ( df[nDbNum] = dAwake(buff, DOPENPARA) ) == NULL ) {
			fFrTo.cSouDbfNum = 0;	    //dAeake() perhaps will reset this
			qsError = 2003;
			return  FALSE;
		    }
		} else
		if( fFrTo.AsqlDatabase[0] == '\0' )
		{
		    if( ( df[nDbNum] = dAwake(buf, DOPENPARA) ) == NULL ) {
			fFrTo.cSouDbfNum = 0;	    //dAeake() perhaps will reset this
			qsError = 2003;
			return  FALSE;
		    }
		} else {
		    char buff[288];

		    strcpy(buff, fFrTo.AsqlDatabase);
		    strcat(buff, "*");
		    strcat(buff, buf);
		    if( ( df[nDbNum] = dAwake(buff, DOPENPARA) ) == NULL ) {
			fFrTo.cSouDbfNum = 0;
			qsError = 2003;
			return  FALSE;
		    }
		}

		strcpy(df[nDbNum]->szAlias, alias);

		//authoriate the df
		{
		    ASQLAUDIT audit;
		    char      *sp, *sp1;
		    char      buf[128];

		    audit.dbId = 0;
		    audit.tbId = df[nDbNum]->tbid;
		    sp = readBtreeData(asqlAudit, (char *)&audit, NULL, 0);
		    if( sp != NULL )
		    { //the right string is as:
		      //`username(upper)`rwe`username(upper)`rwe`

			strcpy(buf, "`");
			strcat(buf, stoUpper(asqlEnv.szUser));
			strcat(buf, "`");

			sp1 = strstr(sp, buf);
			if( sp1 == NULL ) {
			    qsError = 2003;
			    freeBtreeMem(sp);
			    return  FALSE;
			}

			if( (sp1 = strchr(sp1, '`')) == NULL ) {
			    qsError = 2003;
			    freeBtreeMem(sp);
			    return  FALSE;
			}

			//keep the right string
			strZcpy(&fFrTo.auditStr[nDbNum*SIZEOF_AUDITINFO], sp1, 4);

			freeBtreeMem(sp);
		    }
		}

		if( fFrTo.fromExclusive )
			wmtDbfLock(df[nDbNum]);

		//(bHEAD *)(df[nDbNum]->bhs[0]) = IndexAwake((char *)df[nDbNum], df[nDbNum]->name, BTREE_FOR_OPENDBF);
		nDbNum++;
	 }
	 *p = c;
	 while( *p == '\t' || *p == ' ' || *p == ',' ) p++;
	 if( *p == '\0' || *p == '\n' || *p == '\r' )  break;
    } // end of while

    fFrTo.cSouDbfNum = nDbNum;
    fFrTo.cSouFName = df;
    wWord->pcUsingPointer += nVarLength;

#ifdef __BORLANDC__
    dsetbuf( 32000 );
#else
    dsetbuf( 64000 );
#endif

    return  TRUE;

} // end of FormProc


/*---------------
 *  Function Name:      ToPro()
 *  Input:  ASK data describ "TO" segment
 *  Output: to access way
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   ToPro( void )
{
    char *temp1, *temp, c;
    short nVarLength;

    // if now we meet the TO, first close the dtp
#ifdef WSToMT
    if( fFrTo.phuf != NULL ) {
	//needn't
	//wmtDbfLock( fFrTo.targefile );
	/*
	asqlDbCommit();
	fclose(fFrTo.phuf);*/
    } else
	//1998.11.16
	//if( fFrTo.toAppend != '1' )
	if( wmtDbfIsLock( fFrTo.targefile ) )
	{
	    wmtDbfUnLock( fFrTo.targefile );
	}
#endif
    if( fFrTo.targefile != NULL ) {
	//if we have meet TO already, to check wether the dbf is awaked by
	//our ask with cTargetFileName, for the handle can be pass from
	//upper call
	if( fFrTo.cTargetFileName[0] != '\0' && fFrTo.toAppend == '0' ) {

	   if( fFrTo.nOrderby )
		orderbyExec( &fFrTo );

	   dSleep( fFrTo.targefile );
        }
	fFrTo.targefile = NULL;
	if( fFrTo.fieldtable != NULL ) {
		free( fFrTo.fieldtable );
		fFrTo.nFieldNum = 0;
		fFrTo.fieldtable = NULL;
	}
	if( fFrTo.TargetField != NULL ) {
		free( fFrTo.TargetField );
		fFrTo.TargetField = NULL;
	}
    } // end of if

    if( (nVarLength = SearchVarChar("\n\r")) == FALSE ) {
		qsError = 3019;
		return  FALSE;
    }

    temp = wWord->pcUsingPointer;
    wWord->pcUsingPointer += nVarLength;
    c = *wWord->pcUsingPointer;
    *wWord->pcUsingPointer = '\0';

    fFrTo.toAppend = '0';      //donot know
    if( *temp == '(' )
    { //To's explain
        char buf[256];
        int  i;

        temp1 = strchr(temp, ')');
        if( temp1 == NULL ) {
		qsError = 3019;         //to mode set error
		return  FALSE;
        }
        strZcpy(buf, temp+1, (temp1-temp)%255);
        i = strlen(trim(buf));

        if( strnicmp(buf, "APPEND", i) == 0 )
            fFrTo.toAppend = '1';

        temp1 = "P";
        //temp = temp1+1;
    } else {


//dFIELD *GenerateTable( char **TableName, \
//                     dFILE *DfileHandle[], \
//                     unsigned short DfileNum, \
//                     unsigned char *Description )

    fFrTo.TargetField = GenerateDfield(&temp1, fFrTo.cSouFName,\
						   fFrTo.cSouDbfNum, temp);
    }
//    if( fFrTo.TargetField  == NULL ) return( FALSE );

    if( qsError != 0 ) return  FALSE;

#ifdef WSToMT
    if( lServerAsRunning ) {
	if( temp1[1] == ':' || temp[0] == '\\' ) {
	   strZcpy(fFrTo.cTargetFileName, temp1, MAXPATH);
	} else if( temp[0] == '#' ) {
	   sprintf(fFrTo.cTargetFileName, "%s%s.%03X", tmpPath, trim(temp1), intOfThread&0xFFF);
	} else {
	   if( *temp1 != '^' ) {
		strcpy(fFrTo.cTargetFileName, asqlEnv.szAsqlResultPath);
		strncat(fFrTo.cTargetFileName, temp1, MAXPATH-strlen(fFrTo.cTargetFileName));
	   } else {
		strcpy(fFrTo.cTargetFileName, asqlEnv.szAsqlFromPath);
		strncat(fFrTo.cTargetFileName, temp1+1, MAXPATH-strlen(fFrTo.cTargetFileName));
	   }
	}
    } else
#endif

    {
	if( *temp1 == '#' ) {
	    sprintf(fFrTo.cTargetFileName, "%s%s.%03X", tmpPath, trim(temp1), intOfThread&0xFFF);
	} else if( *temp1 != '^' ) {
	    makefilename(fFrTo.cTargetFileName, asqlEnv.szAsqlResultPath, temp1);
	} else {
	    makefilename(fFrTo.cTargetFileName, asqlEnv.szAsqlFromPath, temp1+1);
	}
    }

    *wWord->pcUsingPointer = c;

    return  TRUE;

} // end of function ToProc()



/*---------------
 *  Function Name:      ModiStruPro()
 *  Input:  ASK data describ "ModiStru" segment
 *  Output: to access way
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   ModiStruPro( void )
{
    char  *temp1, *temp, c;
    short  nVarLength;
    char   cTargetFileName[MAXPATH];
    char   buf[MAXPATH];
    dFIELD *TargetField;
    dFILE  *df;
    dFILE  *DfileHandle[1];

    if( (nVarLength = SearchVarChar("\n\r")) == FALSE ) {
	qsError = 3101;
	return  FALSE;
    }

    temp = wWord->pcUsingPointer;
    wWord->pcUsingPointer += nVarLength;
    c = *wWord->pcUsingPointer;
    *wWord->pcUsingPointer = '\0';

    strZcpy(buf, temp, MAXPATH);
    temp1 = strchr(buf, ',');
    if( temp1 == NULL ) {
	//modi stru describe error, not type as tablename,fielddes
	qsError = 3102;	    
	return  FALSE;
    } else {
	*temp1 = '\0';
    }

#ifdef WSToMT
    if( lServerAsRunning ) {
	if( buf[1] == ':' || temp[0] == '\\' ) {
	   strZcpy(cTargetFileName, buf, MAXPATH);
	} else {
	   if( *buf != '^' ) {
		strcpy(cTargetFileName, asqlEnv.szAsqlFromPath);
		strncat(cTargetFileName, buf, MAXPATH-strlen(cTargetFileName));
	   } else {
		strcpy(cTargetFileName, asqlEnv.szAsqlResultPath);
		strncat(cTargetFileName, buf+1, MAXPATH-strlen(cTargetFileName));
	   }
	}
    } else
#endif

    {
	if( *buf != '^' ) {
	    makefilename(cTargetFileName, asqlEnv.szAsqlFromPath, buf);
	} else {
	    makefilename(cTargetFileName, asqlEnv.szAsqlResultPath, buf+1);
	}
    }

    DfileHandle[0] = dAwake(cTargetFileName, DOPENPARA);
    if( DfileHandle[0] == NULL ) {
	qsError = 3103;
	return  FALSE;
    }

    wmtDbfLock(DfileHandle[0]);
    if( dIsAwake(cTargetFileName) > 1 ) {
	//error
	wmtDbfUnLock(DfileHandle[0]);
	dSleep(DfileHandle[0]);
	qsError = 3104;
	return  FALSE;
    }

    TargetField = GenerateDfield(&temp1, DfileHandle, 1, temp);

    if( qsError != 0 ) {
	wmtDbfUnLock(DfileHandle[0]);
	dSleep(DfileHandle[0]);
	qsError = 3105;
	return  FALSE;
    }

    wmtDbfUnLock(DfileHandle[0]);
    dSleep( DfileHandle[0] );

    df = dModiCreate(cTargetFileName, TargetField);
    free( TargetField );

    if( df == NULL ) {
	qsError = 3106;
	return  FALSE;
    }

    dclose( df );

    *wWord->pcUsingPointer = c;

    return  TRUE;

} // end of function ModiStruPro()



/*---------------
 *  Function Name:      TitlePro()
 *  Input:  ASK data describ "TITLE" segment
 *  Output: title access result
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   TitlePro( void )
{
    short nVarLength;
    short i;

    nVarLength = SearchVarChar("\n\r");

    if( nVarLength >= Asql_MAX_TITLELENGTH )
       i = Asql_MAX_TITLELENGTH;
    else
       i = nVarLength + 1;
    strZcpy(fFrTo.cTitleName, (char *)wWord->pcUsingPointer, i);

    wWord->pcUsingPointer += nVarLength;

    return TRUE;

} // end of TitlePro


/*---------------
 *  Function Name:    GroupPro( )
 *  Input:  ASK data describ "GROUPBY" segment
 *  Output: groupby access result
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   GroupPro(void)
{
    char  buf[256];
    char *sz;
    int	  i;

    fFrTo.nGroupby = Asql_GROUPYES;

    IgnoreUselessChar();

    sz = wWord->pcUsingPointer;
    for(i = 0;  sz[i] != '\0' && i < 4090;  i++) {
	if( sz[i] == '\n' || sz[i] == '\r' )
		break;
    }
    if( i <= 0 || i >= 4090) {
	qsError = 5003;
	return  FALSE;
    }

    strZcpy(buf, shrink(subcopy(sz,0,i)), 256);

    wWord->pcUsingPointer += i;

    fFrTo.cGyExact = 0;
    fFrTo.iScopeSkipAdjust = 0;
    fFrTo.wGbKeyLen = SHRT_MAX;

    if( buf[0] == '(' ) {

	char 		buff[256];
	char		tokenBuf[64];
	unsigned short  ui, tui;
	int		i;

	sz = strchr(buf, ')');
	if( sz == NULL ) {
	    qsError = 5003;
	    return  FALSE;
	}

	*sz = '\0';
	strcpy(buff, &buf[1]);
	strcpy(buf, ltrim(sz+1));

	tui = 0;
	for( i = 0;  i < 2;  i++ ) {
	    ui = getAtokenFromBuf(&buff[tui], 64, tokenBuf);
	    if( ui == 0 )
		break;

	    lrtrim(tokenBuf);
	    if( strnicmp(tokenBuf, "EXACT", ui) == 0 )
		fFrTo.cGyExact = 1;
	    else if( strnicmp(tokenBuf, "DESCEND", ui) == 0 )
		fFrTo.iScopeSkipAdjust = -2;
	    else if( strnicmp(tokenBuf, "KEYLEN", 6) == 0 ) {
		char bufA[256];
		char *szA;

		strZcpy(bufA, tokenBuf+6, ui+1);
		szA = strchr(bufA, ':');
		if( szA == NULL ) {
			qsError = 5003;
			return  FALSE;
		}
		fFrTo.wGbKeyLen = atoi(szA+1);
		if( fFrTo.wGbKeyLen <= 0 || fFrTo.wGbKeyLen >= BTREE_MAX_KEYLEN ) {
			qsError = 5003;
			return  FALSE;
		}
	    }

	    tui += ui + 1;
	}
    }

    strcpy(fFrTo.cGroKey, buf);

    return  TRUE;

} /* end of  GroupPro */


/*---------------
 *  Function Name:    OrderbyPro( )
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   OrderbyPro(void)
{
    char  buf[256];
    char *sz;
    int	  i;

    fFrTo.nOrderby = 1;

    IgnoreUselessChar();

    sz = wWord->pcUsingPointer;
    for(i = 0;  sz[i] != '\0' && i < 4090;  i++) {
	if( sz[i] == '\n' || sz[i] == '\r' )
		break;
    }
    if( i <= 0 || i >= 4090) {
	qsError = 5003;
	return  FALSE;
    }

    strZcpy(buf, shrink(subcopy(sz,0,i)), 256);

    wWord->pcUsingPointer += i;

    fFrTo.iScopeSkipAdjust = 0;
    fFrTo.wObKeyLen = SHRT_MAX;

    if( buf[0] == '(' ) {

	char 		buff[256];
	char		tokenBuf[64];
	unsigned short  ui, tui;
	int		i;

	sz = strchr(buf, ')');
	if( sz == NULL ) {
	    qsError = 5003;
	    return  FALSE;
	}

	*sz = '\0';
	strcpy(buff, &buf[1]);
	strcpy(buf, ltrim(sz+1));

	tui = 0;
	for( i = 0;  i < 2;  i++ ) {
	    ui = getAtokenFromBuf(&buff[tui], 64, tokenBuf);
	    if( ui == 0 )
		break;

	    lrtrim(tokenBuf);
	    if( strnicmp(tokenBuf, "DESCEND", ui) == 0 )
		fFrTo.iOrderbyScopeSkipAdjust = -2;
	    else if( strnicmp(tokenBuf, "KEYLEN", 6) == 0 ) {
		char bufA[256];
		char *szA;

		strZcpy(bufA, tokenBuf+6, ui+1);
		szA = strchr(bufA, ':');
		if( szA == NULL ) {
			qsError = 5003;
			return  FALSE;
		}
		fFrTo.wObKeyLen = atoi(szA+1);
		if( fFrTo.wObKeyLen <= 0 || fFrTo.wObKeyLen >= BTREE_MAX_KEYLEN ) {
			qsError = 5003;
			return  FALSE;
		}
	    }

	    tui += ui + 1;
	}
    }

    strZcpy(fFrTo.cOrderbyKey, buf, Asql_KEYWORDLENGTH);

    return  TRUE;

} /* end of OrderbyPro() */



/*---------------
 *  Function Name:  StatPro()
 *  Input:  ASK data describ "ACTION" segment
 *  Output: action access result
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
//#pragma argsused	/* Remed by NiuJingyu */
static short   StatPro( void )
{
    int i;
    char  *s;

    // stat statement should be writen as: stat x,maxy  but stop with '\n'
    IgnoreUselessChar();

    s = wWord->pcUsingPointer;
    for(i = 0;  s[i] != '\0' && i < 4096;  i++) {
	if( s[i] == '\n' || s[i] == '\r' )
		break;
    }
    if( i <= 0 || i >= 4096)     return  FALSE;

    strcpy(AsqlStatTarget, shrink(subcopy(s,0,i)));
    iAsqlStatAction = 1;

    s = strchr(AsqlStatTarget, ',');
    if( s != NULL ) {
        *s++ = '\0';
        asqlStatMaxY = atoi(s);
    }
/*    if( *AsqlStatTailAction == '\0' ) {
	strcpy(AsqlStatTailAction, "iarray(");
    }
    strcat(AsqlStatTailAction, AsqlStatTarget);
    strcat(AsqlStatTailAction, ",");
*/
    wWord->pcUsingPointer += i;

    //default MAX_Y
    //asqlStatMaxY = 1024;

    return TRUE;

} // end of function StatPro()



/*---------------
 *  Function Name:  SumPro()
 *  Input:  ASK data describ "ACTION" segment
 *  Output: action access result
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   SumPro( void )
{
    int   i;
    char *s;

    // sum statement should be writen as: sum total_s, y  but stop with '\n'

    IgnoreUselessChar();

    s = wWord->pcUsingPointer;
    for(i = 0;  s[i] != '\0' && i < 4096;  i++) {
	if( s[i] == '\n' || s[i] == '\r' )
		break;
    }
    if( i <= 0 || i >= 4096)     return  FALSE;

    strcpy(AsqlStatTarget, shrink(subcopy(s,0,i)) );
    iAsqlStatAction = 2;
    wWord->pcUsingPointer += i;

    //default MAX_Y
    //asqlStatMaxY = 1024;

    return TRUE;

} // end of function SumPro()


/*---------------
 *  Function Name:  LabelPro()
 *  author: 1997.10.8 Xilong
 *  Input:  ASK data describ "ACTION" segment
 *  Output: action access result
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   LabelPro( void )
{
    int   i;
    char *s;

    if( fFrTo.labelNum >= Asql_MAXLABEL ) {
	qsError = 3019;		//label too much
	return  FALSE;
    }

    IgnoreUselessChar();

    s = wWord->pcUsingPointer;
    for(i = 0;  s[i] != '\0' && i < 256;  i++) {
	if( s[i] == '\n' || s[i] == '\r' )
		break;
    }
    if( i <= 0 || i >= 256)     return  FALSE;

    strZcpy(fFrTo.asqlLabel[++fFrTo.labelNum].label, \
						   lrtrim(subcopy(s,0,i)), 32);
    fFrTo.asqlLabel[fFrTo.labelNum].etPoint = NULL;
    wWord->pcUsingPointer += i;

    return  TRUE;

} // end of function LabelPro()


/*---------------
 *  Function Name:  DatabasePro()
 *  author: 1997.11.6 Xilong
 *  Input:  ASK data describ "ACTION" segment
 *  Output: action access result
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   DatabasePro( void )
{
    int    i;
    char  *s;
    char   AsqlDatabase[32];

    IgnoreUselessChar();

    s = wWord->pcUsingPointer;
    for(i = 0;  s[i] != '\0' && i < 4090;  i++) {
	if( s[i] == '\n' || s[i] == '\r' )
		break;
    }
    if( i <= 0 || i >= 4090 ) {
	qsError = 2006;		//database describe error
	return  FALSE;
    }

    strZcpy(AsqlDatabase, shrink(subcopy(s,0,i)), 32);
    wWord->pcUsingPointer += i;
    s = GetCfgKey(csuDataDictionary, "DATABASE", AsqlDatabase, "PATH");

    if( s != NULL ) {
	strZcpy(asqlEnv.szAsqlFromPath, s, MAXPATH);
    } else {
	qsError = 2006;		//database describe error
	return  FALSE;
    }
    strcpy(fFrTo.AsqlDatabase, AsqlDatabase);

    return  TRUE;

} // end of function DatabasePro()


/*---------------
 *  Function Name:  ExclusivePro()
 *  author: 1998.1.1 Xilong
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   ExclusivePro( void )
{
    short i;
    char  *s;

    IgnoreUselessChar();

    s = wWord->pcUsingPointer;
    for(i = 0;  s[i] != '\0';  i++) {
	if( s[i] == '\n' || s[i] == '\r' )
		break;
    }

    fFrTo.fromExclusive = atoi(s);
    wWord->pcUsingPointer += i;
    return  TRUE;

} // end of function ExclusivePro()


/*---------------
 *  Function Name:  AsqlSetPro()
 *  author: 1999.5.28 Xilong
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   AsqlSetPro( void )
{
    short i;
    char  *s;
    char  buf[4096], token[32];
    unsigned short ui;

    IgnoreUselessChar();

    s = wWord->pcUsingPointer;
    for(i = 0;  s[i] != '\0';  i++) {
	if( s[i] == '\n' || s[i] == '\r' )
		break;
    }

    strZcpy(buf, s, min(i+1,4096));
    ui = getAtokenFromBuf(buf, 32, token);
    if( stricmp(token, "RELATION") == 0 ) {
	getAtokenFromBuf(buf+ui, 32, token);
	if( stricmp(token, "OFF") == 0 )
		fFrTo.relationOff = 1;
	else	fFrTo.relationOff = 0;
    } else if( stricmp(token, "ESCAPE") == 0 ) {
	getAtokenFromBuf(buf+ui, 32, token);
	if( (token[0] == '"' || token[0] == '\'') && token[1] != '\0' )
	    cEscapeXexp = token[1];
	else
	    cEscapeXexp = token[0];
	
	if( asc_asqlEscape[ cEscapeXexp ] == '\0' )
	    cEscapeXexp = '\\';
    } else
    { //unknown keyword
	strZcpy(szAsqlErrBuf, token, sizeof(szAsqlErrBuf));
	qsError = 3030;
	return  FALSE;
    }

    wWord->pcUsingPointer += i;
    return  TRUE;

} // end of function AsqlSetPro()



/*---------------
 *  Function Name:  RemarkPro()
 *  author: 1999.1.9 Xilong
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   RemarkPro( void )
{
    short i;
    char  *s;

    //IgnoreUselessChar();

    s = wWord->pcUsingPointer;
    for(i = 0;  s[i] != '\0';  i++) {
	if( s[i] == '\n' || s[i] == '\r' )
		break;
    }

    wWord->pcUsingPointer += i;
    return  TRUE;

} // end of function RemarkPro()


/*---------------
 *  Function Name:  Remark2Pro()
 *  author: 1999.1.9 Xilong
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   Remark2Pro( void )
{
    char  *s;

    //IgnoreUselessChar();

    s = strstr(wWord->pcUsingPointer, "*/");
    if( s == NULL ) {
	qsError = 3001;         //comment too long
	return  FALSE;
    }

    wWord->pcUsingPointer = s+2;
    return  TRUE;

} // end of function Remark2Pro()



/*---------------
 *  Function Name:  AsqlUpdatePro()
 *  author: 1997.11.3 Xilong
 *  Input:  ASK data describ "ACTION" segment
 *  Output: action access result
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   AsqlUpdatePro( void )
{
    int   i;
    char  buf[267];

    if( fFrTo.phuf != NULL ) {
	asqlDbCommit();

	for(i = fFrTo.cSouDbfNum - 1;   i >= 0;   i--) {
		freeRecLock( fFrTo.cSouFName[i] );
	}

	chsize( fileno(fFrTo.phuf), 0 );
	fclose(fFrTo.phuf);
    }

    sprintf(buf, "%sUP_D_TMP.%03X", tmpPath, intOfThread&0xFFF);
    //if( (fFrTo.phuf = tmpfile()) == NULL )
    if( (fFrTo.phuf = fopen(buf, "w+b")) == NULL )
        return  FALSE;
    fwrite("\0\0\0\0", sizeof(dFILE *), 1, fFrTo.phuf);

    //1997.12.4 Xilong
    fFrTo.toAppend = '1';
    fFrTo.cTargetFileName[0] = 'P';

    return  TRUE;

} // end of function AsqlUpdatePro()




/*---------------
 *  Function Name:  AveragePro( )
 *  Input:  ASK data describ "ACTION" segment
 *  Output: action access result
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   AveragePro( void )
{
    int   i;
    char *s;

    // sum statement should be writen as: sum total_s, y  but stop with '\n'

    IgnoreUselessChar();

    s = wWord->pcUsingPointer;
    for(i = 0;  s[i] != '\0' && i < 4096;  i++) {
	if( s[i] == '\n' || s[i] == '\r' )
		break;
    }
    if( i <= 0 || i >= 4096)     return  FALSE;

    strcpy(AsqlStatTarget, shrink(subcopy(s,0,i)));
    iAsqlStatAction = 3;

    wWord->pcUsingPointer += i;

    return TRUE;

} // end of function AveragePro()



/*---------------
 *  Function Name:  MaxPro( )
 *  Input:  ASK data describ "ACTION" segment
 *  Output: action access result
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   MaxPro( void )
{
    int   i;
    char *s;

    // sum statement should be writen as: sum total_s, y  but stop with '\n'

    IgnoreUselessChar();

    s = wWord->pcUsingPointer;
    for(i = 0;  s[i] != '\0' && i < 4096;  i++) {
	if( s[i] == '\n' || s[i] == '\r' )
		break;
    }
    if( i <= 0 || i >= 4096)     return  FALSE;

    strcpy(AsqlStatTarget, shrink(subcopy(s,0,i)));
    iAsqlStatAction = 4;

    wWord->pcUsingPointer += i;

    return TRUE;

} // end of function MaxPro()


/*---------------
 *  Function Name:  MinPro( )
 *  Input:  ASK data describ "ACTION" segment
 *  Output: action access result
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   MinPro( void )
{
    int   i;
    char *s;

    // sum statement should be writen as: sum total_s, y  but stop with '\n'

    IgnoreUselessChar();

    s = wWord->pcUsingPointer;
    for(i = 0;  s[i] != '\0' && i < 4096;  i++) {
	if( s[i] == '\n' || s[i] == '\r' )
		break;
    }
    if( i <= 0 || i >= 4096)     return  FALSE;

    strcpy(AsqlStatTarget, shrink(subcopy(s,0,i)));
    iAsqlStatAction = 5;

    wWord->pcUsingPointer += i;

    return TRUE;

} // end of function MinPro()


/*---------------
 *  Function Name:  TryPro( )
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   TryPro( void )
{
    int   i;
    char *s;

    // sum statement should be writen as: sum total_s, y  but stop with '\n'

    IgnoreUselessChar();

    s = wWord->pcUsingPointer;
    for(i = 0;  s[i] != '\0' && i < 4096;  i++) {
	if( s[i] == '\n' || s[i] == '\r' )
		break;
    }
    if( i <= 0 || i >= 4096)     return  FALSE;

    strZcpy(szExcept, trim(subcopy(s,0,i)), 32);

    wWord->pcUsingPointer += i;

    return TRUE;

} // end of function TryPro()


/*---------------
 *  Function Name:  ExceptPro( )
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   ExceptPro( void )
{
    char 	*sz;
    char 	buf[32];

    sz = locateKeywordInBuf(wWord->pcUsingPointer, "ENDEXCEPT", buf, 32);

    if( sz == NULL )
	return  FALSE;

    while( *sz != '\n' && *sz != '\r' && *sz )
	sz++;
    while( *sz == '\n' || *sz == '\r' )
	sz++;

    wWord->pcUsingPointer = sz;

    return TRUE;

} // end of function ExceptPro()



/*---------------
 *  Function Name:  EndExceptPro( )
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   EndExceptPro( void )
{
    char 	*sz;

    sz = wWord->pcUsingPointer;

    while( *sz != '\n' && *sz != '\r' && *sz )
	sz++;
    while( *sz == '\n' || *sz == '\r' )
	sz++;

    wWord->pcUsingPointer = sz;

    return TRUE;

} // end of function EndExceptPro()



/*---------------
 *  Function Name:  ActPro( )
 *  Input:  ASK data describ "ACTION" segment
 *  Output: action access result
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   ActPro( void )
{

    ExecActTable *exExAct;

    wWord->keyWordPossible = 3;
    if( GetWord() == 0 )   return  FALSE;

    if( wWord->xPointer == NULL ) {
	qsError = 1009;
	return  FALSE;
    }

    if( wWord->nType == Asql_FILEEND ) {
	qsError = 1009;
	return  FALSE;
    }

    if(( exExAct = (ExecActTable *)zeroMalloc( sizeof(ExecActTable) )) == NULL) {
	qsError = 1004;
	return  FALSE;
    }

    if( fFrTo.exHead == NULL ) {
	fFrTo.exHead = apExecActTable = exExAct;
    } else {
	apExecActTable->oNext = exExAct;

	apExecActTable->execNext = exExAct;

	apExecActTable = exExAct;
    }

    exExAct->oNext = NULL;
    exExAct->stmType = Asql_ACTION;

    //1998.11.25
    //if( fFrTo.eHead == NULL )
    if( ConditionAppeared == 0 )
		exExAct->nLevel = 'B';   // the action is before CONDITION
    else        exExAct->nLevel = 'A';    // the action is behind CONDITION

    exExAct->xPoint = wWord->xPointer;

    return  TRUE;

} // end of ActPro



/*---------------
 *  Function Name:  GroupActPro()
 *  Input:  ASK data describ "ACTION" segment
 *  Output: action access result
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   GroupActPro( void )
{

    ExecActTable *exExAct;

    wWord->keyWordPossible = 3;
    if( GetWord() == 0 )   return  FALSE;
    if( wWord->nType == Asql_FILEEND )
	return  FALSE;

    if(( exExAct = (ExecActTable *)zeroMalloc( sizeof(ExecActTable) )) == NULL) {
	qsError = 1004;
	return  FALSE;
    }

    if( fFrTo.exHead == NULL ) {
	fFrTo.exHead = apExecActTable = exExAct;
    } else {
	apExecActTable->oNext = exExAct;
	apExecActTable = exExAct;
    }

    exExAct->oNext = NULL;

    exExAct->nLevel = 'G';    // the action is behind CONDITION
    fFrTo.nGroupAction = 'Y';

    exExAct->xPoint = wWord->xPointer;

    return  TRUE;

} // end of GroupActPro


/*---------------
 *  Function Name:  PredicatesPro( )
 *  input:  ASK data describ "PREDICATIES" segment
 *  output: rechanged symbol table
 *  return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   PredicatesPro( void )
{
     short wordlength, j;

     while( TRUE )  {

	if( (wordlength = SearchWordEnd()) == 0 )
		return  FALSE;

	for(j = TYPERETAINWORDBEGIN;  j < TYPERETAINWORDEND; j++) {
		if( !strnicmp( wWord->pcUsingPointer, retainarry[j], \
							    wordlength ) )
			break;
	}
	if( j >= TYPERETAINWORDEND )  {
		//Rem 1997.11.27 Xilong
		//qsError = 3020;
		wWord->keyWordPossible = 2;
		return  TRUE;
	}

	wWord->pcUsingPointer += wordlength;

	switch( j )  {
	    case Asql_INT:
		if( TypePro(INT_TYPE) == FALSE )
			return  FALSE;
		break;
	   case Asql_LONG:
		if( TypePro(LONG_TYPE) == FALSE )
			return  FALSE;
		break;
	   case Asql_FLOAT:
		if( TypePro(FLOAT_TYPE) == FALSE )
			return  FALSE;
		break;
	   case Asql_CHAR:
		if( TypePro(CHR_TYPE) == FALSE )
			return  FALSE;
		break;
	   case Asql_STRING:
		if( TypePro(STRING_TYPE) == FALSE )
			return  FALSE;
		break;
	   case Asql_DATE:
		if( TypePro(DATE_TYPE) == FALSE )
			return  FALSE;
		break;
	    case Asql_INT64:
		if( TypePro(INT64_TYPE) == FALSE )
			return  FALSE;
		break;
	   default:             // this should never occured
		qsError = 3020;
		return  FALSE;
	}
     } // end of while

} // end of function PrePro()


/*---------------
 *  Function Name:      TypePro()
 *  Input:  TYPE describ ( INT , LONG, FLOAT )
 *  Output:
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   TypePro( short type )
{
    short nNameLength, i;
    char *p;
    SysVarOFunType *SysVar;

    while( TRUE ) {

	 p = wWord->pcUsingPointer;

	 if( (nNameLength = SearchVarChar( " :=\t\r\n;,[")) == 0 )
		return  FALSE;

	 if( nNameLength <= 0 || nNameLength > 16 ) {
	      qsError = 3021;
	      return  FALSE;
	 }

	 //test wether it is a really identify
	 for(i = 0, p = wWord->pcUsingPointer;  i < nNameLength;  i++) {
		if( isalnum(*p) )	break;
		else	 	p++;
	 }
	 if( i >= nNameLength ) {
	      qsError = 3021;
	      return  FALSE;
	 } else if( !isalpha(p[i]) ) {
	      qsError = 3021;
	      return  FALSE;
	 }

	 if( (fFrTo.sHead = (SysVarOFunType *)realloc( fFrTo.sHead, \
					(fFrTo.nSybNum+1) * \
					sizeof(SysVarOFunType) ) ) == NULL )
		return  FALSE;

	 SysVar = fFrTo.sHead;
	 memset(&SysVar[fFrTo.nSybNum], 0, sizeof(SysVarOFunType));
	 strZcpy(SysVar[fFrTo.nSybNum].VarOFunName, p, nNameLength+1);

	 p += nNameLength;
	 wWord->pcUsingPointer = p;

	 if( *p == '[' ) {
		SysVar[ fFrTo.nSybNum ].type = ARRAY_TYPE;
		if( ArrayPro(type) == FALSE )
			return  FALSE;
		p = wWord->pcUsingPointer;
	 } else {
		switch( type ) {
		    case INT_TYPE:
			SysVar[fFrTo.nSybNum].type = INT_IDEN;
			SysVar[fFrTo.nSybNum].length = 2;
			break;
		    case LONG_TYPE:
			SysVar[fFrTo.nSybNum].type = LONG_IDEN;
			SysVar[fFrTo.nSybNum].length = 4;
			break;
		    case FLOAT_TYPE:
			SysVar[fFrTo.nSybNum].type = FLOAT_IDEN;
			SysVar[fFrTo.nSybNum].length = 8;
			break;
		    case DATE_TYPE:
			SysVar[fFrTo.nSybNum].type = DATE_IDEN;
			SysVar[fFrTo.nSybNum].length = 8;
			break;
		    case CHR_TYPE:
			SysVar[fFrTo.nSybNum].type = CHR_IDEN;
			SysVar[fFrTo.nSybNum].length = 1;
			break;

		    case STRING_TYPE:
		    // if type is described as:
		    //    STRING str
		    // take the str size as 31
			SysVar[fFrTo.nSybNum].type = STRING_IDEN;
			SysVar[fFrTo.nSybNum].length = MAX_OPND_LENGTH-1;
			break;

		    case INT64_TYPE:
			SysVar[fFrTo.nSybNum].type = INT64_IDEN;
			SysVar[fFrTo.nSybNum].length = 8;
			break;
		}
	 }
	 fFrTo.nSybNum++;

	 // prepare to deal the next predicates such as:
	 // short a, b
	 while( *p == ' ' || *p == '\t' || *p == ',' )  p++;
	 if( *p == '\0' || *p == '\r' || *p == '\n' )    return  TRUE;
	 if( *p == ';' ) {
		p++;
		wWord->pcUsingPointer = p;
		return  TRUE;
	 }
	 wWord->pcUsingPointer = p;
      }

} // end of function TypePro()

/*--------------
 *  Function Name:      ArrayPro()
 *  Input:
 *  Output: array process result
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   ArrayPro(short artype )
{
    char	    temp[128];
    unsigned char   *work;
    short	    nNameLength, DimNum = 0;
#ifdef __BORLANDC__
    short  lNumber = 1;
#else
    long   lNumber = 1;
#endif
    SysVarOFunType  *SysVar;
    int		    i;

    SysVar = (SysVarOFunType *)fFrTo.sHead;
    work = (unsigned char *)(SysVar[ fFrTo.nSybNum ].values);
    (*(ArrayType *)work).ElementType = artype;
    wWord->pcUsingPointer++;            // skip [
    (*(ArrayType *)work).DimNum = 0;

    while( TRUE ) {

	if( (nNameLength = SearchVarChar(",]")) == 0 ) {

	    //dim variable array

	    (*(ArrayType *)work).DimNum = 0;
	    (*(ArrayType *)work).ElementNum = 0;
	    (*(ArrayType *)work).MemSize = 1024;
	    if( ((*(ArrayType *)work).ArrayMem = malloc(1024)) == NULL ) {
		qsError = 1005;
		return  FALSE;
	    }

	    wWord->pcUsingPointer += nNameLength + 1;

	    return  TRUE;

	    //1999.11.2
	    //return  FALSE;
	}

	//check wether the array dim defination is digital only
	for( i = 0;  i < nNameLength;  i++ ) {
	    if( !isdigit( wWord->pcUsingPointer[i] ) )
		break;
	}
	if( i < nNameLength ) {
		qsError = 1010;
		return  FALSE;
	}

	strZcpy(temp, wWord->pcUsingPointer, nNameLength+1);
	
	if( (*(ArrayType *)work).DimNum++ >= MAXARRAYDIM-1 ) {
		qsError = 1011;
		return  FALSE;
	}

	(*(ArrayType *)work).ArrayDim[ DimNum++ ] = abs(atoi( temp ));
	wWord->pcUsingPointer += nNameLength + 1;
	if( *(wWord->pcUsingPointer-1) == ']' )         break;
    }

    for(nNameLength = 0;   nNameLength < DimNum;   nNameLength++ ) {
	  lNumber *= (*(ArrayType *)work).ArrayDim[nNameLength];
    }
    (*(ArrayType *)work).ElementNum = (unsigned short)lNumber;
    switch( artype) {
	   case INT64_TYPE:     lNumber *= sizeof(_int64);         break;
	   case LONG_TYPE:      lNumber *= sizeof(long);           break;
	   case FLOAT_TYPE:     lNumber *= sizeof(double);         break;
	   case CHR_TYPE:       lNumber *= sizeof(char);           break;
	   case INT_TYPE:       lNumber *= sizeof(short);          break;
	   case STRING_TYPE:
	   {   	//lNumber *= sizeof(char);
		SysVar[fFrTo.nSybNum].type = STRING_IDEN;
		SysVar[fFrTo.nSybNum].length = (short)(lNumber+1);

		if( lNumber < MAX_OPND_LENGTH ) {
			//we have fill the values with data, for we take this
			//variable as an array
			//so, clear it now
			memset(SysVar[fFrTo.nSybNum].values, 0, lNumber+1);
			return  TRUE;
		}

		if( (*(long *)(SysVar[fFrTo.nSybNum].values) = \
					    (long)zeroMalloc(lNumber+16)) == (long)NULL ) {
			qsError = 1005;
			return  FALSE;
		}
		return  TRUE;
	   }

	   default:
		qsError = 3020;
		return  FALSE;
    }

    if( ((*(ArrayType *)work).ArrayMem = (char *)malloc(lNumber)) == NULL ) {
	   qsError = 1005;
	   return  FALSE;
    }
    memset( (*(ArrayType *)work).ArrayMem, 0, lNumber );
    (*(ArrayType *)work).MemSize = lNumber;

    return  TRUE;

} // end of function ArrayPro()


/*---------------
 *  Function Name:      wWordFree( )
 *  Utility: free memory use expression table and action table and other
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
void   wWordFree( void )
{

    if( wWord == NULL )         return;

    if( szAsqlErrBuf[0] == '\0' )
	 strcpy(szAsqlErrBuf,"...");
    else strcat(szAsqlErrBuf,"...");
    strcat(szAsqlErrBuf, substr(wWord->pcUsingPointer, 0, sizeof(szAsqlErrBuf)-strlen(szAsqlErrBuf)) );

    if( wWord->nFlush == AsqlExprInFile ) {
	fclose( wWord->fp );
	free( wWord->pcWordAnaBuf );         // free word memory
    }

    free( wWord );

    wWord = NULL;

} // end of function wWordFree()


/*---------------
 *  Function Name:      qsFree()
 *  Utility: free memory use expression table and action table and other
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
void   qsFree( void )
{
    ExprTable      *etPoint;
    CalActOrdTable *paAcTbIt;
    ExecActTable   *exItem;
    short           i;

    FromToStru *mfFrTo = &fFrTo;		//for multithread speed

    // free expression code
    while( mfFrTo->eHead != NULL ) {
	etPoint = mfFrTo->eHead;
	mfFrTo->eHead = etPoint->eNext;
	FreeCode( etPoint->xPointer );
	free( etPoint );
    }

    // free action code
    while( mfFrTo->cHead != NULL ) {
	paAcTbIt = mfFrTo->cHead;
	mfFrTo->cHead = paAcTbIt->aNext;
	FreeCode( paAcTbIt->pxActMidCode );
	free( paAcTbIt );
    }
    mfFrTo->cHead = NULL;

    if( mfFrTo->sHead != NULL ) {
	// free symbol table memory
	for( i = 0;   i < mfFrTo->nSybNum;   i++ ) {
	    if( (mfFrTo->sHead[i]).type == ARRAY_TYPE ) {
		 free( (*( ArrayType* )(mfFrTo->sHead[i]).values).ArrayMem );
	    } else if( (mfFrTo->sHead[i]).type == STRING_IDEN && \
				(mfFrTo->sHead[i]).length >= MAX_OPND_LENGTH ) {
		 free( (char *)*(long *)mfFrTo->sHead[i].values );
	    }
	}
	free( mfFrTo->sHead );       // free symbol memory area
	mfFrTo->sHead = NULL;
	mfFrTo->nSybNum = 0;
    }
    if( mfFrTo->dHead != NULL ) {
	free( mfFrTo->dHead );       // free define memory area
	mfFrTo->dHead = NULL;
	mfFrTo->nDefinNum = 0;
    }

/*  Here we should not do this, for we have free this action when we finish
    to calculate them. they are no use when they are registered, but stay in
    memory.
    change this design 1994.10.8 Xilong
*/
    // free execute order table memory
    while( mfFrTo->exHead != NULL ) {
	exItem = mfFrTo->exHead;
	mfFrTo->exHead = exItem->oNext;
	FreeCode( exItem->xPoint );
	free( exItem );
    }
    mfFrTo->exHead = NULL;

    // free target field
    if( mfFrTo->fieldtable != NULL ) {
	free( mfFrTo->fieldtable );
	mfFrTo->fieldtable = NULL;
    }
    if( mfFrTo->TargetField != NULL ) {
	free( mfFrTo->TargetField );
	mfFrTo->TargetField = NULL;
    }

    apExecActTable = NULL;
    apCalActOrdTable = NULL;
    eEndPoint = NULL;

    //in old version of this program, we use the to check for free
    //but this is an error, for the asqlStatMaxY can be set before
    //alloc memory of asqlStatFlagY, Xilong 1996.03.10
    //if( asqlStatMaxY > 0 ) {
    if( asqlStatFlagY != NULL ) {
	free(asqlStatFlagY);
	asqlStatFlagY = NULL;
	asqlStatMaxY = -1;
    }

    if( mfFrTo->tTree != NULL ) {
	IndexClose(mfFrTo->tTree);
    }

    for(i = 0;   i < mfFrTo->cSouDbfNum;  i++ ) {
	 if( mfFrTo->xKeyExpr[i] != NULL ) {
		FreeCode( mfFrTo->xKeyExpr[i] );
	 }
    }

} // end of qsFree()


/*---------------
 *  Function Name:      qsTableFree()
 *  Utility: free memory use expression table and action table and other
 *           call when finish one statement
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
void   qsTableFree( void )
{
    ExprTable *etPoint;
    CalActOrdTable *paAcTbIt;
    ExecActTable *exItem;
    short i;

    FromToStru *mfFrTo = &fFrTo;		//for multithread speed

    // free expression code
    while( mfFrTo->eHead != NULL ) {
	etPoint = mfFrTo->eHead;
	mfFrTo->eHead = etPoint->eNext;
	FreeCode( etPoint->xPointer );
	free( etPoint );
    }
    mfFrTo->eHead = NULL;

    // free action code
    while( mfFrTo->cHead != NULL ) {
	paAcTbIt = mfFrTo->cHead;
	mfFrTo->cHead = paAcTbIt->aNext;
	FreeCode( paAcTbIt->pxActMidCode );
	free( paAcTbIt );
    }
    mfFrTo->cHead = NULL;

/*  Here we should not do this, for we have free this action when we finish
    to calculate them. they are no use when they are registered, but stay in
    memory.
    change this design 1994.10.8 Xilong
*/
    // free execute order table memory
    while( mfFrTo->exHead != NULL ) {
	exItem = mfFrTo->exHead;
	mfFrTo->exHead = exItem->oNext;
	FreeCode( exItem->xPoint );
	free( exItem );
    }
    mfFrTo->exHead = NULL;

    apExecActTable = NULL;
    apCalActOrdTable = NULL;
    eEndPoint = NULL;

    // added 1995.08.24 Xilong Pei
    iAsqlStatAction = 0;
    AsqlStatTarget[0] = '\0';
    StatementPreAction[0] = '\0';
    //AsqlStatTailAction[0] = '\0';

    if( asqlStatFlagY != NULL  ) {
	free(asqlStatFlagY);
	asqlStatFlagY = NULL;
	asqlStatMaxY = -1;
    }

    if( mfFrTo->tTree != NULL ) {
	IndexClose(mfFrTo->tTree);
    }

    for(i = 0;   i < mfFrTo->cSouDbfNum;  i++ ) {
	 if( mfFrTo->xKeyExpr[i] != NULL ) {
		FreeCode( mfFrTo->xKeyExpr[i] );
		mfFrTo->xKeyExpr[i] = NULL;
	 }
    }

    mfFrTo->nGroupAction = '\0';

} // end of qsTableFree()


/*---------------
 *  Function Name:      QueryPro()
 *  Arguments:
 *  Input:  ASK retain word begin segment ( query segment )
 *  Output: fill expression table and action table in hand structure
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  Call:
 *        CreatExprTableItem()  fill exprssion table
 *        FillActTable()   fill action table
 *        GetWord()        read a word from query segments
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   QueryPro( void )
{
    ExprTable *sp[Asql_MAXBEGINENDNESTDEEP];
    ExprTable *etTailPoint[Asql_MAXBEGINENDNESTDEEP];
    ExprTable *etPoint;
    ExprTable *etPointRem;        // remember the expr item which need parall
    short      sTop, nStackNull;
    short      iTail;
    unsigned char *temp, c;


    etPoint = etPointRem = NULL;
    nStackNull = FALSE;

    wWord->keyWordPossible = 2;
    if( GetWord() == FALSE ) {
	qsError = 3022;
	return  FALSE;
    }

    if( wWord->nType == Asql_FILEEND ) {
	qsError = 3022;
	return  FALSE;
    }

    if( wWord->nInterNumberal != Asql_BEGIN ) {

	while( wWord->nInterNumberal != Asql_BEGIN ) {

	    if( wWord->nInterNumberal == Asql_REM ) {
		if( RemarkPro() == FALSE )
		    return  FALSE;
	    } else {
		qsError = 3022;
		return  FALSE;
	    }

	    wWord->keyWordPossible = 2;
	    if( GetWord() == FALSE ) {
		qsError = 3022;
		return  FALSE;
	    }
	}

    }

    wWord->keyWordPossible = 1;

    sTop = 0;
    iTail = -1;

    while( nStackNull == FALSE )  {

	if( GetWord() == FALSE ) {
	    if( qsError == 0 )
		qsError = 3023;
	    return  FALSE;
	}

	switch( wWord->nType ) {
	    case Asql_EXPRTYPE:
		if( (etPoint = \
			      CreatExprTableItem()) == NULL ) {
			qsError = 1001;
			return  FALSE;
		}
		if( etPointRem != NULL ) {
			if( etPointRem->ExprTableFlag == -sTop ) {
			     // if the appeared expression at the same level of
			     // etPointRem
			     etPointRem->nNextParallelExpr = (MidCodeType *)etPoint;
			} else {
			     // else it will be true action or false action
			     if( etPointRem->nFirstAction == NULL ) {
					etPointRem->nFirstAction = (MidCodeType *)etPoint;
			     } else {
					etPointRem->nSecondAction = (MidCodeType *)etPoint;
			     }
			}

		// to fill the expression tree to avoid to walk through the
		// tree when the compiler meet this kind of statements:
		//      begin
		//              expression
		//              begin
		//                      expression $ action
		//                      begin
		//                          statements(1-->next statement--+
		//                      end              /                 |
//1996.12.29Xilong\\			begin           /                  |
		//\\                        statements(2<-XXX<not this code+
		//\\			end                                |
		//              end         <--XXX< not this node <XXX----X|
		//              statements  <------------------------------+
		//      end
		//
		// with statements(1) and statements(2)
		//	etPointRem->ExprTableFlag == -sTop
		//

		//      begin
		//              expression(->ExprTableFla=0) <---etPointRem
		//              begin
		//                      expression (->ExprTableFla=-1)
		//                      begin
		//                          statements(->ExprTableFla=-2)
		//                      end
//1999.1.5  Xilong\\			begin
		//\\                        statements(->ExprTableFla=-2)
		//				 <---etPoint, CURRENT
		//\\			end
		//              	statements(->ExprTableFla=-1)
		//              end
		//              statements(->ExprTableFla=0)
		//      end
		//
		//SO THE FOLLOWING STATEMENT IS ERROR:
		//    if( etPointRem->ExprTableFlag >= -sTop ) {
		//
		    if( sTop <= -(etPointRem->ExprTableFlag) ) {
			for( ;    iTail >= 0;   iTail-- ) {
			  etTailPoint[iTail]->nNextParallelExpr = (MidCodeType *)etPoint;
			}
		    }
		} // end of if

		etPointRem = etPoint;
		etPointRem->ExprTableFlag = -sTop;

		if( sTop <= 0 ) {
			sp[ 0 ] = etPoint;
		}

		// this means the just statement is for statistics
		if( *StatementPreAction ) {
			if( (etPointRem->nFirstAction = \
			       ActionAnalyse( StatementPreAction, \
			       (short)wWord->nInterNumberal, \
			       &(SysVarOFunType *)fFrTo.sHead,\
			       &fFrTo.nSybNum, &(fFrTo.fieldtable), \
			       &(fFrTo.nFieldNum), \
			       fFrTo.dHead, &(fFrTo.nDefinNum)) ) == NULL ) {
					qsError = 4003;
					return  FALSE;
			}

			*StatementPreAction = '\0';  // make it into null

			if( FillActTable(etPointRem->nFirstAction) == FALSE ) {
				return  FALSE;
			}

			//this method is slow
			//if( strnicmp(StatementPreAction,"staty", 5) != 0 )
			if( StatementPreAction[4] != 'y' ) {
				etPointRem->cFirstActionType = 1;
			}

		}
		break;
	    case Asql_RETAINTYPE:
		switch( wWord->nInterNumberal ) {
		    case Asql_BEGIN:
			sp[ ++sTop ] = etPoint;

			//$$$$$
			//increase the nest deep
			//etTailPoint[++iTail] = NULL;
			break;

		    case Asql_END:
			etTailPoint[++iTail] = etPoint;
			if( sTop > 0 ) {       // this is not the first layer
				etPoint = etPointRem = sp[ sTop-- ];
			} else {
				nStackNull = TRUE;
			}
			break;

		    // this sentence is defined as a action in query
		    // but now it is a predicator ro the following statistics
		    case Asql_STATISTICS:
			StatPro();
			break;

		    case Asql_SUMMER:
			SumPro();
			break;

		    case Asql_LABEL:
			LabelPro();
			break;

		    case Asql_AVERAGE:
			AveragePro();
			break;

		    case Asql_MAX:
			MaxPro();
			break;

		    case Asql_MIN:
			MinPro();
			break;

		    case Asql_REM:
			if( RemarkPro() == FALSE )
			    return  FALSE;
		    break;

		    case Asql_REM2:
			if( Remark2Pro() == FALSE )
			    return  FALSE;
		    break;

		    default:
			qsError = 3023;
			return  FALSE;
		}
		break; // end of retain word

	case Asql_ACTITYPE:

		if( etPointRem == NULL ) {
			qsError = 4006;
			return  FALSE;
		}

		temp = wWord->pcUsingPointer;
		wWord->pcUsingPointer += wWord->nWordLength;
		c = *(wWord->pcUsingPointer);
		*(wWord->pcUsingPointer) = '\0';
//MidCodeType *ActionAnalyse( unsigned char *buffer, short ActionNo, \
//                          SysVarOFunType **SymbolTable, short *SymbolNum, \
//                          char *FieldTable[], short *FieldNum, \
//                          SYSDEFINETYPE *DefineTable, short *DefineNum );
		if( (wWord->xPointer = ActionAnalyse( temp, \
			    (short)wWord->nInterNumberal, \
			    &(SysVarOFunType *)fFrTo.sHead,\
			    &fFrTo.nSybNum, &(fFrTo.fieldtable), \
			    &(fFrTo.nFieldNum), \
			    fFrTo.dHead, &fFrTo.nDefinNum ) ) == NULL )
		{
		        *(wWord->pcUsingPointer) = c;
			strZcpy(szAsqlErrBuf, temp, sizeof(szAsqlErrBuf));
			qsError = 4004;
			return  FALSE;
		}

		*(wWord->pcUsingPointer) = c;

		if( FillActTable(wWord->xPointer) == FALSE )
			return  FALSE;

		if( etPointRem->nFirstAction == NULL ) {
			etPointRem->nFirstAction = wWord->xPointer;
		} else {
			etPointRem->nSecondAction = wWord->xPointer;
		}
		break;

	    case Asql_FILEEND:
		qsError = 3024;
		return  FALSE;
	} // end of switch nType

    } // end of while

    // stack check
    if( nStackNull != TRUE ) {
	qsError = 3024;
	return  FALSE;
    }

    /*if( *AsqlStatTailAction != '\0' ) {
	// dele the tail ','
	AsqlStatTailAction[ strlen(AsqlStatTailAction)-1 ] = '\0';
	strcat(AsqlStatTailAction, ")");
	if( (m_c_AsqlStatTailAction = ActionAnalyse( AsqlStatTailAction, \
			    (short)wWord->nInterNumberal, \
			    &(SysVarOFunType *)fFrTo.sHead,\
			    &fFrTo.nSybNum, &(fFrTo.fieldtable), \
			    &(fFrTo.nFieldNum), \
			    fFrTo.dHead, &fFrTo.nDefinNum ) ) == NULL ) {
		 qsError = 4003;
		 return  FALSE;
	} // end of if
	if( FillActTable(m_c_AsqlStatTailAction) == FALSE )
	{
		qsError = 1001;
		return  FALSE;
	}

    } */

    return  TRUE;

} // end of function


/*--------------
 *  Function Name:      FillActTable
 *  arguments: *fFrTo  action table head
 *             *wWord  action number and action pointer
 *  Input:
 *  Output: fill action table in hand structure
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
short   FillActTable( MidCodeType *xPointer )
{
    CalActOrdTable *paAcTbIt;

    if( (paAcTbIt = \
		(CalActOrdTable *)zeroMalloc( sizeof(CalActOrdTable) )) == NULL) {
	qsError = 1006;
	return  FALSE;
    }

    //zeroMalloc(), needn't do this
    //paAcTbIt->aNext = NULL;
    //paAcTbIt->nCalTimes = 0;     // this action has never run

    paAcTbIt->pxActMidCode = xPointer;
    if( apCalActOrdTable == NULL ) {
	fFrTo.cHead = apCalActOrdTable = paAcTbIt;
    } else {
	apCalActOrdTable->aNext = paAcTbIt;
	apCalActOrdTable = paAcTbIt;
    }

    return  TRUE;

} // end of ActionProcess()



/*--------------
 *  Function Name:      CreatExprTableItem( )
 *  Arguments: *fFrTo  action table head
 *             *wWord  action number and action pointer
 *  Input:
 *  Output:  fill expression table in hand structure
 *  Return:  sucess: table item pointer
 *           unsucess: NULL
 *  qsError:
 *-------------------------------------------------------------------------*/
ExprTable *   CreatExprTableItem( void )
{
    ExprTable *etPoint;

    if(( etPoint = zeroMalloc( sizeof(ExprTable) )) == NULL ) {
	qsError = 1007;
	return  NULL;
    }

    etPoint->eNext = NULL;

    if( apExecActTable != NULL )
    { //1998.6.22 action exec next pointer point to null
	apExecActTable->execNext = NULL;
    }

    if( eEndPoint == NULL ) {
	fFrTo.eHead = etPoint;
     } else {
	eEndPoint->eNext = etPoint;
     }

     if( fFrTo.labelNum >= 0 )
     { //label appeared
	if( fFrTo.asqlLabel[fFrTo.labelNum].etPoint == NULL ) {
	    fFrTo.asqlLabel[fFrTo.labelNum].etPoint = etPoint;
	}
     }

     eEndPoint = etPoint;
     etPoint->xPointer = wWord->xPointer;
     etPoint->nFirstAction = etPoint->nSecondAction = \
					   etPoint->nNextParallelExpr = NULL;

     return  etPoint;

} // end of function CreatExprTableItem()



/*---------------
 *  Function Name:  ActionRecognize( )
 *  Arguments:  Word *
 *  Unility:    recognize a action
 *  Return: sucess : a word ( type, inter numberal ... )
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   ActionRecognize( void )
{
    short i;

    IgnoreUselessChar();

    // the kind of expression as:   $$ a=a+b #$
    if( *wWord->pcUsingPointer == Asql_SEPSYMB )  {
	wWord->xPointer = WordAnalyse( "" );
	wWord->nWordLength = 0;
	wWord->nType = Asql_EXPRTYPE;
	return  TRUE;
    }

    if( (i = SearchExprEnd()) == 0 )     return  FALSE;

    wWord->nType = Asql_ACTITYPE;
    wWord->nInterNumberal = ++nActSerialCode;
    wWord->nWordLength = i;
    wWord->xPointer = NULL;

    return  TRUE;

} // end of function ActionRecognize()


/*---------------
 *  Function Name:  SearchVarChar( )
 *  arguments:  Word buffer *
 *  unility: search a assume string in word buffer
 *  return:  length
 *  qsError:
 *-------------------------------------------------------------------------*/
unsigned short   SearchVarChar( char *send )
{
    unsigned short nLength;

    IgnoreUselessChar();

    while( TRUE ) {
	 nLength = strcspn( wWord->pcUsingPointer, send );

	 if( !wFlush( nLength ) )        break;

    }

    return  nLength;

}


/*---
 *  Function Name:  SearchAssistChar( )
 *  arguments:  Word buffer *
 *  unility: search a assume string in word buffer
 *  return:  length
 *  qsError:
 --------------------------------------------------------*/
short   SearchAssistChar( char *send )  {
      short nLength;

      while( TRUE )  {
	 nLength = strcspn( wWord->pcUsingPointer, send );
	 if( !wFlush( nLength ) ) break;
      }
      return( nLength );
}


/*---------------
 *  Function Name:      AsqlExecute( )
 *  Arguments:
 *  Input:  query and statistics analyst result
 *  Output: fill expression table and action table in hand structure
 *  Return:  sucess: TRUE
 *           unsucess: FALSE
 *  Call:   ... ...
 *  qsError:
 *-------------------------------------------------------------------------*/
short  AsqlExecute( void )
{
    dFILE *dsp;
    dFILE *dtp;
    MidCodeType *xMid;
    ExecActTable *expActTab;
    CalActOrdTable *paAcTbIt;
    short num;
    unsigned short i, k;

    dsp = *( fFrTo.cSouFName );
    if( fFrTo.targefile == NULL ) {
	if( fFrTo.cTargetFileName[0] != '\0' ) {

		// add in 1995.04.19
		/*if( strchr( fFrTo.cTargetFileName, '*' ) != NULL ) {
			strrstr(fFrTo.cTargetFileName,  "*", "");
			goto  AsqlRunJmp;
		}
		*/
                if( fFrTo.toAppend == '1' )
		{
                      dtp = dsp;
#ifdef WSToMT
		      /*if( fFrTo.phuf == NULL )
		      { //I don't know wether update
			wmtDbfLock( dtp );
		      }*/
#endif
		      dseek(dtp, 0L, dSEEK_END);
		} else

		if( fFrTo.nFieldNum > 0 ) {
			// generate the dFIELD table
			num = 0;
			if( fFrTo.TargetField != NULL ) {
				while( *(fFrTo.TargetField[num].field) ) {
					num++;
				}
				fFrTo.nFieldNum += num;
			}
			if( ( fFrTo.TargetField = \
				realloc(fFrTo.TargetField, \
					(fFrTo.nFieldNum+1)*sizeof( dFIELD ) ) ) == NULL ) {
				qsError = 1008;
				return  FALSE;
			}
			if( dsp == NULL ) {
				qsError = 2004;
				return  FALSE;
			}

			for( i = 0;    num < fFrTo.nFieldNum;    num++ ) {
				if( *(char *)(fFrTo.fieldtable[i].FieldName) == '@' ) {
				    memcpy( &(fFrTo.TargetField)[num], \
					&(dsp->field)[ (*(SPECIALFIELDSTRUCT *)(fFrTo.fieldtable[i].FieldName)).FieldNo ], \
					sizeof(dFIELD) );
				} else {
				    if( (k = GetFldid(dsp, fFrTo.fieldtable[i++].FieldName)) == 0xFFFF ) {
					qsError = 3025;
					return  FALSE;
				    }
				    memcpy( &(fFrTo.TargetField)[num], \
							&(dsp->field)[ k ], \
							sizeof(dFIELD) );
				}
			}
			((fFrTo.TargetField)[num].field)[0] = '\0';
			//num = 0;

			//the following is a program body work with DIO
			//{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}
#ifdef WSToMT
			EnterCriticalSection( &dofnCE );
#endif
			if( dIsAwake( fFrTo.cTargetFileName ) ) {
#ifdef WSToMT
				LeaveCriticalSection( &dofnCE );
#endif
				strZcpy(szAsqlErrBuf, fFrTo.cTargetFileName, sizeof(szAsqlErrBuf));
				qsError = 9001;
				return  FALSE;
			}
			dtp = dcreate(fFrTo.cTargetFileName, \
							fFrTo.TargetField);
			if( dtp == NULL ) {
#ifdef WSToMT
			    LeaveCriticalSection( &dofnCE );
#endif
			    qsError = 5002;
			    return  FALSE;
			}
			if( dSetAwake(dtp, &dtp) != 1 ) {
#ifdef WSToMT
			    LeaveCriticalSection( &dofnCE );
#endif
			    dclose(dtp);
			    qsError = 5009;
			    return  FALSE;
			}
#ifdef WSToMT
			LeaveCriticalSection( &dofnCE );
			wmtDbfLock( dtp );
#endif
			//{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}
		} else {
			if( fFrTo.TargetField != NULL ) {

			//the following is a program body work with DIO
			//{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}
#ifdef WSToMT
				EnterCriticalSection( &dofnCE );
#endif
				if( dIsAwake( fFrTo.cTargetFileName ) ) {
#ifdef WSToMT
					LeaveCriticalSection( &dofnCE );
#endif
					strZcpy(szAsqlErrBuf, fFrTo.cTargetFileName, sizeof(szAsqlErrBuf));
					qsError = 9001;
					return  FALSE;
				}
				dtp = dcreate(fFrTo.cTargetFileName, \
							fFrTo.TargetField);
				if( dtp == NULL ) {
#ifdef WSToMT
				    LeaveCriticalSection( &dofnCE );
#endif
				    qsError = 5002;
				    return  FALSE;
				}
				if( dSetAwake(dtp, &dtp) != 1 ) {
#ifdef WSToMT
				    LeaveCriticalSection( &dofnCE );
#endif
				    dclose(dtp);
				    qsError = 5009;
				    return  FALSE;
				}
#ifdef WSToMT
				LeaveCriticalSection( &dofnCE );
				wmtDbfLock( dtp );
#endif
			//{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}
			} else {
//AsqlRunJmp:
				if( (dtp = dAwake(fFrTo.cTargetFileName, \
						DOPENPARA)) == NULL ) {
					//qsError = 5001;

					if( dsp == NULL ) {
						qsError = 2004;
						return  FALSE;
					}

					fFrTo.TargetField = dfcopy(dsp, NULL);
					dtp = dcreate(fFrTo.cTargetFileName, \
							fFrTo.TargetField);
					if( dtp == NULL ) {
					    qsError = 5002;
					    return  FALSE;
					}
					if( dSetAwake(dtp, &dtp) != 1 ) {
					    dclose(dtp);
					    qsError = 5009;
					    return  FALSE;
					}
#ifdef WSToMT
					wmtDbfLock( dtp );
#endif
				} else {
#ifdef WSToMT
					/*if( fFrTo.phuf == NULL )
					{ //I don't know wether update
					    wmtDbfLock( dtp );
					}*/
#endif
					dseek(dtp, 0, dSEEK_END);


					//2000.8.21
					//get the user right string
					//set the flag

				}
			} // end of else
		}
		if( dtp == NULL ) {
			qsError = 5002;
			return  FALSE;
		} else {
			fFrTo.targefile = dtp;
		}
	} else {
		dtp = NULL;
	}
    } else {
	dtp = fFrTo.targefile;
    }

    //1997.3.22
    //add this statement to avoid the initial calculate type error
    NewRec(dsp);

    // iden_switch experssion
    if( MidIden( dsp, dtp ) == FALSE )   return  FALSE;


    // execute the start action
    // seatch OrdExeTable if nLevel is 'B' then action && delete

    // this statement should exist 1995.08.31 Xilong
    num = ONE_MODALITY_ACTION;

    expActTab = fFrTo.exHead;
    while( expActTab != NULL ) {
	if( expActTab->nLevel == 'B' ) {
		xMid = ActionSymbolRegister(expActTab->xPoint,dsp,dtp, \
				  fFrTo.sHead, fFrTo.nSybNum, \
				  fFrTo.dHead, fFrTo.nDefinNum);
		if( xMid != NULL ) {
			strcpy(szAsqlErrBuf, xMid->values);
			qsError = 3026;
			return  FALSE;
		}
		if( ActionCalExpr(expActTab->xPoint, &num, dtp) == LONG_MAX ){
			qsError = 4002;
			return  FALSE;
		}
	} else if( expActTab->nLevel == 'E' ) {
		if( execGramMan(expActTab, dsp, dtp, 'b') == FALSE ) {
#ifdef RUNING_ERR_MESSAGE_ON
			AsqlAnaErrorMessage( "无源" );
#endif
			wWordFree();
			qsFree();
			fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
			return  FALSE;
		}
	}
//      temp = expActTab;
//      FreeCode( temp->xPoint );
//      free( temp );
	expActTab = expActTab->oNext;
    } // end of while

    if( dsp == NULL )
	goto DSP_NULL;

    if( dsp->dbf_flag == SQL_TABLE || dsp->dbf_flag == ODBC_TABLE )
    { //not support groupby. 1998.4.29
	if( fFrTo.nGroupby == Asql_GROUPYES ) {
	    if( SortedGroupExec(dsp, dtp) == FALSE )
		return  FALSE;
	} else {
	    if( GroupNotExec(dsp, dtp) == FALSE )
		return  FALSE;
	}
    } else {
      switch( fFrTo.nGroupby ) {

	case Asql_GROUPYES:

		if( GroupExec(dsp, dtp) == FALSE )
		    return  FALSE;
		break;

	case Asql_GROUPNOT:

		if( fFrTo.eHead == NULL )
		{ //CONDITION
		  //BEGIN	<--- empty here
		  //END
		    break;
		}

		if( fFrTo.szScopeKey[0] != '\0' ) {
		    if( GroupNotScopeExec(dsp, dtp) == FALSE )
			return  FALSE;
		    break;
		}
		if( GroupNotExec(dsp, dtp) == FALSE )
		    return  FALSE;
		break;

	default:
		qsError = 5003;
      }
    } // end of groupby process


DSP_NULL:

    // last work time of action
    paAcTbIt = fFrTo.cHead;
    num = LASTWORKTIMEOFACTION;
    while( paAcTbIt != NULL ) {
      if( paAcTbIt->actionFunRefered ) {
	if( ActionCalExpr(paAcTbIt->pxActMidCode, &(num), dtp) == LONG_MAX ) {
		qsError = 5004;
		return  FALSE;
	}
      }
      paAcTbIt = paAcTbIt->aNext;
    } // end of while

    expActTab = fFrTo.exHead;
    num = ONE_MODALITY_ACTION;
    while( expActTab != NULL ) {
	if( expActTab->nLevel == 'A' ) {
	    xMid = ActionSymbolRegister( expActTab->xPoint, dsp, dtp, \
				fFrTo.sHead, fFrTo.nSybNum, fFrTo.dHead, \
				fFrTo.nDefinNum );
	    if( xMid != NULL ) {
		strcpy(szAsqlErrBuf, xMid->values);
		qsError = 3026;
		return  FALSE;
	    }
	    if( ActionCalExpr(expActTab->xPoint, &num, dtp) == LONG_MAX ) {
		qsError = 4002;
		return  FALSE;
	    }
        } else if( expActTab->nLevel == 'F' ) {
	    if( execGramMan(expActTab, dsp, dtp, 'a') == FALSE ) {
#ifdef RUNING_ERR_MESSAGE_ON
			AsqlAnaErrorMessage( "无源" );
#endif
			wWordFree();
			qsFree();
			fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
			return  FALSE;
		}
	}
//      temp = expActTab;
	expActTab = expActTab->oNext;
  //    FreeCode( temp->xPoint );
 //     free( temp );
    }

    return  TRUE;

} // end of AsqlExecute function


/*---------------
 *  Function Name:      GroupNotExec()
 *  Return:  sucess: TRUE
 *           unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   GroupNotExec( dFILE *dsp, dFILE *dtp )
{
    long 	lRecNum;
    short 	i;
    CalActOrdTable *paAcTbIt;
    char       	*myAsqlStatFlagY;
    int        	myAsqlStatMaxY;

    myAsqlStatFlagY = asqlStatFlagY;
    myAsqlStatMaxY = asqlStatMaxY;

    lRecNum = getRecNum(dsp);
    i = 0;
    /*if( m_c_AsqlStatTailAction != NULL ) {
	if( ActionCalExpr(m_c_AsqlStatTailAction, &(i), dtp ) == LONG_MAX ) {
		qsError = 5005;
		return  FALSE;
	} // end of if
    }*/
    paAcTbIt = fFrTo.cHead;
    while( paAcTbIt != NULL ) {
      if( paAcTbIt->actionFunRefered ) {
	if( ActionCalExpr(paAcTbIt->pxActMidCode, &(i), dtp ) == LONG_MAX ) {
		qsError = 5006;
		return  FALSE;
	} // end of if
	//paAcTbIt->nCalTimes = 1;
      }
      paAcTbIt = paAcTbIt->aNext;
    }

    i = 1;
    // execute the expression action

    dseek(dsp, 0L, dSEEK_SET);
    while( lRecNum-- ) {

#ifdef _AsqlRuningMessage_Assigned
	// call users function to deal with the runing message
	if( AsqlRuningMessage(lRecNum) ) {
		qsError = 7002;
		return  FALSE;
	}
#endif

	// get a row of table
	if( get1rec( dsp ) == NULL )
	{
	    if( dsp->dbf_flag == SQL_TABLE ) {
		if( TgetSqlErrCode() == 100 )
			return  TRUE;
	    }
	    if( dsp->dbf_flag == ODBC_TABLE ) {
		//if( getI_ErrCode() == 100 )
		if( dsp->error == 0 )
		    return  TRUE;
		else
		    strZcpy(szAsqlErrBuf, szOdbcExecResult, sizeof(szAsqlErrBuf) );

	    }
	    qsError = 7002;
	    return  FALSE;
	}

	if( dsp->rec_buf[0] == '*' ) {
	    dseek(dsp, 1, dSEEK_CUR);
	    continue;
	}

	// execute this record
	if( ExecRec( dsp, dtp ) == FALSE )       return  FALSE;

	if( myAsqlStatMaxY > 0 )
	{
		memset(myAsqlStatFlagY, 0, myAsqlStatMaxY);
	}

	/*if( m_c_AsqlStatTailAction != NULL ) {
		if( ActionCalExpr(m_c_AsqlStatTailAction, \
						    &(i), dtp ) == LONG_MAX ) {
			qsError = 5005;
			return  FALSE;
		} // end of if
	}*/

	//skip a record
	if( dsp->rec_p > 0 )
	    dseek(dsp, 1, dSEEK_CUR);
	else
	    dsp->rec_p = 0-dsp->rec_p;

    } // end of while

    return  TRUE;

} // end of function GroupNotExec()



/*--------------
 *  Function Name:      MidIden( )
 *  Utility: place all parments for modile code
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *------------------------------------------------------------------------*/
static short   MidIden( dFILE *df, dFILE *dt )
{
    ExprTable      *etPoint;
    CalActOrdTable *paAcTbIt;
    MidCodeType    *mc;
    short	    i;

    // symbol register the expression

    /*
     ActionSymbolRegister() and ActionCalExpr() will restore the value
     so this come into unnessary

    //this is very important!!! 1997.3.24. Xilong
    askTdf = NULL;
    */
    etPoint =  fFrTo.eHead;
    while( etPoint != NULL ) {
	 mc = SymbolRegister(etPoint->xPointer, df, fFrTo.sHead, \
				fFrTo.nSybNum, fFrTo.dHead, fFrTo.nDefinNum);
	 if( mc != NULL ) {
		strZcpy(szAsqlErrBuf, mc->values, sizeof(szAsqlErrBuf));
		qsError = 3027;
		return  FALSE;
	 }
	 etPoint = etPoint->eNext;
    } // end of while

    // symbol register the action
    paAcTbIt = fFrTo.cHead;
    while( paAcTbIt != NULL ) {
	 mc = ActionSymbolRegister(paAcTbIt->pxActMidCode, \
		 df, dt, fFrTo.sHead, \
		 fFrTo.nSybNum, fFrTo.dHead, fFrTo.nDefinNum);
	 if( mc != NULL ) {
		strZcpy(szAsqlErrBuf, mc->values, sizeof(szAsqlErrBuf));
		qsError = 3026;
		return  FALSE;
	 }

	 //actionFunRefered is assigned by ActionSymbolRegister()
	 //means wether the expression has refered action_function
	 paAcTbIt->actionFunRefered = actionFunRefered;

	 paAcTbIt = paAcTbIt->aNext;
    } // end of while

    for(i = 0;   i < fFrTo.cSouDbfNum;  i++ ) {
	 if( fFrTo.xKeyExpr[i] != NULL ) {
		mc = ActionSymbolRegister(fFrTo.xKeyExpr[i], \
					  df, dt, fFrTo.sHead, \
				 fFrTo.nSybNum, fFrTo.dHead, fFrTo.nDefinNum);
		if( mc != NULL ) {
			strZcpy(szAsqlErrBuf, mc->values, sizeof(szAsqlErrBuf));
			qsError = 3026;
			return  FALSE;
		}
	 }
    }

/********
* this expression is registed from upper statements, 1994.6.29
    if( m_c_AsqlStatTailAction != NULL ) {
	 if( ActionSymbolRegister( m_c_AsqlStatTailAction, \
		 df, dt, fFrTo.sHead, \
		 fFrTo.nSybNum, fFrTo.dHead, fFrTo.nDefinNum) != NULL ) {
		qsError = 3026;
		return  FALSE;
	 }
    } // end of if
***************************************/

    return  TRUE;

} // end of function MidIden()


/*--------------
 *  Function Name:      ExecRec()
 *  Utility: execute one record
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
static short   ExecRec( dFILE *dsp, dFILE *dtp )
{
    ExprTable   *etPoint;
    long        l;

    etPoint = fFrTo.eHead;
    while( etPoint != NULL )  {

	if( etPoint->cFirstActionType )
	{   //stat expression

	    //1999.1.31
	    //  #y1:
	    //  #y2:
	    //  ......
	    //  #x1:  expre1
	    //  #x2:  expre2
	    //  #x3:  expre3
	    //
	    //algorithm:
	    //   when calculate expre1,expre2 or expre3, first check wether
	    // there is any asqlStatFlagY[] is set, if there is nothing set,
	    // skip this expression.
	    //

	    int i;

	    for( i = asqlStatMaxY-1;  i >= 0;   i-- ) {
		if( asqlStatFlagY[i] != 0 ) {
			break;
		}
	    }
	    if( i < 0 ) {
		etPoint = (ExprTable *)etPoint->nNextParallelExpr;
		continue;
	    }
	}

	if( (l = CalExpr(etPoint->xPointer)) == LONG_MAX ) {
	    qsError = 5005;
	    return  FALSE;
	}

	if( l != 0 ) { //****** TRUE ******

	    if( etPoint->nFirstAction != NULL ) {
		// go to the next thread
		if( ((ExprTable *)(etPoint->nFirstAction))->ExprTableFlag < 0 ) {
			etPoint = (ExprTable *)(etPoint->nFirstAction);
		} else {
		    if( ActionCalExpr(etPoint->nFirstAction, \
				&(one), dtp ) >= (LONG_MAX-16) ) {

			    if( GetCurrentResult() == (LONG_MAX-16) ) {
				short i;

				etPoint = NULL;
				for(i = 0;  i <= fFrTo.labelNum;  i++) {
				    if( stricmp(fFrTo.asqlLabel[i].label, \
					fFrTo.jmpLabel) == 0 ) {
					    etPoint = fFrTo.asqlLabel[i].etPoint;
					    break;
					}
				}
			    } else {
				if( errorStamp )
				{ //1998.6.17
				    //qsError = 7001;
				    qsError = 0;
				} else {
				    qsError = 5005;
				}
				return  FALSE;
			    }
		    } else  etPoint = (ExprTable *)etPoint->nNextParallelExpr;
		}
	    } else {
		etPoint = (ExprTable *)etPoint->nNextParallelExpr;
	    }

	} else {                                        //****** FALSE ******

	     if( etPoint->nSecondAction != NULL ) {
		 // go to the next thread
		 if( *(short *)(etPoint->nSecondAction) < 0 ) {
		     etPoint = (ExprTable *)etPoint->nSecondAction;
		 } else {
		     if( ActionCalExpr(etPoint->nSecondAction, \
					  &(one), dtp ) >= (LONG_MAX-16) ) {

			    if( GetCurrentResult() == (LONG_MAX-16) ) {

				short i;

				etPoint = NULL;
				for(i = 0;  i <= fFrTo.labelNum;  i++) {
				    if( stricmp(fFrTo.asqlLabel[i].label, \
					fFrTo.jmpLabel) == 0 ) {
					    etPoint = fFrTo.asqlLabel[i].etPoint;
					    break;
					}
				}
			    } else {
				 if( errorStamp )
				 {
				     //qsError = 7001;
				     qsError = 0;
				 } else {
				     qsError = 5004;
				 }
				 return  FALSE;
			    }
		     } else etPoint = (ExprTable *)etPoint->nNextParallelExpr;
		 }
	    } else {
		etPoint = (ExprTable *)etPoint->nNextParallelExpr;
	    }

	} // end of else. one item calculated.

    } // end of while

    return  TRUE;

} // end of function ExecRec()



/****************************************************************************
 *  Function Name:      AskQS()
 *  Utility: execute one record
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 ***************************************************************************/
short  AskQS(char *FileName, short ConditionType, dFILE *FromTable, \
		    dFILE *ToTable, SysVarOFunType **VariableRegister, \
							short *VariableNum )
{

//    FromToStru fFrTo;

    //the following statements remarked by //@! is because
    //the memset statement added. 1997.11.4 Xilong
    _try {

    memset(&fFrTo, 0, sizeof(FromToStru));

    fFrTo.cTitleName[0] = ' ';		//fFrTo is really
    fFrTo.cSouFName = &FromTable;
    szAsqlErrBuf[0] = '\0';

    qsError = 0;
    ErrorSet.xERROR = XexpOK;
    ErrorSet.string[0] = '\0';

    //reset escape char
    cEscapeXexp = '\\';

    if( FromTable != NULL )     fFrTo.cSouDbfNum = 1;           // no source
    //@!else                        fFrTo.cSouDbfNum = 0;

    //@!fFrTo.cTargetFileName[ 0 ] = '\0';
    fFrTo.targefile = ToTable;
    //@!fFrTo.cGroKey[ 0 ] = '\0';
    //@!fFrTo.phuf = NULL;

    //@!fFrTo.eHead = NULL;
    //@!fFrTo.cHead = NULL;
    //@!fFrTo.dHead = NULL;

    /*
    if( VariableRegister != NULL )       fFrTo.sHead = *VariableRegister;
    //@!else                                 fFrTo.sHead = NULL;

    //@!fFrTo.exHead = NULL;

    if( VariableNum == NULL )   fFrTo.nSybNum = 0;
    else                        fFrTo.nSybNum = *VariableNum;
    */
    //change this. 1998.5.11 Xilong
    if( VariableRegister != NULL )       xSelfBaseVar = *VariableRegister;
    else 	xSelfBaseVar = NULL;
    if( VariableNum == NULL )   nSelfBaseVar = 0;
    else        nSelfBaseVar = *VariableNum;


    //@!fFrTo.nDefinNum = 0;
//    fFrTo.next = 0;
    //@!fFrTo.TargetField = NULL;
    //@!fFrTo.fieldtable = NULL;
    //@!fFrTo.nFieldNum = 0;

    //@!fFrTo.funNum = 0;
    fFrTo.labelNum = -1;

    ConditionAppeared = 0;
    wWord = NULL;

	// init the env
    if( (ConditionType & Asql_USEENV) != Asql_USEENV ) {
	asqlEnv.szAsqlResultPath[0] = '\0';
	asqlEnv.szAsqlFromPath[0] = '\0';
    } else {
	//if use the environment, but not set them, use the system config
      	if( (asqlEnv.szAsqlResultPath[0] == '\0') && \
	                                 asqlEnv.szAsqlFromPath[0] == '\0' )
            memcpy(&asqlEnv, &asqlConfigEnv, sizeof(Asql_ENV));
    }


#ifdef __BORLANDC__
	dsetbuf( 32000 );
#else
	dsetbuf( 64000 );
#endif

    //clear temp table list
    clrTempTableList();

    while( ConditionAppeared == 0 ) {

	errorStamp = 0;

	//
	//try szExcept
	//except szExcept
	//endexcept
	//
	szExcept[0] = '\0';

	AsqlStatTarget[0] = '\0';
	iAsqlStatAction = 0;
	//AsqlSumTarget[0] = '\0';
	//AsqlAvgTarget[0] = '\0';
	StatementPreAction[0] = '\0';
	//AsqlStatTailAction[0] = '\0';
	//m_c_AsqlStatTailAction = NULL;
	asqlStatMaxY = -1;
	fFrTo.tTree = NULL;
	fFrTo.nGroupby = Asql_GROUPNOT;
	fFrTo.nOrderby = 0;

	switch( ConditionType & AsqlExprInFile )  {
		case AsqlExprInFile:
		if( AsqlAnalyse( FileName, NULL ) == FALSE ) {
#ifdef RUNING_ERR_MESSAGE_ON
				AsqlAnaErrorMessage( NULL );
#endif
			asqlCloseFiles(0);
			fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
			return  FALSE;
		}
		break;
		case AsqlExprInMemory:
		if( AsqlAnalyse( NULL, FileName ) == FALSE ) {
#ifdef RUNING_ERR_MESSAGE_ON
			AsqlAnaErrorMessage( NULL );
#endif
			asqlCloseFiles(0);
			fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
			return  FALSE;
		}
		break;
		default:
		    qsError = 5007;
		    fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
		return  FALSE;
	} // end of switch


	XexpUserDefinedVar = &fFrTo.sHead;
	XexpUserDefinedVarNum = &fFrTo.nSybNum;


	// no action and from
	if( fFrTo.cSouDbfNum <= 0 && fFrTo.cTargetFileName[0] == '\0') {
		if( fFrTo.exHead == NULL ) {
#ifdef RUNING_ERR_MESSAGE_ON
		    AsqlAnaErrorMessage( "无源" );
#endif
		    wWordFree();
		    qsFree();
		    fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
		    return  FALSE;
		} else
		{ //SINGLE PORGRAM

		  //
		  // (1)action ...
		  // (2)action ...
		  //
		  // (3)let ...
		  // ................................
		  //
		  // if you don't set expActTab->nLevel to  'b',
		  // the sentense (2) won't be executed by execGramMan()
		  //

		  ExecActTable *expActTab;

		  expActTab = fFrTo.exHead;
		  while( expActTab != NULL ) {
		    expActTab->nLevel = 'b';
		    expActTab = expActTab->oNext;
		  }

		  if( execGramMan(fFrTo.exHead, NULL, NULL, 'b') == FALSE ) {
#ifdef RUNING_ERR_MESSAGE_ON
			AsqlAnaErrorMessage( "无源" );
#endif
			wWordFree();
			qsFree();
			fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
			return  FALSE;
		  }

		  if( ConditionAppeared >= 1 ) {
		    qsTableFree();
		    ConditionAppeared = 0;
		    continue;
		  } else {
		    break;
		  }
		} //SINGLE PORGRAM
	}

	// execute the analysed result
	if( fFrTo.cSouDbfNum <= 1 ) {
		if( AsqlExecute() == FALSE ) {
		    if( errorStamp != 2 ) {
			if( szExcept[0] == '\0' || wWord == NULL ) {
#ifdef RUNING_ERR_MESSAGE_ON
			    AsqlAnaErrorMessage( "运行错" );
#endif
			    asqlCloseFiles(0);
			    wWordFree();
			    qsFree();
			    fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
			    return  FALSE;
			} else {
			    //deal with except
			    char *szp, *szp1;
			    char buf[32];

			    szp1 = wWord->pcWordAnaBuf;
			    while( 1 ) {
				szp = locateKeywordInBuf(szp1, "EXCEPT", buf, 32);
				if( szp == NULL ) {
				    asqlCloseFiles(0);
				    wWordFree();
				    qsFree();
				    fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
				    return  FALSE;
				}

				szp1 = szp;
				while( *szp1 != '\n' && *szp1 != '\r' && *szp1 )
				    szp1++;
				while( *szp1 == '\n' || *szp1 == '\r' )
				    szp1++;

				if( stricmp(buf, szExcept) == 0 ) {
					wWord->pcUsingPointer  = szp1;
					if( GetWord() == FALSE ) {
					    asqlCloseFiles(0);
					    wWordFree();
					    qsFree();
					    fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
					    return  FALSE;
					}
					qsError = 0;
					break;
				}
			    } //while
			} //else
		    } //if
		}
	} else {

		if( AsqlMoreDbfExecute() == FALSE ) {
		    if( errorStamp != 2 ) {
			if( szExcept[0] == '\0' || wWord == NULL ) {
#ifdef RUNING_ERR_MESSAGE_ON
			    AsqlAnaErrorMessage( "运行错" );
#endif
			    asqlCloseFiles(0);
			    wWordFree();
			    qsFree();
			    fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
			    return  FALSE;
			} else {
			    //deal with except
			    char *szp, *szp1;
			    char buf[32];

			    szp1 = wWord->pcWordAnaBuf;
			    while( 1 ) {
				szp = locateKeywordInBuf(szp1, "EXCEPT", buf, 32);
				if( szp == NULL ) {
				    asqlCloseFiles(0);
				    wWordFree();
				    qsFree();
				    fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
				    return  FALSE;
				}

				szp1 = szp;
				while( *szp1 != '\n' && *szp1 != '\r' && *szp1 )
				    szp1++;
				while( *szp1 == '\n' || *szp1 == '\r' )
				    szp1++;

				if( stricmp(buf, szExcept) == 0 ) {
					wWord->pcUsingPointer  = szp1;
					if( GetWord() == FALSE ) {
					    asqlCloseFiles(0);
					    wWordFree();
					    qsFree();
					    fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
					    return  FALSE;
					}
					qsError = 0;
					break;
				}
			    } //while
			} //else
		    }
		}
	}

	if( ConditionAppeared >= 1 )            ConditionAppeared = 0;
	else                                    break;

	qsTableFree();
    } // end of while

//    dSleep( NULL );

#ifdef _AsqlRuningMessage_Assigned
#ifdef __N_C_S
    if( insideTask == 0 ) {
	if( fFrTo.targefile != NULL ) {
		if( fFrTo.cTargetFileName != '\0' ) {
			cxmainShow( &fFrTo );
		}
	}
    }
#endif
#endif

} //end of try

//$$$$$$$$ _try_except_ _try_except_ _try_except_ _try_except_ _try_except_
_except( EXCEPTION_EXECUTE_HANDLER ) {
    
    //2000.7.6
    //wWord = NULL;   //to avoid wWordFree() exception

    qsError = (unsigned short)65535;
}

_try {
    asqlCloseFiles(1);

    wWordFree();
    qsFree();
}
_except( EXCEPTION_EXECUTE_HANDLER ) {
    qsError = (unsigned short)65535;
}
    
    //clear the environment
    memset(&fFrTo, 0, sizeof(FromToStru));

    //fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
    nSelfBaseVar = 0;
    xSelfBaseVar = NULL;

    return  TRUE;

} // end of function AskQS()


/*--------------
 *  Function Name:      asqlCloseFiles()
 *  Utility:
 *    Give some dFile name and the fields description, fill the dFIELD
 *  struct. Fields described as: 0:TableName, 1:@1.2, 2:@2.2, ...,
 *  Fieldname length dec type, ...
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/

static void asqlCloseFiles( int commitOrRollback )
{
    short  i;

#ifdef WSToMT
    if( fFrTo.phuf != NULL ) {
	//needn't
	//wmtDbfLock( fFrTo.targefile );
	if( commitOrRollback )
	    asqlDbCommit();

	for(i = fFrTo.cSouDbfNum - 1;   i >= 0;   i--) {
	    freeRecLock( fFrTo.cSouFName[i] );
	}

	chsize( fileno(fFrTo.phuf), 0 );
	fclose(fFrTo.phuf);
    } else
	//1998.11.16
	//if( fFrTo.toAppend != '1' )
	if( wmtDbfIsLock( fFrTo.targefile ) )
	{
	    wmtDbfUnLock( fFrTo.targefile );
	}
#endif
    // close files
    for( i = 0;  i < fFrTo.cSouDbfNum;  i++ ) {
	if( fFrTo.fromExclusive )
		wmtDbfUnLock(fFrTo.cSouFName[i]);

	 dSleep( fFrTo.cSouFName[i] );
    }
    if( fFrTo.cSouDbfNum > 0 ) {
        free( fFrTo.cSouFName );
        //fFrTo.cSouDbfNum = 0;
    }

    if( fFrTo.targefile != NULL ) {
	if( fFrTo.cTargetFileName != '\0' && fFrTo.toAppend == '0' ) {

	   if( fFrTo.nOrderby )
		orderbyExec( &fFrTo );

	     dSleep( fFrTo.targefile );
	}
    }

    closeTempTableList();

    if( fFrTo.distinctBh != NULL ) {
	IndexDispose( fFrTo.distinctBh );
	//fFrTo.distinctBh = NULL;
    }

} //end of asqlCloseFiles()



/*--------------
 *  Function Name:      GenerateDfield()
 *  Utility:
 *    Give some dFile name and the fields description, fill the dFIELD
 *  struct. Fields described as: 0:TableName, 1:@1.2, 2:@2.2, ...,
 *  Fieldname length dec type, ...
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
dFIELD *   GenerateDfield( char **DbfName, dFILE *DfileHandle[], \
				unsigned short DfileNum, char *Description)
{
    unsigned short  i, j, k;
    unsigned short  FieldNo, FieldNum;
    unsigned char  *p1, *p2, *p;
    dFIELD 	   *Dfield, *vp;
    char            buf[8192];

    if( DfileHandle == NULL || Description == NULL /*|| DfileNum <= 0*/ ) {
	    qsError = 6001;
	    return( NULL );
    }

    strZcpy(buf, Description, 8192);
    Description = buf;

    // deal the description
    while( *Description == ' ' || *Description == '\t')
	Description++;

    p = Description;
    while( *p != '\0' && *p != '\n' && *p != '\r' ) {
	if( *p == '\\' ) {
		while( *p == '\n' || *p == '\r' )       p++;
	}
	p++;
    }
    *p = '\0';

    // generate the table name
    *DbfName = Description;
    if( ( p1 = strchr(Description, ',') ) == NULL ) {
            /* description error or no error */
	    //qsError = 3028;
	    //this is a right grammar

	    return( NULL );
    }
    *p1 = '\0';

    Description = ++p1;

    ltrim( Description );
    if( *Description == '*' ) {
	// get df pointer
	FieldNum = 0;
	for(i = 0;   i < DfileNum;  i++ ) {
		FieldNum += DfileHandle[i]->field_num;
	}
	FieldNum++;

	if( (Dfield = (dFIELD *)calloc(FieldNum, sizeof(dFIELD)) ) == NULL ) {
	    qsError = 1001;                     /* description error */
	    return( NULL );
	}

	// get the new field message
	dfcopy(DfileHandle[0], Dfield);
//	*(char *)( Dfield[ --FieldNum ].field ) = '\0';
//      FieldNum = DfileHandle[0]->field_num;
	for(FieldNum = 0;  Dfield[FieldNum].field[0] != '\0';  FieldNum++);

	for(i = 1;  i < DfileNum;  i++) {
              char szBuf[16];
              strZcpy(szBuf, TrimFileName(DfileHandle[i]->name), 10);
	      for( j = 0;  j < DfileHandle[i]->field_num;  j++ ) {
			// check whether the field appeared
			vp = &(DfileHandle[i]->field[j]);
			for( k = 0;   k < FieldNum;   k++ ) {
				if( stricmp( vp->field, Dfield[k].field ) == 0 )
                                {
				        memcpy( &(Dfield[FieldNum]), vp, sizeof(dFIELD) );
					sprintf(Dfield[FieldNum++].field,"%s.%s", szBuf,(char *)vp);
					break;
				}
			}
			if( k >= FieldNum ) {
				memcpy( &(Dfield[FieldNum++]), vp, sizeof(dFIELD) );
			}
		} // end of for j
	} // end of for i
	Dfield[FieldNum].field[0] = '\0';
	return  Dfield;
    } // end of if *

    // count the fieldnum
    FieldNum = 2;

    if( strchr(p1, '*' ) != NULL ) {
	for(i = 0;  i < DfileNum;  i++) {
	    FieldNum += DfileHandle[i]->field_num;
	}
    }

    while( *p1 != '\0' )        if( *p1++ == ',' )          FieldNum++;


    if( FieldNum <= 1 ) {
	    qsError = 3016;         /* description error: no field */
	    return( NULL );
    }

    // alloc the memory for field
    if( ( Dfield = calloc( FieldNum + 1, sizeof( dFIELD ) ) ) == NULL ) {
	    qsError = 1001;         /* memory alloc error */
	    return( NULL );
    }
    Dfield[ FieldNum ].field[0] = '\0';

    i = 0xFFFF;
    // p1 pointes to the next description, p2
    while( *Description != '\0' ) {

	if( ( p1 = strchr(Description, ',') ) != NULL )
	     *p1 = '\0';

	while( isspace(*Description) ) {
		Description++;
	}

	// get the field's position
	if( ( p2 = strchr(Description, ':') ) == NULL ) {
	     if( ++i >= FieldNum ) {
		 qsError = 1001;         // field too more
		 free( Dfield );
		 return( NULL );
	     }
	} else {
	     *p2 = '\0';
	     if( ( i = atoi( Description ) ) >= FieldNum ) {
		 qsError = 3016;         // field too more
		 free( Dfield );
		 return  NULL;
	     }
	     Description = p2 + 1;
	}
	if( *Description == '@' ) {
	     Description++;
	     if( ( p2 = strchr( Description, '.' ) ) != NULL ) {
		 *p2++ = '\0';
		 memcpy( &Dfield[i], &DfileHandle[atoi(Description)]->\
					field[atoi(p2)], sizeof(dFIELD) );
	     } else {
		 memcpy( &Dfield[i], &DfileHandle[0]->\
				field[atoi(Description)], sizeof(dFIELD) );
	    }
	} else {
	    // field is described as name length dec type
	    j = 0;
	    p2 = Dfield[i].field;
	    while( *Description != '\0' && !isspace( *Description ) && \
		   j < FIELDNAMELEN - 1 )          *p2++ = *Description++;
	    if( j >= FIELDNAMELEN - 1 ) {
		   qsError = 3016;              // field name is too long
		   free( Dfield );
		   return  NULL;
	    }
	    *p2 = '\0';

	    while( *Description != '\0' && !isdigit( *Description ) ) {
		if( !isspace( *Description ) ) {
			qsError = 3016;
			free( Dfield );
			return( NULL );
		} else {
			Description++;
		}
	    }

	    if( *(p2 = Description ) == '\0' ) {          // field is descriped as fieldname
		char *sz;

		sz = strchr(Dfield[i].field, '*');
		if( sz != NULL ) {
		  //find the alias

		  if( sz == Dfield[i].field ) {
			qsError = 3016;
			free( Dfield );
			return  NULL;
		  }
		  
		  *(sz-1) = '\0';
		  sz = Dfield[i].field;
		  for( j = 0;  j < DfileNum;  j++ ) {
			if( stricmp(sz, TrimFileName(DfileHandle[j]->name)) == 0 )
			    break;
		  }
		  if( j >= DfileNum ) {
			for( j = 0;  j < DfileNum;  j++ ) {
			    if( stricmp(sz, DfileHandle[j]->szAlias) == 0 )
				break;
			}
		  }

		  if( j >= DfileNum ) {
			qsError = 3016;
			free( Dfield );
			return  NULL;
		  }

		  for( FieldNo = 0;  FieldNo < DfileHandle[j]->field_num;  FieldNo++ ) {
			memcpy(Dfield[i++].field, &DfileHandle[j]->field[FieldNo], \
					sizeof(dFIELD) - sizeof(char *)*3 );
		  }
		  i--;

		} else {
		  for( j = 0;  j < DfileNum && \
		     ( FieldNo = GetFldid( DfileHandle[j], Dfield[i].field ) ) == 0xFFFF;
			     j++ );
		  if( j >= DfileNum ) {
			qsError = 3016;
			free( Dfield );
			return( NULL );
		  }
		  memcpy(Dfield[i].field, &DfileHandle[j]->field[FieldNo], \
					sizeof(dFIELD) - sizeof(char *)*3 );
		}
	    } else {

		// get the length
		while( *p2 != '\0' && isdigit(*p2) )  p2++;
		if( p2 == Description ) {
			Dfield[i].fieldlen = 10;
			Dfield[i].fielddec = 0;
			Dfield[i].fieldtype = 'C';
			break;                 // break from the while
		} else {
			*p2++ = '\0';
			if( (Dfield[i].fieldlen = atoi( Description )) <= 0 ) {
				Dfield[i].fieldlen = 10;
			}
		}

		// get the dec
		while( isspace( *p2 ) )        p2++;
		Description = p2;

		while( *p2 != '\0' && isdigit(*p2) )  p2++;
		if( p2 == Description ) {
			if( *p2 != '\0' ) {
				Dfield[i].fielddec = 0;
				Dfield[i].fieldtype = *p2++;
				goto haveGetFieldJMP;
			} else {
				Dfield[i].fielddec = 0;
				Dfield[i].fieldtype = 'C';
				break;                 // break from the while
			}
		} else {
			*p2++ = '\0';
			Dfield[i].fielddec = atoi( Description );
		}
		Description = p2;

		// get the type
		while( *Description != '\0' && *Description != ',' && \
			!isalpha( *Description ) ) {
				Description++;
		}
		if( !(isalpha( Dfield[i].fieldtype = *Description) ) ) {
			Dfield[i].fieldtype = 'C';
		}
haveGetFieldJMP:
		;
	    } // end of else
	} // end of else

	if( p1 != NULL )        Description = p1 + 1;
	else    break;

    } // end of else

    *p = ' ';

    return  Dfield;

} // end of function GenerateDfield()


/*---------------
 *  Function Name:      AsqlMoreDbfExecute( )
 *  Arguments:
 *  Input:  query and statistics analyst result
 *  Output: fill expression table and action table in hand structure
 *  Return:  sucess: TRUE
 *           unsucess: FALSE
 *  Call:   ... ...
 *  qsError:
 *-------------------------------------------------------------------------*/
static short  AsqlMoreDbfExecute( void )
{
    dFILE       *dsp;
    dFILE       *dtp;
    dFILE       *df;
    MidCodeType *xMid;
    ExecActTable *expActTab;
    CalActOrdTable *paAcTbIt;
    short       num;

    dFILE       *sdf[MOST_DBFNUM_ONCEQUERY];
    bHEAD       *bh[MOST_DBFNUM_ONCEQUERY];
    char        *FieldName[2];
    char        *ssKeyField[MOST_DBFNUM_ONCEQUERY];
    dFIELD      *dField;
    short       j, k, FieldNum;
    int		i;
    unsigned short pos[MOST_DBFNUM_ONCEQUERY];
    char        buf[512];
    char        *sz;
    long int    RecNo[MOST_DBFNUM_ONCEQUERY];
    short       GoOn;
    void        *vp;
    short       DbfNum;
    long        lRecNum, lp, gcount;
    //long        oldrec_p;
    //char        dioTmpFile[260];
    char        *myAsqlStatFlagY;
    int         myAsqlStatMaxY;
    MidCodeType *xKeyExpr[MOST_DBFNUM_ONCEQUERY];
    char        xLocateStr[MOST_DBFNUM_ONCEQUERY][256];
    char        *nextMainRec;
    short       iScopeSkipAdjust = fFrTo.iScopeSkipAdjust;

    dFIELD 	*fld1, *fld2;

    nextMainRec = &(fFrTo.nextMainRec);
    myAsqlStatFlagY = asqlStatFlagY;
    myAsqlStatMaxY = asqlStatMaxY;

    if( (DbfNum = fFrTo.cSouDbfNum) > MOST_DBFNUM_ONCEQUERY ) {
	qsError = 6001;         //库太多
	return  FALSE;
    }

    // modified 1995.09.05
    memset(ssKeyField, 0, MOST_DBFNUM_ONCEQUERY*sizeof(char *));
    memset(xKeyExpr, 0, MOST_DBFNUM_ONCEQUERY*sizeof(MidCodeType *));

    seperateStr(fFrTo.szKeyField, '=', ssKeyField);

    // the following will generate a temp dbf for calculate the mode dbf
    // expr 1. cal temp dbf field num
    //      2. generate fields info
    //      3. generate the dbf
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //1999.5.28
    fFrTo.lRecNo = &(RecNo[0]);

    // get df pointer
    FieldNum = 0;
    for(i = 0;   i < DbfNum;  i++ ) {
	sdf[i] = fFrTo.cSouFName[i];

	sz = NULL;

	if(ssKeyField[i] == NULL ) {
	     ssKeyField[i] = ssKeyField[0];
	//1998.1.18
	} else {
	     sz = strchr(ssKeyField[i], ',');
	     if( sz != NULL ) {
		*sz = '\0';
		if( (xKeyExpr[i]=WordAnalyse(sz+1)) == NULL ) {
		    qsError = 6002;         //主关键字在相应库中找不到
		    return  FALSE;
		}

		fFrTo.xKeyExpr[i] = xKeyExpr[i];

		if( xKeyExpr[i]->type == END_TYPE ) {
		    qsError = 6002;         //主关键字在相应库中找不到
		    return  FALSE;
		}
	     }
	}

	strZcpy(buf, ssKeyField[i], 255);

	if( isdigit( buf[0] ) ) {

	    // 10, a+b
	    //   ^_____________sz point here now
	    //
	    if( sz == NULL || (i == 0 && xKeyExpr[0] == NULL) ) 
	    {   //the relation of first table shouldn't be descriped as:
		//FROM a,b,*10=12,b1+b2
		//they can only be:
		//FROM a,b,*a1+a2=12b1+b2
		    qsError = 6002;         //主关键字在相应库中找不到
		    return  FALSE;
	    }

	    *sz = ',';

	    //gice it a virtual value
	    pos[0] = 0;
	} else {  ////////////

	// FROM a,b,*a1+a2=b1+b2
	// now the base relation expr is not a field, the value should be calculated
	// so specially remember it
	//
	sz = strchr(buf, '+');
	if( sz != NULL ) {
	    if( i == 0 && xKeyExpr[0] == NULL )
		fFrTo.xKeyExpr[0] = xKeyExpr[0] = WordAnalyse(buf);
	    *sz = '\0';
	}

	if( (pos[i] = GetFldid(sdf[i], buf)) == 0xFFFF ) {
	    if( i == 0 || lrtrim(buf)[0] != '\0' || xKeyExpr[i] != NULL) {
		    qsError = 6002;         //主关键字在相应库中找不到
		    return  FALSE;
	    }
	}
	}  ////////////

	//1999.5.28
	//when i is 0, this value is no use
	if( fFrTo.xKeyExpr[i] != NULL ) {
	    fFrTo.szLocateStr[i] = xLocateStr[i];
	} else {
	    fFrTo.szLocateStr[i] = buf;
	}

	FieldNum += sdf[i]->field_num;

    } //end of for
    FieldNum += 2;

    if( (dField = (dFIELD *)calloc(FieldNum, sizeof(dFIELD)) ) == NULL ) {
	qsError = 1001;
	return  FALSE;
    }

    // get the new field message
    dfcopy(sdf[0], dField);
//    FieldNum = sdf[0]->field_num;
    for(FieldNum = 0;   dField[FieldNum].field[0] != '\0';  FieldNum++);

    for(i = 1;  i < DbfNum;  i++) {

	char szBuf[32];

	if( sdf[i]->szAlias[0] != '\0' ) {
		strZcpy(szBuf, sdf[i]->szAlias, 10);
	} else {
		strZcpy(szBuf, TrimFileName(sdf[i]->name), 10);
	}

	for( j = 0;  j < sdf[i]->field_num;  j++ ) {
	    /* Xilong 1995.09.11 consider *wu_name=wu_code
	    if( j != pos[i] )*/ {
		// check whether the field appeared
		vp = &(sdf[i]->field[j]);
		for( k = 0;   k < FieldNum;   k++ ) {
			if( stricmp( (char *)vp, dField[k].field ) == 0 )
			{ //in old version, the same fieldname cannot
			  //appeared, now we allowed this
			  memcpy( &(dField[FieldNum]), vp, sizeof(dFIELD) );
			  sprintf(dField[FieldNum++].field,"%s.%s", szBuf,(char *)vp);
			  break;
			}
		}
		if( k >= FieldNum ) {
			char *sz;

			memcpy( &(dField[FieldNum]), vp, sizeof(dFIELD) );

			sz = dField[FieldNum++].field;
			strcpy( sz+strlen(sz)+1, szBuf );
		}
	    }
	}
    } // end of for
    *(char *)( dField[ FieldNum ].field ) = '\0';
/*
#ifdef DEBUG
printf("2552:%d\n", heapcheck());
#endif
*/
    // create the target for the source querry
    /*{
	char   *spath = tempnam("","");

	sprintf(dioTmpFile, "ILDIODBF.%03X", intOfThread&0xFFF);
	if( spath != NULL ) {
	    makefilename(dioTmpFile, spath, dioTmpFile);
	    free(spath);
	}
    }*/
    if( (dsp = dTmpCreate(dField)) == NULL ) {
	free( dField );
	qsError = 6003;         //临时文件创建不成功
	return FALSE;
    }
    free( dField );

    // the following will make the fieldstart of temp dbf point to the
    // orient dbf
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    FieldNum = 0;
    fld1 = dsp->field;
    for( i = 0;   i < DbfNum;   i++ ) {

	fld2 = sdf[i]->field;
	k = sdf[i]->field_num;

	for( j = 0;  j < k;   j++ ) {
		//if( (i == 0) || (j != pos[i]) ) Xilong 1995.09.11 consider *wu_name=wu_code
		{
			fld1[ FieldNum++ ].fieldstart = \
						fld2[j].fieldstart;
			/*1997.12.4 Xilong
			if( (name=GetFldid(dsp, sdf[i]->field[j].field)) != 0xFFFF )
			{
				dsp->field[ dsp->fld_id[name] ].fieldstart = \
						sdf[i]->field[j].fieldstart;
			}*/
		}
	}
    } // end of for

    // the following will prepare for index for dbf
    memset(bh, 0, MOST_DBFNUM_ONCEQUERY*sizeof(bHEAD *));
    for( i = 1;  i < DbfNum;  i++ ) {
	if( pos[i] == 0xFFFF )
	{ //appear in FROM, but no releation
	    bh[i] = NULL;
	    continue;
	}

	//2000.8.5
	if( isdigit(ssKeyField[i][0]) )
	    goto  AsqlMoreDbfExecute_DNOTSEEKNDX;

	for(k = 0;   k < sdf[i]->syncBhNum;   k++)
	{
	    if( keyFieldPartCmp((bHEAD *)(sdf[i]->bhs[k]), ssKeyField[i]) == 0 ) {
		bh[i] = IndexAwake((char *)sdf[i], ((bHEAD *)(sdf[i]->bhs[k]))->ndxName,
									BTREE_FOR_OPENDBF);
		break;
	    }
	}

AsqlMoreDbfExecute_DNOTSEEKNDX:

	if( bh[i] == NULL ) {
/*	bh[i] = IndexOpen((char *)sdf[i], buf, BTREE_FOR_OPENDBF );
	if( keyFieldPartCmp(bh[i], ssKeyField[i]) ) {}
		FieldName[0] = ssKeyField[i];
		FieldName[1] = NULL;
*/
		/*added it 1997.4.6, we CANNOT build a index without the
		 *permission of system
		*/
	    sprintf(buf, "AT%02d%03X", i, intOfThread&0xFFF);
	    makefilename(buf, tmpPath, buf);

	    if( isdigit(ssKeyField[i][0]) )
	    {
		char *sz = strchr(ssKeyField[i], ',');
		short iw;

		if( sz == NULL ) {
			qsError = 6004;         //索引创建出错
			bhClose( bh, DbfNum );
			dTmpClose( dsp );
			return  FALSE;
		}

		*sz++ = '\0';
		iw = atoi(ssKeyField[i]);

	        if( (bh[i] = IndexBuildKeyExpr(sdf[i], sz, iw, buf)) == NULL ) {
			qsError = 6004;         //索引创建出错
			bhClose( bh, DbfNum );
			dTmpClose( dsp );
			return  FALSE;
		}

	    } else 
	    { //needn't set the Awake and Open flag,
	      //for the IndexSleep with check them by memory usage

	        if( (bh[i] = IndexBAwake( (char *)sdf[i], ssKeyField[i], buf, \
					    BTREE_FOR_OPENDBF )) == NULL ) {
			qsError = 6004;         //索引创建出错
			bhClose( bh, DbfNum );
			dTmpClose( dsp );
			return  FALSE;
		}
	    }
	}

	//the action relcnt() depend on this seting
	fFrTo.syncBh[i] = bh[i];
    } // end of for

    // the following test target file
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if( fFrTo.targefile == NULL ) {
	if( fFrTo.cTargetFileName[0] != '\0' ) {

		// add in 1995.12.13
		/*if( strchr( fFrTo.cTargetFileName, '*' ) != NULL ) {
			strrstr(fFrTo.cTargetFileName,  "*", "");
			goto  AsqlMoreRunJmp;
		}
		*/
                if( fFrTo.toAppend == '1' )
                {
		      dtp = dsp;
/*#ifdef WSToMT
		      if( fFrTo.phuf == NULL )
		      { //I don't know wether update
			wmtDbfLock( dtp );
		      }
#endif
*/
		      dseek(dtp, 0L, dSEEK_END);
                } else

		if( fFrTo.nFieldNum > 0 ) {
			// generate the dFIELD table
			num = 0;
			if( fFrTo.TargetField != NULL ) {
				while( *(fFrTo.TargetField[num].field) ) {
					num++;
				}
				fFrTo.nFieldNum += num;
			}
			if( ( fFrTo.TargetField = \
				realloc(fFrTo.TargetField, \
					(fFrTo.nFieldNum+1)*sizeof( dFIELD ) ) ) == NULL ) {
				qsError = 1001;
				bhClose( bh, DbfNum );
				dTmpClose( dsp );
				return  FALSE;
			}
			for( i = 0;    num < fFrTo.nFieldNum;    num++ ) {
				if( *(char *)(fFrTo.fieldtable[i].FieldName) == '@' ) {
				    memcpy( &(fFrTo.TargetField)[num], \
					&(dsp->field)[ (*(SPECIALFIELDSTRUCT *)(fFrTo.fieldtable[i].FieldName)).FieldNo ], \
					sizeof(dFIELD) );
				} else {
				    memcpy( &(fFrTo.TargetField)[num], \
					&(dsp->field)[ GetFldid(dsp, fFrTo.fieldtable[i++].FieldName) ], \
					sizeof(dFIELD) );
				}
			}
			((fFrTo.TargetField)[num].field)[0] = '\0';
			//num = 0;

			//the following is a program body work with DIO
			//{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}
#ifdef WSToMT
			EnterCriticalSection( &dofnCE );
#endif
			if( dIsAwake( fFrTo.cTargetFileName ) ) {
#ifdef WSToMT
				LeaveCriticalSection( &dofnCE );
#endif
				strZcpy(szAsqlErrBuf, fFrTo.cTargetFileName, sizeof(szAsqlErrBuf));
				qsError = 9001;
				bhClose( bh, DbfNum );
				dTmpClose( dsp );
				return  FALSE;
			}
			dtp = dcreate(fFrTo.cTargetFileName, \
							fFrTo.TargetField);
			if( dtp == NULL ) {
#ifdef WSToMT
			    LeaveCriticalSection( &dofnCE );
#endif
			    qsError = 5002;
			    bhClose( bh, DbfNum );
			    dTmpClose( dsp );
			    return  FALSE;
			}
			if( dSetAwake(dtp, &dtp) != 1 ) {
#ifdef WSToMT
			    LeaveCriticalSection( &dofnCE );
#endif
			    dclose(dtp);
			    bhClose( bh, DbfNum );
			    dTmpClose( dsp );
			    qsError = 5009;
			    return  FALSE;
			}
#ifdef WSToMT
			LeaveCriticalSection( &dofnCE );
			wmtDbfLock( dtp );
#endif
			//the following is a program body work with DIO
			//{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}
		} else {
			if( fFrTo.TargetField != NULL ) {

			//the following is a program body work with DIO
			//{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}
#ifdef WSToMT
				EnterCriticalSection( &dofnCE );
#endif
				if( dIsAwake( fFrTo.cTargetFileName ) ) {
#ifdef WSToMT
					LeaveCriticalSection( &dofnCE );
#endif
					strZcpy(szAsqlErrBuf, fFrTo.cTargetFileName, sizeof(szAsqlErrBuf));
					qsError = 9001;
					bhClose( bh, DbfNum );
					dTmpClose( dsp );
					return  FALSE;
				}
				dtp = dcreate(fFrTo.cTargetFileName, \
							fFrTo.TargetField);
				if( dtp == NULL ) {
#ifdef WSToMT
				    LeaveCriticalSection( &dofnCE );
#endif
				    qsError = 5002;
				    bhClose( bh, DbfNum );
				    dTmpClose( dsp );
				    return  FALSE;
				}
				if( dSetAwake(dtp, &dtp) != 1 ) {
#ifdef WSToMT
				    LeaveCriticalSection( &dofnCE );
#endif
				    dclose(dtp);
				    bhClose( bh, DbfNum );
				    dTmpClose( dsp );
				    qsError = 5009;
				    return  FALSE;
				}
#ifdef WSToMT
				LeaveCriticalSection( &dofnCE );
				wmtDbfLock( dtp );
#endif
			//{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}
			} else {
//AsqlMoreRunJmp:
				if( (dtp = dAwake(fFrTo.cTargetFileName, \
						DOPENPARA)) == NULL ) {
					//qsError = 5001;
					fFrTo.TargetField = dfcopy(dsp, NULL);
					dtp = dcreate(fFrTo.cTargetFileName, \
							fFrTo.TargetField);
		                        if( dtp == NULL ) {
					    qsError = 5002;
					    bhClose( bh, DbfNum );
					    dTmpClose( dsp );
					    return  FALSE;
					}
					if( dSetAwake(dtp, &dtp) != 1 ) {
					    dclose(dtp);
					    bhClose( bh, DbfNum );
					    dTmpClose( dsp );
					    qsError = 5009;
					    return  FALSE;
					}
				} else {
#ifdef WSToMT
					/*if( fFrTo.phuf == NULL )
					{ //I don't know wether update
					  wmtDbfLock( dtp );
					}*/
#endif
					dseek(dtp, 0L, dSEEK_END);
				}
			} // end of else
		}
		if( dtp == NULL ) {
			qsError = 5002;
			bhClose( bh, DbfNum );
			dTmpClose( dsp );
			return  FALSE;
		} else {
			fFrTo.targefile = dtp;
		}
	} else {
		dtp = NULL;
	}
    } else {
	dtp = fFrTo.targefile;
    }

    //1997.3.22
    //add this statement to avoid the initial calculate type error
    NewRec(dsp);

    // iden_switch experssion
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if( MidIden( dsp, dtp ) == FALSE ) {
	bhClose( bh, DbfNum );
	dTmpClose( dsp );
	return  FALSE;
    }

    // execute the start action
    // seatch OrdExeTable if nLevel is 'B' then action && delete
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // here is a bug.  1994.3.24.
    // if( (expActTab = fFrTo.exHead) != NULL ) {
    expActTab = fFrTo.exHead;
    num = ONE_MODALITY_ACTION;
    while( expActTab != NULL ) {
	if( expActTab->nLevel == 'B' ) {
		xMid = ActionSymbolRegister(expActTab->xPoint,dsp,dtp, \
				  fFrTo.sHead, fFrTo.nSybNum, \
				  fFrTo.dHead, fFrTo.nDefinNum);
		if( xMid != NULL ) {
			strcpy(szAsqlErrBuf, xMid->values);
			bhClose( bh, DbfNum );
			dTmpClose( dsp );
			qsError = 3026;
			return  FALSE;
		}
		if( ActionCalExpr(expActTab->xPoint, &num, dtp) == LONG_MAX ){
			bhClose( bh, DbfNum );
			dTmpClose( dsp );
			qsError = 4002;
			return  FALSE;
		}
	    } else if( expActTab->nLevel == 'E' ) {
		if( execGramMan(expActTab, dsp, dtp, 'b') == FALSE ) {
#ifdef RUNING_ERR_MESSAGE_ON
			AsqlAnaErrorMessage( "无源" );
#endif
			wWordFree();
			qsFree();
			fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
			return  FALSE;
		}
	}
	expActTab = expActTab->oNext;

    } // end of while

    // give the condition action the first work chance
    //    GroupNotExec(dsp, dtp, fFrTo);
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    num = 0;
    /*if( m_c_AsqlStatTailAction != NULL ) {
	if( ActionCalExpr(m_c_AsqlStatTailAction, &(num), dtp) == LONG_MAX ) {
		bhClose( bh, DbfNum );
		qsError = 5005;
		return  FALSE;
	} // end of if
    }*/
    paAcTbIt = fFrTo.cHead;
    while( paAcTbIt != NULL ) {
      if( paAcTbIt->actionFunRefered ) {
	if( ActionCalExpr(paAcTbIt->pxActMidCode, &(num), dtp) == LONG_MAX ) {
		qsError = 5006;
		bhClose( bh, DbfNum );
		dTmpClose( dsp );
		return  FALSE;
	} // end of if
	//paAcTbIt->nCalTimes = 1;
      }
      paAcTbIt = paAcTbIt->aNext;
    }

    //
    //1998.8.4
    //
    if( fFrTo.szScopeKey[0] != '\0' )
    { //scope_more_dbf.______________________________________________________
/////////////////////////////////////////////////////////////////////////////
    bHEAD     *bhg;
    //long        oldrec_p;
    char       ndxName[260];
    int        k;
    char      *sz, *sz1;
    short      iScopeSkipAdjust;

    df = sdf[0];

    dField = &(df->field[df->fld_id[pos[0]]]);

    if( isdigit(fFrTo.szScopeKey[0]) )
    { ///////////////////////////////////////////////////////////////////////
      //
      // FROM (SCOPE: start_rec, end_rec)source_table1, source_table2...
      //
	dseek(df, atoi(fFrTo.szScopeStart)-1, dSEEK_SET);

	lRecNum = min(getRecNum(df), atoi(fFrTo.szScopeEnd)-atoi(fFrTo.szScopeStart)+1);

	while( lRecNum-- ) {

#ifdef _AsqlRuningMessage_Assigned
	   // call users function to deal with the runing message
	   if( AsqlRuningMessage(lRecNum) ) {
		IndexSleep(bhg);
		qsError = 7002;
		return  FALSE;
	   } // end of if

#endif

	   // get a row of table
	   if( get1rec( df ) == NULL ) {
	       qsError = 5006;
	       return  FALSE;
	   }

	   if( df->rec_buf[0] == '*' ) {
		continue;
	   }

	   // execute this record
	   //oldrec_p = fFrTo.cSouFName[0]->rec_p;
	   /*if( ExecRec( df, dtp ) == FALSE ) {
		return  FALSE;
	   }<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>*/

	   if( xKeyExpr[0] != NULL )
	   {
		char *stz;
		stz = (char *)CalExpr(xKeyExpr[0]);
		if( stz != NULL )
		    strZcpy(buf, stz, 255);
		else
		    buf[0] = '\0';
	   } else {
		i = dField->fieldlen;
		memcpy(buf, dField->fieldstart, i);
		//if it is a blank field, make it same as BTREE_BLANKKEY
		for( i--;  i >= 0 && buf[i] == ' ';  i--);
		buf[ i+1 ] = '\0';
	   }

	   for( i = 1;   i < DbfNum;    i++ ) {
		//support this 1998.1.19
		if( xKeyExpr[i] != NULL )
		{
		    char *stz;

		    stz = (char *)CalExpr(xKeyExpr[i]);
		    if( stz == NULL ) {
			xLocateStr[i][0] = '\xFF';
			RecNo[i] = IndexGoTop(bh[i]);
		    } else {
			strZcpy(xLocateStr[i], stz, 255);
			if( fFrTo.relationOff )
			    RecNo[i] = -1;
			else
			    RecNo[i] = IndexSeek(bh[i], xLocateStr[i]);
		    }
		} else {
		    if( pos[i] == 0xFFFF ) {
			RecNo[i] = -1;
		    } else {
			if( fFrTo.relationOff )
			    RecNo[i] = -1;
			else
			    RecNo[i] = IndexSeek(bh[i], buf);
		    }
		}
		if( RecNo[i] > 0 ) {
		    if( get1rec( sdf[i] ) == NULL ) {
			qsError = 7002;
			bhClose( bh, DbfNum );
			dTmpClose( dsp );
			return  FALSE;
		    }


		    if( sdf[i]->rec_buf[0] == '*' ) {
			while( 1 ) {

			    //take the next record
			    if( xKeyExpr[i] != NULL ) {
				if( xLocateStr[i][0] != '\xFF' )
				    RecNo[i] = IndexStrEqSkip(bh[i], xLocateStr[i], 1);
				else
				    RecNo[i] = IndexSkip(bh[i], 1);
			    } else {
				RecNo[i] = IndexStrEqSkip(bh[i], buf, 1);
			    }

			    if( RecNo[i] > 0 ) {
				if( get1rec( sdf[i] ) == NULL ) {
				    qsError = 7002;
				    bhClose( bh, DbfNum );
				    dTmpClose( dsp );
				    return  FALSE;
				}

				if( sdf[i]->rec_buf[0] != '*' )
					break;
			    } else {
				NewRec( sdf[i] );
				break;
			    }
			} //end while(1)
		    }

		} else {
		    NewRec( sdf[i] );       // no record relation to df1
		}
	   } //end of for(i)

	   do {
		// execute this record
		//oldrec_p = sdf[0]->rec_p;
		if( ExecRec( dsp, dtp ) == FALSE ) {
		      bhClose( bh, DbfNum );
		      dTmpClose( dsp );
		      return  FALSE;
		}

		if( myAsqlStatMaxY > 0 )
		{
			memset(myAsqlStatFlagY, 0, myAsqlStatMaxY);
		}

		#ifdef _AsqlRuningMessage_Assigned
		// call users function to deal with the runing message
		if( AsqlRuningMessage(lRecNum) ) {
			qsError = 7002;
			bhClose( bh, DbfNum );

			dTmpClose( dsp );
			return  FALSE;
		} // end of if
		#endif

		/*if( m_c_AsqlStatTailAction != NULL ) {
			if( ActionCalExpr(m_c_AsqlStatTailAction, \
						    &(num), dtp ) == LONG_MAX ) {
				qsError = 5005;
				bhClose( bh, DbfNum );

				dTmpClose( dsp );
				return  FALSE;
			} // end of if
		}*/

		if( *nextMainRec == '\1' ) {

		    *nextMainRec = '\0';
		    if( fFrTo.mtSkipThisSet[0] == '\1' ) {
			memset(fFrTo.mtSkipThisSet, 0, fFrTo.cSouDbfNum);
			break;
		    }

		    for( i = 1;   i < fFrTo.cSouDbfNum;   i++ ) {
			if( fFrTo.mtSkipThisSet[i] == '\1' ) {
			    fFrTo.mtSkipThisSet[i] = '\0';
			    RecNo[i] = -1;
			}
		    }
		}

		GoOn = 0;
		for( i = 1;   i < DbfNum/* && GoOn == 0*/;   i++ ) {
		    if( RecNo[i] < 0 )      continue;
		    if( xKeyExpr[i] != NULL ) {
			if( xLocateStr[i][0] == '\xFF' ) {
			    if( (RecNo[i] = IndexSkip( bh[i], 1 )) > 0 ) {
				if( get1rec( sdf[i] ) == NULL ) {
				    qsError = 7002;
				    bhClose( bh, DbfNum );

				    dTmpClose( dsp );
				    return  FALSE;
				}
			    }
			} else
			if( (RecNo[i] = IndexStrEqSkip( bh[i], xLocateStr[i], 1 )) > 0 ) {
			    if( get1rec( sdf[i] ) == NULL ) {
				qsError = 7002;
				bhClose( bh, DbfNum );

				dTmpClose( dsp );
				return  FALSE;
			    }
			}
		    } else {
			if( (RecNo[i] = IndexStrEqSkip( bh[i], buf, 1 )) > 0 ) {
			    if( get1rec( sdf[i] ) == NULL ) {
				qsError = 7002;
				bhClose( bh, DbfNum );

				dTmpClose( dsp );
				return  FALSE;
			    }
			}
		    }

		    if( RecNo[i] >= 0 ) {
		      if( sdf[i]->rec_buf[0] == '*' ) {
			while( 1 ) {

			    //take the next record
			    if( xKeyExpr[i] != NULL ) {
				RecNo[i] = IndexStrEqSkip(bh[i], xLocateStr[i], 1);
			    } else {
				RecNo[i] = IndexStrEqSkip(bh[i], buf, 1);
			    }

			    if( RecNo[i] > 0 ) {
				if( get1rec( sdf[i] ) == NULL ) {
				    qsError = 7002;
				    bhClose( bh, DbfNum );

				    dTmpClose( dsp );
				    return  FALSE;
				}

				if( sdf[i]->rec_buf[0] != '*' ) {
					GoOn = 1;
					break;
				}
			    } else {
				break;
			    }
			} //end while(1)
		      } else {
			GoOn = 1;
		      }
		    }

		} // end of for

		if( GoOn != 0 ) {
		    for( i = 1;   i < DbfNum;   i++ ) {
			if( RecNo[i] < 0 )
			    NewRec( sdf[i] );
		    }
		}

	   } while( GoOn != 0 );

	   if( df->rec_p < 0 )
	   { //recalue the current record, for it is already NEW
	       df->rec_p = 0-df->rec_p;
	   }

	   if( myAsqlStatMaxY > 0 )
	   {
		memset(myAsqlStatFlagY, 0, myAsqlStatMaxY);
	   }

	   // calculate
	   dseek(df, 1, dSEEK_CUR);

	} // end of while

	goto NOT_RECNO_SCOPE;
	//goto to process the last chance of action
	//<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>

    } //
      //
      // FROM (SCOPE: start_rec, end_rec)source_table1, source_table2...
      //
      ///////////////////////////////////////////////////////////////////////

    iScopeSkipAdjust = fFrTo.iScopeSkipAdjust;

    //
    //scope and more dbf.
    //
    for(k = 0;   k < df->syncBhNum;   k++)
    {
	if( keyFieldCmp((bHEAD *)(df->bhs[k]), fFrTo.szScopeKey) != 0 ) {
	    bhg = IndexAwake((char *)df, ((bHEAD *)(df->bhs[k]))->ndxName,
							  BTREE_FOR_OPENDBF);
	    goto MBH_OK1;
	}
    } //no else here

    //
    //generate the index name:
    //TableName^ScopeKey1^ScopeKey2...^intOfThread
    strcpy(ndxName, df->name);
    if( ( sz = strrchr(ndxName, '.') ) != NULL ) {
	*sz = '\0';
    }
    strcat(ndxName, "^");
    strcpy(buf, fFrTo.szScopeKey);
    for(i = 0;  buf[i] != '\0';  i++) {
	if( buf[i] == '+' )
		buf[i] = '^';
    }

    strcat(ndxName, buf);
    sprintf(buf, "^%03X", intOfThread&0xFFF);
    strcat(ndxName, buf);

    makeTrimFilename(ndxName, tmpPath, ndxName);

    //index name is OK in ndxName

    bhg = IndexAwake((char *)df, ndxName, BTREE_FOR_OPENDBF);
    if( keyFieldCmp(bhg, fFrTo.szScopeKey) == 0 ) {
	if( bhg != NULL )
	    IndexSleep(bhg);

	bhg = IndexBAwake((char *)df, fFrTo.szScopeKey, ndxName, BTREE_FOR_OPENDBF);
    }

MBH_OK1:
    if( bhg == NULL ) {
	qsError = 2005;
	return  FALSE;
	// error use index
    }

    fFrTo.syncBh[0] = bhg;

    IndexLocate(bhg, fFrTo.szScopeStart);

    lRecNum = getRecNum(df);
    sz = fFrTo.szScopeEnd;
    sz1 = _BtreeKeyBuf;

    /* the upper statements shouldn't be changed to the following
       for IndexSeek() just to locate an area of records
       1998.8.20
    if( IndexSeek(bhg, fFrTo.szScopeStart) == LONG_MIN ) {
	lRecNum = 0;
    } else {
	lRecNum = getRecNum(df);
	sz = fFrTo.szScopeEnd;
	sz1 = _BtreeKeyBuf;
    }*/

    while( lRecNum-- ) {

#ifdef _AsqlRuningMessage_Assigned
	// call users function to deal with the runing message
	if( AsqlRuningMessage(lRecNum) ) {
		IndexSleep(bhg);
		qsError = 7002;
		return  FALSE;
	} // end of if

#endif

	// get a row of table
	if( get1rec( df ) == NULL ) {
	    qsError = 7002;
	    IndexSleep(bhg);
	    return  FALSE;
	}

	if( df->rec_buf[0] == '*' ) {
	    if( IndexSkip(bhg, 1+iScopeSkipAdjust) == LONG_MIN )
	    { //now we meet the bottom of the tree
		break;
	    }
	    continue;
	}

	if( iScopeSkipAdjust == -2 )
	{ //descending
	    if( bStrCmp(sz, sz1, bhg->keyLen) > 0 ) {
		break;
	    }
	} else
	{ //ascending
	    if( bStrCmp(sz, sz1, bhg->keyLen) < 0 ) {
		break;
	    }
	}

	/*if( bStrCmp(fFrTo.szScopeEnd, IndexGetKeyContent(bhg), bhg->keyLen) < 0 ) {
	    break;
	}*/

	// execute this record
	//oldrec_p = fFrTo.cSouFName[0]->rec_p;
	/*if( ExecRec( dsp, dtp ) == FALSE ) {
	    IndexSleep(bh);
	    return  FALSE;
	}*/

	if( xKeyExpr[0] != NULL )
	{
	    char *stz;
	    stz = (char *)CalExpr(xKeyExpr[0]);
	    if( stz != NULL ) {
		strZcpy(buf, stz, 255);
	    } else {
		buf[0] = '\0';
	    }
	} else {
	    i = dField->fieldlen;
	    memcpy(buf, dField->fieldstart, i);
	    //if it is a blank field, make it same as BTREE_BLANKKEY
	    for( i--;  i >= 0 && buf[i] == ' ';  i--);
	    buf[ i+1 ] = '\0';
	}

	for( i = 1;   i < DbfNum;    i++ ) {
		//support this 1998.1.19
		if( xKeyExpr[i] != NULL )
		{
		    char *stz;

		    stz = (char *)CalExpr(xKeyExpr[i]);
		    if( stz == NULL ) {
			xLocateStr[i][0] = '\xFF';
			RecNo[i] = IndexGoTop(bh[i]);
		    } else {
			strZcpy(xLocateStr[i], stz, 255);
			if( fFrTo.relationOff )
			    RecNo[i] = -1;
			else
			    RecNo[i] = IndexSeek(bh[i], xLocateStr[i]);
		    }
		} else {
		    if( pos[i] == 0xFFFF ) {
			RecNo[i] = -1;
		    } else {
			if( fFrTo.relationOff )
			    RecNo[i] = -1;
			else
			    RecNo[i] = IndexSeek(bh[i], buf);
		    }
		}
		if( RecNo[i] > 0 ) {
		    if( get1rec( sdf[i] ) == NULL ) {
			qsError = 7002;
			bhClose( bh, DbfNum );
			dTmpClose( dsp );
			return  FALSE;
		    }


		    if( sdf[i]->rec_buf[0] == '*' ) {
			while( 1 ) {

			    //take the next record
			    if( xKeyExpr[i] != NULL ) {
				if( xLocateStr[i][0] != '\xFF' )
				    RecNo[i] = IndexStrEqSkip(bh[i], xLocateStr[i], 1);
				else
				    RecNo[i] = IndexSkip(bh[i], 1);
			    } else {
				RecNo[i] = IndexStrEqSkip(bh[i], buf, 1);
			    }

			    if( RecNo[i] > 0 ) {
				if( get1rec( sdf[i] ) == NULL ) {
				    qsError = 7002;
				    bhClose( bh, DbfNum );
				    dTmpClose( dsp );
				    return  FALSE;
				}

				if( sdf[i]->rec_buf[0] != '*' )
					break;
			    } else {
				NewRec( sdf[i] );
				break;
			    }
			} //end while(1)
		    }

		} else {
		    NewRec( sdf[i] );       // no record relation to df1
		}
	} //end of for(i)

	do {
		// execute this record
		//oldrec_p = sdf[0]->rec_p;
		if( ExecRec( dsp, dtp ) == FALSE ) {
		      bhClose( bh, DbfNum );
		      dTmpClose( dsp );
		      return  FALSE;
		}

		if( myAsqlStatMaxY > 0 )
		{
			memset(myAsqlStatFlagY, 0, myAsqlStatMaxY);
		}

		#ifdef _AsqlRuningMessage_Assigned
		// call users function to deal with the runing message
		if( AsqlRuningMessage(lRecNum) ) {
			qsError = 7002;
			bhClose( bh, DbfNum );

			dTmpClose( dsp );
			return  FALSE;
		} // end of if
		#endif

		/*if( m_c_AsqlStatTailAction != NULL ) {
			if( ActionCalExpr(m_c_AsqlStatTailAction, \
						    &(num), dtp ) == LONG_MAX ) {
				qsError = 5005;
				bhClose( bh, DbfNum );

				dTmpClose( dsp );
				return  FALSE;
			} // end of if
		}*/

		if( *nextMainRec == '\1' ) {

		    *nextMainRec = '\0';
		    if( fFrTo.mtSkipThisSet[0] == '\1' ) {
			memset(fFrTo.mtSkipThisSet, 0, fFrTo.cSouDbfNum);
			break;
		    }

		    for( i = 1;   i < fFrTo.cSouDbfNum;   i++ ) {
			if( fFrTo.mtSkipThisSet[i] == '\1' ) {
			    fFrTo.mtSkipThisSet[i] = '\0';
			    RecNo[i] = -1;
			}
		    }
		}

		GoOn = 0;
		for( i = 1;   i < DbfNum/* && GoOn == 0*/;   i++ ) {
		    if( RecNo[i] < 0 )      continue;
		    if( xKeyExpr[i] != NULL ) {
			if( xLocateStr[i][0] == '\xFF' ) {
			    if( (RecNo[i] = IndexSkip( bh[i], 1 )) > 0 ) {
				if( get1rec( sdf[i] ) == NULL ) {
				    qsError = 7002;
				    bhClose( bh, DbfNum );

				    dTmpClose( dsp );
				    return  FALSE;
				}
			    }
			} else
			if( (RecNo[i] = IndexStrEqSkip( bh[i], xLocateStr[i], 1 )) > 0 ) {
			    if( get1rec( sdf[i] ) == NULL ) {
				qsError = 7002;
				bhClose( bh, DbfNum );

				dTmpClose( dsp );
				return  FALSE;
			    }
			}
		    } else {
			if( (RecNo[i] = IndexStrEqSkip( bh[i], buf, 1 )) > 0 ) {
			    if( get1rec( sdf[i] ) == NULL ) {
				qsError = 7002;
				bhClose( bh, DbfNum );

				dTmpClose( dsp );
				return  FALSE;
			    }
			}
		    }

		    if( RecNo[i] >= 0 ) {
		      if( sdf[i]->rec_buf[0] == '*' ) {
			while( 1 ) {

			    //take the next record
			    if( xKeyExpr[i] != NULL ) {
				RecNo[i] = IndexStrEqSkip(bh[i], xLocateStr[i], 1);
			    } else {
				RecNo[i] = IndexStrEqSkip(bh[i], buf, 1);
			    }

			    if( RecNo[i] > 0 ) {
				if( get1rec( sdf[i] ) == NULL ) {
				    qsError = 7002;
				    bhClose( bh, DbfNum );

				    dTmpClose( dsp );
				    return  FALSE;
				}

				if( sdf[i]->rec_buf[0] != '*' ) {
					GoOn = 1;
					break;
				}
			    } else {
				break;
			    }
			} //end while(1)
		      } else {
			GoOn = 1;
		      }
		    }

		} // end of for

		if( GoOn != 0 ) {
		    for( i = 1;   i < DbfNum;   i++ ) {
			if( RecNo[i] < 0 )
			    NewRec( sdf[i] );
		    }
		}

	} while( GoOn != 0 );

	if( df->rec_p < 0 )
	{ //recalue the current record, for it is already NEW
	       df->rec_p = 0-df->rec_p;
	       IndexSkip(bhg,-1);
	}


	if( myAsqlStatMaxY > 0 )
	{
		memset(myAsqlStatFlagY, 0, myAsqlStatMaxY);
	}

	/*if( m_c_AsqlStatTailAction != NULL ) {
		if( ActionCalExpr(m_c_AsqlStatTailAction, \
						    &(i), dtp ) == LONG_MAX ) {
			IndexSleep(bh);
			qsError = 5005;
			return  FALSE;
		} // end of if
	}*/
	// calculate
	if( IndexSkip(bhg, 1+iScopeSkipAdjust) == LONG_MIN )
	{ //now we meet the bottom of the tree
	    break;
	}

    } // end of while

    IndexSleep(bhg);

    goto  NOT_RECNO_SCOPE;

    } else

    switch( fFrTo.nGroupby ) {

	case Asql_GROUPYES:
	{
	     bHEAD  *gbh;
#ifdef _DEF_groupWassymbol
	     dFIELD *keyFld;
#endif
	     char   keyBuf[256];
	     char   szGbNdx[256];
	     signed char cmpRsut;

	     dField = &(sdf[0]->field[sdf[0]->fld_id[pos[0]]]);
	     // create the target for the source querry

	     sprintf(szGbNdx, "GBNDX%03X", intOfThread&0xFFF);
	     makefilename(szGbNdx, tmpPath, szGbNdx);

	     //build the index, no before index for reference
	     if( fFrTo.wGbKeyLen == SHRT_MAX ) {
		FieldName[0] = fFrTo.cGroKey;
		FieldName[1] = NULL;

		gbh = IndexBuild((char *)sdf[0], FieldName, szGbNdx, BTREE_FOR_OPENDBF);
	     } else {
		gbh = IndexBuildKeyExpr(sdf[0], fFrTo.cGroKey, \
						fFrTo.wGbKeyLen, \
						szGbNdx);
	     }

	     if( gbh == NULL ) {
		qsError = 5008;
		strZcpy(szAsqlErrBuf, fFrTo.cGroKey, sizeof(szAsqlErrBuf));
		bhClose(bh, DbfNum);
		dTmpClose(dsp);
		return  FALSE;
		// error use index
	     }

#ifdef _DEF_groupWassymbol
	     { //cut into one field
		char  *sp;
		short fldId;

		strcpy(keyBuf, fFrTo.cGroKey);
		if( (sp = strchr(keyBuf, '+')) != NULL )
			*sp = '\0';
		fldId = GetFldid(dsp, keyBuf);
		if( fldId == 0xFFFF ) {
			qsError = 5008;
			bhClose(bh, DbfNum);
			dTmpClose(dsp);
			return  FALSE;
		}
		keyFld = getFieldInfo(dsp, fldId);
	     }
#endif
	     if( iScopeSkipAdjust == -2 ) {
		IndexGoBottom(gbh);
	     } else {
		IndexGoTop(gbh);
	     }

	     if( fFrTo.nGroupAction )
	     { //register the groupaction
		MidCodeType  *xMid;

		expActTab = fFrTo.exHead;
		while( expActTab != NULL ) {
		    if( expActTab->nLevel == 'G' ) {
			xMid = ActionSymbolRegister(expActTab->xPoint,dsp,dtp, \
				  fFrTo.sHead, fFrTo.nSybNum, \
				  fFrTo.dHead, fFrTo.nDefinNum);
			if( xMid != NULL ) {
				strcpy(szAsqlErrBuf, xMid->values);
				qsError = 3026;
				bhClose(bh, DbfNum);
				dTmpClose(dsp);
				return  FALSE;
			}
		    }
		    expActTab = expActTab->oNext;
		}
	     } //end of register

	     strcpy(keyBuf, IndexGetKeyContent(gbh));
	     gcount = 0;

#ifdef _DEF_groupWassymbol
	     groupWassymbol(dtp, keyFld, keyBuf, 0);
#endif
	     // Begin to run for every record
	     //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	     //num = 1;
	     lRecNum = getRecNum(sdf[0]);
	     while( lRecNum-- ) {

	     #ifdef _AsqlRuningMessage_Assigned
	     // call users function to deal with the runing message
	     if( AsqlRuningMessage(lRecNum) ) {
		qsError = 7002;
		bhClose(bh, DbfNum);
		dTmpClose(dsp);
		IndexClose(gbh);
		unlink(szGbNdx);
		return  FALSE;
	     } // end of if
	     #endif

	     // get a row of table
	     if( get1rec(sdf[0]) == NULL )
	     {
		if( dsp->dbf_flag == SQL_TABLE ) {
		    if( TgetSqlErrCode() == 100 )
			break;
		}
		
		if( dsp->dbf_flag == ODBC_TABLE ) {
		//if( getI_ErrCode() == 100 )
		    
		    //break;
		    //change this as the following, 2001.7.7 XIlong

		    if( dsp->error != 0 )
			strZcpy(szAsqlErrBuf, szOdbcExecResult, sizeof(szAsqlErrBuf) );
		    break;

		}

		qsError = 7002;
		bhClose(bh, DbfNum);
		dTmpClose(dsp);
		IndexClose(gbh);
		unlink(szGbNdx);
		return  FALSE;
	     }

	     if( sdf[0]->rec_buf[0] == '*' ) {
		// calculate
		if( IndexSkip(gbh,1+iScopeSkipAdjust) == LONG_MIN ) {
			break;
		}

		if( gcount < 1 ) {
		    strcpy(keyBuf, IndexGetKeyContent(gbh));
		    continue;
		} else {
		    goto MGROUP_YES_JMP;
		}
	     } else {
		gcount++;
	     }

	     //get relative key into buf
	     if( xKeyExpr[0] != NULL )
	     {
		char *stz;

		stz = (char *)CalExpr(xKeyExpr[0]);
		if( stz != NULL ) {
		    strZcpy(buf, stz, 255);
		} else {
		    buf[0] = '\0';
		}
	     } else {

		i = dField->fieldlen;
		memcpy(buf, dField->fieldstart, i);
		//if it is a blank field, make it same as BTREE_BLANKKEY
		for( i--;  i >= 0 && buf[i] == ' ';  i--);
		buf[ i+1 ] = '\0';
	     }

	     for( i = 1;   i < DbfNum;    i++ ) {
		//support this 1998.1.19
		if( xKeyExpr[i] != NULL )
		{
		    char *stz;

		    stz = (char *)CalExpr(xKeyExpr[i]);
		    if( stz == NULL ) {
			xLocateStr[i][0] = '\xFF';
			RecNo[i] = IndexGoTop(bh[i]);
		    } else {
			strZcpy(xLocateStr[i], stz, 255);
			if( fFrTo.relationOff )
			    RecNo[i] = -1;
			else
			    RecNo[i] = IndexSeek(bh[i], xLocateStr[i]);
		    }
		} else {
		    if( pos[i] == 0xFFFF ) {
			RecNo[i] = -1;
		    } else {
			if( fFrTo.relationOff )
			    RecNo[i] = -1;
			else
			    RecNo[i] = IndexSeek(bh[i], buf);
		    }
		}
		if( RecNo[i] > 0 ) {
		    if( get1rec( sdf[i] ) == NULL ) {
			qsError = 7002;
			bhClose( bh, DbfNum );
			dTmpClose( dsp );
			IndexClose(gbh);
			unlink(szGbNdx);
			return  FALSE;
		    }

		    if( sdf[i]->rec_buf[0] == '*' ) {
			while( 1 ) {

			    //take the next record
			    if( xKeyExpr[i] != NULL ) {
				if( xLocateStr[i][0] != '\xFF' ) {
				    RecNo[i] = IndexStrEqSkip(bh[i], xLocateStr[i], 1);
				} else {
				    RecNo[i] = IndexSkip(bh[i], 1);
				}
			    } else {
				RecNo[i] = IndexStrEqSkip(bh[i], buf, 1);
			    }

			    if( RecNo[i] > 0 ) {
				if( get1rec( sdf[i] ) == NULL ) {
				    qsError = 7002;
				    bhClose( bh, DbfNum );
				    dTmpClose( dsp );
				    IndexClose(gbh);
				    unlink(szGbNdx);
				    return  FALSE;
				}

				if( sdf[i]->rec_buf[0] != '*' )
					break;
			    } else {
				NewRec( sdf[i] );
				break;
			    }
			} //end while(1)
		    }

		} else {
		    NewRec( sdf[i] );       // no record relation to df1
		}
	     }

	     do {  //run for every sub dbf record
		// execute this record
		if( ExecRec( dsp, dtp ) == FALSE ) {
		      bhClose( bh, DbfNum );
		      dTmpClose( dsp );
		      IndexClose(gbh);
		      unlink(szGbNdx);
		      return  FALSE;
		}

		if( myAsqlStatMaxY > 0 )
		{
		    memset(myAsqlStatFlagY, 0, myAsqlStatMaxY);
		}

		/*if( m_c_AsqlStatTailAction != NULL ) {
		    if( ActionCalExpr(m_c_AsqlStatTailAction, \
						    &(i), dtp ) == LONG_MAX ) {
			qsError = 5005;
			bhClose(bh, DbfNum);
			dTmpClose(dsp);
			IndexClose(gbh);
			unlink(szGbNdx);
			return  FALSE;
		    } // end of if
		}*/

		if( *nextMainRec == '\1' ) {

		    *nextMainRec = '\0';
		    if( fFrTo.mtSkipThisSet[0] == '\1' ) {
			memset(fFrTo.mtSkipThisSet, 0, fFrTo.cSouDbfNum);
			break;
		    }

		    for( i = 1;   i < fFrTo.cSouDbfNum;   i++ ) {
			if( fFrTo.mtSkipThisSet[i] == '\1' ) {
			    fFrTo.mtSkipThisSet[i] = '\0';
			    RecNo[i] = -1;
			}
		    }
		}

		GoOn = 0;
		for( i = 1;   i < DbfNum/* && GoOn == 0*/;   i++ ) {
		    if( RecNo[i] < 0 )      continue;

		    if( xKeyExpr[i] != NULL ) {
			if( xLocateStr[i][0] == 'xFF' ) {
			    if( (RecNo[i] = IndexSkip(bh[i], 1)) >= 0 ) {
				if( get1rec( sdf[i] ) == NULL ) {
				    qsError = 7002;
				    bhClose(bh, DbfNum);
				    dTmpClose(dsp);
				    IndexClose(gbh);
				    unlink(szGbNdx);
				    return  FALSE;
				}
			    }
			} else
			if( (RecNo[i] = IndexStrEqSkip(bh[i], xLocateStr[i], 1)) >= 0 ) {
			    if( get1rec( sdf[i] ) == NULL ) {
				qsError = 7002;
				bhClose(bh, DbfNum);
				dTmpClose(dsp);
				IndexClose(gbh);
				unlink(szGbNdx);
				return  FALSE;
			    }
			}
		    } else {
			if( (RecNo[i] = IndexStrEqSkip(bh[i], buf, 1)) >= 0 ) {
			    if( get1rec( sdf[i] ) == NULL ) {
				qsError = 7002;
				bhClose(bh, DbfNum);
				dTmpClose(dsp);
				IndexClose(gbh);
				unlink(szGbNdx);
				return  FALSE;
			    }
			}
		    }

		    if( RecNo[i] >= 0 ) {
		      if( sdf[i]->rec_buf[0] == '*' ) {
			while( 1 ) {

			    //take the next record
			    if( xKeyExpr[i] != NULL ) {
				RecNo[i] = IndexStrEqSkip(bh[i], xLocateStr[i], 1);
			    } else {
				RecNo[i] = IndexStrEqSkip(bh[i], buf, 1);
			    }

			    if( RecNo[i] > 0 ) {
				if( get1rec( sdf[i] ) == NULL ) {
				    qsError = 7002;
				    bhClose(bh, DbfNum);
				    dTmpClose(dsp);
				    IndexClose(gbh);
				    unlink(szGbNdx);
				    return  FALSE;
				}

				if( sdf[i]->rec_buf[0] != '*' ) {
					GoOn = 1;
					break;
				}
			    } else {
				break;
			    }
			} //end while(1)
		      } else {
			GoOn = 1;
		      }
		    }
		} // end of for

		if( GoOn != 0 ) {
		    for( i = 1;   i < DbfNum;   i++ ) {
			if( RecNo[i] < 0 )
			    NewRec( sdf[i] );
		    }
		}

	     } while ( GoOn != 0 );


	     // calculate
	     if( IndexSkip(gbh,1+iScopeSkipAdjust) == LONG_MIN ) {
		 break;
	     }

MGROUP_YES_JMP:
	     // if get the end of bh, this expression is right
	     if( fFrTo.cGyExact == 1 ) {
		 cmpRsut = bExactStrCmp(keyBuf, IndexGetKeyContent(gbh), gbh->keyLen);
	     } else {
		 cmpRsut = bFuzzyStrCmp(keyBuf, IndexGetKeyContent(gbh), gbh->keyLen);
	     }

	     if( cmpRsut != 0 )
	     {

		//let the action work with the last same record
		IndexSkip(gbh, -1-iScopeSkipAdjust);

		if( fFrTo.nGroupAction )
		{
		  //execute groupaction
		  expActTab = fFrTo.exHead;
		  while( expActTab != NULL ) {
		    if( expActTab->nLevel == 'G' ) {

			//if( ActionCalExpr(expActTab->xPoint, &one, dtp) == LONG_MAX ){
			if( ActionCalExpr(expActTab->xPoint, &wGroupbyAction, dtp) == LONG_MAX ){

				qsError = 5005;
				bhClose(bh, DbfNum);
				dTmpClose(dsp);
				IndexClose(gbh);
				unlink(szGbNdx);
				return  FALSE;
			}
		    } // end of if
		    expActTab = expActTab->oNext;
		  } // end of while
		}

#ifdef _DEF_groupWassymbol
		// write the one dimension to file
		groupWassymbol(dtp, keyFld, keyBuf, 1);
#endif

		IndexSkip(gbh,1+iScopeSkipAdjust);
		strcpy(keyBuf, IndexGetKeyContent(gbh));
		gcount = 0;
	     }

	     } // end of while

	     //run for the last record
	     if( fFrTo.nGroupAction && gcount > 0 ) {
		 expActTab = fFrTo.exHead;
		 while( expActTab != NULL ) {
		    if( expActTab->nLevel == 'G' ) {

			//if( ActionCalExpr(expActTab->xPoint, &one, dtp) == LONG_MAX ){
			if( ActionCalExpr(expActTab->xPoint, &wGroupbyAction, dtp) == LONG_MAX ){

				qsError = 5005;
				bhClose(bh, DbfNum);
				dTmpClose(dsp);
				IndexClose(gbh);
				unlink(szGbNdx);
				return  FALSE;
			}
		    } // end of if
		    expActTab = expActTab->oNext;
		 } // end of while
	     }

#ifdef _DEF_groupWassymbol
	     groupWassymbol(dtp, keyFld, keyBuf, 1);

	     groupWassymbol(dtp, keyFld, keyBuf, 2);
#endif
	     IndexClose(gbh);
	     unlink(szGbNdx);

	}
	break;

	case Asql_GROUPNOT:
	{
	  // Begin to run for every record
	  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	  if( fFrTo.eHead == NULL )
	  { //CONDITION
	    //BEGIN	<--- empty here
	    //END
		break;
	  }

	  //num = 1;
	  df = sdf[0];
	  lRecNum = getRecNum(df);
	  // get a row of table and run
	  dseek( df, 0L, dSEEK_SET );
	  lp = 0;
	  dField = &(df->field[df->fld_id[pos[0]]]);

	  while( lRecNum-- )
	  {
	     {
		dseek(df, lp++, dSEEK_SET);
		if( get1rec(df) == NULL )
		{
			if( df->dbf_flag == SQL_TABLE ) {
			    if( TgetSqlErrCode() == 100 )
				break;
			}
			if( df->dbf_flag == ODBC_TABLE ) {
			    //if( getI_ErrCode() == 100 )
			    
			    //2001.7.7 Xilong
			    if( df->error != 0 )
				strZcpy(szAsqlErrBuf, szOdbcExecResult, sizeof(szAsqlErrBuf) );

			    break;
			}

			qsError = 7002;
			bhClose( bh, DbfNum );

			dTmpClose( dsp );

			return  FALSE;
		}

		if( df->rec_buf[0] == '*' ) continue;

		if( xKeyExpr[0] != NULL )
		{
		   char *stz;
		   stz = (char *)CalExpr(xKeyExpr[0]);
		   if( stz != NULL )
			strZcpy(buf, stz, 255);
		   else
			buf[0] = '\0';
		} else {
		   i = dField->fieldlen;
		   memcpy(buf, dField->fieldstart, i);
		   //if it is a blank field, make it same as BTREE_BLANKKEY
		   for( i--;  i >= 0 && buf[i] == ' ';  i--);
		   buf[i+1] = '\0';
		}
	     }

	     for( i = 1;   i < DbfNum;    i++ ) {
		//support this 1998.1.19
		if( xKeyExpr[i] != NULL )
		{
		    char *stz;

		    stz = (char *)CalExpr(xKeyExpr[i]);

		    /*
		    if( GetCurrentResultType() != STRING_TYPE ) {
		    }
		    */

		    if( stz == NULL ) {
			xLocateStr[i][0] = '\xFF';
			RecNo[i] = IndexGoTop(bh[i]);
		    } else {
			strZcpy(xLocateStr[i], stz, 255);
			if( fFrTo.relationOff )
			    RecNo[i] = -1;
			else
			    RecNo[i] = IndexSeek(bh[i], xLocateStr[i]);
		    }
		} else {
		    if( pos[i] == 0xFFFF ) {
			RecNo[i] = -1;
		    } else {
			if( fFrTo.relationOff )
			    RecNo[i] = -1;
			else
			    RecNo[i] = IndexSeek(bh[i], buf);
		    }
		}
		if( RecNo[i] > 0 ) {
		    if( get1rec( sdf[i] ) == NULL ) {
			qsError = 7002;
			bhClose( bh, DbfNum );
			dTmpClose( dsp );
			return  FALSE;
		    }


		    if( sdf[i]->rec_buf[0] == '*' ) {
			while( 1 ) {

			    //take the next record
			    if( xKeyExpr[i] != NULL ) {
				if( xLocateStr[i][0] == '\xFF' )
				    RecNo[i] = IndexSkip(bh[i], 1);
				else
				    RecNo[i] = IndexStrEqSkip(bh[i], xLocateStr[i], 1);
			    } else {
				RecNo[i] = IndexStrEqSkip(bh[i], buf, 1);
			    }

			    if( RecNo[i] > 0 ) {
				if( get1rec( sdf[i] ) == NULL ) {
				    qsError = 7002;
				    bhClose( bh, DbfNum );
				    dTmpClose( dsp );
				    return  FALSE;
				}

				if( sdf[i]->rec_buf[0] != '*' )
					break;
			    } else {
				NewRec( sdf[i] );
				break;
			    }
			} //end while(1)
		    }

		} else {
		    NewRec( sdf[i] );       // no record relation to df1
		}
	     } //end of for(i)

	     do {
		// execute this record
		//oldrec_p = sdf[0]->rec_p;
		if( ExecRec( dsp, dtp ) == FALSE ) {
		      bhClose( bh, DbfNum );
		      dTmpClose( dsp );
		      return  FALSE;
		}

		if( myAsqlStatMaxY > 0 )
		{
			memset(myAsqlStatFlagY, 0, myAsqlStatMaxY);
		}

		#ifdef _AsqlRuningMessage_Assigned
		// call users function to deal with the runing message
		if( AsqlRuningMessage(lRecNum) ) {
			qsError = 7002;
			bhClose( bh, DbfNum );

			dTmpClose( dsp );
			return  FALSE;
		} // end of if
		#endif

		/*if( m_c_AsqlStatTailAction != NULL ) {
			if( ActionCalExpr(m_c_AsqlStatTailAction, \
						    &(num), dtp ) == LONG_MAX ) {
				qsError = 5005;
				bhClose( bh, DbfNum );

				dTmpClose( dsp );
				return  FALSE;
			} // end of if
		}*/

		if( *nextMainRec == '\1' ) {

		    *nextMainRec = '\0';
		    if( fFrTo.mtSkipThisSet[0] == '\1' ) {
			memset(fFrTo.mtSkipThisSet, 0, fFrTo.cSouDbfNum);
			break;
		    }

		    for( i = 1;   i < fFrTo.cSouDbfNum;   i++ ) {
			if( fFrTo.mtSkipThisSet[i] == '\1' ) {
			    fFrTo.mtSkipThisSet[i] = '\0';
			    RecNo[i] = -1;
			}
		    }
		}

		GoOn = 0;
		for( i = 1;   i < DbfNum/* && GoOn == 0*/;   i++ ) {
		    if( RecNo[i] < 0 )      continue;
		    if( xKeyExpr[i] != NULL ) {
			if( xLocateStr[i][0] == '\xFF' ) {
			    if( (RecNo[i] = IndexSkip( bh[i], 1 )) > 0 ) {
				if( get1rec( sdf[i] ) == NULL ) {
				    qsError = 7002;
				    bhClose( bh, DbfNum );

				    dTmpClose( dsp );
				    return  FALSE;
				}
			    }
			} else
			if( (RecNo[i] = IndexStrEqSkip( bh[i], xLocateStr[i], 1 )) > 0 ) {
			    if( get1rec( sdf[i] ) == NULL ) {
				qsError = 7002;
				bhClose( bh, DbfNum );

				dTmpClose( dsp );
				return  FALSE;
			    }
			}
		    } else {
			if( (RecNo[i] = IndexStrEqSkip( bh[i], buf, 1 )) > 0 ) {
			    if( get1rec( sdf[i] ) == NULL ) {
				qsError = 7002;
				bhClose( bh, DbfNum );

				dTmpClose( dsp );
				return  FALSE;
			    }
			}
		    }

		    if( RecNo[i] >= 0 ) {
		      if( sdf[i]->rec_buf[0] == '*' ) {
			while( 1 ) {

			    //take the next record
			    if( xKeyExpr[i] != NULL ) {
				RecNo[i] = IndexStrEqSkip(bh[i], xLocateStr[i], 1);
			    } else {
				RecNo[i] = IndexStrEqSkip(bh[i], buf, 1);
			    }

			    if( RecNo[i] > 0 ) {
				if( get1rec( sdf[i] ) == NULL ) {
				    qsError = 7002;
				    bhClose( bh, DbfNum );

				    dTmpClose( dsp );
				    return  FALSE;
				}

				if( sdf[i]->rec_buf[0] != '*' ) {
					GoOn = 1;
					break;
				}
			    } else {
				break;
			    }
			} //end while(1)
		      } else {
			GoOn = 1;
		      }
		    }

		} // end of for

		if( GoOn != 0 ) {
		    for( i = 1;   i < DbfNum;   i++ ) {
			if( RecNo[i] < 0 )
			    NewRec( sdf[i] );
		    }
		}

	     } while( GoOn != 0 );

	     if( df->rec_p < 0 )
	     { //recalue the current record, for it is already NEW
	       df->rec_p = 0-df->rec_p;
	       lp--;
	     }
	  } //end of while every record
	}
	break;

	default:
		qsError = 5003;

    } // end of case

NOT_RECNO_SCOPE:

    // last work time of action
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    num = LASTWORKTIMEOFACTION;
    paAcTbIt = fFrTo.cHead;
    while( paAcTbIt != NULL ) {
      if( paAcTbIt->actionFunRefered ) {
	if( ActionCalExpr(paAcTbIt->pxActMidCode, &num, dtp) == LONG_MAX ) {
		qsError = 5004;
		bhClose( bh, DbfNum );

		dTmpClose( dsp );
		return  FALSE;
	}
      }
      paAcTbIt = paAcTbIt->aNext;
    } // end of while

    expActTab = fFrTo.exHead;
    num = ONE_MODALITY_ACTION;
    // here is a bug. 1994.3.24.
    // if( expActTab != NULL )
    while( expActTab != NULL ) {
	if( expActTab->nLevel == 'A' ) {
	    xMid = ActionSymbolRegister( expActTab->xPoint, dsp, dtp, \
				fFrTo.sHead, fFrTo.nSybNum, fFrTo.dHead, \
				fFrTo.nDefinNum );
	    if( xMid != NULL ) {
		strcpy(szAsqlErrBuf, xMid->values);
		qsError = 3026;
		bhClose( bh, DbfNum );

		dTmpClose( dsp );
		return  FALSE;
	    }
	    if( ActionCalExpr(expActTab->xPoint, &num, dtp) == LONG_MAX ) {
		qsError = 4002;
		bhClose( bh, DbfNum );

		dTmpClose( dsp );
		return  FALSE;
	    }
	} else if( expActTab->nLevel == 'F' ) {
		if( execGramMan(expActTab, dsp, dtp, 'a') == FALSE ) {
#ifdef RUNING_ERR_MESSAGE_ON
			AsqlAnaErrorMessage( "无源" );
#endif
			wWordFree();
			qsFree();
			fFrTo.cTitleName[0] = '\0'; 	//fFrTo is clear
			return  FALSE;
		}
	}

//      temp = expActTab;
	expActTab = expActTab->oNext;
//      FreeCode( temp->xPoint );
//      free( temp );
    } // end of while

    bhClose( bh, DbfNum );
    dTmpClose( dsp );
    // free memory

    return  TRUE;

} // end of AsqlMoreDbfExecute()



/*---------------
 *  Function Name:      bhClose()
 *  Arguments:
 **************************************************************************/
static void   bhClose( bHEAD *bh[], short num )
{
    for( num--;   num > 0;   num-- ) {
	IndexSleep( bh[num] );
    }
} // end of function bhClose()


/*---------------
 *  Function Name:      InitAsqlEnv()
 *  Arguments:
 **************************************************************************/
void InitAsqlEnv( void )
{
    asqlEnv.szAsqlResultPath[0] = '\0';
    asqlEnv.szAsqlFromPath[0] = '\0';

} // end of function InitAsqlEnv()


/*---------------
 *  Function Name:      GroupExec()
 *  Return:  sucess: TRUE
 *           unsucess: FALSE
 *  Notice:
 *    1997.10.20 the GROUPBY cannot appeared in asql when FROM is sqlsource
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   GroupExec( dFILE *dsp, dFILE *dtp )
{
    long          lRecNum, gcount;
    short         i, k;
    CalActOrdTable *paAcTbIt;
    char          keyBuf[256];
    bHEAD        *bh;
#ifdef _DEF_groupWassymbol
    dFIELD       *keyFld;
#endif
    //long        oldrec_p;
    ExecActTable *expActTab;
    char         *myAsqlStatFlagY;
    int           myAsqlStatMaxY;
    signed char   cmpRsut;
    short         iScopeSkipAdjust = fFrTo.iScopeSkipAdjust;

    myAsqlStatFlagY = asqlStatFlagY;
    myAsqlStatMaxY = asqlStatMaxY;

    lRecNum = getRecNum(dsp);
    i = 0;
    /*if( m_c_AsqlStatTailAction != NULL ) {
	if( ActionCalExpr(m_c_AsqlStatTailAction, &(i), dtp ) == LONG_MAX ) {
		qsError = 5005;
		return  FALSE;
	} // end of if
    }*/
    paAcTbIt = fFrTo.cHead;
    while( paAcTbIt != NULL ) {
      if( paAcTbIt->actionFunRefered ) {
	if( ActionCalExpr(paAcTbIt->pxActMidCode, &(i), dtp ) == LONG_MAX ) {
		qsError = 5006;
		return  FALSE;
	} // end of if
	//paAcTbIt->nCalTimes = 1;
      }
      paAcTbIt = paAcTbIt->aNext;
    }

    i = 1;
    // execute the expression action

/*    dseek(dsp, 0L, dSEEK_SET);
*/
    bh = NULL;
    for(k = 0;   k < dsp->syncBhNum;   k++)
    {
	if( keyFieldCmp((bHEAD *)(dsp->bhs[k]), fFrTo.cGroKey) != 0 ) {
		bh = IndexAwake((char *)dsp, \
			((bHEAD *)(dsp->bhs[k]))->ndxName, BTREE_FOR_OPENDBF);
		break;
	}
    }
    if( bh == NULL ) {
	char  szGbNdx[260];

	// create the target for the source query
	sprintf(szGbNdx, "GBNDX%03X", intOfThread&0xFFF);
	makefilename(szGbNdx, tmpPath, szGbNdx);

	if( fFrTo.wGbKeyLen == SHRT_MAX ) {
	    bh = IndexBAwake((char *)dsp, fFrTo.cGroKey, szGbNdx, BTREE_FOR_OPENDBF);
	} else {
	    bh = IndexBuildKeyExpr(dsp, fFrTo.cGroKey, fFrTo.wGbKeyLen, \
								     szGbNdx);
	}
    }

    if( bh == NULL ) {
	strZcpy(szAsqlErrBuf, fFrTo.cGroKey, sizeof(szAsqlErrBuf));
	qsError = 5008;
	return  FALSE;
	// error use index
    }
    fFrTo.syncBh[0] = bh;

#ifdef _DEF_groupWassymbol
    { //cut into one field
	char  *sp;
	short fldId;

	strcpy(keyBuf, fFrTo.cGroKey);
	if( (sp = strchr(keyBuf, '+')) != NULL )
		*sp = '\0';
	fldId = GetFldid(dsp, keyBuf);
	if( fldId == 0xFFFF ) {
		strZcpy(szAsqlErrBuf, fFrTo.cGroKey, sizeof(szAsqlErrBuf));
		qsError = 5010;
		return  FALSE;
	}
	keyFld = getFieldInfo(dsp, fldId);
    }
#endif

    if( iScopeSkipAdjust == -2 ) {
	IndexGoBottom(bh);
    } else {
	IndexGoTop(bh);
    }

    if( fFrTo.nGroupAction )
    { //register the groupaction
	ExecActTable *expActTab;
	MidCodeType  *xMid;

	expActTab = fFrTo.exHead;
	while( expActTab != NULL ) {
		if( expActTab->nLevel == 'G' ) {
			xMid = ActionSymbolRegister(expActTab->xPoint,dsp,dtp, \
				  fFrTo.sHead, fFrTo.nSybNum, \
				  fFrTo.dHead, fFrTo.nDefinNum);
			if( xMid != NULL ) {
				strcpy(szAsqlErrBuf, xMid->values);
				IndexSleep(bh);
				qsError = 3026;
				return  FALSE;
			}
		}
		expActTab = expActTab->oNext;
	}
    }
    strcpy(keyBuf, IndexGetKeyContent(bh));
    gcount = 0;

#ifdef _DEF_groupWassymbol
    groupWassymbol(dtp, keyFld, keyBuf, 0);
#endif

    while( lRecNum-- ) {

#ifdef _AsqlRuningMessage_Assigned
	// call users function to deal with the runing message
	if( AsqlRuningMessage(lRecNum) ) {
		IndexSleep(bh);
		qsError = 7002;
		return  FALSE;
	} // end of if
#endif

	// get a row of table
	if( get1rec( dsp ) == NULL ) {
#ifdef _DEF_groupWassymbol
	    groupWassymbol(dtp, keyFld, keyBuf, 2);
#endif
	    IndexSleep(bh);
	    qsError = 7002;
	    return  FALSE;
	}
	if( dsp->rec_buf[0] == '*' ) {
	    if( IndexSkip(bh,1+iScopeSkipAdjust) == LONG_MIN )
	    { //now we meet the bottom of the tree
		break;
	    }
	    if( gcount < 1 )
	    { //there is no record appears in this group
		strcpy(keyBuf, IndexGetKeyContent(bh));
		continue;
	    } else {
		goto GROUP_EXEC_JMP;
	    }
	} else {
	    gcount++;
	}

	// execute this record
	//oldrec_p = fFrTo.cSouFName[0]->rec_p;
	if( ExecRec( dsp, dtp ) == FALSE ) {
#ifdef _DEF_groupWassymbol
	    groupWassymbol(dtp, keyFld, keyBuf, 2);
#endif
	    IndexSleep(bh);
	    return  FALSE;
	}

	if( myAsqlStatMaxY > 0 )
	{
	    memset(myAsqlStatFlagY, 0, myAsqlStatMaxY);
	}

	/*if( m_c_AsqlStatTailAction != NULL ) {
		if( ActionCalExpr(m_c_AsqlStatTailAction, \
						    &(i), dtp ) == LONG_MAX ) {
			groupWassymbol(dtp, keyFld, keyBuf, 2);
			IndexSleep(bh);
			qsError = 5005;
			return  FALSE;
		} // end of if
	}*/

	// calculate
	if( IndexSkip(bh,1+iScopeSkipAdjust) == LONG_MIN )
        { //now we meet the bottom of the tree
            break;
        }

GROUP_EXEC_JMP:
	if( fFrTo.cGyExact == 1 ) {
	    cmpRsut = bExactStrCmp(keyBuf, IndexGetKeyContent(bh), bh->keyLen);
	} else {
	    cmpRsut = bFuzzyStrCmp(keyBuf, IndexGetKeyContent(bh), bh->keyLen);
	}
	if( cmpRsut != 0 ) {

		//let the action work with the last same record
		IndexSkip(bh, -1-iScopeSkipAdjust);

		if( fFrTo.nGroupAction )
		{
		  //execute groupaction
		  expActTab = fFrTo.exHead;
		  while( expActTab != NULL ) {
		    if( expActTab->nLevel == 'G' ) {

			//if( ActionCalExpr(expActTab->xPoint, &one, dtp) == LONG_MAX ){
			if( ActionCalExpr(expActTab->xPoint, &wGroupbyAction, dtp) == LONG_MAX ){

#ifdef _DEF_groupWassymbol
				groupWassymbol(dtp, keyFld, keyBuf, 2);
#endif
				IndexSleep(bh);
				qsError = 4002;
				return  FALSE;
			}
		    } // end of if
		    expActTab = expActTab->oNext;
		  } // end of while
		}

#ifdef _DEF_groupWassymbol
		// write the one dimension to file
		groupWassymbol(dtp, keyFld, keyBuf, 1);
#endif
		IndexSkip(bh, 1+iScopeSkipAdjust);
		strcpy(keyBuf, IndexGetKeyContent(bh));
		gcount = 0;
	}

    } // end of while

    //run for the last record

    if( fFrTo.nGroupAction && gcount > 0 ) {
	expActTab = fFrTo.exHead;
	while( expActTab != NULL ) {
	    if( expActTab->nLevel == 'G' ) {

		//if( ActionCalExpr(expActTab->xPoint, &one, dtp) == LONG_MAX ){
		if( ActionCalExpr(expActTab->xPoint, &wGroupbyAction, dtp) == LONG_MAX ){

#ifdef _DEF_groupWassymbol
			groupWassymbol(dtp, keyFld, keyBuf, 2);
#endif
			IndexSleep(bh);
			qsError = 4002;
			return  FALSE;
		}
	    } // end of if
	    expActTab = expActTab->oNext;
	} // end of while
    }
#ifdef _DEF_groupWassymbol
    groupWassymbol(dtp, keyFld, keyBuf, 1);

    groupWassymbol(dtp, keyFld, keyBuf, 2);
#endif

    IndexSleep(bh);

    return  TRUE;

} // end of function GroupExec()


/*---------------
 *  Function Name:      SortedGroupExec()
 *  Return:  sucess: TRUE
 *           unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   SortedGroupExec( dFILE *dsp, dFILE *dtp )
{
    long          lRecNum, gcount;
    short         i;
    CalActOrdTable *paAcTbIt;
    char          keyBuf[256];
    char          keyTmpBuf[256];
#ifdef _DEF_groupWassymbol
    dFIELD       *keyFld;
#endif
    //long        oldrec_p;
    ExecActTable *expActTab;
    char         *myAsqlStatFlagY;
    int           myAsqlStatMaxY;
    int		  cmpRsut;

    MidCodeType   *mpp;
    char          *sz = NULL;
    char	  tmpBuf[4096*2];
    int		  firstRecord = 1;

    myAsqlStatFlagY = asqlStatFlagY;
    myAsqlStatMaxY = asqlStatMaxY;

    lRecNum = getRecNum(dsp);
    i = 0;
    /*if( m_c_AsqlStatTailAction != NULL ) {
	if( ActionCalExpr(m_c_AsqlStatTailAction, &(i), dtp ) == LONG_MAX ) {
		qsError = 5005;
		return  FALSE;
	} // end of if
    }*/
    paAcTbIt = fFrTo.cHead;
    while( paAcTbIt != NULL ) {
      if( paAcTbIt->actionFunRefered ) {
	if( ActionCalExpr(paAcTbIt->pxActMidCode, &(i), dtp ) == LONG_MAX ) {
		qsError = 5006;
		return  FALSE;
	} // end of if
	//paAcTbIt->nCalTimes = 1;
      }
      paAcTbIt = paAcTbIt->aNext;
    }

    i = 1;
    // execute the expression action

/*    dseek(dsp, 0L, dSEEK_SET);
*/

    mpp = WordAnalyse(fFrTo.cGroKey);
    if( mpp != NULL ) {
	if( SymbolRegister( mpp, dsp, NULL, 0, NULL, 0) != NULL ) {
		FreeCode( mpp );
		qsError = 5003;
		return  FALSE;
	}
    } else {
	qsError = 5003;
	return  FALSE;
    }

#ifdef _DEF_groupWassymbol
    { //cut into one field
	char  *sp;
	short fldId;

	strcpy(keyBuf, fFrTo.cGroKey);
	if( (sp = strchr(keyBuf, '+')) != NULL )
		*sp = '\0';

	fldId = GetFldid(dsp, keyBuf);
	if( fldId == 0xFFFF ) {
		if( mpp != NULL )
			FreeCode( mpp );
		qsError = 5010;
		return  FALSE;
	}
	keyFld = getFieldInfo(dsp, fldId);
    }
#endif

    if( fFrTo.nGroupAction )
    { //register the groupaction
	ExecActTable *expActTab;
	MidCodeType  *xMid;

	expActTab = fFrTo.exHead;
	while( expActTab != NULL ) {
		if( expActTab->nLevel == 'G' ) {
			xMid = ActionSymbolRegister(expActTab->xPoint,dsp,dtp, \
				  fFrTo.sHead, fFrTo.nSybNum, \
				  fFrTo.dHead, fFrTo.nDefinNum);
			if( xMid != NULL ) {
				strcpy(szAsqlErrBuf, xMid->values);
				FreeCode( mpp );
				qsError = 3026;
				return  FALSE;
			}
		}
		expActTab = expActTab->oNext;
	}
    }

    gcount = 0;

#ifdef _DEF_groupWassymbol
    groupWassymbol(dtp, keyFld, keyBuf, 0);
#endif

    while( lRecNum-- ) {

#ifdef _AsqlRuningMessage_Assigned
	// call users function to deal with the runing message
	if( AsqlRuningMessage(lRecNum) ) {
		FreeCode( mpp );
		qsError = 7002;
		return  FALSE;
	} // end of if
#endif

	// get a row of table
	if( get1rec( dsp ) == NULL ) {

#ifdef _DEF_groupWassymbol
	    groupWassymbol(dtp, keyFld, keyBuf, 2);
#endif
	    if( dsp->dbf_flag == SQL_TABLE ) {
		if( TgetSqlErrCode() == 100 )
			break;		    //break from while()
	    }
	    if( dsp->dbf_flag == ODBC_TABLE ) {
		//if( getI_ErrCode() == 100 )
		    
		    //2001.7.7 Xilong
		    if( dsp->error != 0 )
			strZcpy(szAsqlErrBuf, szOdbcExecResult, sizeof(szAsqlErrBuf) );

		    break;		    //break from while()
	    }

	    FreeCode( mpp );
	    qsError = 7002;
	    return  FALSE;
	}

	sz = (char *)CalExpr( mpp );
	sz = strZcpy(keyTmpBuf, sz, 255);
	if( firstRecord ) {
	    strcpy(keyBuf, sz);
	    firstRecord = 0;
	}


	/*
	//gcount++;
	// execute this record
	//oldrec_p = fFrTo.cSouFName[0]->rec_p;
	if( ExecRec( dsp, dtp ) == FALSE ) {
#ifdef _DEF_groupWassymbol
	    groupWassymbol(dtp, keyFld, keyBuf, 2);
#endif
	    FreeCode( mpp );
	    return  FALSE;
	}

	if( myAsqlStatMaxY > 0 )
	{
	    memset(myAsqlStatFlagY, 0, myAsqlStatMaxY);
	}
	*/

	/*if( m_c_AsqlStatTailAction != NULL ) {
		if( ActionCalExpr(m_c_AsqlStatTailAction, \
						    &(i), dtp ) == LONG_MAX ) {
			groupWassymbol(dtp, keyFld, keyBuf, 2);
			qsError = 5005;
			return  FALSE;
		} // end of if
	}*/

	if( fFrTo.cGyExact == 1 ) {
	    cmpRsut = strcmp(keyBuf, sz);
	} else {
	    cmpRsut = strcmp(keyBuf, sz);
	}
	if( cmpRsut != 0 ) {

		//let the action work with the last same record
		memcpy(tmpBuf, dsp->rec_buf, dsp->rec_len);
		memcpy(dsp->rec_buf, dsp->rec_tmp, dsp->rec_len);

		if( fFrTo.nGroupAction )
		{
		  //execute groupaction
		  expActTab = fFrTo.exHead;
		  while( expActTab != NULL ) {
		    if( expActTab->nLevel == 'G' ) {

			//if( ActionCalExpr(expActTab->xPoint, &one, dtp) == LONG_MAX ){
			if( ActionCalExpr(expActTab->xPoint, &wGroupbyAction, dtp) == LONG_MAX ){

#ifdef _DEF_groupWassymbol
				groupWassymbol(dtp, keyFld, keyBuf, 2);
#endif
				FreeCode( mpp );
				qsError = 4002;
				return  FALSE;
			}
		    } // end of if
		    expActTab = expActTab->oNext;
		  } // end of while
		}

#ifdef _DEF_groupWassymbol
		// write the one dimension to file
		groupWassymbol(dtp, keyFld, keyBuf, 1);
#endif

		memcpy(dsp->rec_buf, tmpBuf, dsp->rec_len);
		strcpy(keyBuf, sz);
		//gcount = 0;
	}

	//2000.8.21
	//gcount++;
	// execute this record
	//oldrec_p = fFrTo.cSouFName[0]->rec_p;
	if( ExecRec( dsp, dtp ) == FALSE ) {
#ifdef _DEF_groupWassymbol
	    groupWassymbol(dtp, keyFld, keyBuf, 2);
#endif
	    FreeCode( mpp );
	    return  FALSE;
	}

	if( myAsqlStatMaxY > 0 )
	{
	    memset(myAsqlStatFlagY, 0, myAsqlStatMaxY);
	}


	memcpy(dsp->rec_tmp, dsp->rec_buf, dsp->rec_len);

	//skip a record
	if( dsp->rec_p > 0 )
	    dseek(dsp, 1, dSEEK_CUR);
	else
	    dsp->rec_p = 0-dsp->rec_p;

    } // end of while

    //run for the last record

    if( fFrTo.nGroupAction )
    /*if( gcount > 0 ) */{
	expActTab = fFrTo.exHead;
	while( expActTab != NULL ) {
	    if( expActTab->nLevel == 'G' ) {

		//if( ActionCalExpr(expActTab->xPoint, &one, dtp) == LONG_MAX ){
		if( ActionCalExpr(expActTab->xPoint, &wGroupbyAction, dtp) == LONG_MAX ){

#ifdef _DEF_groupWassymbol
			groupWassymbol(dtp, keyFld, keyBuf, 2);
#endif
			FreeCode( mpp );
			qsError = 4002;
			return  FALSE;
		}
	    } // end of if
	    expActTab = expActTab->oNext;
	} // end of while
    }

#ifdef _DEF_groupWassymbol
    groupWassymbol(dtp, keyFld, keyBuf, 1);

    groupWassymbol(dtp, keyFld, keyBuf, 2);
#endif

    FreeCode( mpp );

    return  TRUE;

} // end of function SortedGroupExec()



/*---------------
 *  Function Name:      GroupNotScopeExec()
 *  Return:  sucess: TRUE
 *           unsucess: FALSE
 *  Notice:
 *    1997.10.20 the SCOPE cannot appeared in asql when FROM is sqlsource
 *    SCOPE is no use when groupby appeared
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   GroupNotScopeExec( dFILE *dsp, dFILE *dtp )
{
    long       lRecNum;
    short      i;
    CalActOrdTable *paAcTbIt;
    bHEAD      *bh;
    //long        oldrec_p;
    char        ndxName[256];
    char       *myAsqlStatFlagY;
    int        	myAsqlStatMaxY;
    int        	k;
    char       *sz, *sz1;
    char       	buf[Asql_KEYWORDLENGTH];

    short	iScopeSkipAdjust;

    if( isdigit(fFrTo.szScopeKey[0]) )
    { ///////////////////////////////////////////////////////////////////////
      //
      // FROM (SCOPE: start_rec, end_rec)source_table
      //
      //

        myAsqlStatFlagY = asqlStatFlagY;
	myAsqlStatMaxY = asqlStatMaxY;

	i = 0;
	paAcTbIt = fFrTo.cHead;
        while( paAcTbIt != NULL ) {
	  if( paAcTbIt->actionFunRefered ) {
	    if( ActionCalExpr(paAcTbIt->pxActMidCode, &(i), dtp ) == LONG_MAX ) {
		qsError = 5006;
		return  FALSE;
	    } // end of if
	    //paAcTbIt->nCalTimes = 1;
	  }
	  paAcTbIt = paAcTbIt->aNext;
        }

	dseek(dsp, atoi(fFrTo.szScopeStart)-1, dSEEK_SET);

        lRecNum = min(getRecNum(dsp), atoi(fFrTo.szScopeEnd)-atoi(fFrTo.szScopeStart)+1);

        while( lRecNum-- ) {

#ifdef _AsqlRuningMessage_Assigned
	   // call users function to deal with the runing message
	   if( AsqlRuningMessage(lRecNum) ) {
		IndexSleep(bh);
		qsError = 7002;
		return  FALSE;
	   } // end of if

#endif

	   // get a row of table
	   if( get1rec( dsp ) == NULL ) {
	       qsError = 5006;
	       return  FALSE;
	   }

	   if( dsp->rec_buf[0] == '*' ) {
		continue;
	   }

	   // execute this record
	   //oldrec_p = fFrTo.cSouFName[0]->rec_p;
	   if( ExecRec( dsp, dtp ) == FALSE ) {
		return  FALSE;
	   }

	   if( myAsqlStatMaxY > 0 )
	   {
		memset(myAsqlStatFlagY, 0, myAsqlStatMaxY);
	   }

	   // calculate
	   dseek(dsp, 1, dSEEK_CUR);

	} // end of while

	return  TRUE;

    } //
      //
      // FROM (SCOPE: start_rec, end_rec)source_table
      ///////////////////////////////////////////////////////////////////////

    iScopeSkipAdjust = fFrTo.iScopeSkipAdjust;

    myAsqlStatFlagY = asqlStatFlagY;
    myAsqlStatMaxY = asqlStatMaxY;

    lRecNum = getRecNum(dsp);
    i = 0;
    /*if( m_c_AsqlStatTailAction != NULL ) {
	if( ActionCalExpr(m_c_AsqlStatTailAction, &(i), dtp ) == LONG_MAX ) {
		qsError = 5005;
		return  FALSE;
	} // end of if
    }*/
    paAcTbIt = fFrTo.cHead;
    while( paAcTbIt != NULL ) {
      if( paAcTbIt->actionFunRefered ) {
	if( ActionCalExpr(paAcTbIt->pxActMidCode, &(i), dtp ) == LONG_MAX ) {
		qsError = 5006;
		return  FALSE;
	} // end of if
	//paAcTbIt->nCalTimes = 1;
      }
      paAcTbIt = paAcTbIt->aNext;
    }

    i = 1;
    // execute the expression action

/*    dseek(dsp, 0L, dSEEK_SET);
*/

    for(k = 0;   k < dsp->syncBhNum;   k++)
    {
	if( keyFieldCmp((bHEAD *)(dsp->bhs[k]), fFrTo.szScopeKey) != 0 ) {
		bh = IndexAwake((char *)dsp, ((bHEAD *)(dsp->bhs[k]))->ndxName,
								BTREE_FOR_OPENDBF);
		goto BH_OK1;
	}
    } //no else here

    strcpy(ndxName, dsp->name);
    if( ( sz = strrchr(ndxName, '.') ) != NULL ) {
	*sz = '\0';
    }
    strcat(ndxName, "^");
    strcpy(buf, fFrTo.szScopeKey);
    for(i = 0;  buf[i] != '\0';  i++) {
	if( buf[i] == '+' )
		buf[i] = '^';
    }

    strcat(ndxName, buf);
    sprintf(buf, "^%03X", intOfThread&0xFFF);
    strcat(ndxName, buf);

    makeTrimFilename(ndxName, tmpPath, ndxName);

    bh = IndexAwake((char *)dsp, ndxName, BTREE_FOR_OPENDBF);
    if( keyFieldCmp(bh, fFrTo.szScopeKey) == 0 ) {
	if( bh != NULL )
	    IndexSleep(bh);

	bh = IndexBAwake((char *)dsp, fFrTo.szScopeKey, ndxName, BTREE_FOR_OPENDBF);
    }

BH_OK1:
    if( bh == NULL ) {
	qsError = 2005;
	return  FALSE;
	// error use index
    }

    fFrTo.syncBh[0] = bh;

    IndexLocate(bh, fFrTo.szScopeStart);
    sz = fFrTo.szScopeEnd;
    sz1 = _BtreeKeyBuf;
    while( lRecNum-- ) {

#ifdef _AsqlRuningMessage_Assigned
	// call users function to deal with the runing message
	if( AsqlRuningMessage(lRecNum) ) {
		IndexSleep(bh);
		qsError = 7002;
		return  FALSE;
	} // end of if

#endif

	// get a row of table
	if( get1rec( dsp ) == NULL ) {
	    qsError = 7002;
	    IndexSleep(bh);
	    return  FALSE;
	}

	if( dsp->rec_buf[0] == '*' ) {
	    if( IndexSkip(bh,1+iScopeSkipAdjust) == LONG_MIN )
	    { //now we meet the bottom of the tree
		break;
	    }
	    continue;
	}

	if( iScopeSkipAdjust == -2 )
	{ //descending
	    if( bStrCmp(sz, sz1, bh->keyLen) > 0 ) {
		break;
	    }
	} else
	{ //ascending
	    if( bStrCmp(sz, sz1, bh->keyLen) < 0 ) {
		break;
	    }
	}

	/*if( bStrCmp(fFrTo.szScopeEnd, IndexGetKeyContent(bh), bh->keyLen) < 0 ) {
	    break;
	}*/

	// execute this record
	//oldrec_p = fFrTo.cSouFName[0]->rec_p;
	if( ExecRec( dsp, dtp ) == FALSE ) {
	    IndexSleep(bh);
	    return  FALSE;
	}

	if( myAsqlStatMaxY > 0 )
	{
		memset(myAsqlStatFlagY, 0, myAsqlStatMaxY);
	}

	/*if( m_c_AsqlStatTailAction != NULL ) {
		if( ActionCalExpr(m_c_AsqlStatTailAction, \
						    &(i), dtp ) == LONG_MAX ) {
			IndexSleep(bh);
			qsError = 5005;
			return  FALSE;
		} // end of if
	}*/

	// calculate
	if( IndexSkip(bh,1+iScopeSkipAdjust) == LONG_MIN )
	{ //now we meet the bottom of the tree
	    break;
	}

    } // end of while

    IndexSleep(bh);

    return  TRUE;

} // end of function GroupNotScopeExec()



short groupWassymbol(dFILE *tdf, dFIELD *keyFld, char *keyBuf, short CurState)
{
    int         i, j;
    WSToMT static dFILE  *gsDbf;
    char       *dbfName;
    SysVarOFunType *UsrVar = *XexpUserDefinedVar;

    if( CurState == 0 ) {

	dFIELD  *field;
	char    nameBuf[MAXPATH];

	gsDbf = NULL;

	if( fFrTo.nGroupAction )
	    return  0;

	if( tdf != NULL ) {
	    dbfName = tdf->name;
	} else {
	    return  0;
	}

	changeFilenameExt(nameBuf, dbfName, szGroupFileExt);
	if( *XexpUserDefinedVarNum < 1 ) {
		unlink(nameBuf);
		return  0;
	}

	if( (field = zeroMalloc((*XexpUserDefinedVarNum+2) * sizeof(dFIELD))) == NULL ) {
		return  1;
	}

	memcpy(&field[0], keyFld, sizeof(dFIELD)-sizeof(char *)*3);
	for(i = 1, j = 0;  i < *XexpUserDefinedVarNum+1;  i++) {
		strncpy(field[i].field, UsrVar[j].VarOFunName, FIELDNAMELEN-1);
		field[i].field[FIELDNAMELEN-1] = '\0';
		switch( UsrVar[j++].type ) {
		    case LONG_IDEN:
			field[i].fieldtype = 'N';
			field[i].fieldlen = 10;
			field[i].fielddec = 0;
			break;
		    case FLOAT_IDEN:
			field[i].fieldtype = 'N';
			field[i].fieldlen = 19;
			field[i].fielddec = 4;
			break;
		    case STRING_IDEN:
			field[i].fieldtype = 'C';
			field[i].fieldlen = 32;
			field[i].fielddec = 0;
			break;
		} // end os switch
	} // end of for
	field[i].field[0] = '\0';
	gsDbf = dcreate(nameBuf, field);
	free(field);
	if( dSetAwake(gsDbf, &gsDbf) != 1 ) {
	    dclose(gsDbf);
	    return  1;
	}
#ifdef WSToMT
	wmtDbfLock( gsDbf );
#endif
    } else if( gsDbf != NULL ) {

	if( CurState == 1 ) {

	    put_fld(gsDbf, 0, keyBuf);
	    for(i = 0;  i < *XexpUserDefinedVarNum;  i++) {
		PutField(gsDbf, (short)(i+1), UsrVar[i].values);

		// set the variable to 0
		memset(UsrVar[i].values, 0, MAX_OPND_LENGTH);

	    } // end of for
	    putrec(gsDbf);
	} else {
	    if( CurState == 2 ) {
#ifdef WSToMT
		wmtDbfUnLock( gsDbf );
#endif
		dSleep( gsDbf );
	    }
	}
    	
	for(i = 0;  i < *XexpUserDefinedVarNum;  i++) {
	    // set the variable to 0
	    if( UsrVar[i].type != STRING_IDEN )
		memset(UsrVar[i].values, 0, MAX_OPND_LENGTH);
	} // end of for
    }

    return  0;

} // end of function groupWassymbol()







#ifdef RUNING_ERR_MESSAGE_ON
void AsqlAnaErrorMessage( char *msg )
{
    if( errorStamp )    return;

    if( msg == NULL ) {
	sprintf(pubBuf,
		"%s\n%s\n%s", AsqlErrorMes(), szAsqlErrBuf, \
					       GetErrorMes( GeterrorNo() ) );
    } else {
	sprintf(pubBuf, "%s\n%s\n%s", msg, AsqlErrorMes(),
					       GetErrorMes( GeterrorNo() ) );
    }
#ifdef __N_C_S
	messageBoxRect(initRect(38, 14, 40, 10), pubBuf, mfError|mfOKButton);
#endif
//    messageBox(pubBuf, mfError|mfOKButton);
}

#endif


#ifdef _AsqlRuningMessage_Assigned
char AsqlRuningMessage( long recno )
{
#ifdef __N_C_S
      if( (recno > 100) ) {
	if( recno % 100 != 0 )  return  0;

	// added 1995.09.22 for Stop
	if( kbhit() ) {
		if( getch() == '\x1B' )         return '\x1B';
	}
      } else {
	if( recno % 10 != 0 )   return  0;
      }
#else
      if( (recno > 100) ) {
	if( recno % 100 != 0 )  return  0;
      } else {
	if( recno % 10 != 0 )   return  0;
      }
#endif
      sprintf(pubBuf, "%ld", recno);

#ifdef __N_C_S
      sendMessage( (View *)&bRuningText, evBroadcast, cmChangeText, pubBuf);
      ((View *)&bRuningText)->focusDraw( (View *)&bRuningText );
#else
      csRuningMessage(9, pubBuf);
#endif
      return  0;
}

#endif


/*---------------
 *  Function Name:  genGramMan()
 *  Input:  void, wWord
 *  Return: mem pointer
 *  qsError:
 *-------------------------------------------------------------------------*/
static ExecActTable *genGramMan( void )
{
    ExecActTable *exExAct;

    if(( exExAct = (ExecActTable *)zeroMalloc( sizeof(ExecActTable) )) == NULL) {
	qsError = 1004;
	return  FALSE;
    }

    exExAct->xPoint = wWord->xPointer;
    if( fFrTo.exHead == NULL ) {
	fFrTo.exHead = apExecActTable = exExAct;
    } else {
	apExecActTable->oNext = exExAct;
	apExecActTable = exExAct;
    }

    exExAct->oNext = NULL;

    return  exExAct;

} //end of genGramMan()


/*---------------
 *  Function Name:  execGramMan()
 *  Input:  ph: -1, pre, 1, ahead
 *  Return: mem pointer
 *  qsError:
 *-------------------------------------------------------------------------*/
static short  execGramMan(ExecActTable *exExAct, dFILE *dsp, dFILE *dtp, char abg)
{
    MidCodeType  *xMid;
    short	 num;
    long	 l;
    long 	 recno = 1;
    char         savedabg;
    ExecActTable *savedexExAct;

    num = ONE_MODALITY_ACTION;
    ASKACTIONCURRENTSTATEshort = &num;

    savedexExAct = exExAct;
    savedabg = exExAct->nLevel;

    exExAct->nLevel = abg;
    while( exExAct != NULL ) {
	if( exExAct->nLevel == abg ) {

#ifdef _AsqlRuningMessage_Assigned
		if( AsqlRuningMessage( recno++ ) != 0 ) {
			qsError = 7002;
                        savedexExAct->nLevel = savedabg;
			return  FALSE;
		}
#endif
		if( exExAct->regEd == 0 ) {
			xMid = ActionSymbolRegister(exExAct->xPoint,dsp,dtp, \
				  fFrTo.sHead, fFrTo.nSybNum, \
				  fFrTo.dHead, fFrTo.nDefinNum);
			if( xMid != NULL ) {
				strcpy(szAsqlErrBuf, xMid->values);
				qsError = 3026;
                                savedexExAct->nLevel = savedabg;
				return  FALSE;
			}
			exExAct->regEd = 1;
		}
		switch( exExAct->stmType ) {
		    case Asql_LET:
			if( ActionCalExpr(exExAct->xPoint, &num, dtp) == LONG_MAX ) {
				qsError = 4002;
                                savedexExAct->nLevel = savedabg;
				return  FALSE;
			}
			exExAct = exExAct->execNext;
			break;
		    case Asql_ACTION:
			if( ActionCalExpr(exExAct->xPoint, &num, dtp) == LONG_MAX ) {
				qsError = 4002;
                                savedexExAct->nLevel = savedabg;
				return  FALSE;
			}
			exExAct = exExAct->execNext;
			break;
		    case Asql_IF:
		    case Asql_WHILE:
			if( (l = CalExpr(exExAct->xPoint)) == LONG_MAX ) {
				qsError = 5005;
                                savedexExAct->nLevel = savedabg;
				return  FALSE;
			}

			if( l != 0 ) { //****** TRUE ******
				exExAct = exExAct->execNext;
			} else {
				exExAct = exExAct->gotoStm;
			}
			break;
		    case Asql_GOTO:
		    case Asql_EXIT:
		    case Asql_LOOP:
			exExAct = exExAct->execNext;
			break;
		    case Asql_CALL:
		    {
			ExecActTable *callExAct = exExAct->execNext;

			if( execGramMan(exExAct->gotoStm, dsp, dtp, abg) == FALSE ) {
				qsError = 7007;
                                savedexExAct->nLevel = savedabg;
				return  FALSE;
			}
			exExAct = callExAct;
		    }
		    break;
		    default:
			qsError = 5005;
                        savedexExAct->nLevel = savedabg;
			return  FALSE;
		}
	} else {
		exExAct = exExAct->oNext;
	}
    } // end of while

    savedexExAct->nLevel = savedabg;
    return  TRUE;

} //end of execGramMan()


/*---------------
 *  Function Name:  programPro()
 *  Input:  abg: 'B', pre, 'A', ahead
 *  Return: mem pointer
 *  qsError:
 *  NOTICE: this function need above 1K stack space
 *-------------------------------------------------------------------------*/
static short programPro( void )
{
    ExecActTable  nullEAT;

    ExecActTable  *eatSp[Asql_MAXBEGINENDNESTDEEP];
    ExecActTable  *eatTailPoint[Asql_MAXBEGINENDNESTDEEP];
    short iSTop, iTail;
    char  nLevel;
    short nInterNumberal;
    ExecActTable *exExAct = NULL, *exExActRem, *exExActGotoRem;
    short i;
    int   firstSentense = 1;

    //set nullEAT
    memset(&nullEAT, 0, sizeof(ExecActTable));

    //1998.11.25
    //if( fFrTo.eHead == NULL )
    if( ConditionAppeared == 0 )
	nLevel = 'b';
    else
	nLevel = 'a';

    iSTop = iTail = -1;

    exExActRem = exExActGotoRem = NULL;
    while( wWord->nType != Asql_FILEEND ) {

	    if( wWord->nType != Asql_RETAINTYPE )
	    { //every ASQL statement must start with a keyword
		qsError = 6006;
		return  FALSE;
	    }

	    switch( nInterNumberal = wWord->nInterNumberal ) {
		case Asql_LET:
		case Asql_ACTION:
		{
		    wWord->keyWordPossible = 3;
		    if( GetWord() == 0 )   return  FALSE;
		    if( wWord->nType == Asql_FILEEND )
			return  FALSE;

		    if( (exExAct = genGramMan()) == NULL )
			return  FALSE;

                    if( firstSentense ) {
                        firstSentense = 0;
			if( nLevel == 'b' )
			    exExAct->nLevel = 'E';      //entry
                        else if( nLevel == 'a' )
			    exExAct->nLevel = 'F';      //entry
                    } else {
		        exExAct->nLevel = nLevel;
                    }
		    exExAct->stmType = nInterNumberal;

		    if( exExActRem != NULL ) {
			exExActRem->execNext = exExAct;
		    } else {
			for( ;   iTail >= 0;  iTail-- ) {
				if( eatTailPoint[iTail] == NULL )
				{ //segment flag
					iTail--;
					break;
				}
				eatTailPoint[iTail]->execNext = exExAct;
			}

			if( exExActGotoRem != NULL ) {
				exExActGotoRem->gotoStm = exExAct;
				exExActGotoRem = NULL;
			}
		    }

		    exExActRem = exExAct;

		}
		break;
		case Asql_IF:
		case Asql_WHILE:
		{
		    wWord->keyWordPossible = 3;
		    if( GetWord() == 0 )   return  FALSE;
		    if( wWord->nType == Asql_FILEEND )
			return  FALSE;

		    if( (exExAct = genGramMan()) == NULL )
			return  FALSE;

                    if( firstSentense ) {
                        firstSentense = 0;
			if( nLevel == 'b' )
                            exExAct->nLevel = 'E';
                        else if( nLevel == 'a' )
                            exExAct->nLevel = 'F';
                    } else {
		        exExAct->nLevel = nLevel;
                    }
		    exExAct->stmType = nInterNumberal;
		    exExAct->gotoStm = NULL;

		    if( exExActRem != NULL ) {
			exExActRem->execNext = exExAct;
		    } else {
			for( ;   iTail >= 0;  iTail-- ) {
				if( eatTailPoint[iTail] == NULL )
				{ //segment flag
					iTail--;
					break;
				}
				eatTailPoint[iTail]->execNext = exExAct;
			}

			if( exExActGotoRem != NULL ) {
				exExActGotoRem->gotoStm = exExAct;
				exExActGotoRem = NULL;
			}
		    }

		    //increase nest deep
		    eatTailPoint[++iTail] = NULL;

		    //fill the stack for ELSE Skip, and NEXT Skip
		    eatSp[ ++iSTop ] = exExActRem = exExAct;

		}
		break;
		case Asql_ELSE:
		{
		    if( iSTop >= 0 && eatSp[iSTop]->stmType == Asql_IF )
		    {
			//exExActGotoRem is the IF's false action
			exExActGotoRem = eatSp[iSTop];
			eatTailPoint[++iTail] = exExAct;
		    } else {
			//else without if
			qsError = 7009;
			return  FALSE;
		    }
		}
		break;
		case Asql_ENDIF:
		{
		    //look for Asql_IF
		    //the stack now could be:
		    // | Asql_WHILE
		    // | Asql_IF__ <--- filled already
		    // | Asql_EXIT
		    // | Asql_LOOP
		    for( i = iSTop;  i >= 0;  i-- )
		    {
			short  k = eatSp[i]->stmType;
			if( k != Asql_EXIT && k != Asql_LOOP && \
							k != Asql_GOTO )
				break;
		    }

		    if( i < 0 ) {
			//endif without if
			qsError = 7003;
			return  FALSE;
		    }

		    if( i == iSTop && eatSp[iSTop]->stmType == Asql_IF )
		    {

			//IF
			//[ELSE]
			//ENDIF
			//there must be a statement between IF and ELSE
			//or IF and ENDIF or ELSE and ENDIF
			if( exExAct->stmType != Asql_LET ) {
			    //the just statement must be Asql_LET
			    //the just statement could be:
			    // (1) Asql_LET
			    //     ENDWHILE
			    //
			    // (2) Asql_LET
			    //     ENDIF
			    qsError = 7008;
			    return  FALSE;
			}
			exExActGotoRem = eatSp[iSTop--];
			if( exExActGotoRem->gotoStm != NULL )
			{       //if no ELSE, FALSE statement will be the
				//following statement
				//make the NEXT of IF statement to be the
				//next statement
				exExActGotoRem = NULL;
			}

			eatTailPoint[++iTail] = exExAct;
		    } else {
			exExActGotoRem = eatSp[i];
			if( exExActGotoRem->gotoStm != NULL ) {
				exExActGotoRem = NULL;
			}

			eatTailPoint[++iTail] = exExAct;

			//virtual delete the stack item
			eatSp[i] = &nullEAT;
		    }
		    exExActRem = NULL;
		}
		break;
		case Asql_ENDWHILE:
		{
		    short  i;

		    //look for Asql_WHILE
		    //the stack now could be:
		    // | Asql_WHILE
		    // | Asql_IF__ <--- filled already
		    // | Asql_EXIT
		    // | Asql_LOOP
		    for( i = iSTop;  i >= 0;  i-- )
		    {
			if( eatSp[i]->stmType == Asql_WHILE )
				break;
		    }

		    if( i < 0 ) {
			//endwhile without while
			qsError = 7004;
			return  FALSE;
		    }

		    {	//WHILE
			//ENDWHILE
			//there must be a statement between WHILE and ENDWHILE

			if( (exExAct = genGramMan()) == NULL )
				return  FALSE;

                        if( firstSentense ) {
                            firstSentense = 0;
                            if( nLevel == 'b' )
                                exExAct->nLevel = 'E';
                            else if( nLevel == 'a' )
                                 exExAct->nLevel = 'F';
                        } else {
		            exExAct->nLevel = nLevel;
                        }
			exExAct->stmType = Asql_GOTO;

			if( exExActRem != NULL ) {
				exExActRem->execNext = exExAct;
			} else {
				for( ;   iTail >= 0;  iTail-- ) {
				    if( eatTailPoint[iTail] == NULL )
				    { //segment flag
					iTail--;
					break;
				    }
				    eatTailPoint[iTail]->execNext = exExAct;
				}

				if( exExActGotoRem != NULL ) {
					exExActGotoRem->gotoStm = exExAct;
					exExActGotoRem = NULL;
				}
			    } //give upper statement goto statement
		    }
		    exExAct->execNext = exExActGotoRem = eatSp[i];

		    for( ;  iSTop >= i;  iSTop-- ) {
			switch( eatSp[iSTop]->stmType ) {
			    case Asql_EXIT:
				eatTailPoint[++iTail] = eatSp[iSTop];
				break;
			    case Asql_LOOP:
				eatSp[iSTop]->execNext = eatSp[i];
			}
		    }
		    exExActRem = NULL;
		}
		break;
		case Asql_EXIT:
		{
			//exExActGotoRem is the WHILE's false action
			if( exExAct->stmType != Asql_LET && \
					exExAct->stmType != Asql_ACTION )
			{
			    if( (exExAct = genGramMan()) == NULL )
				return  FALSE;

                            if( firstSentense ) {
                                firstSentense = 0;
                                if( nLevel == 'b' )
                                    exExAct->nLevel = 'E';
                                else if( nLevel == 'a' )
                                    exExAct->nLevel = 'F';
                            } else {
		                exExAct->nLevel = nLevel;
                            }
			    exExAct->stmType = Asql_EXIT;
			    if( exExActRem != NULL ) {
				exExActRem->execNext = exExAct;
			    } else {
				for( ;   iTail >= 0;  iTail-- ) {
				    if( eatTailPoint[iTail] == NULL )
				    { //segment flag
					iTail--;
					break;
				    }
				    eatTailPoint[iTail]->execNext = exExAct;
				}
			    }

			    if( exExActGotoRem != NULL ) {
				exExActGotoRem->gotoStm = exExAct;
				exExActGotoRem = NULL;
			    }
			}
			eatSp[ ++iSTop ] = exExAct;
		}
		break;
		case Asql_LOOP:
		{
		    if( exExAct == NULL ) {
			//loop without while
			qsError = 7005;
			return  FALSE;
		    }

		    //test wether it is in a repeator
		    for( i = iSTop;  i >= 0;  i-- )
		    {
			if( eatSp[i]->stmType == Asql_WHILE )
				break;
		    }

		    if( i < 0 ) {
			//loop without while
			qsError = 7005;
			return  FALSE;
		    }

		    //let the last statement's next statement to be the WHILE.
		    //if there is no LET statement, generate an empty one
		    //exExActGotoRem is the WHILE's false action
		    if( exExAct->stmType != Asql_LET && \
					exExAct->stmType != Asql_ACTION )
		    {
			    if( (exExAct = genGramMan()) == NULL )
				return  FALSE;

			    if( firstSentense ) {
				firstSentense = 0;
				if( nLevel == 'b' )
				    exExAct->nLevel = 'E';
				else if( nLevel == 'a' )
				    exExAct->nLevel = 'F';
			    } else {
				exExAct->nLevel = nLevel;
			    }
			    exExAct->stmType = Asql_LOOP;

			    //indenpendednt statements appeared
			    if( exExActRem != NULL ) {
				exExActRem->execNext = exExAct;
			    } else {
				for( ;   iTail >= 0;  iTail-- ) {
				    if( eatTailPoint[iTail] == NULL )
				    { //segment flag
					iTail--;
					break;
				    }
				    eatTailPoint[iTail]->execNext = exExAct;
				}
			    }

			    if( exExActGotoRem != NULL ) {
				exExActGotoRem->gotoStm = exExAct;
				exExActGotoRem = NULL;
			    }
		    }
		    exExAct = eatSp[i];
		}
		break;
		case Asql_CALL:
		{ //later program
		     short      wordlength;
		     short 	i;

		     //get function Name
		     if( (wordlength = SearchWordEnd()) == 0 )
			return  FALSE;

		     for(i = 0;  i < fFrTo.funNum;  i++) {
			if( strnicmp(fFrTo.asqlFun[i].funName, \
				    wWord->pcUsingPointer, wordlength) == 0 )
				break;
		     }
		     if( i >= fFrTo.funNum ) {
			qsError = 7006;
			return  FALSE;
		     } else {
			    if( (exExAct = genGramMan()) == NULL )
				return  FALSE;

                            if( firstSentense ) {
                                firstSentense = 0;
                                if( nLevel == 'b' )
                                    exExAct->nLevel = 'E';
                                else if( nLevel == 'a' )
                                    exExAct->nLevel = 'F';
                            } else {
		                exExAct->nLevel = nLevel;
                            }
			    exExAct->stmType = Asql_CALL;
			    exExAct->gotoStm = fFrTo.asqlFun[i].exExAct;

			    if( exExActRem != NULL ) {
				exExActRem->execNext = exExAct;
			    } else {
				for( ;   iTail >= 0;  iTail-- ) {
				    if( eatTailPoint[iTail] == NULL )
				    { //segment flag
					iTail--;
					break;
				    }
				    eatTailPoint[iTail]->execNext = exExAct;
				}
			    }

			    if( exExActGotoRem != NULL ) {
				exExActGotoRem->gotoStm = exExAct;
				exExActGotoRem = NULL;
			    }
		     }

		     exExActRem = exExAct;
		     wWord->pcUsingPointer += wordlength;

		}
		break;

		case Asql_REM:
		     if( RemarkPro() == FALSE )
			return  FALSE;
		break;

		case Asql_REM2:
		    if( Remark2Pro() == FALSE )
			return  FALSE;
		break;

		default:
		{
		     wWord->unget = 1;
		     return  TRUE;
		}
	    } //end of switch

	    if( GetWord() == 0 )   return  FALSE;
    }//end of while

    return  TRUE;

} // end of programPro()



/*==============
*                               bExactStrCmp()
*===========================================================================*/
signed char  bExactStrCmp(const char *s1, const char *s2, short maxlen)
{
    /*short 		 i;
    register signed char _AL, _AH;

    for( i = 0;  i < maxlen;  i++ ) {
	_AL = s1[i];
	_AH = s2[i];
	if( _AL && _AH ) {
	    if( _AL != _AH )
	    {
		if( _AL == ' ' ) {
			return  (signed char)1;	// ' ' is the biggest
		}
		if( _AH == ' ' ) {
			return  (signed char)-1;	// ' ' is the biggest
		}
		return _AL - _AH;
	    }
	    //continue to compare the next char
	} else {
	    if( _AL )	return  (signed char)1;
	    if( _AH )	return  (signed char)-1;
	    return	(signed char)0;
	}
    }

    return  (signed char)0;
    */

    while (--maxlen && *s1 && *s1 == *s2)
    {
	s1++;
	s2++;
    }

    return( *(signed char *)s1 - *(signed char *)s2 );


} // end of function bExactStrCmp()


/*==============
*                               bFuzzyStrCmp()
*===========================================================================*/
signed char  bFuzzyStrCmp(const char *s1, const char *s2, short maxlen)
{
    short i;

#ifndef __BORLANDC__
    register signed char _AL, _AH;
#endif

#ifdef GeneralSupport
    for( i = 0;  i < maxlen;  i++ ) {
	if( s1[i] != s2[i] && s1[i] != '*' && s2[i] != '*' )
	{
		if( s1[i] && s2[i] )		return s1[i] - s2[i];
		return	0;
	}
    }
#else

    for( i = 0;  i < maxlen;  i++ ) {
	_AL = s1[i];
	_AH = s2[i];
	if( _AL && _AH ) {
	    if( _AL != _AH )
	    {
		/*if( _AL == ' ' ) {
			return  (signed char)1;	// ' ' is the biggest
		}
		if( _AH == ' ' ) {
			return  (signed char)-1;	// ' ' is the biggest
		}*/
		return _AL - _AH;
	    }
	    //continue to compare the next char
	} else {
	    if( i > 0 )
		return	(signed char)0;
	    return  _AL - _AH;
	}
    }

#endif

    return  (signed char)0;


} // end of function bFuzzyStrCmp()

/////////////////
//    a_token string
//    ^ .... ^
//    ui     ui1+ui
//////////////////////////////////////////////////////////////////////////
unsigned short getAtokenFromBuf(char *sz, int tokenBufLen, char *tokenBuf)
{
    unsigned short ui = 0;
    unsigned short ui1;
    int            tokenBufGot = 0;
    char           buf[512];

    tokenBuf[0] = '\0';

    for(ui = 0;  sz[ui] == ' ' || sz[ui] == '\t';  ui++);

    if( sz[ui] == '"' ) {
	for( ;  sz[ui] != '\0';  ui++)
	{
	    if( sz[ui] == '"' && sz[ui-1] != '\\' ) {
		strZcpy(buf, &(sz[1]), min(510, (int)ui));
		cnStrToStr(tokenBuf, buf, '\\', (short)tokenBufLen);
		tokenBufGot = 1;
		break;
	    }
	}
	ui1 = strcspn( &(sz[ui+1]), ", \t" );
    } else {
	ui1 = strcspn(&sz[ui], ", \t");
    }

    if( ui1 == 0 ) {
	if( *sz == '\0' )
	    return  0;
    }

    if( tokenBufGot == 0 ) {
	strZcpy(tokenBuf, sz+ui, min(tokenBufLen, ui1+1));
    }

    ui += ui1;

    return  ui;

} //end of getAtokenFromBuf()



#ifndef bStrCmpOptimized
/*==============
*                               bStrCmp()
*===========================================================================*/
short bStrCmp(const char *s1, const char *s2, short maxlen)
{
/*    short i;

#ifndef __BORLANDC__
    register signed char _AL, _AH;
#endif
*/
#ifdef GeneralSupport
    for( i = 0;  i < maxlen;  i++ ) {
	if( s1[i] != s2[i] && s1[i] != '*' && s2[i] != '*' )
	{
		if( s1[i] && s2[i] )		return s1[i] - s2[i];
		return	0;
	}
    }
#else
    /*
    for( i = 0;  i < maxlen;  i++ ) {
	_AL = s1[i];
	_AH = s2[i];
	if( _AL && _AH ) {
	    if( _AL != _AH )
	    {
		if( _AL == ' ' ) {
			return  (signed char)1;	// ' ' is the biggest
		}
		if( _AH == ' ' ) {
			return  (signed char)-1;	// ' ' is the biggest
		}
		return _AL - _AH;
	    }
	    //continue to compare the next char
	} else {
	    return	(signed char)0;
	}
    }
    */
    while (--maxlen && *s1 && *s1 == *s2)
    {
	s1++;
	s2++;
    }

    return( *(unsigned char *)s1 - *(unsigned char *)s2 );

#endif

    /*return  (signed char)0;*/


} // end of function bStrCmp()
#endif







/*---------------
 *  Function Name:      TextPro()
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short   TextPro( void )
{
    short nVarLength;
    short i;
    char  buf[128];
    char  *sz;

    nVarLength = SearchVarChar("\n\r");

    if( nVarLength >= 32 )
       i = 32;
    else
       i = nVarLength + 1;
    strZcpy(buf, (char *)wWord->pcUsingPointer, i);

    wWord->pcUsingPointer += nVarLength + 1;

    if( *(wWord->pcUsingPointer) == '\n' || *(wWord->pcUsingPointer) == '\r' )
	(wWord->pcUsingPointer)++;
    
    //find the end mark \nENDTEXT or \rENDTEXT
    for( sz = wWord->pcUsingPointer;  *sz != '\0';  sz++) {
	if( *sz == '\n' || *sz == '\r') {
	    if( strnicmp(sz+1, "ENDTEXT", 7) == 0 )
		break;
	}
    }

    if( *sz == '\0' ) {
	qsError = 7010;
	return  FALSE;
    }

    if( addOneVarToSybTab(buf, wWord->pcUsingPointer, sz-wWord->pcUsingPointer) == FALSE ) {
	
	if( qsError == 0 )
	    qsError = 1001;
	return  FALSE;
    }

    wWord->pcUsingPointer = sz + 1 + 7;

    return TRUE;

} // end of TextPro



/*---------------
 *  Function Name:      addOneVarToSybTab()
 *  Return: sucess: TRUE
 *          unsucess: FALSE
 *  qsError:
 *-------------------------------------------------------------------------*/
static short addOneVarToSybTab(char *szTextName, char *sp, int iSzLen)
{
    SysVarOFunType  *SysVar;
    char            *sz1;

    if( (fFrTo.sHead = (SysVarOFunType *)realloc( fFrTo.sHead, \
					(fFrTo.nSybNum+1) * \
					sizeof(SysVarOFunType) ) ) == NULL ) {
	qsError = 1012;
	return  FALSE;
    }

    SysVar = fFrTo.sHead;
    strZcpy(SysVar[fFrTo.nSybNum].VarOFunName, szTextName, 32);
    
    SysVar[fFrTo.nSybNum].type = STRING_IDEN;
    SysVar[fFrTo.nSybNum].length = iSzLen;

    if( iSzLen < MAX_OPND_LENGTH ) {
	strZcpy(SysVar[fFrTo.nSybNum].values, sp, iSzLen+1);    
    } else {
	if( (sz1 = malloc(iSzLen+16)) == (long)NULL ) {
	    qsError = 1012;
	    return  FALSE;
	}
	*(long *)(SysVar[fFrTo.nSybNum].values) = (long)sz1;
	strZcpy(sz1, sp, iSzLen+1);
    }
    
    fFrTo.nSybNum++;

    return  TRUE;

} //addOneVarToSybTab()



/*************** end of file asqlana.c  Copyright: Xilong Pei ***************/
