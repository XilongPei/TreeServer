/***************
* HZTH.C
* MIS lower supportment -- Chinese Replace Main modular.
*
* author: Yong Hu,  1992,
*         Guangxian Hou, 1994,
*         Xilong Pei, 1992-2000
* Notice:
*    add DBPATH in code.dbf support which point to a DBname or file path
*    2000.5.31 support ec_m8(#1,2)(3,4)
*              CodeDbfId = SHRT_MIN+2   (#1,2)
*              CodeDbfId = SHRT_MIN+1   '12'(1,2)
*              CodeDbfId = SHRT_MIN     (1,2)
*              CodeDbfId = -CodeDbfId-MAX_CODE_DBF_NUM; ec_m8(#1,2)
*	    -2*MAX_CODE_DBF_NUM ... ... -MAX_CODE_DBF_NUM ... ... 0 ... ... MAX_CODE_DBF_NUM
*	    ec_m8(#1,2)			ec_m8(*1,2)		  ec_m8(1,2)
*
* copyright (c):  Shanghai Withub Vision Software Co., Ltd.
*****************************************************************************/


#define  _HzthRuningMessage_Assigned
#define  __HZTH_C_

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#include <memory.h>
#else
#include <mem.h>
#endif
#include <math.h>
#include <LIMITS.H>

#ifdef   UNIX_V
	 #include <unistd.h>
	 #include <sys/types.h>
	 #include <sys/stat.h>

#else
	 #include <io.h>
	 #include <dos.h>
	 #include <sys\types.h>
	 #include <sys\stat.h>
	 #include <share.h>
#endif

#ifdef   __TURBOC__
	 #include <malloc.h>
	 #include <alloc.h>
#else
	 #include <malloc.h>
#endif

#include "dio.h"
#include "mistring.h"
#include "strutl.h"
#include "hzth.h"
#include "btree.h"
#include "wst2mt.h"
#include "filetool.h"
#include "cfg_fast.h"
#include "ts_dict.h"

#ifdef _HzthRuningMessage_Assigned
#ifdef  WIN32
	void tgErrorWin(char *buf);
#else
	#include "msgbox.h"
	#include "cfg_fast.h"
	#include "view.h"
	#include "int24.h"
	#include "pubvar.h"
	#include "uvutl.h"
#endif
	void HzthRuningMessage( char *s );
#endif


/*===================
*                  ONLY USE IN HZTH  MOUDLE
*===========================================================================*/
static unsigned char *hReplace( unsigned short CodeLibNo, char *SrcCode );

static long LocateFetchPage( long PageNo );

static unsigned short  TransCodeBufMan( FILE *stream, char *buf, long Len );

static int  Strbsearch(  unsigned short CodeLibNo, char *SrcCode );

static int  HZTHbsearch( unsigned short CodeLibNo, void *SrcCode );

static unsigned short CompressWriteRead( void *SrcCode, char rwFlag, \
			 CODE_LIB_MES_STRUCT *Code_LibMes, void *DestCode );

static CODE_STRUCT *hGetCodeRec( short CodeDbfId, \
				 unsigned short BeginPos, char *ModStr, \
				 CODE_STRUCT *CodeStruct, char FunSel );
static char *hExtReplace( unsigned short CodeDbfNo, char *Code );

static short FuzzyCmp( char *s1, char *s2, int len );


/*======================
*               ONLY USE IN HZTH_BIN_SEARCH
*===========================================================================*/
typedef int (* ComparePtr)( const void *, const void * );
static int CharComp( const char *Char1, const char *Char2 );
static int ShortComp( const short *Short1, const short *Short2 );
static int LongComp( const long *Long1, const long *Long2 );

/*=====================
*                SOME  GLOBEL    VARIABLE DECLARATION
*===========================================================================*/
WSToMT	short hERROR;
static CODE_LIB_MES_STRUCT *CodeDbfCtrlTab=NULL;
static CODE_PAGE_STRUCT    *WordsPageTab=NULL;
static char *CodeLibBuf=NULL;
static unsigned short MAX_PAGE=DEF_MAX_PAGE;
static int   HAND_Words=0;
static short InitFlag = 0;
static short MAX_CODE_DBF_NUM;
static char CtrlLibFileName[MAX_FILESPEC];
static char cWords[MAX_HZTH_RESULT_LEN];
static char szWords[MAX_HZTH_RESULT_LEN];
static short LeftPageSpace=CODE_LIB_PAGE_SIZE;

/******************
*                     1. short hClose()
*        Close all open file, free all alloced memory.
*****************************************************************************/
_declspec(dllexport) short hClose( void )
{
    CODE_LIB_MES_STRUCT *cdct = NULL;

    if( InitFlag == 0 )
	return  1;

    //to avoid use HZTH while hClose is processing
    cdct = CodeDbfCtrlTab;
    CodeDbfCtrlTab = NULL;

    // Close the global_control table
    if( cdct != NULL )    free( cdct );

    // Close the chinese_words page_frame
    if( WordsPageTab != NULL )      free( WordsPageTab );

    // Close the numeric_code liner_buffer
    if( CodeLibBuf != NULL )        free( CodeLibBuf );

    // Close the handle of the chinese file
    if( HAND_Words > 0 )            close( HAND_Words );

    HAND_Words = InitFlag = 0;
    WordsPageTab   = NULL;
    CodeLibBuf     = NULL;

    return  1;

} /* End of function hClose() */


/*****************
*                       2. short hSetPageNum()
*             Set MAX available page number in HZTH moudle.
*****************************************************************************/
short hSetPageNum( short PageNumber )
{
    if( PageNumber <= 0 )               return      -1;

    if( PageNumber != MAX_PAGE ) {
	if( InitFlag == 1 ) {  /* IF   current status of HZTH is active
				* THEN you must make it dead.
				*/
		hClose();      /* MAKE InitFlag == 0 */

		if( PageNumber > DEF_MAX_PAGE )    MAX_PAGE = DEF_MAX_PAGE;
		else                               MAX_PAGE = PageNumber;

		hOpen( CtrlLibFileName ); /* Now, you may use it ! */

	} else { // End of IF
		if( PageNumber > DEF_MAX_PAGE )    MAX_PAGE = DEF_MAX_PAGE;
		else                               MAX_PAGE = PageNumber;
	}
    } // End of if

    return  1;

} /* End of function hSetPageNum() */


/****************
*                    3.  short hGetPageNum()
*          Get  used  page quantites in HZTH module.
*****************************************************************************/
short hGetPageNum( void )
{

    return  MAX_PAGE;

} /* End of function hGetPageNum() */


