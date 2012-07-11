/********
 * XEXP.H
 * Expression Process
 *
 * Copyright:
 *     Shanghai Withub Vision Software Co., Ltd. 2000
 **************************************************************************/

////////////////////////////////////////////////////////////////////////////
// _int32 scope: -2147483648~2147483647
// _int64 scope: -9223372036854775808~9223372036854775807
//                1234567890123456789 1234567890123456789
////////////////////////////////////////////////////////////////////////////

//max width in xexp.c long express number
#define _MAX_LONG_NUM_WIDTH   9


#include <windows.h>
#include <stdio.h>
#include <limits.h>
#include "dio.h"

/*!---------------------- Forehead Announce -----------------------------!*
 *!  User may define XexpCaseSensitive to distinguish character case     !*
 *!  User may define XexpDebugOn to make more check of expression        !*
 *!----------------------------------------------------------------------!*/

#ifndef _IncludedXEXPProgramHead_
#define _IncludedXEXPProgramHead_        "XEXP v1.00"

#define XEXP_FUNCTION_SERVICE

// importand defination!!! for we use normal function as action function
// 1994.12.8 Xilong
#define PD_style        cdecl


#define MAX_BUFFER_LENGTH       128
#define MAX_OPND_NUMBER         256
#define MAX_OPND_LENGTH         32            /* middle varable */
					      /* long double need 10 bytes */
#define MAX_ATOM_LENGTH         8             /* =sizeof(double) atom size in MidCodeType */

#define MAX_FUN_PARA		100

#define MAX_IDEN_LENGTH         (MAX_OPND_LENGTH-1)
#define MAX_OPTR_NUMBER         64
#define FLOAT_PRECISION		2.2204460492503131e-016
#define FLOAT_MAX               1.7E+300
#define FLOAT_MIN               -1.7E+300
#define _ArrayOrFunNestDeep_    8

//first time of action is 0
#define LASTWORKTIMEOFACTION    -1
#define ONE_MODALITY_ACTION     -2
#define CONSTRUCTION_ACTION	0
#define NORMAL_ACTION		1
#define GROUPBY_ACTION		8


//operator define must consider the priority table,
//for example less than 100
#define OR_TYPE         0
#define AND_TYPE        1
#define NOT_TYPE        2
#define EQ_TYPE         3
#define ABSOLUTE_EQ     4
#define GT_TYPE         5
#define LT_TYPE         6
#define GE_TYPE         7
#define LE_TYPE         8
#define NEQ_TYPE        9
#define ADD_TYPE        10
#define SUB_TYPE        11
#define MUL_TYPE        12
#define DIV_TYPE        13
#define MOD_TYPE        14
#define BITR_TYPE       15
#define BITAND_TYPE     16
#define BITOR_TYPE      17
#define BITNOT_TYPE     18
#define BITXOR_TYPE     19
#define LEFT_TYPE       20
#define RIGHT_TYPE      21
#define FUNB_TYPE       22          /* function call begin type */
#define FUNE_TYPE       23          /* function end type        */
//we change this defination 1995.12.16, for we deal NEG_TYPE with
//data_deal_method in xexp.c
//#define NEG_TYPE        24          /* - */
#define STORE_TYPE      25          /* := */
#define PARAL_TYPE      26          /* ,; */
#define QUSTN_TYPE      27          /* ? */
#define LABEL_TYPE      28          /* L:*/
#define ARRAYB_TYPE     29          /* [ */
#define ARRAYE_TYPE     30          /* ] */
#define END_TYPE        31
#define INSIDE_TYPE     999

//data type
#define IDEN_TYPE    1050
#define STRING_IDEN  1051
#define DATE_IDEN    1052
#define CHR_IDEN     1053
#define INT_IDEN     1054
#define LONG_IDEN    1055
#define FLOAT_IDEN   1056
#define INT64_IDEN   1057

#define FIELD_IDEN   1060
#define NFIELD_IDEN  1061       // dec is 0 and bytes less than 10, long
#define FFIELD_IDEN  1062       // dec isnot 0, float
#define LFIELD_IDEN  1063       // logic, short
#define DFIELD_IDEN  1064       // date, char[8]
#define MFIELD_IDEN  1065       // memo
#define CFIELD_IDEN  1066       // chr[n]
#define SFIELD_IDEN  1067       // recno()
//#define INT64FIELD_IDEN 1068

#define CHR_TYPE     1070
#define INT_TYPE     1071		//2147483647~-2147483648
#define LONG_TYPE    1072
#define FLOAT_TYPE   1073
#define INT64_TYPE   1074		//2000.3.3 Added
#define DATE_TYPE    1075
#define STRING_TYPE  1076
#define LOGIC_TYPE   1077
#define ARRAY_TYPE   1078

//ONE_PARA_OPTR_TYPE is NEG_TYPE
//I have seen one parameter optr again, I believe that it should beside '+' ,'-'...
#define ONE_PARA_OPTR_TYPE	1124
#define NEG_TYPE     1124          /* - */
#define VARADDR_TYPE 1125          /* & */


#define VIDEN_TYPE    (IDEN_TYPE  | 0x1000)
#define VSTRING_IDEN  (STRING_IDEN| 0x1000)
#define VDATE_IDEN    (DATE_IDEN  | 0x1000)
#define VCHR_IDEN     (CHR_IDEN   | 0x1000)
#define VINT_IDEN     (INT_IDEN   | 0x1000)
#define VLONG_IDEN    (LONG_IDEN  | 0x1000)
#define VFLOAT_IDEN   (FLOAT_IDEN | 0x1000)
#define VINT64_IDEN   (INT64_IDEN | 0x1000)

