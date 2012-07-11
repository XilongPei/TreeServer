/****************
*     HZTH.H   MIS lower supportment -- Replace Chinese
*
*     Last Rewritten By Guangxian Hou  In  April,  1994
*
* Copywright:  Shanghai Institute of Railway Technology  3.00
****************************************************************************/

#ifndef  _InReplaceHead_

#define  _InReplaceHead_        "CHINESE_REPLACE 3.00"

/*===============
*                      CONST DIMENSION
*==========================================================================*/
#define MAX_FILE_NAME_LEN       13        /* FILE NAME LENGTH  */
#define MAX_FILESPEC            81        /* MAX FILE SPEC     */
#define CODE_LIB_PAGE_SIZE      128       /* MEMORY BLOCK SIZE
					   * this should be less than 256
					   */

#ifdef  UNIX_V
	#define MAX_CODE_FILE_SIZE      0x7FFFFFFF
#else
	//redefine this for win32 support
	//#define MAX_CODE_FILE_SIZE      0xFFFF       /* MAX CODE FILE SIZE */
	#define MAX_CODE_FILE_SIZE      0x7FFFFFFF
#endif

#define DEF_MAX_PAGE            64                   /* MAX PAGE NUMBER IN MEMORY */
#define FILENAME_EXT            ".COD"               /* EXTEND NAME OF FILE */
#define MAX_PAGE_USETIMES       255                  /* MEMORY PAGE MAX USE TIMES */
#define MAX_FIELD_NAMELEN       12                   /* MAX FIELDNAME LENGTH */
#define MAX_EEXPR_LEN           256
#define MAX_CEXPR_LEN           256
#define MAX_HZTH_RESULT_LEN	256
#define HZTH_MAX_SEG		6

#if (MAX_HZTH_RESULT_LEN < MAX_CEXPR_LEN)
    #error MAX_HZTH_RESULT_LEN should larger than MAX_CEXPR_LEN
#endif

// Field name define
#define CODE_TYPE_FIELD_NAME    "CODE_TYPE"
#define HZTH_C_EXPR             "C_EXPR"
#define HZTH_E_EXPR             "E_EXPR"
#define HZTH_C_BRIF             "C_BRIF"
#define HZTH_C_RELATION         "RELATION"
#define HZTH_DBPATH        	"DBPATH"


/*===============
		       ERROR_CODE  DIFINATION
*==========================================================================*/
#define NO_MEM               1000
#define NO_OPEN              1001
#define NO_FIELD             1002
#define NO_CODE              1003
#define NO_PARA              1004
#define ERR_LIB              1005
#define BIG_NUM              1006
#define NO_LIB               1007
#define ERR_CODE             1008
#define ERR_LIB_ID           2000
#define ERR_HZTH_METHOD      3000


/*===============
*                      STRUCT DIMENSION
*==========================================================================*/
typedef struct tagCODE_LIB_MES_STRUCT {
   unsigned char    DbfName[13];            /* DBF FILE NAME                */
   long             CodeOffset;             /* FIRST CODE'S OFFSET          */
   long   	    BeginPageNum;           /* CHINESE BEGINNING PAGE NO.   */
   unsigned char    BeginPageOffset;        /* CHINESE OFFSET IN FIRST PAGE */
   long		    PageNum;                /* CHINESE LIB USED PAGE NUMBER */
   unsigned char    CodeLen;                /* EACH CODE LENGTH             */
   unsigned char    StorageLen;             /* storage length               */
   unsigned char    ExprLen;                /* EACH CHINESE CODE LENGTH     */
   long             CodeNum;                /* CODE QUANTITES IN USE LIB    */
   char             CompressMethod;         /* 'c' 'i' 'l' 's': string      */
//   unsigned short   segIndex[HZTH_MAX_SEG]; /* segmengt index               */
} CODE_LIB_MES_STRUCT;