/******************
*                        4. short hBuildCodeLib()
*        Build code library into inside avalible type from DBF type.
*****************************************************************************/
_declspec(dllexport) unsigned short hBuildCodeLib( char *CodeDbfControlName )
{

    bHEAD *bh;
    dFILE *df, *dfCtrl;
    CODE_LIB_MES_STRUCT CodeLibMes;   // Used to store globl control lib
    FILE *fpCtrl, *fpCode, *fpWords;
    char *nStr, *SplitPath, FileName[MAX_FILESPEC];
    char FieldBuf[CODE_LIB_PAGE_SIZE];
    char *cExprFldName=HZTH_C_EXPR, *eExprFldName=HZTH_E_EXPR;
    char *cBrifFldName=HZTH_C_BRIF, *cRelationFldName=HZTH_C_RELATION;
    char *eCompressFldName=CODE_TYPE_FIELD_NAME;
    unsigned short eExprFldID, cExprFldID, cBrifFldID;
    unsigned short dbpathFldID;
    unsigned short cRelationFldID, eCompressFldID;
    short psCodeLib, StorageLen;

    short   eExprFldLen, cBrifFldLen, cRelationFldLen;
    long    cRelationSpace;
    long    OffsetFile;
    long    DiskPhyOffset;

    // These varitities are used in compress_writed
    char  chBuf;
    short iBuf;
    long  lBuf;
    char *fieldName[2];
    char  sbuf[256];
    char *seeds[16];
    long  l, ll;
    /*--------------------------------------------------------------------*/

    if( InitFlag == 1 ) {  /*IF    a HZTH has been there
			    *THEN  Firstly, you must hClose() it!
			    */
	hClose();           // Make InitFlag = 0
    }

    fieldName[1] = NULL;

    psCodeLib = StorageLen = 0;
    OffsetFile = 0;
    DiskPhyOffset = 0;

    // global variable inititlize
    LeftPageSpace = CODE_LIB_PAGE_SIZE;

    dfCtrl = dopen( CodeDbfControlName, (short)(O_RDONLY|O_BINARY), (short)SH_DENYRW, (short)S_IREAD );
    if( dfCtrl == NULL ) {
	hERROR = NO_LIB;
	return  0xFFFF;
    }

    eCompressFldID = GetFldid( dfCtrl, eCompressFldName );
    cExprFldID     = GetFldid( dfCtrl, cExprFldName );
    dbpathFldID    = GetFldid( dfCtrl, HZTH_DBPATH );

    if( eCompressFldID == 0xFFFF || cExprFldID == 0xFFFF ) {
	hERROR = NO_FIELD;
	return  0xFFFF;
    }

    MAX_CODE_DBF_NUM = (short)dfCtrl->rec_num;

    /*-------------------Finish the open of the CODE.DBF-------------------*/

    /*-------------------------------------------------------
     *  Make path of code.cod
     *  that means there is the ccode.cod &
     *  dcode.cod & all the *lib in the same direction
     *----------
     */

    SplitPath = strdup( CodeDbfControlName );

    if( ( nStr = strrchr( SplitPath, '\\' ) ) != NULL ) {
	      nStr++;
	      nStr[0] = '\0';
    }  else  // End of IF
	      SplitPath[0] = '\0';       // Get the split_path of CtrlLibPath

    strcpy( FileName, SplitPath );
    if( ( nStr = strrchr( CodeDbfControlName, '\\' ) ) != NULL ) {
	      nStr++;
	      strcat( FileName, nStr );
    } else
	      strcat( FileName, CodeDbfControlName );  /* Get the file name
							* include path
							*/
    if( ( nStr = strrchr( FileName, '.' ) ) != NULL )  nStr[0] = '\0';

    strcat( FileName, FILENAME_EXT );            // Make...\\CODE.COD
#ifdef _HzthRuningMessage_Assigned
	HzthRuningMessage( FileName );
#endif
    if( ( fpCtrl = fopen( FileName, "wb" ) ) == NULL ) {
	dERROR = NO_OPEN;
	free( SplitPath );
	return     0xFFFF;
    }
    fseek(fpCtrl, 0, SEEK_SET);

    strcpy( FileName, SplitPath );             // Make CCODE.COD
    strcat( FileName, "C" );

    if( ( nStr = strrchr( CodeDbfControlName, '\\' ) ) != NULL ) {
	      nStr++;
	      strcat( FileName, nStr );
    } else
	      strcat( FileName, CodeDbfControlName );  /* Get the file name
						 * include path
						 */
    if( ( nStr = strrchr( FileName, '.' ) ) != NULL )  nStr[0] = '\0';

    strcat( FileName, FILENAME_EXT );           // Make...\\CCODE.COD
#ifdef _HzthRuningMessage_Assigned
	HzthRuningMessage( FileName );
#endif
    if( ( fpCode = fopen( FileName, "wb" ) ) == NULL ) {
	free( SplitPath );
	fclose( fpCtrl );
	hERROR = NO_OPEN;
	return    0xFFFF;
    }
    fseek(fpCode, 0, SEEK_SET);

    strcpy( FileName, SplitPath );           // Make DCODE.COD
    strcat( FileName, "D" );

    if( ( nStr = strrchr( CodeDbfControlName, '\\' ) ) != NULL ) {
	      nStr++;
	      strcat( FileName, nStr );
    } else
	      strcat( FileName, CodeDbfControlName );   /* Get the file name
						  * include path
						  */
    if( ( nStr = strrchr( FileName, '.' ) ) != NULL )  nStr[0] = '\0';

    strcat( FileName, FILENAME_EXT );                   // Make...\\DCODE.COD
#ifdef _HzthRuningMessage_Assigned
	HzthRuningMessage( FileName );
#endif
    if( ( fpWords = fopen( FileName, "wb" ) ) == NULL ) {
#ifdef _HzthRuningMessage_Assigned
	    char buf[256];
	    sprintf(buf, "无法打开文件:%s", FileName);

	    tgErrorWin( buf );
#endif
	free( SplitPath );
	fclose( fpCtrl );
	fclose( fpCode );
	hERROR = NO_OPEN;
	return    0xFFFF;
    }
    fseek( fpWords, 0, dSEEK_SET);

    // Finish build file
    while(  psCodeLib < dfCtrl->rec_num ) {

	getrec( dfCtrl );

	if( dfCtrl->rec_buf[0] == '*' ) {
		psCodeLib++;
		continue;
	}


	//1998.5.14
	//get_fld( dfCtrl, cExprFldID, CodeLibMes.DbfName);
	get_fld( dfCtrl, cExprFldID, sbuf );
	seperateStr( sbuf, '`', seeds );
	if( seeds[0] != NULL ) {
		eExprFldName=HZTH_E_EXPR;
		cBrifFldName = HZTH_C_BRIF;
		cRelationFldName=HZTH_C_RELATION;
		strZcpy(CodeLibMes.DbfName, seeds[0], 13);
		if( seeds[1] != NULL ) {
		    eExprFldName = seeds[1];
		    if( seeds[2] != NULL ) {
			cBrifFldName = seeds[2];
			if( seeds[3] != NULL )
				cRelationFldName = seeds[3];
		    }
		} //end of if
	} else {
#ifdef _HzthRuningMessage_Assigned
	    tgErrorWin( "Describe with ` error" );
#endif
	    free( SplitPath );
	    fclose( fpCtrl );
	    fclose( fpCode );
	    fclose( fpWords );
	    dclose( dfCtrl  );

	    hERROR = NO_LIB;
	    return     0xFFFF;
	} //end of modi in 1997

	fieldName[0] = eExprFldName;
	ltrim( CodeLibMes.DbfName );
/*	if( strrchr( CodeLibMes.DbfName, '.' ) == NULL )
		strcat( CodeLibMes.DbfName, ".DBF" );
*/
	if( dbpathFldID != 0xFFFF ) {
		get_fld( dfCtrl, dbpathFldID, sbuf );
		if( strchr(sbuf, '\\') != NULL ) {
		    beSurePath( sbuf );
		    strcpy( FileName, sbuf );
		    strcat( FileName, CodeLibMes.DbfName );
		} else {
		    char *s;

		    lrtrim( sbuf );
		    if( stricmp(sbuf, "SYSTEM") == 0 ) {
			s = GetCfgKey(csuDataDictionary, "SYSTEM", "DBROOT", "PATH");
		    } else {
			s = GetCfgKey(csuDataDictionary, "DATABASE", sbuf, "PATH");
		    }
		    if( s != NULL ) {
			strcpy( FileName, s );
		    } else {
			//error
			free( SplitPath );
			fclose( fpCtrl );
			fclose( fpCode );
			fclose( fpWords );
			dclose( dfCtrl  );

			hERROR = ERR_CODE;
			return  0xFFFF;
		    }
		}
	} else {
	    strcpy( FileName, SplitPath );
	    strcat( FileName, CodeLibMes.DbfName );
	}

#ifdef ONTEST
   printf( "正在编译代码库 %s           \r", FileName);
#endif
#ifdef _HzthRuningMessage_Assigned
	HzthRuningMessage( FileName );
#endif

	if( ( df = dopen( FileName, (short)(O_RDONLY|O_BINARY), (short)SH_DENYRW, (short)S_IREAD ))==NULL ) {
#ifdef _HzthRuningMessage_Assigned
	    char buf[256];

	    sprintf(buf, "无法打开文件:%s", FileName);
	    
	    tgErrorWin( buf );
#endif
	    free( SplitPath );
	    fclose( fpCtrl );
	    fclose( fpCode );
	    fclose( fpWords );
	    dclose( dfCtrl  );

	    hERROR = NO_LIB;
	    return     0xFFFF;
	}

	//  Location of Output Field  */
	cBrifFldID  = GetFldid( df, cBrifFldName );
	eExprFldID  = GetFldid( df, eExprFldName );
	cRelationFldID = GetFldid( df, cRelationFldName );

	if( cBrifFldID == 0xFFFF || eExprFldID == 0xFFFF ) {
	      free( SplitPath );
	      fclose( fpCtrl );
	      fclose( fpCode );
	      fclose( fpWords );
	      dclose( dfCtrl  );

	      hERROR = NO_FIELD;
	      return    0xFFFF;
	}

	eExprFldLen = (short)(df->field[eExprFldID].fieldlen);
	cBrifFldLen = (short)(df->field[cBrifFldID].fieldlen);

	if( cRelationFldID != 0xFFFF )
	    cRelationFldLen = (short)(df->field[cRelationFldID].fieldlen);
	else
	    cRelationFldLen = 0;

	// test if there is a relation field
	if( cRelationFldID == 0xFFFF ) {
		StorageLen = cBrifFldLen;
		CodeLibMes.StorageLen = (unsigned char)cBrifFldLen;
	} else {
		StorageLen = cBrifFldLen + cRelationFldLen;
		CodeLibMes.StorageLen = (unsigned char)StorageLen;
	}

	// The code taked memory

	//1999.11.4 Xilong Pei, get the max size
	cRelationSpace = StorageLen * df->rec_num;

	if( psCodeLib > 0 ) { // It isn't the first
		TransCodeBufMan( fpWords, NULL, cRelationSpace );
		/*Dispose the rest room in the current page_frame
		 *the function of the function can adjust the page_frame
		 *use the current page_frame(old) or new_alloc a page_frame
		 */
		OffsetFile = ftell( fpWords );
	}

	CodeLibMes.CodeLen = (unsigned char)eExprFldLen;

//	memset(CodeLibMes.segIndex, 0, sizeof(unsigned short)*HZTH_MAX_SEG);
	CodeLibMes.ExprLen = (unsigned char)cBrifFldLen;
	CodeLibMes.CodeOffset = DiskPhyOffset;
	CodeLibMes.BeginPageNum = OffsetFile / CODE_LIB_PAGE_SIZE;
	CodeLibMes.BeginPageOffset = OffsetFile % CODE_LIB_PAGE_SIZE;

	//1999.11.4
	//CodeLibMes.CodeNum = df->rec_num;
	//CodeLibMes.PageNum = cRelationSpace / CODE_LIB_PAGE_SIZE;
	//if( cRelationSpace % CODE_LIB_PAGE_SIZE != 0 )   CodeLibMes.PageNum++;

	// Get the Compress method from dfCtrl
	get_fld( dfCtrl, eCompressFldID, FieldBuf);

	// Set compress_method of each record in the Code.DBF
	if( toupper( *(char *)FieldBuf ) == 'S' || CodeLibMes.CodeLen > 9 ) {
		CodeLibMes.CompressMethod = 's';
	} else {
		switch( CodeLibMes.CodeLen ) {
			case 1:
			case 2:
				CodeLibMes.CompressMethod = 'c';
				break;
			case 3:
			case 4:
				CodeLibMes.CompressMethod = 'i';
				break;
			default:
				CodeLibMes.CompressMethod = 'l';
		} // End of switch
	} // End of switch

	bh = IndexBuild((char *)df, fieldName, "HZTHNDX", BTREE_FOR_OPENDBF);
	l = df->rec_num;
	IndexGoTop(bh);

	ll = 0;
	while( l-- ) { // Dispose with each primitive code libray

	   //getrec() not get1rec, to avoid bh is NULL
	   getrec( df );

	   if( df->rec_buf[0] == '*' ) {
	      //IndexSkip() will check wether the bh is NULL
	      IndexSkip(bh, 1);
	      continue;
	   }

	   ll++;

	   get_fld( df, eExprFldID, FieldBuf );

/*	   { // count the segment info
		unsigned short zeroCnt;

		if( (zeroCnt = strrchrCnt(FieldBuf, '0')) < HZTH_MAX_SEG ) {
			(CodeLibMes.segIndex[zeroCnt])++;
		} else {
			(CodeLibMes.segIndex[HZTH_MAX_SEG-1])++;
		}
	   }
*/

	   switch( CodeLibMes.CompressMethod ) { // Write data to CCODE.COD
		case 's':
			// Write a string to Codefile
			fwrite( FieldBuf, sizeof(char), eExprFldLen, fpCode);
			DiskPhyOffset += eExprFldLen;
			break;
		case 'c':
			// Write a char or a two_digit data to CodeFile
			CompressWriteRead( FieldBuf, 'c', &CodeLibMes, &chBuf );
			fwrite( &chBuf, sizeof(char), 1, fpCode);
			DiskPhyOffset += sizeof(char);
			break;
		case 'i':
			// Write a short to  Codefile
			CompressWriteRead( FieldBuf, 'i', &CodeLibMes, &iBuf );
			fwrite( &iBuf, sizeof(short),  1, fpCode );
			DiskPhyOffset += sizeof(short);
			break;
		case 'l':
			// Write a long to  Codefile
			CompressWriteRead( FieldBuf, 'l', &CodeLibMes, &lBuf );
			fwrite( &lBuf, sizeof(long),  1, fpCode );
			DiskPhyOffset += sizeof(long);
	      } // End of switch

	      /*-----------End of write  all kinds of data to Codefile-----*/

	      // Really write chinese_words to word file--DCODE.COD
	      get_fld(df, cBrifFldID, FieldBuf);
	      if( cBrifFldLen != CodeLibMes.StorageLen )
			get_fld( df, cRelationFldID, &FieldBuf[cBrifFldLen] );

	      TransCodeBufMan( fpWords, FieldBuf, CodeLibMes.StorageLen);


	      //IndexSkip() will check wether the bh is NULL
	      IndexSkip(bh, 1);
	} /* End of while to each code libary */

	IndexDispose(bh);
	dclose( df );

	//1999.11.4
	cRelationSpace = StorageLen * ll;
	CodeLibMes.CodeNum = ll;
	CodeLibMes.PageNum = cRelationSpace / CODE_LIB_PAGE_SIZE;
	if( cRelationSpace % CODE_LIB_PAGE_SIZE != 0 )   CodeLibMes.PageNum++;

	fwrite( &CodeLibMes, sizeof(CODE_LIB_MES_STRUCT), 1, fpCtrl );
	psCodeLib++;

    } /* End of while */

    fclose( fpCtrl );
    fclose( fpWords );
    fclose( fpCode );
    dclose( dfCtrl );

    free( SplitPath );

    return      1;

} // End of function hBuildCodeLib