#define VFIELD_IDEN   (FIELD_IDEN | 0x1000)
#define VNFIELD_IDEN  (NFIELD_IDEN| 0x1000)
#define VFFIELD_IDEN  (FFIELD_IDEN| 0x1000)
#define VLFIELD_IDEN  (LFIELD_IDEN| 0x1000)
#define VDFIELD_IDEN  (DFIELD_IDEN| 0x1000)
#define VMFIELD_IDEN  (MFIELD_IDEN| 0x1000)
#define VCFIELD_IDEN  (CFIELD_IDEN| 0x1000)
#define VSFIELD_IDEN  (SFIELD_IDEN| 0x1000)
#define VCHR_TYPE     (CHR_TYPE   | 0x1000)
#define VINT_TYPE     (INT_TYPE   | 0x1000)
#define VLONG_TYPE    (LONG_TYPE  | 0x1000)
#define VFLOAT_TYPE   (FLOAT_TYPE | 0x1000)
#define VINT64_TYPE   (INT64_TYPE | 0x1000)
#define VDATE_TYPE    (DATE_TYPE  | 0x1000)
#define VSTRING_TYPE  (STRING_TYPE| 0x1000)
#define VLOGIC_TYPE   (LOGIC_TYPE | 0x1000)
#define VARRAY_TYPE   (ARRAY_TYPE | 0x1000)
#define VNEG_TYPE     (NEG_TYPE   | 0x1000)


#define ASQLActionFunType       40


/*********
 *  declare data types for programs
 ****************************************************************************/

/* NOTICE: the first 3 field cannot be changed position, it should be the
 * same order to the OpndType !!!
 * In C 16 bit type, every malloc at least is 16 byte
 **/
typedef struct tagMidCodeType {
	short   type;
	long    length;	    //short   length;
		/* the string's length or the function's parameter num
		 * when the type is IDEN_TYPE it stores the IDEN_TYPE's type
		 * when the type is ARRAY the length is the Dimmention
		 */
	struct  tagMidCodeType   *next;
	unsigned  char  values[MAX_ATOM_LENGTH];
		/* length variable item
		 * if it is a string which is longer than 8,
		 * it call for a bigger memory to store the string.
		 */
} MidCodeType;

#define OPND_STATIC_LENGTH      (sizeof(MidCodeType)-MAX_ATOM_LENGTH+MAX_OPND_LENGTH)

typedef struct {
	char  Mark;             /* '@'  */
	char  IntervalChar;     /* '\0', use for replace current field */
	unsigned short LibNo;
	unsigned short FieldNo;
} SPECIALFIELDSTRUCT;

typedef struct {
	unsigned char *pSourceStart;     /* this must be the first field */
	unsigned short wSourceid;
	dFILE    *pSourceDfile;
	unsigned char *pTargetStart;
	unsigned short wTargetid;
	dFILE    *pTargetDfile;
	void     *ResultMem;
} dFIELDWHENACTION;


/* NOTICE: the first 3 field cannot be changed position, it should be the
 * same order to the MidCodeType !!!
 **/
typedef struct {
	short           type;
	unsigned  char  values[ MAX_OPND_LENGTH ];
	long            length;		//2001.9.6 short length;
	unsigned  char  *oval;          // fast pointer
} OpndType;

typedef short     OptrType;

//xexp error string length
#define XES_LENGTH	132
typedef struct {
       short xERROR;                       // Error code
       char  string[ XES_LENGTH ];         // save the error symbol
} ErrorType;

enum Xexp_errors {              /* xexp error codes */
       XexpOK = 0,
       iSymbolUsedError,
       iUndefineVar,
       iNoMem,
       iUseTabel,
       iNoDBF,
       iNoField,
       iDBFNoInTabel,
       iActionParaError,
       iNoThisDataType,
       iNestToDepth,
       iNoMatchArray,
       iNoAction,
       iNoMatchFun,
       iInternalError,
       iInVaildOpndType,
       iToManyOPD,
       iToLittOPD,
       iToLittOPr,
       iToManyOPr,
       iFailFunCall,
       iOPrNoMatch,
       iUnExpectEnd,
       iBadUseOPr,
       iNoOPr,
       iToManySymbol,
       iOPrError,
       iMathError,
       iDimError,
       iFileRead,
       iFileWrite,
       iTypeNoCompt,
       iCircleNoMatch,
       iBadExp,
       iQuotoErr,
       iToolittleFP,
       iSubstrStartErr,
       iRelCntParaErr,
       iRecordNoTO,
       iStatXPt,
       iSumXPt,
       iSigmaYP,
       iSigmaXP,
       iFailCreateFile,
       iWsymbolParaLess,
       iMaxXPt,
       iMinXPt,
       iAvgXPt,
       iNoTargetField,
       iHZTHnoInit,
       iAsqlUserReadonly,
       iFailWriteRec,
       iRelSkipParaErr,
       iFailToLock,
       iSysvarParaErr,
       iSysParaErr,
       iBadUseOfSemicolon,
       iNoIndexToUseIn_uniquekey,
       iFirstParaErrIn_uniquekey,
       iRefTabelErr,
       iErrArrayUse,
       iStatArrayUseErr,
       iXEXPErrorTail
};

typedef short PD_style (*PFD)(OpndType *lpOpnd, short ParaNum, short *OpndTop);
typedef short PD_style (*ACTIONPFD)(OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					  short *CurState );

#define MAXARRAYDIM         ( MAX_OPND_LENGTH - 16 ) / 4
typedef struct tagArrayType {
	unsigned short ElementType;
	unsigned short DimNum;
	long 	       ElementNum;
	long 	       MemSize;
	long	       ArrayDim[ MAXARRAYDIM ];
	void           *ArrayMem;
} ArrayType;
//when DimNum is 0, it means:
//    	it is an variable array
//	ElementNum: the element number
//	MemSize: the memory size, which increased step is
//	decided by ElementType
//



typedef struct {
	short  type;
	char   VarOFunName[32];
	unsigned char values[ MAX_OPND_LENGTH ];
		/* (long)values()
		 * when the variable is ARRAY type the values stored as
		 * ArrayType
		 */
	long length;   //2001.9.6 short length;
} SysVarOFunType;

typedef struct {
	char DefineName[32];
	char DefineValues[32];
} SYSDEFINETYPE;

typedef struct {
	char FieldName[FIELDNAMELEN];
} FIELDNAMETYPE;

