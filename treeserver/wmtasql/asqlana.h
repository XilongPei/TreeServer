/***************************************************************************\
 * ASQLANA.H
 * Author: LianChun Song    1992.4
 * Rewrite by Xilong Pei    1993.6 1997.10
 *
 * Copyright: SRIT MIS Research. 1992-1993
 *            East-Union Computer Service Co., Ltd. 1993-1996
 *            CRSC 1997
 *	      Shanghai Withub Software Co., Ltd. 1999-2000
\***************************************************************************/

#ifndef _IncludedASQLANAProgramHead_
#define _IncludedASQLANAProgramHead_        "AsqlAna v2.00"

#include <stdio.h>
#include "dir.h"

#include "dio.h"
#include "btree.h"

/* define struct or data for query && statistics moudel */
#ifndef TRUE
	#define TRUE         1
#endif

#ifndef FALSE
	#define FALSE        0
#endif

#define Asql_MAX_TITLELENGTH    80
#define Asql_FILENAMELENGTH     260        // include path and the tail '\0'
#define ASQL_MAX_REL_EXPR_LEN	512
#define Asql_KEYWORDLENGTH      256
#define Asql_MAXBEGINENDNESTDEEP        100

#ifdef __BORLANDC__
   #define Asql_CHARBUFFSIZE      4096
#else
   #define Asql_CHARBUFFSIZE      16384
#endif

//2000.3.5 Xilong Pei
//#define MOST_DBFNUM_ONCEQUERY	10
#define MOST_DBFNUM_ONCEQUERY	64

#define AsqlExprInMemory        0
#define AsqlExprInFile          1
#define Asql_USEENV		2

// separator symbol
#define Asql_SEPSYMB          '$'
#define Asql_STATSYMB         '#'

#define Asql_RETAINTYPE       1001
#define Asql_EXPRTYPE         1002
#define Asql_ACTITYPE         1003
#define Asql_STRINGTYPE       1006
#define Asql_FILEEND          1007

#define Asql_STANDARDREAD     1201
#define Asql_STRINGREAD       1202

#define Asql_STATISTWAY         1
#define Asql_QUERYWAY           2
#define Asql_GROUPYES           1
#define Asql_GROUPNOT           2

#define Asql_LINEEXPR           1
#define Asql_COLUEXPR           2


#define SIZEOF_AUDITINFO	4


#include "xexp.h"

typedef   SysVarOFunType  SymTable;

typedef struct _word {
    char name[Asql_FILENAMELENGTH];     // file name and path
    FILE  *fp;                          // handle of input data file such as QS.ECF
    
    //short  nCol;                        // word start postion in qs.ecf
    //short  nLine;
    int    nCol;                        // word start postion in qs.ecf
    int    nLine;
    
    //short  nWordLength;                 // word length
    int    nWordLength;                 // word length

    short  nReadMethod;                 // read way
    short  nType;                       // word type
    short  nInterNumberal;              // word inter numberal
    MidCodeType *xPointer;              // word middle code type
    
    //unsigned short  nCharNum;
    int    nCharNum;			//let it allowed the script exceed 64K

    short  nFlush;                      // AsqlExpr type
    short  nStopRead;                   // StatementsAllInMemory
    short  unget;
    short  keyWordPossible;
    unsigned char *pcWordAnaBuf;        // buffer pointer
    unsigned char *pcUsingPointer;      // using char pointer
} Word;

// this define the size of the following array
//#define Asql_RETAINNUM        29


// the position of INT in retainarry
#define TYPERETAINWORDBEGIN   40
#define Asql_INT              (TYPERETAINWORDBEGIN)
#define Asql_LONG             (TYPERETAINWORDBEGIN+1)
#define Asql_FLOAT            (TYPERETAINWORDBEGIN+2)
#define Asql_CHAR	      (TYPERETAINWORDBEGIN+3)
#define Asql_STRING	      (TYPERETAINWORDBEGIN+4)
#define Asql_DATE	      (TYPERETAINWORDBEGIN+5)
#define Asql_INT64	      (TYPERETAINWORDBEGIN+6)
#define TYPERETAINWORDEND     (TYPERETAINWORDBEGIN+7)


#define Asql_NULL             -1


#define Asql_ACTION           1101
#define Asql_AVERAGE	      1102
#define Asql_BEGIN            1103
#define Asql_CALL	      1104
#define Asql_COMMIT	      1105
#define Asql_CONDITION_PRA    1106
#define Asql_DATABASE         1107
#define Asql_DEFINE           1108
#define Asql_ELSE	      1109
#define Asql_END              1110

#define Asql_ENDEXCEPT        1111

#define Asql_ENDIF	      1112
#define Asql_ENDWHILE	      1113
#define Asql_EXCEPT	      1114

