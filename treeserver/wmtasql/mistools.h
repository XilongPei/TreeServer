/*********
 *  mistools.h
 *
 *
 * Copywright: Shanghai Institute of Railway Technology  1991, 1992
 *********************************************************************/

#ifndef _IncludedMistoolsProgramHead_
#define _IncludedMistoolsProgramHead_	1.01

#ifndef _IncludedDIOProgramHead_
#include "dio.h"
#endif
#include "xexp.h"

#define MisMaxCodeLength 	16
#define MAX_OPDBF	 	8

extern  char FLAG_subDbfCarry;

typedef struct tagDateType {
	short year;
	char month;
	char day;
} DATETYPE;


DATETYPE  DateMinusToDate( char * Date1, char *Date2, char *Result );
    /*
    */
long  DateMinusToLong( char *Date1, char *Date2);
    /*
    */
long  DateAddToLong( char *Date, long DayNum, char *Result );
    /*
    */
dFILE *  dTableSum(dFILE *dft, dFILE *dfs);
    /*
    */
short  DbfUniteOneToMore( short DbfNum, char *TargetDbf, \
						char *szField, char *dbf[] );
    /*
    */
short  DbfInterSection( short DbfNum, char *TargetDbf, \
						char *szField, char *dbf[] );
short  DbfCarryDbfs( short DbfNum, char *TargetDbf, \
			   char *szField, char *delFld, char *dbf[] );
long DbfLocateKey(dFILE *df, short field, char *key);
long DbfLocateExpr(dFILE *df, MidCodeType *m_c_code);
long DbfLocateSexpr(dFILE *df, char *szExpr);

short ModDbf( dFILE *df1, dFILE *df2 );

#endif