typedef struct {
	short  type;
	char VarOFunName[32];
	short PD_style (*FarProc)(OpndType *lpOpnd, short ParaNum, short *OpndTop);
	short varnum1;
	short varnum2;
} XexpFunType;

typedef struct {
	short  type;
	char   VarOFunName[32];
	short  PD_style (*FarProc)(OpndType *lpOpnd, short ParaNum, short *OpndTop, \
			 short *CurState );
	short  varnum1;
	short  varnum2;
} AskActionType;


typedef struct tagXEXP_ENV {
	short  optrStackFund;
	short  opndStackFund;
	short  optr_top;
	short  opnd_top;
} XEXP_ENV;


/*********
 *  declare global variables
 **************************************************************************/

//the user readonly variable number in SysVar[], the readonly variable must 
//be defined at the head of the table
#define SysVarAsqlUserReadonlyNum   2

#ifdef INrEXPRANALYSIS
WSToMT SysVarOFunType SysVar[] = {           /* function must at forward */
	{STRING_IDEN, "_ASQLUSER", "", 31},
	{STRING_IDEN, "_ASQLTO", "", 31},
	{INT_IDEN, "_QSERROR", "", -2},
	{INT_IDEN, "_XERROR", "", -2},
//	{LONG_IDEN, "_RECNO", "\0\0\0\0", 4},
//	{LONG_IDEN, "_LONGTMP", "\0\0\0\0", 4},
//	{FLOAT_IDEN, "_FLOATTMP", "\0\0\0\0\0\0\0\0", sizeof(double)},
//	{STRING_IDEN, "_STRINGTMP", "", 0}
};

short SysVarNum = sizeof(SysVar)/sizeof(SysVarOFunType);

//support askQS to PASS IN the variable table 1998.5.11
WSToMT short    nSelfBaseVar=0;
WSToMT SysVarOFunType *xSelfBaseVar=NULL;

WSToMT short rTYPE;
//WSToMT short xERROR;
WSToMT dFILE *askTdf = NULL;      /* self use */
static short myZero = 0;
WSToMT short  *ASKACTIONCURRENTSTATEshort = &myZero;      /* self use */

//the updateFlag is set by store_g() when it run
WSToMT short  updateFlag;

WSToMT short actionFunRefered;

#ifdef ENGLISH
char *XexpErrorMessage[] = {
	"No error detected",
	"Bad use or Unrencognized symbol ", // save the symbol in xchar
	"Undefine  symbol ",                // save the symbol in string
	"Out of Memory ",
	"Tabel must be specified ",
	"DBF not found",
	"Field not found",
	"DBF not int tabel",
	"Action parameter error",
	"Invalid data type",
	"Nest too depth",
	"Array call error",                 // too A]
	"Action not found",
	"Function call error",              // too f)
	"Internal error",
	"Opand type error",
	"InVaild opand",                    // to many opand
	"Expect an opand",                  // to little opand
	"Expect an operate symbol",
	"InVaild operate symbol",
	"Function call failure",
	"Operate symbol not match",
	"Unexpect end of expression",
	"InVaild used operate symbol",
	"Expect an operate symbol",
	"Additional symbol",
	"Operate error",                        //operate symbol in
	"Math operate error",
	"Array dimension type error",
	"Read only of file",
	"Write only of file",
	"Type not comptible",
	"Circle not match",
	"Expression error",
	"Quoto error",
	"No defined error",
	"Too little function parameter",
	"iSubstrStartErr",
	"iRelCntParaErr",
	"iRecordNoTO",
	"iStatXPt",
	"iSumXPt",
	"iSigmaYP",
	"iSigmaXP",
	"iXEXPErrorTail",
	"iWsymbolParaLess",
	"iMaxXPt",
	"iMinXPt",
	"iAverageXPt",
	"iNoTargetField",
	"iHZTHnoInit",
	"iAsqlUserReadonly",
	"iFailWriteRec",
	"iRelSkipParaErr",
	"iFailToLock"
	"iSysvarParaErr",
	"iSysParaErr",
	"iBadUseOfSemicolon",
	"iNoIndexToUseIn_uniquekey",
	"iFirstParaErrIn_uniquekey",
	"iRefTabelErr",
	"iErrArrayUse",
	"iStatArrayUseErr"
};
#else
char *XexpErrorMessage[] = {
	"",                                 //XexpOK "未发现错误",
	"变量引用错或使用不当",             //iSymbolUsedError save the symbol in xchar
	"变量未定义",                       //iUndefineVar save the symbol in string
	"内存不够",                         //iNoMem
	"必须指定数据库",                   //iUseTabel
	"变量没有声明或书写错",             //iNoDBF
	"找不到域",                         //iNoField
	"表内无此域",                       //iDBFNoInTabel
	"动作参数错",                       //iActionParaError
	"数据类型错",			    //iNoThisDataType
	"嵌套太深",                         //iNestToDepth
	"数组使用错",                       //iNoMatchArray too A]
	"无此动作",                         //iNoAction
	"函数调用错",                       //iNoMatchFun too f)
	"内部错",                           //iInternalError
	"操作数类型错",                     //iInVaildOpndType
	"操作数太多",                       //iToManyOPD too many opands
	"操作数太少",                       //iToLittOPD too few opands
	"缺运算符",                         //iToLittOPr
	"多运算符",                         //iToManyOPr
	"函数引用失败,操作数不足或类型不对",//iFailFunCall
	"操作符不符原型",                   //iOPrNoMatch
	"表达式不全",                       //iUnExpectEnd
	"运算符无效",                       //iBadUseOPr
	"少运算符",                         //iNoOPr
	"多余运算符",                       //iToManySymbol
	"操作错",                           //iOPrError operate symbol in
	"数学运算错",                       //iMathError
	"数组下标类型错",                   //iDimError
	"文件只读",                         //iFileRead
	"文件只写",                         //iFileWrite
	"类型不兼容",                       //iTypeNoCompt
	"括号不匹配",                       //iCircleNoMatch
	"表达式错",                         //iBadExp
	"引号错",                           //iQuotoErr
	"引用函数时参数不足",               //iToolittleFP
	"Substr()函数起始位置错误",         //iSubstrStartErr
	"RelCnt()参数错",                   //iRelCntParaErr
	"Record()引用，但没有目标定义",     //iRecordNoTO
	"Statx()第2个参数应该是数组",       //iStatXPt
	"Sumx()第2个参数应该是二维数组",    //iSumXPt
	"Sigmay()参数错",                   //iSigmaYP
	"Sigmax()参数错",                   //iSigmaXP
	"文件创建失败",                     //iFailCreateFile
	"wsymbol()参数错",                  //iWsymbolParaLess
	"xMax()第2个参数应该是二维数组",    //iMaxXPt
	"xMin()第2个参数应该是二维数组",    //iMinXPt
	"xAvg()第2,3个参数应该是二维数组，且相应维数定义相同", //iAvgXPt
	"对目标表赋值，但目标表不存在，或对源进行域赋值，但书写形式不是\"表名.域名\"形式", //iNoTargetField
	"代码库没有加载，hzth()",	    //iHZTHnoInit
	"_ASQLUSER为系统只读变量",          //iAsqlUserReadonly
	"写记录时数据检查出错",             //iFailWriteRec
	"RelSkip()参数错",                  //iRelSkipParaErr
	"资源锁定失败",                     //iFailToLock
	"函数Sysvar()引用时参数错",         //iSysvarParaErr
	"函数sys()参数错",
	"表达式中不正常使用了分号",
	"uniquekey()引用时，用来计算的表中没有设置好索引",
	"In_uniquekey第一个参数不是域名",
	"指定数据库不正确",                 //iRefTabelErr
	"数组访问越界",			    //iErrArrayUse
	"统计数组访问越界"		    //iStatArrayUseErr
};
#endif