typedef struct tagCODE_PAGE_STRUCT {
   unsigned char   chbuf[CODE_LIB_PAGE_SIZE];   /* CHINESE BLOCK IN MEMORY */
   long            PageNo;                      /* PAGE NO.                */
   unsigned char   PageUseTimes;                /* FREQUENCES OF PAGE_USED */
} CODE_PAGE_STRUCT;

typedef struct tagREPLACE_DBF_STRUCT {
   unsigned char  efName[MAX_FIELD_NAMELEN];    /* REPLACE LIB FILE NAME */
   unsigned char  ReplaceMark;                  /* REPLACE MARK          */
   short          dDbf;                         /* USED CODE LIB NO.     */
} REPLACE_DBF_STRUCT;

typedef struct tagCODE_STRUCT {               /* SINGLE CODE REPLACE STRUCT */
   char QueryMethod;                          /* QUERY METHOD               */
   char Code[MAX_EEXPR_LEN];                  /* REPLACE CODE NAME          */
   char Chinese[MAX_CEXPR_LEN];               /* REPLACED CHINESE STRING    */
   unsigned short DbfNo;                      /* DBF FILE NO. INCLUDED      */
   unsigned short CodeNo;                     /* CURRENT CODE No            */
   long     Position;                   /* CURRENT CODE LIB POSITION  */
} CODE_STRUCT;

typedef struct tagHZTH_METHOD_STRU {
    short 	   CodeDbfId;
    unsigned char  chStart;
    unsigned char  chFldLen;
} HZTH_METHOD_STRU;

/*================
*  EXTERN USE FUNCTION DIMENSION
*==========================================================================*/
extern WSToMT	short hERROR;

#ifdef __HZTH_C_
_declspec(dllexport) unsigned short hBuildCodeLib( char *CodeDbfControlName );
   /* Build code library shorto inside avalible type from DBF type.
   **/
_declspec(dllexport) short hOpen( char *CodeCodControlName );
   /* Open code library and build inside data struct
   **/
_declspec(dllexport) short hClose( void );
   /* Close all open file, free all alloced memory.
   **/

_declspec(dllexport) char *hReplaceByName( char *CodeDbfName, char *Code );
   /* Single field replace with code name.
   **/

_declspec(dllexport) char *hReplaceById( short CodeDbfId, char *Code );
   /* Single field replace with code No.
   **/

_declspec(dllexport) short hSetPageNum( short PageNumber );
   /* Set MAX available page number in HZTH module.
   **/
_declspec(dllexport) short hGetPageNum( void );
   /* Get used page quantites in HZTH module.
   **/
_declspec(dllexport) CODE_STRUCT *hGetCodeRecByName( char *CodeDbfName, unsigned short BeginPos,\
				       char *ModStr, CODE_STRUCT *CodeStruct, \
				       char FunSel );
   /* Search for correspond chinese string with code libriary name.
   ** ModStr is a module.
   ** FunSel: 'N': get the BeginPos'th code record
   **         'S': get the record which start with ModStr
   **              (e.g. "12": "1234", "1245"...) FROM BeginPos
   **         'E': get the record which end with ModStr
   **              (e.g. "00": "1200", "2300"...) FROM BeginPos
   **	       'M': get the record which is 'equal' to the record.
   **		   (e.g. "0*10": "0110", "0210"...) FROM BeginPos.
   **         'F': get the record which is fuzzy suit for ModStr
   **              (e.g. "Inst": "ShangHai Railway Inst.",
   **              "Inst of Intelligent", ...) FROM BeginPos
   ** Note: BeginPos begin from 0
   **----------------------------------------------------------------------*/