#define Asql_EXCLUSIVE	      1115
#define Asql_EXIT	      1116
#define Asql_FROM             1117
#define Asql_FUNCTION	      1118
#define Asql_GOTO	      1119
#define Asql_GROUPACTION      1120
#define Asql_GROUPBY          1121
#define Asql_IF	      	      1122
#define Asql_LABEL	      1123
#define Asql_LET	      1124
#define Asql_LOOP	      1125
#define Asql_MAX	      1126
#define Asql_MIN	      1127
#define Asql_MODISTRU	      1128

#define Asql_ORDERBY	      1129

#define Asql_PREDICATES       1130
#define Asql_RETURN	      1131

#define Asql_SET       	      1132
#define Asql_STATISTICS       1133
#define Asql_SUMMER	      1134
#define Asql_TEXT	      1135
#define Asql_TITLE            1136
#define Asql_TO               1137
#define Asql_TRY              1138

#define Asql_UPDATE           1139
#define Asql_WHILE	      1140

#define Asql_REM	      9999
#define Asql_REM2	      9998

#define Asql_EXPRESSION       0
#define Asql_ACTIONEXPR       1

/* the defination defined in ver. 1
typedef struct _expr {
	MidCodeType  *xPointer;
	short nFirstAction;
	short nSecondAction;
	short nNextParallelExpr;
	struct _expr *eNext;
} ExprTable;
*/
typedef struct _expr {
	short        ExprTableFlag;     // 0xFFFF always. 1st item always !!!
	char	     cFirstActionType;  //0, normal, 1: stat or sum x action
	char	     cSecondActionType; //no use, now!
	MidCodeType  *xPointer;
	MidCodeType  *nFirstAction;
	MidCodeType  *nSecondAction;
	MidCodeType  *nNextParallelExpr;
	struct _expr *eNext;
} ExprTable;

typedef struct _act {
	MidCodeType *pxActMidCode;
	short nCalTimes;
	short actionFunRefered;
	struct _act *aNext;
} CalActOrdTable;

typedef struct _actexor {        	 //action exexute order
	MidCodeType *xPoint;		 //expression pointer
	char  	    nLevel;              //order sign: B.efore A.head G.roup
	char  	    regEd;		 //already register
	short 	    stmType;		 //statement type
	struct _actexor *gotoStm;	 //IF WHILE false jmp
	struct _actexor *execNext;	 //the next run statement
	struct _actexor *oNext;          //link them for memory free
} ExecActTable;

typedef struct _tagFun {
	char 	      funName[32];
	ExecActTable *exExAct;
} ASQLFUN;

typedef struct _tagLABEL {
	char 	     label[32];
	ExprTable   *etPoint;
} ASQLLABEL;

typedef struct _tagAUDIT {
	short	    dbId;	//database id, no use
	short	    tbId;	//table id
} ASQLAUDIT;

#define Asql_MAXFUN 	32
#define Asql_MAXLABEL   32

typedef struct _frto {
	char cTitleName[Asql_MAX_TITLELENGTH];  // title
	dFILE **cSouFName;                      // query source file name
	short cSouDbfNum;                         // query DBF number
	char  szKeyField[ASQL_MAX_REL_EXPR_LEN];

	char cTargetFileName[ Asql_FILENAMELENGTH ]; // target filename
	dFIELD *TargetField;                    // target field dFIELD table
	FIELDNAMETYPE *fieldtable;              // target field string table
	short  nFieldNum;                       // target field number
	dFILE *targefile;                       // target file dFILE
	bHEAD *tTree;

	char nGroupby;                          // group by sign
	char cGyExact;
	char nGroupAction;			// groupaction appears
	char cGroKey[ Asql_KEYWORDLENGTH ];     // group by key words

	ExecActTable   *exHead;                 // action expression head
	ExprTable      *eHead;                  // expression head
	SYSDEFINETYPE  *dHead;                  // define table
	short          nDefinNum;               // defination number
	CalActOrdTable *cHead;                  // calculate table
	SysVarOFunType *sHead;                  // variable table
	short nSybNum;                          // variable number

	ASQLFUN asqlFun[Asql_MAXFUN+1];
	short   funNum;

	ASQLLABEL asqlLabel[Asql_MAXLABEL+1];
	short   labelNum;
	char    jmpLabel[32];

	FILE    *phuf;                            //handle of update file
	bHEAD   *syncBh[MOST_DBFNUM_ONCEQUERY];
	MidCodeType *xKeyExpr[MOST_DBFNUM_ONCEQUERY];

	bHEAD   *distinctBh;

	char    toAppend;

	char 	szScopeKey[ Asql_KEYWORDLENGTH ];       // scope key words
	char 	szScopeStart[ Asql_KEYWORDLENGTH ];     // scope start keyword
	char 	szScopeEnd[ Asql_KEYWORDLENGTH ];       // scope start keyword

	char    nextMainRec;
	char    mtSkipThisSet[MOST_DBFNUM_ONCEQUERY];
	char    fromExclusive;

	char    AsqlDatabase[32];
	char    auditStr[MOST_DBFNUM_ONCEQUERY * SIZEOF_AUDITINFO + 1];
						//one table has 4 byte audit
						//information

	short   iScopeSkipAdjust;		//-2:decending, 0:ascending

	short   wGbKeyLen;

	char    nOrderby;                          // group by sign
	char    cOrderbyKey[ Asql_KEYWORDLENGTH ]; // group by key words
	short   wObKeyLen;
	short   iOrderbyScopeSkipAdjust;
	//bHEAD   *orderbyBh;

	char	relationOff;
	long    *lRecNo;
	char    *szLocateStr[MOST_DBFNUM_ONCEQUERY];

	int	insideInt;
//	struct _frto *next;                       // next pointer
} FromToStru;