#else
extern WSToMT dFILE  *askTdf;
extern WSToMT short  *ASKACTIONCURRENTSTATEshort;
extern WSToMT SysVarOFunType SysVar[];
extern short SysVarNum;
extern WSToMT short rTYPE;
//extern WSToMT short xERROR;
extern WSToMT ErrorType ErrorSet;
extern WSToMT short  updateFlag;
extern HINSTANCE    hLibXexp;
extern WSToMT short actionFunRefered;

extern char cEscapeXexp;


//DO NOT CHANGE THE four VARIABLE WITHOUT KNOW WHAT IT WILL CAUSE!!!
//---------------------------------------------------------------------------
extern WSToMT short opnd_top;
extern WSToMT short optr_top;

//set the following 2 variable for CalExpr call CalExpr itself
extern WSToMT short optrStackFund;
extern WSToMT short opndStackFund;
//---------------------------------------------------------------------------
//support askQS to PASS IN the variable table 1998.5.11
extern WSToMT short    nSelfBaseVar;
extern WSToMT SysVarOFunType *xSelfBaseVar;
//---------------------------------------------------------------------------


#endif
// define the position
#define SysVar_ASQLUSER		0
#define SysVar_ASQLTO		1
#define SysVar_QSERROR		2
#define SysVar_XERROR		3
//#define SysVar_RECNO            1
//#define SysVar_LONGTMP          2
//#define SysVar_FLOATTMP         3
//#define SysVar_STRINGTMP        4

/*********
 *  declare function prototypes
 *  Example For Calculate A Expression:
 *     1. word_ana(), get the middle code expression
 *        now you can select write_expr() to write the middle expression
 *        to diskette.
 *        You can use read_expr to replace step 1.
 *     2. check_grammar().
 *     3. iden_switch(), identify switch to their position. normally this
 *        is no selectable when function, variableor, array appears in
 *        expression
 *     4. cal_expr(), calculate a middle expression.
 **************************************************************************/

#ifdef INrEXPRANALYSIS
_declspec(dllexport) MidCodeType * PD_style WordAnalyse( unsigned char *buffer );
     /* expression word analysis, return the pointer of middle code expression,
      * return NULL if error found, rTYPE store the position.
      */
_declspec(dllexport) short PD_style FreeCode( MidCodeType *m_c_head );
     /* free the memory of expression expressed as MidCodeType
      */

_declspec(dllexport) MidCodeType * PD_style SymbolRegister( MidCodeType *m_c_head, dFILE *tb, \
			     SysVarOFunType *UserSymbol, unsigned short SymbolNum, \
			     SYSDEFINETYPE *DefineTable, unsigned short DefineNum );
     /* switch the identify ( sys variable and dFILE item ) to their
      * position.
      * success return NULL; else return the pointer of node where error found.
      */
_declspec(dllexport) long PD_style CalExpr( MidCodeType *m_c_head );
     /* calculate expression stored MidCodeType
      * success return the result. fail: return ONG_MAX
      */
_declspec(dllexport) long PD_style GetCurrentResult( void );
     /* get the current calculate result
      */
_declspec(dllexport) short PD_style GetCurrentResultType( void );
     /* get the current calculate result
      */
_declspec(dllexport) short PD_style GetUpdateFlag( void );
     /* get the current calculate result
      */
#else
_declspec(dllimport) MidCodeType * PD_style WordAnalyse( unsigned char *buffer );
_declspec(dllimport) short PD_style FreeCode( MidCodeType *m_c_head );
_declspec(dllimport) MidCodeType * PD_style SymbolRegister( MidCodeType *m_c_head, dFILE *tb, \
			     SysVarOFunType *UserSymbol, unsigned short SymbolNum, \
			     SYSDEFINETYPE *DefineTable, unsigned short DefineNum );
_declspec(dllimport) long PD_style CalExpr( MidCodeType *m_c_head );
_declspec(dllimport) long PD_style GetCurrentResult( void );
_declspec(dllimport) short PD_style GetCurrentResultType( void );
_declspec(dllimport) short PD_style GetUpdateFlag( void );
#endif

MidCodeType * PD_style ActionAnalyse( unsigned char *buffer, short ActionNo, \
			    SysVarOFunType **SymbolTable, short *SymbolNum, \
			    FIELDNAMETYPE **FieldTable, short *FieldNum, \
			    SYSDEFINETYPE *DefineTable, short *DefineNum );
     /*
      */