_declspec(dllexport) CODE_STRUCT *hGetCodeRecById( short CodeDbfId, \
				     unsigned short BeginPos, char *ModStr, \
				     CODE_STRUCT *CodeStruct, char FunSel );
   /* Search for correspond chinese string with code libriary id.
   ** ModStr is a module.
   ** FunSel: 'N': get the BeginPos'th code record
   **         'S': get the record which start with ModStr
   **              (e.g. "12": "1234", "1245"...) FROM BeginPos
   **         'E': get the record which end with ModStr
   **              (e.g. "00": "1200", "2300"...) FROM BeginPos
   **	       'M': get the record which is 'equal' to the record.
   **		   (e.g. "0*10": "0110", "0210"...) FROM BeginPos.
   **         'F': get the record which is fuzzy suit for ModStr
   **              (e.g. "Inst": "ShangHai Railway Inst.",
   **              "Inst of Intelligent", ...) FROM BeginPos
   ** Note: BeginPos begin from 0
   ** Return:
   **  	success: the pointer to the struct
   **	fail   : the pointer to the struct, but the CodeStruct->Position
   **		 is 0xFFFF
   **-----------------------------------------------------------------------*/

_declspec(dllexport) HZTH_METHOD_STRU *hMethodTranslate( char *sHzthMethod );
   /* Translate a regular string to a REGULAR
   **/
_declspec(dllexport) void freeHzthMethodStru(HZTH_METHOD_STRU *p);

_declspec(dllexport) char *hReplaceByMethod( HZTH_METHOD_STRU *HzthMethod, char *Code );
   /* Single field replace with HzthMethod.
   **/

_declspec(dllexport) char *hExtReplaceByMethod(HZTH_METHOD_STRU *HzthMethod, char *Code);
   /* Single field external(RELATION) replace with HzthMethod.
   **/

_declspec(dllexport) char *hExtReplaceByName( char *CodeDbfName, char *Code );
   /* Single field replace with code name.
   **/

_declspec(dllexport) char *hExtReplaceById( short CodeDbfId, char *Code );
   /* Single field replace with code No.
   **/

_declspec(dllexport) short hGetCodeDbfId( char *CodeDbfName );
   /* Get the ID of the code_lib short global_control_table
   **/
_declspec(dllexport) unsigned short hCheckCodeValid( char *CodeDbfName, char *Code );
   /* When you want to check the validity of the Code in the CodeDbfName,
   ** but you aren't sure to desided if you should really replace the code,
   ** herence you might call it for the sure!
   */
_declspec(dllexport) long hGetCodeDbfRecNum( short CodeDbfId );
   /* Get code num
   */
_declspec(dllexport) long hGetSegCodeNum( short CodeDbfId, char *ModStr );

_declspec(dllexport) short hGetCexprLen( HZTH_METHOD_STRU *HzthMethod );
   /* Get code num
   */

_declspec(dllexport) CODE_LIB_MES_STRUCT *hGetCodeLibInfo( short CodeDbfid );
_declspec(dllexport) short getHzthInitFlag(void);
_declspec(dllexport) char *getHzthInfoStr(void);
#else
_declspec(dllimport) unsigned short hBuildCodeLib( char *CodeDbfControlName );
   /* Build code library shorto inside avalible type from DBF type.
   **/
_declspec(dllimport) short hOpen( char *CodeCodControlName );
   /* Open code library and build inside data struct
   **/
_declspec(dllimport) short hClose( void );
   /* Close all open file, free all alloced memory.
   **/

_declspec(dllimport) char *hReplaceByName( char *CodeDbfName, char *Code );
   /* Single field replace with code name.
   **/

_declspec(dllimport) char *hReplaceById( short CodeDbfId, char *Code );
   /* Single field replace with code No.
   **/

_declspec(dllimport) short hSetPageNum( short PageNumber );
   /* Set MAX available page number in HZTH module.
   **/
_declspec(dllimport) short hGetPageNum( void );
   /* Get used page quantites in HZTH module.
   **/