typedef struct tagAsql_ENV {
	char szAsqlResultPath[MAXPATH];
	char szAsqlFromPath[MAXPATH];
	char szUser[32];
} Asql_ENV;

#ifdef _InTJCXProgramBody_
	WSToMT	unsigned short qsError;
	WSToMT	char szAsqlErrBuf[256];
	WSToMT	Asql_ENV       asqlEnv = {"", ""};
	Asql_ENV asqlConfigEnv = {"", ""};
	char 	 *szGroupFileExt = ".ONE";

//		        A,B,C,D,E,F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, -
short  asqlKeyHash[27]={0,2,3,6,8,16,18,21,21,22,22,22,25,28,28,29,30,30,31,34,38,39,39,40,40,40,40};
static char *retainarry[] = {
/* 0*/	"ACTION", \
/* 1*/	"AVERAGE", \
/* 2*/	"BEGIN", \

/* 3*/	"CALL", \
/* 4*/  "COMMIT", \
/* 5*/	"CONDITION",\

/* 6*/	"DATABASE", \
/* 7*/	"DEFINE", \
/* 8*/	"ELSE", \
/* 9*/	"END", \
/*10*/  "ENDEXCEPT", \

/*11*/	"ENDIF", \
/*12*/	"ENDWHILE", \
/*13*/	"EXCEPT", \

/*14*/	"EXCLUSIVE", \
/*15*/	"EXIT", \
/*16*/	"FROM", \
/*17*/	"FUNCTION", \

/*18*/	"GOTO", \
/*19*/	"GROUPACTION", \
/*20*/	"GROUPBY", \

/*21*/	"IF", \
/*22*/	"LABEL", \
/*23*/	"LET", \
/*24*/	"LOOP", \
/*25*/	"MAX", \
/*26*/	"MIN", \
/*27*/	"MODISTRU", \

/*28*/  "ORDERBY",

/*29*/	"PREDICATES", \
/*30*/	"RETURN", \

/*31*/  "SET", \
/*32*/	"STATISTICS", \
/*33*/	"SUMMER", \

/*34*/  "TEXT", \
/*35*/	"TITLE", \
/*36*/	"TO", \
/*37*/	"TRY", \

/*38*/	"UPDATE", \
/*39*/	"WHILE", \

/*40*/	"INT", \
/*41*/	"LONG", \
/*42*/	"FLOAT", \
/*43*/	"CHAR", \
/*44*/	"STRING", \
/*45*/	"DATE", \
/*46*/  "INT64"
};
#else
	extern WSToMT	unsigned short qsError;
	extern WSToMT	char szAsqlErrBuf[256];
	extern WSToMT	Asql_ENV       asqlEnv;
	extern Asql_ENV asqlConfigEnv;
	extern char 	 *szGroupFileExt;
	extern WSToMT	char           errorStamp;
	extern WSToMT	char           insideTask;
#endif

//------------------------- Function Prototype ----------------------------
short  AsqlAnalyse( char *pFilename, char *buf );
short  AsqlExecute( void );

// This module use with any normal user, just this function's call is enough
short  AskQS(char *FileName, short ConditionType, dFILE *FromTable, \
		    dFILE *ToTable, SysVarOFunType **VariableRegister, \
							short *VariableNum );
void InitAsqlEnv( void );
//-------------------------------------------------------------------------//

// if you want the asqlana to run this function, should give this define
// #define _AsqlRuningMessage_Assigned
extern char AsqlRuningMessage( long recno );
extern void AsqlAnaErrorMessage( char *msg );
extern void AsqlResultMessage( char *cxmain, long l );

#endif




/************************* end of asqlana.h *********************************/