/*****************
*                        5. short hOpen()
*            Open code library and build inside data struct
*****************************************************************************/
_declspec(dllexport) short hOpen( char *CodeCodControlName )
{
    short handCtrl, handCode, handWords;
    char FilePath[MAX_FILESPEC], Path[MAX_FILESPEC], *npStr;
    long eExprFileLen, cExprFileLen, CtrlFileLen;
    short  i, cExprPagesSum, CurMaxPage;

    if( InitFlag == 1 )                 return     1;

    strcpy( FilePath, CodeCodControlName );
    strcpy( CtrlLibFileName, CodeCodControlName );

    // Split the control_file path */
    if( ( npStr = strrchr( FilePath, '\\' ) ) != NULL ) {
	  npStr++;
	  npStr[0] = '\0';
    } else
	  FilePath[0] = '\0';

    /*  Install the Code libriary  */
    strcpy( Path, FilePath );
    strcat( Path, "C" );
    if( ( npStr = strrchr( CodeCodControlName, '\\' ) ) != NULL )
	  npStr++;
    else
	  npStr = &CodeCodControlName[0]; // Make ...\\CCODE.

    strcat( Path, npStr );
    if( ( npStr = strrchr( Path, '.' ) ) != NULL )
	  npStr[0] = '\0';

    strcat( Path, FILENAME_EXT );           // Make ....\\CCODE.COD

    /*  READONLY Open Code Segment File  */
    handCode = open( Path, O_RDONLY | O_BINARY );
    if( handCode == -1)         return( -1 );


#ifdef UNIX_V                             // Get Numric_code_libary length
    lseek( handCode, 0L, SEEK_END );      // If length longer than 64K for DOS
    eExprFileLen = tell( handCode );
#else                                     // Then QUIT!
    eExprFileLen = filelength( handCode );//
#endif

    if( eExprFileLen <= -1 || eExprFileLen >= MAX_CODE_FILE_SIZE  ) {
	  close( handCode );
	  return      -1;
    }  /* End of if */

    CodeLibBuf = (char *)malloc( eExprFileLen + 1 );
    if( CodeLibBuf == NULL ) {
	close( handCode );
	return( -1 );
    }  /* End of if */

    lseek( handCode, 0L, SEEK_SET );
    read( handCode, &CodeLibBuf[0], (unsigned)eExprFileLen );
    close( handCode );       /*Form the Numeric_buffer_table */

    /*  Install Control Code Libriary */
    strcpy( Path, CodeCodControlName );
    if( ( npStr = strrchr( Path, '.') ) != NULL ) npStr[0] = '\0';
    strcat( Path, FILENAME_EXT );      // Make "CODE.COD"==Control_file */

    /*  READONLY Open Code Libriary Control File  */
    handCtrl = open( Path, O_RDONLY|O_BINARY );
    if( handCtrl < 0 ) {
	free( CodeLibBuf );
	return( -1 );
    }

#ifdef UNIX_V
    lseek( handCtrl, 0L, SEEK_END );
    CtrlFileLen = tell( handCtrl );
#else
    CtrlFileLen = filelength( handCtrl );
#endif

    MAX_CODE_DBF_NUM =   CtrlFileLen / sizeof( CODE_LIB_MES_STRUCT );

    // Get the number of Control_Libriary_table
    CodeDbfCtrlTab = calloc(  MAX_CODE_DBF_NUM, \
				sizeof( CODE_LIB_MES_STRUCT ) );

    if( CodeDbfCtrlTab == NULL ) {
	close( handCtrl );
	free( CodeLibBuf );
	return( -1 );
    }

    lseek( handCtrl, 0L, SEEK_SET );
    for( i = 0; i < MAX_CODE_DBF_NUM; i++ ) // Make the global control_table
	 read( handCtrl, &CodeDbfCtrlTab[i], \
			(unsigned)sizeof( CODE_LIB_MES_STRUCT ));

    close( handCtrl );

    // Install the dcode.cod libriary  */
    strcpy( Path, FilePath );
    strcat( Path, "D" );
    if( ( npStr = strrchr( CodeCodControlName, '\\' ) ) != NULL )
	  npStr++;
    else
	  npStr = &CodeCodControlName[0]; // Make ...\\DCODE.

    strcat( Path, npStr );
    if( ( npStr = strrchr( Path, '.' ) ) != NULL )
	  npStr[0] = '\0';

    strcat( Path, FILENAME_EXT );           // Make ...\\DCODE.COD

    // Open Chinese code libriary and Install in memory  */
    handWords = open( Path, O_RDONLY | O_BINARY );
    HAND_Words = handWords;
    if( handWords == -1 ) {

	free( CodeDbfCtrlTab );

	CodeDbfCtrlTab = NULL;
	free( CodeLibBuf );
	CodeLibBuf = NULL;

	return      -1;
    } /* End of if*/

#ifdef  UNIX_V
    lseek( handWords, 0L, SEEK_END );
    cExprFileLen = tell( handWords );
#else
    cExprFileLen = filelength( handWords );
#endif

    // Get the number of page_frame of the chinese words file
    cExprPagesSum = cExprFileLen / CODE_LIB_PAGE_SIZE;
    if( cExprFileLen % CODE_LIB_PAGE_SIZE != 0 ) cExprPagesSum++;

    //1999.10.11
    if( MAX_PAGE <= 0 )
	MAX_PAGE=DEF_MAX_PAGE;

    // Set the right page quantites
    CurMaxPage = (unsigned)( ( MAX_PAGE > ( (unsigned)cExprPagesSum ) ) \
				? ( (unsigned)cExprFileLen ) : MAX_PAGE );
    MAX_PAGE = CurMaxPage;

    CurMaxPage *= 2;   // Alloc memory_page_frame sumed up MaxPage
    do {

	CurMaxPage /= 2;
	WordsPageTab = calloc( CurMaxPage, sizeof( CODE_PAGE_STRUCT ) );

    } while( WordsPageTab == NULL && CurMaxPage >= 1 );

    if( CurMaxPage < 1 || WordsPageTab == NULL ) {
       /*If    Can't alloc memory for the page_frame
	*Then  CLOSE all the job has been done
	*/

	if( WordsPageTab != NULL ) {
	    free( WordsPageTab );
	    WordsPageTab = NULL;
	}

	free( CodeDbfCtrlTab );
	CodeDbfCtrlTab = NULL;
	free( CodeLibBuf );
	CodeLibBuf = NULL;
	close( handWords );
	return    -1;
    }

    MAX_PAGE = CurMaxPage;

    lseek( handWords, 0L, SEEK_SET );
    for( i = 0; i < MAX_PAGE; i++ ) {    /*Continuely load the chinese_wrods
					  *Fill in the chinese_words_page_frame
					  */
	WordsPageTab[i].PageNo = i;
	WordsPageTab[i].PageUseTimes = 1;

	read( handWords, WordsPageTab[i].chbuf, (unsigned)CODE_LIB_PAGE_SIZE );
    }

    InitFlag = 1;
    return      1;

} // End of function hOpen()


/*****************
*                 6. unsigned char hGetCodeDbfId()
*        Get the ID of the code_lib short global_control_table
*****************************************************************************/
_declspec(dllexport) short hGetCodeDbfId( char *CodeDbfName )
{
    unsigned short i;
    char SrcCodeDbfName[MAX_FILESPEC];

    if( CodeDbfName == NULL )
	return  -1;

     // Make code_lib_.dbf */
    strcpy( SrcCodeDbfName, CodeDbfName );
    if( strrchr( SrcCodeDbfName, '.' ) == NULL )
	strcat( SrcCodeDbfName, ".DBF" );

    for( i = 0;  i < MAX_CODE_DBF_NUM;  i++ ) {
	 if( !stricmp( CodeDbfCtrlTab[i].DbfName, SrcCodeDbfName ) ) {
		return     i; /*Find the order_number of the chinese_code.dbf
			       *short the global control_table
			       */
	 }
    }

    return  -1;

} // End of function hGetCodeDbfId()



/*===================
*                 8. short TransCodeBufMan()
*           Write  Chinese  Words   to  Stream  File
*===========================================================================*/
unsigned short TransCodeBufMan( FILE *stream, char *buf, long Len )
{
    char c=' ';

    if( buf == NULL ) { /*Alloc a Page fame
			 *In order to write for the first time
			 */
	if( Len > LeftPageSpace ) {
	    /*
	     *------------------------
	     *IF the all  bytes of the
	     *chinese words will taked up, which included in the
	     *code_library maybe more than the capaticity of a cur_page_frame
	     *THEN re_alloc a page_frame
	     *OR use shared the page_frame with current page_frame.
	     *------------------------
	     */

	     fwrite( &c, sizeof(char), LeftPageSpace, stream );
/*	      while( LeftPageSpace-- > 0 ) {
		  fwrite( &c, sizeof(char), 1, stream );
	      } // Only write in the size of page_ frame
*/	      LeftPageSpace = CODE_LIB_PAGE_SIZE;
	 }
	 return  1;
    }   /* End of if */

   if( LeftPageSpace < Len ) {   /* If   there isn't no more space for a words
			      * Then write ' ' to the rubblish
			      */

	while( LeftPageSpace-- ) {
	    fwrite( &c, sizeof(char), 1, stream );
	}
	LeftPageSpace = CODE_LIB_PAGE_SIZE;
   } // End of if

   // If there is enough memory to hold the words
   fwrite( buf, sizeof(char), Len, stream);
   LeftPageSpace  -= (short)Len;
   return    0;

}  // End of function TransCodeBufMan()



/*================
*                 9. short CompressWriteRead()
*   While you make file of Numeric Code, you can call it in this pattern
*         :CompressWriteRead( SrcCode, LowerLetter, DestCode, Code_Lib_Mes );
*   While you read from Numeric Code,  you can call it in this pattern
*         :CompressWriteRead( SrcCode, UpperLetter, DestCode, Code_Lib_Mes );
*   Both pattern it can return the requested EnCompressCode or DeCompressCode!
*===========================================================================*/
unsigned short CompressWriteRead( void *SrcCode, char rwFlag, \
			 CODE_LIB_MES_STRUCT *Code_LibMes, void *DestCode )
{

    char DestStr[MAX_EEXPR_LEN];
    short i, j;
    // When rwFlag is lower case, that is compress a code
    // Else De_compress

//    memset( (char *)&DestStr[0], '\0', Code_LibMes->CodeLen+1);
    switch( rwFlag ) {  // En_Compress the string

	  case 's':     // It is a string. You Can't Compress it.
		strcpy( (char *)DestCode, (char *)SrcCode );
		return  10;
	  case 'c':     // IF it is a  char you can't compressit
			// But if it is a data you can PRESS it!
		if( Code_LibMes->CodeLen == 1 )
			*(char *)DestCode = *(char *)SrcCode;
		else
			*(char *)DestCode = atoi( (char *)SrcCode );
		return  1;
	  case 'i':      // It is four_digit data
		*(short *)DestCode = atoi( SrcCode );
		return  2;
	  case 'l':      // It is a five--- nine Data
		*(long *)DestCode = atol( SrcCode );
		return  1;
	  case 'S':      // It is a string
		strcpy( (char *)DestCode, (char *)SrcCode );
		return  10;
	  case 'C':      // It is a char or a two_digit data
	      if( Code_LibMes->CodeLen == 1 ) {
		     *(char *)DestCode = *(char *)SrcCode;
		     *((char *)DestCode+1) = '\0';
		     return  1;
		} else {
		     if( *(char *)SrcCode < '\x0A' ) {
			sprintf( (char *)DestCode+1, "%d", *(char *)SrcCode );
			*(char *)DestCode = '0';
		     } else {
			sprintf( (char *)DestCode, "%d", *(char *)SrcCode );
		     }
		}
		return   1;
	  case 'I':      // It is a Four_digit data
		i = sprintf( DestStr, "%d", *(short *)SrcCode );
		j = Code_LibMes->CodeLen - i;
		memset(DestCode, '0', j);
		strcpy((char *)DestCode + j, DestStr);
		return   2;
	  case 'L':      // It is a five--nine digit data
		i = sprintf( DestStr, "%ld", *(long *)SrcCode );
		j = Code_LibMes->CodeLen - i;
		memset(DestCode, '0', j);
		strcpy((char *)DestCode + j, DestStr);
		return   4;
	  default:
		return  0;
    } // End of switch  */
}