_declspec(dllimport) CODE_STRUCT *hGetCodeRecByName( char *CodeDbfName, unsigned short BeginPos,\
				       char *ModStr, CODE_STRUCT *CodeStruct, \
				       char FunSel );
   /* Search for correspond chinese string with code libriary name.
   ** ModStr is a module.
   ** FunSel: 'N': get the BeginPos'th code record
   **         'S': get the record which start with ModStr
   **              (e.g. "12": "1234", "1245"...) FROM BeginPos
   **         'E': get the record which end with ModStr
   **              (e.g. "00": "1200", "2300"...) FROM BeginPos
   **	       'M': get the record which is 'equal' to the record.
   **		   (e.g. "0*10": "0110", "0210"...) FROM BeginPos.
   **         'F': get the record which is fuzzy suit for ModStr
   **              (e.g. "Inst": "ShangHai Railway Inst.",
   **              "Inst of Intelligent", ...) FROM BeginPos
   ** Note: BeginPos begin from 0
   **----------------------------------------------------------------------*/

_declspec(dllimport) CODE_STRUCT *hGetCodeRecById( short CodeDbfId, \
				     unsigned short BeginPos, char *ModStr, \
				     CODE_STRUCT *CodeStruct, char FunSel );
   /* Search for correspond chinese string with code libriary id.
   ** ModStr is a module.
   ** FunSel: 'N': get the BeginPos'th code record
   **         'S': get the record which start with ModStr
   **              (e.g. "12": "1234", "1245"...) FROM BeginPos
   **         'E': get the record which end with ModStr
   **              (e.g. "00": "1200", "2300"...) FROM BeginPos
   **	       'M': get the record which is 'equal' to the record.
   **		   (e.g. "0*10": "0110", "0210"...) FROM BeginPos.
   **         'F': get the record which is fuzzy suit for ModStr
   **              (e.g. "Inst": "ShangHai Railway Inst.",
   **              "Inst of Intelligent", ...) FROM BeginPos
   ** Note: BeginPos begin from 0
   ** Return:
   **  	success: the pointer to the struct
   **	fail   : the pointer to the struct, but the CodeStruct->Position
   **		 is 0xFFFF
   **-----------------------------------------------------------------------*/

_declspec(dllimport) HZTH_METHOD_STRU *hMethodTranslate( char *sHzthMethod );
   /* Translate a regular string to a REGULAR
   **/

_declspec(dllimport) void freeHzthMethodStru(HZTH_METHOD_STRU *p);

_declspec(dllimport) char *hReplaceByMethod( HZTH_METHOD_STRU *HzthMethod, char *Code );
   /* Single field replace with HzthMethod.
   **/

_declspec(dllimport) char *hExtReplaceByMethod(HZTH_METHOD_STRU *HzthMethod, char *Code);
   /* Single field external(RELATION) replace with HzthMethod.
   **/

_declspec(dllimport) char *hExtReplaceByName( char *CodeDbfName, char *Code );
   /* Single field replace with code name.
   **/

_declspec(dllimport) char *hExtReplaceById( short CodeDbfId, char *Code );
   /* Single field replace with code No.
   **/

_declspec(dllimport) short hGetCodeDbfId( char *CodeDbfName );
   /* Get the ID of the code_lib short global_control_table
   **/
_declspec(dllimport) unsigned short hCheckCodeValid( char *CodeDbfName, char *Code );
   /* When you want to check the validity of the Code in the CodeDbfName,
   ** but you aren't sure to desided if you should really replace the code,
   ** herence you might call it for the sure!
   */
_declspec(dllimport) long hGetCodeDbfRecNum( short CodeDbfId );
   /* Get code num
   */
_declspec(dllimport) long hGetSegCodeNum( short CodeDbfId, char *ModStr );

_declspec(dllimport) short hGetCexprLen( HZTH_METHOD_STRU *HzthMethod );
   /* Get code num
   */

_declspec(dllimport) CODE_LIB_MES_STRUCT *hGetCodeLibInfo( short CodeDbfid );
_declspec(dllimport) short getHzthInitFlag(void);
_declspec(dllimport) char *getHzthInfoStr(void);
#endif

#endif