MidCodeType * PD_style CheckGrammar( MidCodeType *m_c_head, dFILE *tb, short FieldIndex,\
		    SysVarOFunType *UserSymbol, short SymbolNum, short RepairFlag );
     /* if the middle code expression is check pass, it return 0
      * else return the position found error. If tb is NULL, the check won't
      * check the identify.
      */
MidCodeType * PD_style ActionSymbolRegister( MidCodeType *m_c_head, \
			     dFILE *stb, dFILE *ttb, \
			     SysVarOFunType *UserSymbol, short SymbolNum, \
			     SYSDEFINETYPE *DefineTable, short DefineNum );
     /* Purpose:
      *     MidCodeType *ActionSymbolRegister( MidCodeType *m_c_head, \
      *                      dFILE *stb, dFILE *ttb, \
      *                      SysVarOFunType *UserSymbol, short SymbolNum, \
      *                      SYSDEFINETYPE *DefineTable, short DefineNum );
      *
      * switch the identify ( sys variable and dFILE item ) to their
      * position.
      * success return NULL; else return the pointer of node where error found.
      */
long PD_style ActionCalExpr(MidCodeType *m_c_head, short *CurState, dFILE *tdf);
     /* Purpose:
      *     ActionCalExpr(MidCodeType *m_c_head, short CurState, dFILE *tdf);
      * calculate expression stored MidCodeType
      */
MidCodeType * PD_style ReadCode(FILE *CodeFileHandle);
     /* success return the MidCodeExpression's pointer, else NULL
      * read from the file's current position
      */
short PD_style WriteCode( MidCodeType *m_c_head, FILE *CodeFileHandle );
     /* success write return the write num in byte, else less than 0 and set the xERROR
      * write from the file's current position.
      */
short PD_style GeterrorNo( void );
     /* get error no
      */
void  PD_style ClearError( void );
     /* clear error no
      */
char * PD_style GetErrorMes( short ErrorNo );
     /* get error message
      */
SysVarOFunType * PD_style SymbolTableSearch( const unsigned char *Key, \
				SysVarOFunType *Base, short BaseKeyNum );
AskActionType * PD_style AskActionSearch( const unsigned char *Key, \
				AskActionType *Base, short BaseKeyNum );

unsigned char * PD_style ItemConditionOptimize( unsigned char *Condition, \
				      unsigned char *CurrentDfile, \
				      unsigned char *CurrentField, \
				      MidCodeType **MidCodeBuf );
	/*
	 */
short PD_style _TypeAlign( OpndType *lpOpnd, short ParaNum, short AlignType );
	/*
	 */
long PD_style xGetOpndLong(OpndType *lpOpnd);
short PD_style xGetOpndShort(OpndType *lpOpnd);
double PD_style xGetOpndFloat(OpndType *lpOpnd);
char * PD_style xGetOpndString(OpndType *lpOpnd);
char PD_style xGetOpndChr(OpndType *lpOpnd);
int PD_style xIsOpndField(OpndType *lpOpnd);

void saveXexpEnv( XEXP_ENV *xenv );
void restoreXexpEnv( XEXP_ENV *xenv );

/*<<<<<<<<<<<<<<<<<<<<< service for xexp: function >>>>>>>>>>>>>>>>>>>>>>>>*/

#ifdef XEXP_FUNCTION_SERVICE