/******************
*                  10. char hReplaceByName()
*         Single field replace with code name.
****************************************************************************/
_declspec(dllexport) char *hReplaceByName( char *CodeDbfName, char *Code )
{
    short CodeDbfLib_ID;

    if( CodeDbfName == NULL || Code == NULL ) {
	hERROR = NO_PARA;
	return  NULL;
    }

    CodeDbfLib_ID = hGetCodeDbfId( CodeDbfName );
    if( CodeDbfLib_ID == -1 || CodeDbfLib_ID >= MAX_CODE_DBF_NUM ) {
	 hERROR = ERR_LIB;
	 return  NULL;
    }

    return  hReplace( CodeDbfLib_ID, Code );

} // End of function hReplaceByName



/*******************
*                    11. char *hReplaceById()
*             Single field replace with code No.
*****************************************************************************/
_declspec(dllexport) char *hReplaceById( short CodeDbfId, char *Code )
{

     if( CodeDbfId >= MAX_CODE_DBF_NUM || CodeDbfId < 0 ) {
	 hERROR = ERR_LIB;
	 return  NULL;
     }

     if( Code == NULL ) {
	 hERROR = NO_PARA;
         return  NULL;
     }

     return        hReplace( CodeDbfId,  Code );

} // End of function hReplaceById()


/*=================
*                     12. hReplace ()
*         It can make ReplaceById() & ReplaceByName into realizes
*===========================================================================*/
unsigned char *hReplace( unsigned short CodeLibNo, char *SrcCode )
{
    int    cWordsPosition, cWordsNum, i;
    short  FirstPageWordsNum, LeftWordsNum, cWordsLen;
    unsigned short PageOffset;
    long   PageNo;
    char   CodeBuf[MAX_EEXPR_LEN];
    short  cExprLen;

    // Only when the global varity of CodeDbfCtrlTab is non NULL
    // It can prove to that you have hOpen() the hzth moudle for
    // the success!
    if( CodeDbfCtrlTab == NULL ) {
	hERROR = NO_OPEN;
	return    NULL;
    }

    if( strchr(SrcCode, '`') != NULL ) {
	return  strZcpy(cWords, strchr(SrcCode, '`')+1, MAX_HZTH_RESULT_LEN-1);
    }

    // Get index value from  chinese_begin_page
    switch( CodeDbfCtrlTab[CodeLibNo].CompressMethod ) {
	case 'c':
	case 'i':
	case 'l':
		i = 0;
		while( isdigit( SrcCode[i] ) )		i++;

		if( SrcCode[i] != '\0' ||  \
		    strlen( SrcCode ) != CodeDbfCtrlTab[CodeLibNo].CodeLen ) {
		    hERROR = ERR_CODE;
		    return  NULL;
		}

		CompressWriteRead( SrcCode, \
				   CodeDbfCtrlTab[CodeLibNo].CompressMethod, \
				   &CodeDbfCtrlTab[CodeLibNo], CodeBuf );
		cWordsPosition = HZTHbsearch( CodeLibNo, CodeBuf );
		break;
	case 's':
	default:
		cWordsPosition = HZTHbsearch(CodeLibNo, SrcCode);
    } // End of check

    if( cWordsPosition < 0 )
	return     NULL;

    switch( CodeDbfCtrlTab[CodeLibNo].CompressMethod ) {
	case 'i':
		cWordsPosition /= sizeof(short);
		break;
	case 'l':
		cWordsPosition /= sizeof(long);
		break;
/*	case 's':
		cWordsPosition /= CodeDbfCtrlTab[CodeLibNo].CodeLen;
*/    }

    cWordsLen = CodeDbfCtrlTab[CodeLibNo].StorageLen;

    // How many this kind words it cab be in each page_number
    cWordsNum = CODE_LIB_PAGE_SIZE / cWordsLen;

    // How many chinese words, contented in the first page */
    FirstPageWordsNum = ( CODE_LIB_PAGE_SIZE - \
			 CodeDbfCtrlTab[CodeLibNo].BeginPageOffset ) / cWordsLen;

    if( cWordsPosition < FirstPageWordsNum ) {  /*If the chinese_code in the
						 *first page_frame
						 */
	PageNo = CodeDbfCtrlTab[CodeLibNo].BeginPageNum;
	PageOffset = CodeDbfCtrlTab[CodeLibNo].BeginPageOffset + \
		     cWordsPosition * cWordsLen;
    } else {                                     /*Else borned new page_frame
						  *order_number & page_offset
						  */
	LeftWordsNum = cWordsPosition - FirstPageWordsNum;

	PageNo = CodeDbfCtrlTab[CodeLibNo].BeginPageNum + 1 + \
		 LeftWordsNum / cWordsNum;

	PageOffset = ( LeftWordsNum % cWordsNum) * cWordsLen;
    }

    // Get Corresponding Chinese_string real & phycial  Page
    PageNo = LocateFetchPage( PageNo );

    if( PageNo > MAX_PAGE ) {
	hERROR = NO_CODE;
	return( NULL );
    }

    cExprLen = CodeDbfCtrlTab[CodeLibNo].ExprLen;
    strncpy( cWords, &WordsPageTab[PageNo].chbuf[PageOffset], cExprLen );

    cWords[cExprLen] = '\0';

    return      cWords;

} // End of function hReplace()




/*=================
*                     13. LocateFetchPage()
*        If the requested page in the memory then return pageno
*        But if it is fault_page then  fetch the requested page int hard disk
*===========================================================================*/
long LocateFetchPage( long PageNo )
{

    int  MinUseTimes, ReplacePageNo, i;

    MinUseTimes = MAX_PAGE_USETIMES;
    // Get a replaced page
    // ReplacePageNo = 0;
    for( i = 0; i < MAX_PAGE && WordsPageTab[i].PageNo != PageNo; i++ )
	if( MinUseTimes > WordsPageTab[i].PageUseTimes ) {
		MinUseTimes = WordsPageTab[i].PageUseTimes;
		ReplacePageNo = i;
	}

    if( i < MAX_PAGE )          ReplacePageNo = i;

    // Fetch a new Page if it is page_fault
    if( i >= MAX_PAGE ) { // HAND_Words is a global chinese wods file handle
	WordsPageTab[ReplacePageNo].PageNo = PageNo;

	lseek( HAND_Words, (long)( (long)PageNo * CODE_LIB_PAGE_SIZE ), SEEK_SET );
	read( HAND_Words, WordsPageTab[ReplacePageNo].chbuf, CODE_LIB_PAGE_SIZE);
    }

    if( ++WordsPageTab[ReplacePageNo].PageUseTimes >= MAX_PAGE_USETIMES ) {
						/*If MAP page_frame in the memory
						 *Then  use_times ++
						 */
	for( i = 0; i < MAX_PAGE; i++ ) {
		if( ( WordsPageTab[i].PageUseTimes /= 3 ) <= 0 )
			WordsPageTab[i].PageUseTimes++;
	}
    }
    /*------
     *Detect the use_times of each page that it's use_times has more
     *than MAX_PAGE_USETIMES
     */

    return      ReplacePageNo;

} // End of function LocateFetchPage



/*====================
*                    14. int HZTHbsearch()
*===========================================================================*/
int HZTHbsearch( unsigned short CodeLibNo, void *SrcCode )
{
    char *ptr;

    switch( CodeDbfCtrlTab[CodeLibNo].CompressMethod ) {
	case 'c':
	  ptr =  bsearch((char *)SrcCode, \
		 &CodeLibBuf[CodeDbfCtrlTab[CodeLibNo].CodeOffset], \
		 CodeDbfCtrlTab[CodeLibNo].CodeNum, \
		 sizeof( char ), \
		 (ComparePtr)CharComp
		 );
	  if( ptr == NULL )           return   -1;
	  return abs(&CodeLibBuf[CodeDbfCtrlTab[CodeLibNo].CodeOffset]-ptr);

	case 'i':
	  ptr =  bsearch((short *)SrcCode, \
		 &CodeLibBuf[CodeDbfCtrlTab[CodeLibNo].CodeOffset], \
		 CodeDbfCtrlTab[CodeLibNo].CodeNum, \
		 sizeof( short ), \
		 (ComparePtr)ShortComp
		 );
	  if( ptr == NULL )            return  -1;
	  return abs(&CodeLibBuf[CodeDbfCtrlTab[CodeLibNo].CodeOffset]-ptr);
	case 'l':
	  ptr =   bsearch((long *)SrcCode, \
		  &CodeLibBuf[CodeDbfCtrlTab[CodeLibNo].CodeOffset], \
		  CodeDbfCtrlTab[CodeLibNo].CodeNum, \
		  sizeof( long ), \
		  (ComparePtr)LongComp
		  );
	   if( ptr == NULL )            return   -1;
	   return abs(&CodeLibBuf[CodeDbfCtrlTab[CodeLibNo].CodeOffset]-ptr);
	case 's':
	  return  Strbsearch( CodeLibNo, SrcCode );
       }  // End of switch

       return  0;
}



/*==============
*                    15. Only use in  HZTHbsearch()
*===========================================================================*/
int CharComp( const char *Char1, const char *Char2 )
{
   if( *Char1 > *Char2 )     return  1;
   if( *Char1 < *Char2 )     return  -1;
   return  0;
}

int ShortComp( const short *Short1, const short *Short2 )
{
   if( *Short1 > *Short2 )     return  1;
   if( *Short1 < *Short2 )     return  -1;
   return  0;
}

int LongComp( const long *Long1, const long *Long2 )
{
    if( *Long1 > *Long2 )       return  1;
    if( *Long1 < *Long2 )       return  -1;
    return  0;
}



/*===================
*                  16. short Strbseach()
*        When compress_method is 's', you must it to bin_search
*===========================================================================*/
int Strbsearch(  unsigned short CodeLibNo, char *SrcCode )
{
    long  Low=0, High, Mid;
    int   SearchResult;
    short CodeLen = CodeDbfCtrlTab[CodeLibNo].CodeLen;
    long  CodeOffset, offset;


    offset = CodeDbfCtrlTab[CodeLibNo].CodeOffset;
    High = CodeDbfCtrlTab[CodeLibNo].CodeNum - 1;

    while( Low <= High ) {

	Mid = ( Low + High) / 2;
	CodeOffset = offset + Mid * CodeLen;
	if( !( SearchResult = strncmp( SrcCode, &CodeLibBuf[CodeOffset], \
								CodeLen ) ) )
		return    Mid;

	if( SearchResult > 0 )          Low  = Mid + 1;
	else                            High = Mid - 1;
    }
    return    -1;
}   /* End of function Strbsearch */



