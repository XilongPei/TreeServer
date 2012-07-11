/*****************
 * ASQLXEXP.H
 *
 * copyright (c) Shanghai Tiedao University 1998
 *               CRSC 1998
 * author:  Xilong Pei
 ****************************************************************************/

#ifndef __ASQLXEXP_H_
#define __ASQLXEXP_H_

#include <windows.h>
#include <stdio.h>
#include <limits.h>

#define dFILE  void
#define FIELDNAMELEN	32

#define XEXP_FUNCTION_SERVICE

#define PD_style        cdecl


#define MAX_BUFFER_LENGTH       128
#define MAX_OPND_NUMBER         256
#define MAX_OPND_LENGTH         32            /* middle varable */
					      /* long double need 10 bytes */
#define MAX_ATOM_LENGTH         8             /* =sizeof(double) atom size in MidCodeType */

#define MAX_IDEN_LENGTH         (MAX_OPND_LENGTH-1)
#define MAX_OPTR_NUMBER         64
#define FLOAT_PRECISION		0.00000000000000000001
#define FLOAT_MAX               1.7E+300
#define FLOAT_MIN               -1.7E+300
#define _ArrayOrFunNestDeep_    8

//first time of action is 0
#define LASTWORKTIMEOFACTION    -1
#define ONE_MODALITY_ACTION     -2

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
#define BITL_TYPE       14
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
#define FIELD_IDEN   1060
#define NFIELD_IDEN  1061       /* dec is 0 and bytes less than 10, long */
#define FFIELD_IDEN  1062       /* dec isnot 0, float   */
#define LFIELD_IDEN  1063       /* logic, short           */
#define DFIELD_IDEN  1064       /* date, char[8]        */
#define MFIELD_IDEN  1065       /* memo         */
#define CFIELD_IDEN  1066       /* chr[n]       */
#define SFIELD_IDEN  1067       /* recno()      */
#define CHR_TYPE     1070
#define INT_TYPE     1071
#define LONG_TYPE    1072
#define FLOAT_TYPE   1073
#define DATE_TYPE    1074
#define STRING_TYPE  1075
#define LOGIC_TYPE   1076
#define ARRAY_TYPE   1077
#define NEG_TYPE     1124          /* - */
#define VARADDR_TYPE 1125          /* & */

#define VIDEN_TYPE    (IDEN_TYPE  | 0x1000)
#define VSTRING_IDEN  (STRING_IDEN| 0x1000)
#define VDATE_IDEN    (DATE_IDEN  | 0x1000)
#define VCHR_IDEN     (CHR_IDEN   | 0x1000)
#define VINT_IDEN     (INT_IDEN   | 0x1000)
#define VLONG_IDEN    (LONG_IDEN  | 0x1000)
#define VFLOAT_IDEN   (FLOAT_IDEN | 0x1000)
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
	short   length;
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
	short           length;
	unsigned  char  *oval;          // fast pointer
} OpndType;

typedef short     OptrType;

//xexp error string length
#define XES_LENGTH	32
typedef struct {
       short xERROR;                       // Error code
       char  string[ MAX_OPND_LENGTH ];    // save the error symbol
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
       iQuotoErr
};

typedef short PD_style (*PFD)(OpndType *lpOpnd, short ParaNum, short *OpndTop);
typedef short PD_style (*ACTIONPFD)(OpndType *lpOpnd, short ParaNum, short *OpndTop, \
					  short *CurState );

#define MAXARRAYDIM         ( MAX_OPND_LENGTH - 12 ) / 2
typedef struct tagArrayType {
	unsigned short ElementType;
	unsigned char  DimNum;
	unsigned short ElementNum;
	unsigned short MemSize;
	unsigned short ArrayDim[ MAXARRAYDIM ];
	void           *ArrayMem;
} ArrayType;

typedef struct {
	short  type;
	char   VarOFunName[32];
	unsigned char values[ MAX_OPND_LENGTH ];
		/* (long)values()
		 * when the variable is ARRAY type the values stored as
		 * ArrayType
		 */
	short length;
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
	short varnum;
} XexpFunType;

typedef struct {
	short  type;
	char   VarOFunName[32];
	short PD_style (*FarProc)(OpndType *lpOpnd, short ParaNum, short *OpndTop, \
			 short *CurState );
	short varnum;
} AskActionType;

// define the position
#define SysVar_RECNO            0
#define SysVar_LONGTMP          1
#define SysVar_FLOATTMP         2
#define SysVar_STRINGTMP        3
#define SysVar_BKEY		4

#endif