short PD_style _xAtol(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xVal(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xStr(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xMax(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xMin(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xSum(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xAverage(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xSubstr(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xSubstrB(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xSysDate(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xSysTime(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xNull(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xLength( OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xAge(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xDate(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xPostcode(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xUniqueKeyword(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xMoudleString(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xIsDbfDelete(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xDbfRecno(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xDbfRecnum(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xLocate(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xAt(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xInt(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xRound(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xCeil(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xFloor(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xSqrt(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xExp(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xLog(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xTrim(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xLtrim(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xRtrim(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xUpper(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xLower(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xShrink(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xSpace(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xFile( OpndType *lpOpnd, short ParaNum, short *OpndTop );
short PD_style _xHzth(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xArrayInit(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xInputStr(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xSysVar(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xDbfVal(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xRelCnt(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xFmtStr(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xEncrypt( OpndType *lpOpnd, short ParaNum, short *OpndTop );
short PD_style _xDecrypt( OpndType *lpOpnd, short ParaNum, short *OpndTop );
short PD_style _xTStrGet(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xTStrPut( OpndType *lpOpnd, short ParaNum, short *OpndTop );
short PD_style _xXin(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xXina(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xRelSkip(OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xTTotal( OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xHighNum( OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xLowNum( OpndType *lpOpnd, short ParaNum, short *OpndTop);
short PD_style _xMidNum( OpndType *lpOpnd, short ParaNum, short *OpndTop);
#endif


/*<<<<<<<<<<<<<<<<<<<<<   service for ask action   >>>>>>>>>>>>>>>>>>>>>>>>*/
#ifdef XEXP_FUNCTION_SERVICE
short PD_style _ASK_MAX(OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_MIN(OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_SUM(OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_AVG(OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_WSYMBOL( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState );
short PD_style _ASK_CNT(OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_SUM(OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_x_SUM(OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_REC(OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_ReadStatistics( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_WriteStatistics( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _Ask_StatInit( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _Ask_AsqlStat_X( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _Ask_AsqlSum_X( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _Ask_AsqlSys( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _Ask_AsqlStat_Y( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_UniteDbf( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_InsetDbf( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_Packdbf( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_Delete( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_RECORD( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					short *CurState );
short PD_style _ASK_WSYMBOL( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState );
short PD_style _ASK_VIEWNO( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState );
short PD_style _ASK_DbfZap( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		       short *CurState );
short PD_style _ASK_ErrorStamp( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState );
short PD_style _ASK_CalRec( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState );
short PD_style _ASK_FillTable( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		       short *CurState );
short PD_style _ASK_GenTable(OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		       short *CurState);
short PD_style _ASK_Print( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState );
short PD_style _ASK_SigmaY( OpndType *lpOpnd, short ParaNum, \
					short *OpndTop, short *CurState );
short PD_style _ASK_SigmaX( OpndType *lpOpnd, short ParaNum, \
					short *OpndTop, short *CurState );
short PD_style _ASK_AppRec( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState );
short PD_style _ASK_InitRec( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState );
short PD_style _ASK_RecPack( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState );
short PD_style _ASK_Goto( OpndType *lpOpnd, short ParaNum, \
					short *OpndTop, short *CurState );
short PD_style _ASK_NextMainRec( OpndType *lpOpnd, short ParaNum, \
					short *OpndTop, short *CurState );
short PD_style _ASK_Web_Print( OpndType *lpOpnd, short ParaNum, \
					short *OpndTop, short *CurState );
short PD_style _ASK_Web_Bpnt( OpndType *lpOpnd, short ParaNum, \
					short *OpndTop, short *CurState );
short PD_style _ASK_MVIEWNO( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState );
short PD_style _ASK_Distinct( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState );
short PD_style _ASK_InsRec( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState );
short PD_style _ASK_Tarray( OpndType *lpOpnd, short ParaNum, \
					     short *OpndTop, short *CurState );
short PD_style _ASK_DbtToFile( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState );
short PD_style _ASK_DbtFromFile( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState );
short PD_style _ASK_CallASQL( OpndType *lpOpnd, short ParaNum, \
					short *OpndTop, short *CurState );
short PD_style _Ask_AsqlAvg_X( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		       short *CurState );
short PD_style _Ask_AsqlMax_X( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		       short *CurState );
short PD_style _Ask_AsqlMin_X( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		       short *CurState );
short PD_style _ASK_SQL( OpndType *lpOpnd, short ParaNum, \
					short *OpndTop, short *CurState );
short PD_style _ASK_RelSeek( OpndType *lpOpnd, short ParaNum, \
					short *OpndTop, short *CurState );
short PD_style _ASK_AddVarArray( OpndType *lpOpnd, short ParaNum, \
			    short *OpndTop, short *CurState );
short PD_style _ASK_ClearVarArray( OpndType *lpOpnd, short ParaNum, \
			    short *OpndTop, short *CurState );
short PD_style _ASK_Array2XML( OpndType *lpOpnd, short ParaNum, \
			    short *OpndTop, short *CurState );
short PD_style _ASK_Array2HTML( OpndType *lpOpnd, short ParaNum, \
			    short *OpndTop, short *CurState );

enum FunId {_AskActionFunAtol_,    \
	_Ask_askavg = 1,/* "askavg",    _ASK_MAX}, */
	_Ask_askcnt,    /* "askcnt",    _ASK_MAX}, */
	_Ask_askmax,    /* "askmax",    _ASK_MAX}, */
	_Ask_askmin,    /* "askmin",    _ASK_MIN}, */
	_Ask_askrec,    /* "askrec",    _ASK_REC}, */
	_Ask_asksum,    /* "asksum",    _ASK_MAX}, */
	_Ask_askxsum,   /* "askxsum",   _ASK_x_SUM}, */
	_Ask_delete,    /* "delete",    _Ask_Delete}, */
	_Ask_idbf,      /* "idbf",      _ASK_InsetDbf}, */
	_Ask_packdbf,   /* "packdbf",   _ASK_PackDbf},*/
	_Ask_rarray,    /* "rarray",    _ASK_ReadStatistics}, */
	_Ask_record,    /* "record",    _ASK_RECORD}, */
	_Ask_statx,     /* "statx",     _Ask_AsqlStat_X}, */
	_Ask_staty,     /* "staty",     _Ask_AsqlStat_Y}, */
	_Ask_sumx,      /* "sumx",      _Ask_AsqlSum_X}, */
	_Ask_sys,       /* "sys",       _Ask_AsqlSys}, */
	_Ask_udbf,      /* "udbf",      _ASK_UniteDbf}, */
	_Ask_viewno,    /* "viewno",    _ASK_VIEWNO}, */
	_Ask_warray,    /* "warray",    _ASK_WriteStatistics} */
	_Ask_wsymbol,   /* "wsymbol",   _ASK_WriteStatistics},*/
	_Ask_Dbfzap,    /* "wsymbol",   _ASK_WriteStatistics},*/
	_Ask_Errorstamp,
	_Ask_Calrec,
	_Ask_FillTable,
	_Ask_GenTable,
	_Ask_Print,
	_Ask_SigmaX,
	_Ask_SigmaY,
        _ASK_appRec,
        _ASK_initRec,
        _ASK_recPack,
	_ASK_goto,
	_ASK_Nextmainrec,
	_ASK_Web_print,
	_ASK_Mvewno,
	_ASK_WebBpnt,
	_ASK_distinct,
	_ASK_insRec,
	_ASK_tarray,
	_ASK_DbttoFile,
	_ASK_DbtfromFile,
	_ASK_Callasql,
	_Ask_AsqlAvg_x,
	_Ask_AsqlMax_x,
	_Ask_AsqlMin_x,
	_ASK_sql,
	_ASK_relseek,
	_ASK_addVarArray,
	_ASK_clearVarArray,
	_ASK_ARRAY2XML,
	_ASK_ARRAY2HTML
};

#define _AskActionFun_Max_      10001
#define _Ask_ReadStatistics     10002
#define _Ask_WriteStatistics    20003

#endif

/************** system value stored in golobo variable *******************/
#ifdef INrEXPRANALYSIS
#ifdef XEXP_FUNCTION_SERVICE

//		        A,B,C,D,E,F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, -
short SysFunHash[27] = {0,4,4,5,7,8,11,11,12,16,16,16,21,24,25,25,26,26,32,42,48,50,51,51,56,56,56};
XexpFunType SysFun[] = {           /* function must at forward */
	{LONG_TYPE, "age", _xAge, 1,1},				//0
	{LONG_TYPE, "at", _xAt, 2,2},                      	//1
	{LONG_TYPE, "atol", _xAtol, 1,1},                  	//2
	{LONG_TYPE, "average", _xAverage, 0,MAX_FUN_PARA},      //3

	{LONG_TYPE, "ceil", _xCeil, 1,1},                       //4

	{DATE_TYPE,   "date", _xDate, 1,1},                  	//5
	{LONG_TYPE,   "dbfval", _xDbfVal, 3,MAX_FUN_PARA},      //6
	{FLOAT_TYPE,  "exp", _xExp, 1,1},                   	//7
	{STRING_TYPE, "file", _xFile, 1,1},                     //8
	{LONG_TYPE,   "floor", _xFloor, 1,1},                   //9
	{STRING_TYPE, "fmtstr", _xFmtStr, 2,2},                 //10

	{STRING_TYPE, "hzth", _xHzth, 2,2},                	//11
	{LONG_TYPE, "iarray", _xArrayInit, 1,MAX_FUN_PARA},    	//12
	{STRING_TYPE, "inputstr", _xInputStr, 1,1},        	//13
	{LONG_TYPE,  "int", _xInt, 1,1},                   	//14
	{LOGIC_TYPE,  "isdel", _xIsDbfDelete, 1,1},        	//15
	{LONG_TYPE, "len", _xLength, 1,1},                 	//16
	{LONG_TYPE, "locate", _xLocate, 2,MAX_FUN_PARA},        //17
	{FLOAT_TYPE, "log", _xLog, 1,1},                   	//18
	{STRING_TYPE, "lower", _xLower, 1,1},              	//19
	{STRING_TYPE, "ltrim", _xLtrim, 1,1},              	//20

	{LONG_TYPE, "max", _xMax, 1,MAX_FUN_PARA},              //21
	{LONG_TYPE, "min", _xMin, 1,MAX_FUN_PARA},              //22
	{LONG_TYPE, "mdstr", _xMoudleString, 2,MAX_FUN_PARA},   //23

	{LONG_TYPE, "null", _xNull, 1,1},                       //24

	{LONG_TYPE, "postcode", _xPostcode, 1,1},               //25

	{LONG_TYPE,   "recno", _xDbfRecno, 1,1},             	//26
	{LONG_TYPE,   "recnum", _xDbfRecnum, 1,1},             	//27
	{LONG_TYPE,   "relcnt", _xRelCnt, 1,2},              	//28
	{LONG_TYPE,   "relskip", _xRelSkip, 3,3},              	//29
	{LONG_TYPE,   "round", _xRound, 1,1},                   //30
	{STRING_TYPE, "rtrim", _xRtrim, 1,1},                   //31

	{STRING_TYPE, "shrink", _xShrink, 1,1},                   //32
	{STRING_TYPE, "space", _xSpace, 1,1},                     //33
	{FLOAT_TYPE,  "sqrt", _xSqrt, 1,1},                       //34
	{STRING_TYPE, "str", _xStr, 1,2},                  	  //35
	{STRING_TYPE, "substr", _xSubstr, 3,3},            	  //36
	{STRING_TYPE, "substrb", _xSubstrB, 3,3},            	  //37
	{LONG_TYPE,   "sum", _xSum, 1,MAX_FUN_PARA},              //38
	{STRING_TYPE, "sysdate", _xSysDate, 0,1},          	  //39
	{STRING_TYPE, "systime", _xSysTime, 0,1},          	  //40
	{LONG_TYPE,   "sysvar", _xSysVar, 1,3},              	  //41

	{STRING_TYPE, "tdecrypt", _xDecrypt, 2,2},             	  //42
	{STRING_TYPE, "tencrypt", _xEncrypt, 2,2},             	  //43
	{STRING_TYPE, "trim", _xTrim, 1,1},                	  //44
	{STRING_TYPE, "tstrget", _xTStrGet, 3,3},              	  //45
	{STRING_TYPE, "tstrput", _xTStrPut, 4,4},              	  //46
	{LONG_TYPE,   "ttotal",  _xTTotal, 2,MAX_FUN_PARA},       //47


	{LONG_TYPE, "uniquekey", _xUniqueKeyword, 1,3},		  //48
	{STRING_TYPE, "upper", _xUpper, 1,1},              	  //49
	{LONG_TYPE, "val", _xVal, 1,1},                    	  //50

	{LONG_TYPE, "xin", _xXin, 2,MAX_FUN_PARA},                //51
	{LONG_TYPE, "xina", _xXina, 2,MAX_FUN_PARA},              //52
	{LONG_TYPE, "xhighnum", _xHighNum, 2,2},              	  //53
	{LONG_TYPE, "xlownum", _xLowNum, 2,2},              	  //54
	{LONG_TYPE, "xmidnum", _xMidNum, 1,3},              	  //55

	{LONG_TYPE, "",  NULL, 0,MAX_FUN_PARA}                    //56
};
#else
XexpFunType *SysFun = NULL;
#endif

#ifdef XEXP_FUNCTION_SERVICE
//		        A,B,C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, -
short ActFunHash[27] = {0,9,11,14,16,17,18,20,20,23,23,23,23,24,25,25,27,27,31,38,39,40,41,45,50,50,51};
AskActionType AskActionTable[] = {

	{_ASK_addVarArray,   "addvar",  _ASK_AddVarArray, 2,2},  	      	      //0

	{_ASK_appRec,   "apprec",       _ASK_AppRec, 1,MAX_FUN_PARA},  	      //1
	{_Ask_askavg,   "askavg",       _ASK_AVG, 1,MAX_FUN_PARA},            //2
	{_Ask_askcnt,   "askcnt",       _ASK_CNT, 1,MAX_FUN_PARA},            //3
	{_Ask_askmax,   "askmax",       _ASK_MAX, 1,MAX_FUN_PARA},            //4
	{_Ask_askmin,   "askmin",       _ASK_MIN, 1,MAX_FUN_PARA},            //5
	{_Ask_askrec,   "askrec",       _ASK_REC, 1,2},            	      //6
	{_Ask_asksum,   "asksum",       _ASK_SUM, 1,MAX_FUN_PARA},            //7
	{_Ask_askxsum,  "askxsum",      _ASK_x_SUM, 2,MAX_FUN_PARA},          //8
	{_ASK_DbtfromFile,"bfromfile",	_ASK_DbtFromFile, 2,2},   	      //9
	{_ASK_DbttoFile,  "btofile",  	_ASK_DbtToFile, 2,2},      	      //10
	{_ASK_Callasql, "calasql",	_ASK_CallASQL,1,1},		      //11
	{_Ask_Calrec,   "calrec",       _ASK_CalRec, 1,MAX_FUN_PARA},         //12

	{_ASK_clearVarArray,"clearvar", _ASK_ClearVarArray, 1,1},             //13


	{_Ask_delete,   "delete",       _ASK_Delete, 1,MAX_FUN_PARA},         //14
	{_ASK_distinct, "distinct",     _ASK_Distinct, 1,2},                  //15

	{_Ask_Errorstamp,"errstamp",    _ASK_ErrorStamp, 1,MAX_FUN_PARA},     //16
	{_Ask_FillTable,"filltable",    _ASK_FillTable, 1,MAX_FUN_PARA},      //17
	{_Ask_GenTable, "gentable",     _ASK_GenTable, 1,MAX_FUN_PARA},       //18
	{_ASK_goto,     "goto",         _ASK_Goto, 1,1},                      //19
	{_Ask_idbf,     "idbf",         _ASK_InsetDbf, 1,MAX_FUN_PARA},       //20
	{_ASK_initRec,  "initrec",      _ASK_InitRec, 1,1},                   //21
	{_ASK_insRec,   "insrec",       _ASK_InsRec, 1,MAX_FUN_PARA},         //22
	{_ASK_Mvewno,	"mvewno",	_ASK_MVIEWNO, 1,1},		      //23
	{_ASK_Nextmainrec,  "nextmrec", _ASK_NextMainRec, 1,1},               //24
	{_Ask_packdbf,  "packdbf",      _ASK_Packdbf, 1,MAX_FUN_PARA},        //25
	{_Ask_Print,    "print",        _ASK_Print, 1,MAX_FUN_PARA},          //26
	{_Ask_rarray,   "rarray",       _ASK_ReadStatistics, 2,2}, 	      //27
	{_Ask_record,   "record",       _ASK_RECORD, 1,1},                    //28
	{_ASK_recPack,  "recpack",      _ASK_RecPack, 0,0},        	      //29
	{_ASK_relseek, 	"relseek",	_ASK_RelSeek, 1, 1},                  //30

	{_Ask_SigmaX,   "sigmax",	_ASK_SigmaX, 1,MAX_FUN_PARA},         //31
	{_Ask_SigmaY,   "sigmay",	_ASK_SigmaY, 1,MAX_FUN_PARA},         //32

	{_ASK_sql,      "sql",          _ASK_SQL, 1, MAX_FUN_PARA},	      //33

	{_Ask_statx,    "statx",        _Ask_AsqlStat_X, 1,MAX_FUN_PARA},     //34
	{_Ask_staty,    "staty",        _Ask_AsqlStat_Y, 1,MAX_FUN_PARA},     //35
	{_Ask_sumx,     "sumx",         _Ask_AsqlSum_X, 3,MAX_FUN_PARA},      //36
	{_Ask_sys,      "sys",          _Ask_AsqlSys, 1,MAX_FUN_PARA},        //37

	{_ASK_tarray,   "tarray",	_ASK_Tarray,  9,9},                   //38

	{_Ask_udbf,     "udbf",         _ASK_UniteDbf, 1,MAX_FUN_PARA},       //39
	{_Ask_viewno,   "viewno",       _ASK_VIEWNO, 1,MAX_FUN_PARA},         //40
	{_ASK_WebBpnt,  "webprint",     _ASK_Web_Bpnt, 1,MAX_FUN_PARA},       //41
	{_ASK_Web_print,"webstream",    _ASK_Web_Print, 1,MAX_FUN_PARA},      //42
	{_Ask_warray,   "warray",       _ASK_WriteStatistics, 2,MAX_FUN_PARA},//43
	{_Ask_wsymbol,  "wsymbol",      _ASK_WSYMBOL, 1,1},        	      //44

	{_ASK_ARRAY2HTML,"xarray2html", _ASK_Array2HTML,4,5},		      //45
	{_ASK_ARRAY2XML, "xarray2xml",  _ASK_Array2XML,4,4},		      //46
	{_Ask_AsqlAvg_x, "xavg",        _Ask_AsqlAvg_X,3,MAX_FUN_PARA},	      //47
	{_Ask_AsqlMax_x, "xmax",	_Ask_AsqlMax_X,3,MAX_FUN_PARA},       //48
	{_Ask_AsqlMin_x, "xmin",	_Ask_AsqlMin_X,3,MAX_FUN_PARA},       //49
	{_Ask_Dbfzap,   "zap",          _ASK_DbfZap, 1,MAX_FUN_PARA},         //50
	{0,             "",             NULL,        0,MAX_FUN_PARA}          //51
};
unsigned short AskActionFunNum = sizeof(AskActionTable)/sizeof(AskActionType)-1;
		       // this nummber shows the AskActionTable's number
#else
AskActionType *AskActionTable = NULL;
//this nummber shows the AskActionTable's number
unsigned short AskActionFunNum = 0;
#endif

#ifdef XEXP_FUNCTION_SERVICE
// this nummber shows the SysFun's number
unsigned short SysFunNum = sizeof(SysFun)/sizeof(XexpFunType)-1;
#else
unsigned short SysFunNum = 0;
#endif

#else
extern WSToMT SysVarOFunType **XexpUserDefinedVar;
extern WSToMT short    *XexpUserDefinedVarNum;
extern XexpFunType SysFun[];
extern AskActionType AskActionTable[];
extern unsigned short SysFunNum;
extern unsigned short AskActionFunNum;

#endif
/*<<<<<<<<<<<<<<< the body of functions is in XEXPFUN.C >>>>>>>>>>>>>>>>>>*/

#endif