/*******************
*CODE_STRUCT *hGetCodeRecByName( char *CodeDbfName, unsigned short BeginPos,\
				 char *ModStr, CODE_STRUCT *CodeStruct, \
				 char FunSel );
*        17. Search for correspond chinese string with code libriary name.
*            ModStr is a module.
*            FunSel:
	      'N': get the BeginPos'th code record
*             'S': get the record which start with ModStr
*                  (e.g. "12": "1234", "1245"...) FROM BeginPos
*             'E': get the record which end with ModStr
*                  (e.g. "00": "1200", "2300"...) FROM BeginPos
*              'M': get the record which is 'equal' to the record.
*                  (e.g. "0*10": "0110", "0210"...) FROM BeginPos.
*             'F': get the record which is fuzzy suit for ModStr
*                  (e.g. "Inst": "ShangHai Railway Inst.",
*                  "Inst of Intelligent", ...) FROM BeginPos
*****************************************************************************/
_declspec(dllexport) CODE_STRUCT *hGetCodeRecByName( char *CodeDbfName, unsigned short BeginPos,\
				char *ModStr, CODE_STRUCT *CodeStruct, \
				char FunSel )
{
     short CodeDbfId;

     CodeDbfId = hGetCodeDbfId( CodeDbfName );
     if( CodeDbfId >= MAX_CODE_DBF_NUM || CodeDbfId < 0 ) {
	 hERROR = ERR_LIB;
	 return  NULL;
     }

     if( BeginPos >= CodeDbfCtrlTab[CodeDbfId].CodeNum ) {
	 hERROR = NO_PARA;
	 return  NULL;
     }

     return   hGetCodeRec( CodeDbfId, BeginPos, ModStr, CodeStruct, FunSel );

} // End of hGetCodeRecByName()



/**********************
*       CODE_STRUCT *hGetCodeRecById( unsigned short CodeDbfId, \
*                                     unsigned short BeginPos, char *ModStr, \
*                                     CODE_STRUCT *CodeStruct, char FunSel )
*        17. Search for correspond chinese string with code libriary id.
*            ModStr is a module.
*              FunSel:
	       'N': get the BeginPos'th code record
*              'S': get the record which start with ModStr
*                  (e.g. "12": "1234", "1245"...) FROM BeginPos
*              'E': get the record which end with ModStr
*                  (e.g. "00": "1200", "2300"...) FROM BeginPos
*              'M': get the record which is 'equal' to the record.
*                  (e.g. "0*10": "0110", "0210"...) FROM BeginPos.
*              'F': get the record which is fuzzy suit for ModStr
*                  (e.g. "Inst": "ShangHai Railway Inst.",
*                  "Inst of Intelligent", ...) FROM BeginPos
*****************************************************************************/
_declspec(dllexport) CODE_STRUCT *hGetCodeRecById( short CodeDbfId, \
				     unsigned short BeginPos, char *ModStr, \
				     CODE_STRUCT *CodeStruct, char FunSel )
{
    //... ... -2*MAX_CODE_DBF_NUM ... ... -MAX_CODE_DBF_NUM ... ... 0 ... ... MAX_CODE_DBF_NUM
    //... ... ec_m8(#1,2)		  ec_m8(*1,2)		     ec_m8(1,2)
 
    if( CodeDbfId < 0 ) {
	 if( CodeDbfId < -SHRT_MIN+2 ) {
	    hERROR = ERR_LIB_ID;
	    return  NULL;
	 }

	 if( CodeDbfId > -MAX_CODE_DBF_NUM )
	    CodeDbfId = -CodeDbfId;
	 else {
	    CodeDbfId = -CodeDbfId-MAX_CODE_DBF_NUM;
	 }
     }

     if( CodeDbfId >= MAX_CODE_DBF_NUM  ) {
	hERROR = ERR_LIB_ID;
	return  NULL;
     }

     if( BeginPos >= CodeDbfCtrlTab[CodeDbfId].CodeNum ) {
	hERROR = NO_PARA;
	return  NULL;
     }

     return   hGetCodeRec( CodeDbfId, BeginPos, ModStr, CodeStruct, FunSel );

} // End of hGetCodeRecById()



/**********************
*       CODE_STRUCT *hGetCodeRecByMethod( HZTH_METHOD_STRU *HzthMethod, \
*                                     unsigned short BeginPos, char *ModStr, \
*                                     CODE_STRUCT *CodeStruct, char FunSel )
*        17. Search for correspond chinese string with code libriary id.
*            ModStr is a module.
*****************************************************************************/
_declspec(dllexport) CODE_STRUCT *hGetCodeRecByMethod( HZTH_METHOD_STRU *HzthMethod, \
				     unsigned short BeginPos, char *ModStr, \
				     CODE_STRUCT *CodeStruct )
{
     short CodeDbfId = HzthMethod->CodeDbfId;
     char str[MAX_EEXPR_LEN];

     if( CodeDbfId < 0 ) {
	CodeDbfId = -CodeDbfId;
        if( CodeDbfId >= MAX_CODE_DBF_NUM ) {
                hERROR = NO_PARA;
		return  NULL;
        }

	memset(str, '0', CodeDbfCtrlTab[CodeDbfId].CodeLen);
	memcpy(str, ModStr, strlen(ModStr));
	memset(str+HzthMethod->chStart, '*', HzthMethod->chFldLen);

	if( BeginPos > CodeDbfCtrlTab[CodeDbfId].CodeNum ) {
		hERROR = NO_PARA;
		return  NULL;
	}

	return   hGetCodeRec( CodeDbfId, BeginPos, ModStr, CodeStruct, 'M' );
     }

     return  NULL;

} // End of hGetCodeRecByMethod()




/*================
*  18.  CODE_STRUCT *hGetCodeRec( unsigned short CodeDbfId, \
*                                 unsigned short BeginPos, char *ModStr, \
*                                 CODE_STRUCT *CodeStruct, char FunSel )
*            Search for correspond chinese string with code libriary id.
*            ModStr is a module.
*              FunSel:
*              'N': get the BeginPos'th code record
*              'S': get the record which start with ModStr
*                  (e.g. "12": "1234", "1245"...) FROM BeginPos
*              'E': get the record which end with ModStr
*                  (e.g. "00": "1200", "2300"...) FROM BeginPos
*              'M': get the record which is 'equal' to the record.
*                  (e.g. "0*10": "0110", "0210"...) FROM BeginPos.
*              'F': get the record which is fuzzy suit for ModStr
*                  (e.g. "Inst": "ShangHai Railway Inst.",
*                  "Inst of Intelligent", ...) FROM BeginPos
*    Caution:
*        CodeStruct must be initialized when the first call
*===========================================================================*/
CODE_STRUCT *hGetCodeRec( short CodeDbfId, \
				 unsigned short BeginPos, char *ModStr, \
				      CODE_STRUCT *CodeStruct, char FunSel )
{
   char  chBuf, CompressMethod, *sWords;
   short iBuf;
   long  CodeNum;
   long  lBuf;
   char  strBuf[MAX_EEXPR_LEN], nModStr[MAX_EEXPR_LEN];
   short CodeLen, j;
   long  CodeOffset;
   int   i, ModStrLen;

   CodeOffset = CodeDbfCtrlTab[CodeDbfId].CodeOffset;
   CodeLen    = CodeDbfCtrlTab[CodeDbfId].CodeLen;
   CompressMethod = CodeDbfCtrlTab[CodeDbfId].CompressMethod;
   CodeNum    = CodeDbfCtrlTab[CodeDbfId].CodeNum;

   CodeStruct->DbfNo = CodeDbfId;
   CodeStruct->Code[0] = '\0';
   CodeStruct->Chinese[0] = '\0';

   switch( FunSel ) {
     case 'N':

       CodeStruct->QueryMethod = 'N';

       switch( CompressMethod ) {
	 case 'c':
	    chBuf = *(char *)&CodeLibBuf[CodeOffset + BeginPos];
	    CompressWriteRead( &chBuf, 'C', \
					 &CodeDbfCtrlTab[CodeDbfId], strBuf );
	    break;
	 case 'i':
	    iBuf = *(short *)&CodeLibBuf[CodeOffset + BeginPos * sizeof(short)];
	    CompressWriteRead( &iBuf, 'I', \
					 &CodeDbfCtrlTab[CodeDbfId], strBuf );
	    break;
	 case 'l':

	    lBuf = *(long *)&CodeLibBuf[CodeOffset + BeginPos * sizeof(long)];
	    CompressWriteRead( &lBuf, 'L', \
					 &CodeDbfCtrlTab[CodeDbfId], strBuf );
	    break;
	case 's':
	    strncpy(strBuf, &CodeLibBuf[CodeOffset + BeginPos * CodeLen], CodeLen );
      } // End of inner switch
      strBuf[CodeLen] = '\0';

      CodeStruct->Position = BeginPos;
      strcpy( CodeStruct->Code, strBuf );
      strcpy( CodeStruct->Chinese, hReplaceById( CodeDbfId, strBuf ) );

      return  CodeStruct;               // End of switch 'N'

      case 'S':

	CodeStruct->QueryMethod = 'S';
	strcpy( strBuf, ModStr );
	// add '*'
	ModStrLen = strlen( ModStr );
	memset( &strBuf[ ModStrLen ], '*', CodeLen - ModStrLen );
	strBuf[ CodeLen ] = '\0';
	goto hGetCodeRec_M;

     case 'E':

	CodeStruct->QueryMethod = 'E';
	// Add '*'
	ModStrLen = CodeLen - strlen( ModStr );
	memset( strBuf, '*', ModStrLen );
	strBuf[ ModStrLen ] = '\0';
	strcat( strBuf, ModStr );
	goto hGetCodeRec_M;

     case 'M':
	CodeStruct->QueryMethod = 'M';
	strcpy( strBuf, ModStr );

hGetCodeRec_M:
	ModStrLen = strlen( ModStr );
//	CodeStruct->Position = 0xFFFF;

	// if it is the first record, initialize it
	if( (j = BeginPos) == 0 ) {
		CodeStruct->CodeNo = 0;
		CodeStruct->Position = 0;
	}

	// forward test
	if( CodeStruct->CodeNo <= BeginPos ) {

	   i = CodeStruct->Position;
	   BeginPos -= CodeStruct->CodeNo;
	   CodeStruct->CodeNo = j;

	   switch( CompressMethod ) {
	       case 'c':
		   for( ;   i < CodeNum;   i++ ) {
			 chBuf = *(char *)&CodeLibBuf[CodeOffset + i];
			 CompressWriteRead(&chBuf, 'C', \
					   &CodeDbfCtrlTab[CodeDbfId], nModStr );

			if( !FuzzyCmp( strBuf, nModStr, ModStrLen ) ) {
			    if( BeginPos == 0 ) {
				CodeStruct->Position = i;
				strcpy( CodeStruct->Code, nModStr );
				strcpy( CodeStruct->Chinese, \
					  hReplaceById( CodeDbfId, nModStr ) );
				break;
			    } else {
				BeginPos--;
			    }
			} // End of if
		  } // End of for
		  break; // End of switch 'c'
	      case 'i':
		  for( ;   i < CodeNum;   i++ ) {
			 iBuf = *(short *)&CodeLibBuf[CodeOffset + i*sizeof(short)];
			 CompressWriteRead(&iBuf, 'I', \
					   &CodeDbfCtrlTab[CodeDbfId], nModStr );

			 if( !FuzzyCmp( strBuf, nModStr, ModStrLen ) ) {
			     if( BeginPos == 0 ) {
				CodeStruct->Position = i;
				strcpy( CodeStruct->Code, nModStr );
				strcpy( CodeStruct->Chinese, \
					  hReplaceById( CodeDbfId, nModStr ) );
				break;
			     } else {
				BeginPos--;
			     }
			 } // End of if
		  } // End of for
		  break; // End of switch 'i'
	    case 'l':
		  for( ;   i < CodeNum;   i++ ) {
			 lBuf = *(long *)&CodeLibBuf[CodeOffset + i*sizeof(long)];
			 CompressWriteRead(&lBuf, 'L', \
					   &CodeDbfCtrlTab[CodeDbfId], nModStr );
			 if( !FuzzyCmp( strBuf, nModStr, ModStrLen ) ) {
			     if( BeginPos == 0 ) {
				CodeStruct->Position = i;
				strcpy( CodeStruct->Code, nModStr );
				strcpy( CodeStruct->Chinese, \
					  hReplaceById( CodeDbfId, nModStr ) );
				break;
			     } else {
				BeginPos--;
			     }
			} // End of if
		     } // End of for
		     break; // End of switch 'l'
	    default:
		  for( ;   i < CodeNum;   i++ ) {
			strncpy(nModStr, &CodeLibBuf[CodeOffset + i*CodeLen], CodeLen);
			nModStr[CodeLen] = '\0';

			if( !FuzzyCmp( strBuf, nModStr, ModStrLen ) ) {
			     if( BeginPos == 0 ) {
				CodeStruct->Position = i;
				strcpy( CodeStruct->Code, nModStr );
				strcpy( CodeStruct->Chinese, \
					  hReplaceById( CodeDbfId, nModStr ) );
				break;
			     } else {
				BeginPos--;
			     }
			} // End of if
		     } // End of for

	    } // End of inner switch

	} else {

	   BeginPos = CodeStruct->CodeNo - BeginPos;
	   CodeStruct->CodeNo = j;
	   i = CodeStruct->Position;

	   switch( CompressMethod ) {
	       case 'c':
		   for( ;   i >= 0;   i-- ) {
			 chBuf = *(char *)&CodeLibBuf[CodeOffset + i];
			 CompressWriteRead(&chBuf, 'C', \
					   &CodeDbfCtrlTab[CodeDbfId], nModStr );

			if( !FuzzyCmp( strBuf, nModStr, ModStrLen ) ) {
			    if( BeginPos == 0 ) {
				CodeStruct->Position = i;
				strcpy( CodeStruct->Code, nModStr );
				strcpy( CodeStruct->Chinese, \
					  hReplaceById( CodeDbfId, nModStr ) );
				break;
			    } else {
				BeginPos--;
			    }
			} // End of if
		  } // End of for
		  break; // End of switch 'c'
	      case 'i':
		  for( ;   i >= 0;   i-- ) {
			 iBuf = *(short *)&CodeLibBuf[CodeOffset + i*sizeof(short)];
			 CompressWriteRead(&iBuf, 'I', \
					   &CodeDbfCtrlTab[CodeDbfId], nModStr );

			 if( !FuzzyCmp( strBuf, nModStr, ModStrLen ) ) {
			     if( BeginPos == 0 ) {
				CodeStruct->Position = i;
				strcpy( CodeStruct->Code, nModStr );
				strcpy( CodeStruct->Chinese, \
					  hReplaceById( CodeDbfId, nModStr ) );
				break;
			     } else {
				BeginPos--;
			     }
			 } // End of if
		  } // End of for
		  break; // End of switch 'i'
	    case 'l':
		  for( ;   i >= 0;   i-- ) {
			 lBuf = *(long *)&CodeLibBuf[CodeOffset + i*sizeof(long)];
			 CompressWriteRead(&lBuf, 'L', \
					   &CodeDbfCtrlTab[CodeDbfId], nModStr );
			 if( !FuzzyCmp( strBuf, nModStr, ModStrLen ) ) {
			     if( BeginPos == 0 ) {
				CodeStruct->Position = i;
				strcpy( CodeStruct->Code, nModStr );
				strcpy( CodeStruct->Chinese, \
					  hReplaceById( CodeDbfId, nModStr ) );
				break;
			     } else {
				BeginPos--;
			     }
			} // End of if
		     } // End of for
		     break; // End of switch 'l'
	    default:
		  for( ;   i >= 0;   i-- ) {
			strncpy(nModStr, &CodeLibBuf[CodeOffset + i*CodeLen], CodeLen);
			nModStr[CodeLen] = '\0';

			if( !FuzzyCmp( strBuf, nModStr, ModStrLen ) ) {
			     if( BeginPos == 0 ) {
				CodeStruct->Position = i;
				strcpy( CodeStruct->Code, nModStr );
				strcpy( CodeStruct->Chinese, \
					  hReplaceById( CodeDbfId, nModStr ) );
				break;
			     } else {
				BeginPos--;
			     }
			} // End of if
		     } // End of for

	    } // End of inner switch
	} // end of else
	break; // End of switch 'S', 'M', 'E'

      case 'F':
	   CodeStruct->Position = 0xFFFF;
	   CodeStruct->QueryMethod = 'F';
	   for( i = 0; i < CodeNum; i++ ) { // Find each words
	      CodeOffset  = CodeDbfCtrlTab[CodeDbfId].CodeOffset;
	      switch( CompressMethod ) {
		 case 'c':
		       CodeOffset += i;
		       chBuf = *(char *)&CodeLibBuf[CodeOffset];
		       CompressWriteRead(&chBuf, 'C', \
					 &CodeDbfCtrlTab[CodeDbfId], strBuf );
		       break;
		case 'i':
		       CodeOffset += i * sizeof(short);
		       iBuf = *(short *)&CodeLibBuf[CodeOffset];
		       CompressWriteRead(&iBuf, 'I', \
				       &CodeDbfCtrlTab[CodeDbfId],  strBuf );
		       break;
		case 'l':
		       CodeOffset += i * sizeof(long);
		       lBuf = *(long *)&CodeLibBuf[CodeOffset];
		       CompressWriteRead(&lBuf, 'L', \
					 &CodeDbfCtrlTab[CodeDbfId], strBuf );
		       break;
		default:
		       CodeOffset += i * CodeLen;
		       strncpy( strBuf, &CodeLibBuf[CodeOffset], CodeLen );
		       strBuf[CodeLen] = '\0';

	     } // End of inner switch 'Compress Method '

	     sWords =  hReplaceById( CodeDbfId, strBuf );
	     if( strstr( sWords, ModStr ) != NULL ) {
		if( BeginPos == 0 ) {
			CodeStruct->Position = CodeOffset;
			strcpy( CodeStruct->Code, strBuf );
			strcpy( CodeStruct->Chinese, sWords );
			break;
		 } else {
			BeginPos--;
		 }
	     } // End of if
	  } // End of for

      } // End of Outer switch 'FunSel'

      return     CodeStruct;

} // end of function hGetCodeRec()


/*******************
*                 char *hExtReplaceByName()
*           Single field replace with code name.
****************************************************************************/
_declspec(dllexport) char *hExtReplaceByName( char *CodeDbfName, char *Code )
{
    unsigned short CodeDbfLibID;

    if( CodeDbfName == NULL || Code == NULL ) {
	hERROR = NO_PARA;
	return  NULL;
    }

    CodeDbfLibID = hGetCodeDbfId( CodeDbfName );
    if( CodeDbfLibID >= MAX_CODE_DBF_NUM || CodeDbfLibID < 0 ) {
	 hERROR = ERR_LIB;
	 return  NULL;
    }

    return  hExtReplace( CodeDbfLibID, Code );

}  // End of hExtReplaceByName()




/******************
*                 char *hExtReplaceById()
*          Single field replace with code No.
****************************************************************************/
_declspec(dllexport) char *hExtReplaceById( short CodeDbfId, char *Code )
{
    if( CodeDbfId >= MAX_CODE_DBF_NUM ) {
	 hERROR = ERR_LIB;
	 return  NULL;
     }

     if( Code == NULL ) {
	 hERROR = NO_PARA;
	 return       NULL;
     }

     return        hExtReplace( CodeDbfId,  Code );

} // End of function hExtReplaceById()


/*================
*               char *hExtReplace()
*           Lower for hExtReplaceByName() && hExtReplaceById()
*==========================================================================*/
char *hExtReplace( unsigned short CodeDbfNo, char *SrcCode )
{
    int    cWordsPosition, cWordsNum, i=0, flag;
    unsigned short  FirstPageWordsNum, LeftWordsNum, cWordsLen, PageOffset;
    long   PageNo;
    char   CodeBuf[MAX_EEXPR_LEN];
    short  cExprLen;


    // Get index value from  chinese_begin_page
    switch( CodeDbfCtrlTab[CodeDbfNo].CompressMethod ) {
	case 'c':
	case 'i':
	case 'l':
		flag = 0;
		while( SrcCode[i] != '\0' ) {  // Check the valid of code
		   if( !isdigit( SrcCode[i++] ) ) {
		       flag = 1;
		       break;
		   } // End of if
		} // End of while

		if( flag == 1 ||  \
		    strlen( SrcCode ) != CodeDbfCtrlTab[CodeDbfNo].CodeLen ) {
		    hERROR = ERR_CODE;
		    return  NULL;
		}

		CompressWriteRead( SrcCode, \
				   CodeDbfCtrlTab[CodeDbfNo].CompressMethod, \
				   &CodeDbfCtrlTab[CodeDbfNo], CodeBuf );
		cWordsPosition = HZTHbsearch( CodeDbfNo, CodeBuf );
		break;
	case 's':
	default:
		cWordsPosition = HZTHbsearch(CodeDbfNo, SrcCode);
    } // End of check

    if( cWordsPosition  < 0 )     return     NULL;

    switch( CodeDbfCtrlTab[CodeDbfNo].CompressMethod  ) {
	case  'i':
	       cWordsPosition /= sizeof(short);
	       break;
	case  'l':
	       cWordsPosition /= sizeof(long);
	       break;
	case 's':
	       cWordsPosition /= CodeDbfCtrlTab[CodeDbfNo].CodeLen;
    }

    cWordsLen = CodeDbfCtrlTab[CodeDbfNo].StorageLen;
    cExprLen  = CodeDbfCtrlTab[CodeDbfNo].ExprLen;

    // How many this kind words it cab be in each page_number
    cWordsNum = CODE_LIB_PAGE_SIZE / cWordsLen;

    // How many chinese words, contented in the first page */
    FirstPageWordsNum = ( CODE_LIB_PAGE_SIZE - \
			 CodeDbfCtrlTab[CodeDbfNo].BeginPageOffset ) / cWordsLen;

    if( cWordsPosition < FirstPageWordsNum ) {  /*If the chinese_code in the
						 *first page_frame
						 */
	PageNo = CodeDbfCtrlTab[CodeDbfNo].BeginPageNum;
	PageOffset = CodeDbfCtrlTab[CodeDbfNo].BeginPageOffset + \
		     cWordsPosition * cWordsLen;
    } else {                                     /*Else borned new page_frame
						  *order_number & page_offset
						  */
	LeftWordsNum = cWordsPosition - FirstPageWordsNum;

	PageNo = CodeDbfCtrlTab[CodeDbfNo].BeginPageNum + 1 + \
		 LeftWordsNum / cWordsNum;

	PageOffset = ( LeftWordsNum % cWordsNum) * cWordsLen;
    }

    // Get Corresponding Chinese_string real & phycial  Page
    PageNo = LocateFetchPage( PageNo );

    if( PageNo > MAX_PAGE ) {
	hERROR = NO_CODE;
	return( NULL );
    }

    cExprLen = CodeDbfCtrlTab[CodeDbfNo].ExprLen;
    strncpy( cWords, &WordsPageTab[PageNo].chbuf[PageOffset + cExprLen], \
						      cWordsLen - cExprLen );
    cWords[cWordsLen - cExprLen] = '\0';

    return      cWords;

} // End of function hReplace()



/*****************
*              HZTH_METHOD_STRU *hMethodTranslate()
*            Translate a regular string to a REGULAR
*  Methods:
*      ec_m8(1,3)(1,3)
*      @9(1,3)
*      ('as')
*      ex_m8(*1,2)ec_m8(*3,2)ec_m8(*5,2)ec_m8(1,6)
*****************************************************************************/
_declspec(dllexport) HZTH_METHOD_STRU *hMethodTranslate( char *sHzthMethod )
{
    HZTH_METHOD_STRU *HzthMethod;
    short nWork, nSum, nStr, nstrWorkLen;
    short nFinish;
    char *strWork, CodeDbfName[MAX_FILE_NAME_LEN];
    short CodeDbfId, Len;
    char  chStart, chFldLen;
    short HzthResultLen;

    if( sHzthMethod == NULL )	return  NULL;
    shrink( sHzthMethod );
    if( *sHzthMethod == '\0' )	return  NULL;


    nSum = 0;
    nStr = 0;
    while( sHzthMethod[nStr] != '\0' )
	   if( sHzthMethod[nStr++] == '(' )     nSum++;
    if( nSum == 0 )     nSum = 1;

    HzthMethod = (HZTH_METHOD_STRU *)calloc( nSum +  1, \
					     sizeof(HZTH_METHOD_STRU) );
    if( HzthMethod == NULL )
    {
	hERROR = ERR_HZTH_METHOD;
	return  NULL;
    }

    // Initlize the control header information
    HzthMethod[0].CodeDbfId = nSum;
    HzthMethod[0].chStart   = 0xFF;
    HzthMethod[0].chFldLen  = 0xFF;

    nSum = 1;
    nStr = 0;
    HzthResultLen = 0;
    CodeDbfId = MAX_CODE_DBF_NUM + 1;
    // strWork = strchr( HzthMethod, '@' );
    while( sHzthMethod[nStr] != '\0' ) {
       switch( sHzthMethod[nStr] ) {
	  case '@':
	       if( !isdigit( sHzthMethod[++nStr] ) ) {
		   free( HzthMethod );
		   hERROR = ERR_HZTH_METHOD;
		   return NULL;
	       }
	       CodeDbfId = atoi( &sHzthMethod[nStr] ) - 1;	// start from 1
	       nFinish = 1;
	       strWork = strchr( &sHzthMethod[nStr], '(' );

	       if( strWork == NULL  )  Len = 0;
	       else Len = strlen( strWork );

	       nstrWorkLen = strlen( sHzthMethod ) - Len - nStr;

	       nStr += nstrWorkLen;
	       if( CodeDbfId >= MAX_CODE_DBF_NUM || (Len == 0 && \
					sHzthMethod[ nStr ] != '\0' ) ) {
		   free( HzthMethod );
		   hERROR = ERR_HZTH_METHOD;
		   return NULL;
	       }

	       if( sHzthMethod[nStr] != '(' ) {
		   chStart  = '\xFF';
		   chFldLen = '\xFF';
		   nFinish  = 3;
	       }
	       break;        // End of switch  '@'
	  case '(':
	      // skip the '(' and test
	      if( !isdigit( sHzthMethod[++nStr] ) ) {
		   switch( sHzthMethod[nStr] ) {
			case '*':
				if( CodeDbfId == MAX_CODE_DBF_NUM + 1 )	
				{
				    free( HzthMethod );
				    hERROR = ERR_HZTH_METHOD;
				    return  NULL;
				}

				CodeDbfId = -CodeDbfId;
				nStr++;
				break;

                        case '#':
                                if( CodeDbfId != MAX_CODE_DBF_NUM + 1 )
                                    CodeDbfId = -CodeDbfId-MAX_CODE_DBF_NUM;
                                else
                                    CodeDbfId = SHRT_MIN+2;
                                break;

			case '\'':
				CodeDbfId = SHRT_MIN+1;
				nFinish = 3;
				if( sHzthMethod[++nStr] != '\'' ) {
					chStart = sHzthMethod[nStr++];
				} else {
					chStart = '.';
				}
				if( sHzthMethod[nStr] != '\'' ) {
					chFldLen = sHzthMethod[nStr++];
				} else {
					chFldLen = '\0';
				}
				// skip to '
				while( sHzthMethod[nStr++] != '\'' );
				// skip to )
				while( sHzthMethod[nStr++] != ')' );
				//nStr++;
				break;

			default:
				free( HzthMethod );
				hERROR = ERR_HZTH_METHOD;
				return NULL;
		   }

		   if( nFinish == 3 )	break;

	      }

	      chStart = atoi( &sHzthMethod[nStr] ) - 1;		// start from 1
	      nFinish = 2;
	      strWork = strchr( &sHzthMethod[nStr], ',' );

	      if( strWork == NULL  )  Len = 0;
	      else Len = strlen( strWork );

	      if( chStart > MAX_EEXPR_LEN || Len == 0 ) {
		  free( HzthMethod );
		  hERROR = ERR_HZTH_METHOD;
		  return NULL;
	      }

	      nstrWorkLen = strlen( sHzthMethod ) -  Len  - nStr;

	      nStr += nstrWorkLen;
	      break;        // End of switch  '('
	 case ',':
	      // skip the ',' and test
	      if( !isdigit( sHzthMethod[++nStr] ) || nFinish != 2 ) {
		   free( HzthMethod );
		   hERROR = ERR_HZTH_METHOD;
		   return NULL;
	      }
	      chFldLen = atoi( &sHzthMethod[nStr] );
	      strWork = strchr( &sHzthMethod[nStr], ')' );

	      if( strWork == NULL  )  Len = 0;
	      else Len = strlen( strWork );

	      nstrWorkLen = strlen( sHzthMethod ) - Len - nStr;

	      if( chFldLen > MAX_EEXPR_LEN || Len == 0 ) {
		   free( HzthMethod );
		   hERROR = ERR_HZTH_METHOD;
		   return NULL;
	      }

	      nStr += nstrWorkLen;

	      // no codebase
	      if( CodeDbfId >= MAX_CODE_DBF_NUM ) {
		  // free( HzthMethod );
		  CodeDbfId = SHRT_MIN;
	      }
	      nFinish = 3;

	      if( sHzthMethod[nStr] != ')' ) {
		  free( HzthMethod );
		  hERROR = ERR_HZTH_METHOD;
		  return NULL;
	      }
	      nStr++;   // skip the ')'

	      break;  // End of switch  ','

	default:
	      nWork = nStr;
	      nstrWorkLen = 0;
	      while( sHzthMethod[nWork] != '\0' && \
		     sHzthMethod[nWork] != '@' && sHzthMethod[nWork] != '('&& \
		     sHzthMethod[nWork] != ',' && sHzthMethod[nWork] != ')' ) {
			nstrWorkLen++;
			nWork++;
	      }

	      if( nstrWorkLen > MAX_FILE_NAME_LEN ) {
		  free( HzthMethod );
		  hERROR = ERR_HZTH_METHOD;
		  return NULL;
	      }
	      strncpy( CodeDbfName, &sHzthMethod[nStr], nstrWorkLen );
	      CodeDbfName[nstrWorkLen] = '\0';

	      if( ( CodeDbfId = hGetCodeDbfId( CodeDbfName ) ) == -1 ) {
		   free( HzthMethod );
		   hERROR = ERR_HZTH_METHOD;
		   return  NULL;
	      }

	      nFinish = 1;

	      nStr += nstrWorkLen;
	      if( sHzthMethod[nStr] != '(' ) {
			chStart  = '\xFF';
			chFldLen = '\xFF';
			nFinish  = 3;
	      }

       } // End of switch( sHzthMethod[nStr] )

       if( nFinish == 3 ) {
	   HzthMethod[nSum].CodeDbfId = CodeDbfId;
	   HzthMethod[nSum].chStart   = chStart;
	   HzthMethod[nSum].chFldLen  = chFldLen;

	   if( CodeDbfId == SHRT_MIN ) {
		HzthResultLen  += chFldLen;
	   } else if( CodeDbfId >= 0 ) {
		HzthResultLen  += CodeDbfCtrlTab[CodeDbfId].ExprLen;
	   }

	   nFinish = 0;
	   CodeDbfId = MAX_CODE_DBF_NUM + 1;
	   nSum++;
//	   HzthResultLen = 0;

       }

   } // End of while

   if( HzthResultLen >= MAX_HZTH_RESULT_LEN ) {
	 free( HzthMethod );
	 hERROR = ERR_HZTH_METHOD;
	 return NULL;
   }
   HzthMethod[0].chFldLen = (unsigned char)HzthResultLen;

   return  HzthMethod;

} // end of function hMethodTranslate()



/*****************
*               char *hReplaceByMethod()
*           Single field replace with HzthMethod.
*****************************************************************************/
_declspec(dllexport) char *hReplaceByMethod( HZTH_METHOD_STRU *HzthMethod, char *Code )
{
     char *cDestWords;
     char DestCode[MAX_EEXPR_LEN];
     unsigned char chStart, chFldLen;
     short CodeDbfId;
     short i, rpCode;
     short CodeDbfIdReady;

     // szWords[0] = '\0';
     // by Jingyu Niu 2000.07.31
     memset( szWords, 0, MAX_HZTH_RESULT_LEN );

     rpCode = 0;
     CodeDbfIdReady = -1;
     for( i = 1; i <= HzthMethod[0].CodeDbfId; i++ ) {

	chStart   = HzthMethod[i].chStart;
	chFldLen  = HzthMethod[i].chFldLen;

	rpCode++;

	//*********************************************************************
        //2000.5.28
        CodeDbfId = HzthMethod[i].CodeDbfId;

        if( CodeDbfId <= -MAX_CODE_DBF_NUM )
	{
	    switch( CodeDbfId ) {
	    case SHRT_MIN:
	    { //
             //ec_m8(#1,2)(1,2)......
             //           ^---now

                if( CodeDbfIdReady < 0 ) {
                    //(#1,2)(...)
		    goto HZTH_replacedirectly;    
                }

                CodeDbfId = CodeDbfIdReady;
		CodeDbfIdReady = -1;

                goto HZTH_replacedirectly;

            }
	    break;

	    case SHRT_MIN+1:
		goto HZTH_replacedirectly;
	    break;

	    default:
             //
	     //ec_m8(#1,2)(#1,2)
	     //ec_m8(#1,2)(#1,2)(1,2)
             //ec_m8(#1,2)ec_m8(#1,2)
	     //           ^---this code define will be ommited

                if( CodeDbfIdReady <= 0 ) 
		{ //
		  //we will use this CodeDbfId, to reHZTH, but it doesn't exist
		    if( CodeDbfId != SHRT_MIN+2 || CodeDbfId >= 0 ) 
		    {
			CodeDbfId = -CodeDbfId-MAX_CODE_DBF_NUM;
		    } else {
			hERROR = ERR_HZTH_METHOD;
			return   NULL;                  
		    }
                } else {	    
                    CodeDbfId = CodeDbfIdReady;
                }
	    } //end of switch
	 	

	   if( chFldLen != 0xFF ) {
		strZcpy(DestCode, &Code[chStart], chFldLen+1);
		cDestWords = hReplaceById(CodeDbfId, DestCode);
	   } else { // End of if
		cDestWords = hExtReplaceById(CodeDbfId, Code);
	   }
	   if( cDestWords == NULL ) {
		//error, no way to replace
                hERROR = ERR_HZTH_METHOD;
                return   NULL;
	   } else {
                CodeDbfIdReady = hGetCodeDbfId(cDestWords);
		if( CodeDbfIdReady < 0 ) {
		    hERROR = ERR_HZTH_METHOD;
		    return   NULL;
		}
	   }
           
	   continue;               ////^^^^^^

	} //end of if( CodeDbfId <= -MAX_CODE_DBF_NUM )

HZTH_replacedirectly:

        if( CodeDbfId < 0 ) {
	   switch( CodeDbfId ) {
	       case SHRT_MIN+1:
		     DestCode[0] = chStart;
		     DestCode[1] = chFldLen;
		     DestCode[2] = '\0';
		     strcat(szWords, DestCode);
		     break;
	       case SHRT_MIN:
		     strZcpy(DestCode, &Code[chStart], chFldLen+1);
		     strcat(szWords, DestCode);
		     break;
	       default:
		     rpCode--;
	   }
	} else {
	   if( chFldLen != 0xFF ) {
		strZcpy(DestCode, &Code[chStart], chFldLen+1);
		cDestWords = hReplaceById(CodeDbfId, DestCode);
	   } else { // End of if
		cDestWords = hReplaceById(CodeDbfId, Code);
	   }
	   if( cDestWords == NULL ) {
		// in HZTH field "education(1,3)education(4,3)"
		// can fill with 3 chars
		if( rpCode <= 1 ) {
			hERROR = ERR_HZTH_METHOD;
			return   NULL;
		}
	   } else {
		strcat(szWords, cDestWords);
	   }
	} // end of else
     } // End of for

     if( rpCode <= 1 && szWords[0] == '\0' )	return  NULL;
     return  strcpy(cWords, szWords);

} // end of function hReplaceByMethod()


/*****************
*               char *hExtReplaceByMethod()
*           Single field replace with HzthMethod.
*****************************************************************************/
_declspec(dllexport) char *hExtReplaceByMethod(HZTH_METHOD_STRU *HzthMethod, char *Code)
{
     char *cDestWords;
     char DestCode[MAX_EEXPR_LEN];
     unsigned char chStart, chFldLen;
     short CodeDbfId;
     short i, rpCode;

     szWords[0] = '\0';

     rpCode = 0;
     for( i = 1; i <= HzthMethod[0].CodeDbfId; i++ ) {

	chStart   = HzthMethod[i].chStart;
	chFldLen  = HzthMethod[i].chFldLen;

	rpCode++;
	if( (CodeDbfId = HzthMethod[i].CodeDbfId) < 0 ) {
	   switch( CodeDbfId ) {
	       case SHRT_MIN+1:
		     DestCode[0] = chStart;
		     DestCode[1] = chFldLen;
		     DestCode[2] = '\0';
		     strcat(szWords, DestCode);
		     break;
	       case SHRT_MIN:
		     strZcpy(DestCode, &Code[chStart], chFldLen+1);
		     strcat( szWords, DestCode );
		     break;
	       default:
		     rpCode--;
	   }
	} else {
	   if( chFldLen != 0xFF ) {
		strZcpy(DestCode, &Code[chStart], chFldLen+1);
		cDestWords = hExtReplace(CodeDbfId, DestCode);
	   } else { // End of if
		cDestWords = hExtReplace(CodeDbfId, Code);
	   }
	   if( cDestWords == NULL ) {
		// in HZTH field "education(1,3)education(4,3)"
		// can fill with 3 chars
		if( rpCode <= 1 ) {
			hERROR = ERR_HZTH_METHOD;
			return   NULL;
		}
	   } else {
		strcat( szWords, cDestWords );
	   }
	} // end of else
     } // End of for

     if( rpCode <= 1 && szWords[0] == '\0' )	return  NULL;
     return  strcpy(cWords, szWords);

} // end of function hExtReplaceByMethod()


/*================
*               short FuzzyCmp()
*           Lower for hExtReplaceByName() && hExtReplaceById()
*==========================================================================*/
short FuzzyCmp( char *s1, char *s2, int len )
{
    int i;

    for( i = 0;  i < len;  i++ ) {
	if( toupper(s1[i]) != toupper(s2[i]) && s1[i] != '*' && s2[i] != '*' )
		return s1[i] - s2[i];
    }
    return  0;

} // end of function FuzzyCmp()



/************
*    Prototype:extern unsigned short hCheckCodeValid( char *CodeDbfName, char *Code );
*    Describe: When you want to check the validity of the Code in the CodeDbfName,
*    but you aren't sure to desided if you should really replace the code,
*    herence you might call it for the sure!
*    Date:     June 23, 1994
****************************************************************************/
unsigned short hCheckCodeValid( char *CodeDbfName, char *Code )
{
    short CodeDbfId;
    short flag, cWordsPosition, i=0;
    char   CodeBuf[MAX_EEXPR_LEN];

    // Check the validity of the parameters!
    if( CodeDbfName == NULL || Code == NULL ) {
	hERROR = NO_PARA;
	return   0;
    }

    // Only when the global varity of CodeDbfCtrlTab is non NULL
    // It can prove to that you have hOpen() the hzth moudle for
    // the success!
    if( CodeDbfCtrlTab == NULL ) {
	hERROR = NO_OPEN;
	return    0;
    }

    CodeDbfId = hGetCodeDbfId( CodeDbfName );
    if( CodeDbfId == -1 ) {
	hERROR = NO_LIB;
	return   0;
    }
    // Get index value from  chinese_begin_page
    switch( CodeDbfCtrlTab[CodeDbfId].CompressMethod ) {
	case 'c':
	case 'i':
	case 'l':
		flag = 0;
		while( Code[i] != '\0' ) {  // Check the valid of code
		   if( !isdigit( Code[i++] ) ) {
		       flag = 1;
		       break;
		   } // End of if
		} // End of while

		if( flag == 1 ||  \
		    strlen( Code ) != CodeDbfCtrlTab[CodeDbfId].CodeLen ) {
		    hERROR = ERR_CODE;
		    return   0;
		}

		CompressWriteRead( Code, \
				   CodeDbfCtrlTab[CodeDbfId].CompressMethod, \
				   &CodeDbfCtrlTab[CodeDbfId], CodeBuf );
		cWordsPosition = HZTHbsearch( CodeDbfId, CodeBuf );
		break;
	case 's':
	default:
		cWordsPosition = HZTHbsearch(CodeDbfId, Code);
    } // End of check

    if( cWordsPosition  < 0 )     return     0;

    return     1;


} // End of function hCheckCodeValid()


/************
*			hGetCodeDbfRecNum()
****************************************************************************/
_declspec(dllexport) long hGetCodeDbfRecNum( short CodeDbfId )
{

    if( CodeDbfId < 0 )		CodeDbfId = -CodeDbfId;

    if( CodeDbfId >= MAX_CODE_DBF_NUM ) {
	 hERROR = ERR_LIB;
	 return  0;
    }
    return  CodeDbfCtrlTab[CodeDbfId].CodeNum;

} // end of function GetCodeDbfRecNum()


/************
*			hGetSegCodeNum()
****************************************************************************/
long hGetSegCodeNum( short CodeDbfId, char *ModStr )
{

    long           count;
    unsigned short i;
    unsigned char  chBuf;
    unsigned short iBuf;
    unsigned long  lBuf;
    char           nModStr[MAX_EEXPR_LEN];
    long           CodeNum, CodeOffset;
    unsigned short ModStrLen;
    unsigned short CodeLen;

    if( CodeDbfId < 0 )		CodeDbfId = -CodeDbfId;

    if( CodeDbfId >= MAX_CODE_DBF_NUM ) {
	 hERROR = ERR_LIB;
	 return  0;
    }

    count = 0;
    CodeNum = CodeDbfCtrlTab[CodeDbfId].CodeNum;
    CodeOffset = CodeDbfCtrlTab[CodeDbfId].CodeOffset;
    ModStrLen = strlen( ModStr );
    CodeLen = CodeDbfCtrlTab[CodeDbfId].CodeLen;

    switch( CodeDbfCtrlTab[ CodeDbfId ].CompressMethod ) {
	case 'c':
	   for( i = 0;   i < CodeNum;   i++ ) {
		chBuf = *(char *)&CodeLibBuf[CodeOffset + i];
		CompressWriteRead(&chBuf, 'C', \
					&CodeDbfCtrlTab[CodeDbfId], nModStr );

		if( !FuzzyCmp( ModStr, nModStr, ModStrLen ) ) {
			count++;
		} // End of if
	   } // End of for
	   break; // End of switch 'c'
	case 'i':
	   for( i = 0;   i < CodeNum;   i++ ) {
		iBuf = *(unsigned short *)&CodeLibBuf[CodeOffset + i*sizeof(short)];
		CompressWriteRead(&iBuf, 'I', \
					&CodeDbfCtrlTab[CodeDbfId], nModStr );

		if( !FuzzyCmp( ModStr, nModStr, ModStrLen ) ) {
			count++;
		} // End of if
	   } // End of for
	   break; // End of switch 'i'
	case 'l':
	   for( i = 0;   i < CodeNum;   i++ ) {
		lBuf = *(unsigned long *)&CodeLibBuf[CodeOffset + i*sizeof(long)];
		CompressWriteRead(&lBuf, 'L', \
					&CodeDbfCtrlTab[CodeDbfId], nModStr );
		if( !FuzzyCmp( ModStr, nModStr, ModStrLen ) ) {
			count++;
		} // End of if
	   } // End of for
	   break; // End of switch 'l'
	default:
	   for( i = 0;   i < CodeNum;   i++ ) {
		strncpy(nModStr, &CodeLibBuf[CodeOffset + i*ModStrLen], CodeLen);
		nModStr[CodeLen] = '\0';

		if( !FuzzyCmp( ModStr, nModStr, ModStrLen ) ) {
			count++;
		} // End of if
	   } // End of for

       } // End of inner switch

    return  count;

} // end of function hGetSegCodeNum()


/************
*			hGetCexprLen()
****************************************************************************/
short hGetCexprLen( HZTH_METHOD_STRU *HzthMethod )
{
    return  HzthMethod[0].chFldLen;
} // end of function hGetCexprLen()


/************
*				hGetCodeLibInfo()
****************************************************************************/
_declspec(dllexport) CODE_LIB_MES_STRUCT *hGetCodeLibInfo( short CodeDbfid )
{
    if( CodeDbfid < 0 )		CodeDbfid = -CodeDbfid;
    if( CodeDbfid >= MAX_CODE_DBF_NUM ) {
	 hERROR = ERR_LIB;
	 return  NULL;
    }

    return  &CodeDbfCtrlTab[ CodeDbfid ];
}


/************
*			getHzthInitFlag()
****************************************************************************/
_declspec(dllexport) short getHzthInitFlag(void)
{
    return  InitFlag;
} //end of getHzthInitFlag

/************
*			getHzthInfoStr()
****************************************************************************/
_declspec(dllexport) char *getHzthInfoStr(void)
{
    return  szWords;
} //end of getHzthInfoStr()


/************
*			freeHzthMethodStru()
****************************************************************************/
_declspec(dllexport) void freeHzthMethodStru(HZTH_METHOD_STRU *p)
{
    if( p != NULL )
	free(p);
} //end of freeHzthMethodStru()






#ifdef _HzthRuningMessage_Assigned
void HzthRuningMessage( char *s )
{
#ifdef  WIN32
      strZcpy(szWords, s, MAX_HZTH_RESULT_LEN);
#else
      sendMessage( (View *)&bRuningText, evBroadcast, cmChangeText, s);
      ((View *)&bRuningText)->focusDraw( (View *)&bRuningText );
#endif
} // end of function

#ifdef  WIN32
void tgErrorWin(char *buf)
{
    MessageBox(NULL, buf, "错误", MB_OK|MB_ICONERROR);
}
#endif
#endif


/****************************************************************************\
 |--------END--------OF---------HZTH--------DEVELOPMENT--------SOFTWARE-----|
\****************************************************************************/
