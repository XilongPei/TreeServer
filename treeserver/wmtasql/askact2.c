/*********
 * askact2.c
 * this is a part of askact.c, serperate them for overlay swap
 *
 * copyright (c) EastUnion Computer Service Ltd., Co. 1995
 *               CRSC 1997
 * author:  Xilong Pei
 ****************************************************************************/

#define FOR_TGMIS

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <errno.h>
#include <io.h>
#include <dos.h>
#include <time.h>

#include "dir.h"

#include "mistring.h"
#include "mistools.h"
#include "dio.h"
#include "dioext.h"
#include "xexp.h"
#include "btree.h"
#include "asqlana.h"
#include "wst2mt.h"
#include "diodbt.h"
#include "btreeext.h"
#include "strutl.h"
#include "ts_const.h"
#include "wmtasql.h"
#include "filetool.h"
#include "memutl.h"
#include "odbc_dio.h"
#include "t_lock_r.h"
#include "ndx_man.h"
#include "ts_dict.h"
#include "hzth.h"
#include "asqlutl.h"
#include "dbs_util.h"

#ifdef FOR_TGMIS
       #include "syscall.h"
#endif
/*------------
 !                      _Ask_AsqlSys()
 ! protocol: sys( sysFunNo, parameter )
 !           sysFunNo:    1  OS shell
 !                        2  mkdir(dirs)
 !			  3  delete(file)
 !			 10  createdatabase(databasename)
 !			 11  deletedatabase(databasename)
 !			100  uncatch one table, drop table
 !		       2001  build index and register them
 !		       2002  drop  index and register them
 !		       3001  build hzth library
 !		      32123  backup dictionary
 !		      32124  reopen dictionary
 !-----------------------------------------------------------------------*/
//#pragma argsused
short PD_style _Ask_AsqlSys( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState )
// OpndType *lpOpnd;               pointer of opnd stack
// short ParaNum;                  parameter number for this action
// short *OpndTop;                 system opnd top
// short *CurState;                the action sequence working state
{
    char  parameter[256];
    char  *sz;
    short sysFunNo;
    int   i;
extern WSToMT FromToStru fFrTo;

    switch( lpOpnd[0].type ) {
	    case LONG_TYPE:
		 sysFunNo = (short)*(long *)lpOpnd[0].values;
		 break;
	    case INT_TYPE:
		 sysFunNo = *(short *)lpOpnd[0].values;
		 break;
	    case CHR_TYPE:
		 sysFunNo = (short)*(char *)lpOpnd[0].values;
		 break;
	    default:
		 sysFunNo = 0;
    }

    switch( sysFunNo ) {
	    case 3001:
	    {
	    //
	    //build hzth
	    //
		char *s;
		char  buf[MAXPATH];

		s = GetCfgKey(csuDataDictionary, "SYSTEM", NULL, "CODE");
		if( s != NULL ) {
		    strZcpy(buf, s, MAXPATH);
		    if( hBuildCodeLib(buf) != 1 )
		    {
			strZcpy(ErrorSet.string, buf, XES_LENGTH);
			return  1;
		    }
		    if( hOpen(buf) != 1 )
		    {
			strZcpy(ErrorSet.string, buf, XES_LENGTH);
			return  1;
		    }
		}
		*OpndTop -= ParaNum;    /* maintain the opnd stack */
		return( 0 );
	    }

	    //
	    //secret
	    //
	    case 32123:
	    {
		//if( stricmp(asqlEnv.szUser, "ADMIN") == 0 )
		{
		   dictDupFile();
		}
		*OpndTop -= ParaNum;    /* maintain the opnd stack */
		return( 0 );
	    }

	    //
	    //secret
	    //
	    case 32124:
	    {
		//if( stricmp(asqlEnv.szUser, "ADMIN") == 0 )
		{
		    dictReopenCfgFile();
		}
		*OpndTop -= ParaNum;    /* maintain the opnd stack */
		return( 0 );
	    }
    };

    sz = xGetOpndString(&lpOpnd[1]);
    if( sz == NULL )
	parameter[0] = '\0';
    else
	strZcpy(parameter, sz, 255);

    switch( sysFunNo ) {
	    case 1:
#ifndef _WINDOWS_
#ifdef FOR_TGMIS
		systemCall(parameter);
#else
		system(parameter);
#endif
#else
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	HANDLE  hProcess;
	DWORD	dwExitCode;


	_try {
		ZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(si);
		si.wShowWindow = SW_HIDE;
		if( CreateProcess(NULL, parameter, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) )
		{
			hProcess = pi.hProcess;
			CloseHandle(pi.hThread);
			if( WaitForSingleObject(hProcess, INFINITE) != WAIT_FAILED ) {
				GetExitCodeProcess(hProcess, &dwExitCode);
				CloseHandle(hProcess);
			}
		} else {
			//ErrorSet.xERROR = GetLastError();
			dwExitCode = FormatMessage(
				    FORMAT_MESSAGE_IGNORE_INSERTS |FORMAT_MESSAGE_FROM_SYSTEM, \
				    NULL,
				    GetLastError(),
				    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
				    parameter,
				    256,
				    NULL );
			strZcpy(ErrorSet.string, parameter, XES_LENGTH);
			return  1;
		}
	}
	
	_except( EXCEPTION_EXECUTE_HANDLER ) {
	    return  1;
	}
}
#endif
		break;
	    case 2:
		mkdirs(parameter);
		break;
	    case 3:
		unlink(parameter);
		break;
	    case 10:
		buildDatabase(parameter);
		break;
	    case 11:
		dropDatabase(parameter);
		break;
	    case 100:
		uncatchTable(parameter);
		break;

	    case 2001:
	    {
	    //
	    //create index parameter on fFrTo.cSouDbfNum[i] (lpOpnd[3])
	    //
		if( ParaNum < 4 ) {
			ErrorSet.xERROR = iSysParaErr;
			return  1;
		}

		i = xGetOpndLong(&lpOpnd[2]) - 1;

		if( i < 0 || i >= fFrTo.cSouDbfNum ) {
			ErrorSet.xERROR = iSysParaErr;
			return  1;
		}

		sz = xGetOpndString(&lpOpnd[3]);

		if( fFrTo.AsqlDatabase[0] == '\0' ) {
		    buildIndexAndRegThem("DBROOT", fFrTo.cSouFName[i], parameter, sz);
		} else {
		    buildIndexAndRegThem(fFrTo.AsqlDatabase, fFrTo.cSouFName[i], parameter, sz);
		}

		break;
	    }

	    case 2002:
	    {
	    //
	    //drop index parameter on fFrTo.cSouDbfNum[i]
	    //
		if( ParaNum < 3 ) {
			ErrorSet.xERROR = iSysParaErr;
			return  1;
		}

		i = xGetOpndLong(&lpOpnd[2]) - 1;

		if( i < 0 || i >= fFrTo.cSouDbfNum ) {
			ErrorSet.xERROR = iSysParaErr;
			return  1;
		}

		if( fFrTo.AsqlDatabase[0] == '\0' ) {
		    dropIndexAndRegThem("DBROOT", fFrTo.cSouFName[i], parameter);
		} else {
		    dropIndexAndRegThem(fFrTo.AsqlDatabase, fFrTo.cSouFName[i], parameter);
		}
		break;
	    }
    }

    *OpndTop -= ParaNum;    /* maintain the opnd stack */

    return( 0 );

} /* end of function _Ask_AsqlSys() */




/*------------
 !                      _ASK_CalRec()
 ! protocol: calrec
 !-----------------------------------------------------------------------*/
//#pragma argsused
short PD_style _ASK_CalRec( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState )
{
    dFIELDWHENACTION *p;
    dFILE 	     *tdf;
    int               i;
extern WSToMT FromToStru fFrTo;

    if( *CurState == 0 ) {
	int freeMark = 0;

	for(i = 0;  i < ParaNum;  i++ ) {
	    if( lpOpnd[i].type == STRING_TYPE && \
					lpOpnd[i].length >= MAX_OPND_LENGTH ) {
		free((char *)*(long *)lpOpnd[i].values);
		freeMark = 1;
	    }
	}

	*OpndTop -= ParaNum;
	if( freeMark ) 	return  1;

	return  0;
    }

    if( *CurState > 0 ) {

	if( xIsOpndField(&lpOpnd[0]) == 0 )
	{
	    i = xGetOpndLong(&lpOpnd[0]) - 1;
	    if( i < 0 || i >= fFrTo.cSouDbfNum ) {
		ErrorSet.xERROR = iRefTabelErr;
		sprintf(ErrorSet.string, "calrec(%d)", i+1);
		return  1;
	    }

	    tdf = fFrTo.cSouFName[i];

	    if( tdf->rec_p <= 0 )
	    {
		*OpndTop -= ParaNum;
		return  0;
	    }

	    if( fFrTo.phuf != NULL )
	    {
		fseek(fFrTo.phuf, -4, SEEK_CUR);

		if( ( i = checkRecValid(tdf) ) != 0 ) {
		   ErrorSet.xERROR = iFailFunCall;
		   sprintf(ErrorSet.string, "calrec(%d),Field:%s", tdf->error,
			   tdf->field[i-1].field);
		   return  1;
		}

		if( toLockRec(tdf, tdf->rec_p) != 0 ) {
		   ErrorSet.xERROR = iFailToLock;
		   sprintf(ErrorSet.string, "calrec(%d)", -1);
		   return  1;
		}

		fwrite(&tdf, sizeof(dFILE *), 1, fFrTo.phuf);
		fwrite(&(tdf->rec_p), sizeof(long), 1, fFrTo.phuf);
		fwrite(tdf->rec_buf, tdf->rec_len, 1, fFrTo.phuf);

		fwrite("\0\0\0\0", sizeof(dFILE *), 1, fFrTo.phuf);
	    } else {
		if( put1rec( tdf ) == NULL ) {
			ErrorSet.xERROR = iFailFunCall;
			sprintf(ErrorSet.string, "calrec(err_fldid:%d)", \
						 fFrTo.cSouFName[i]->error);
			return  1;
		}
	    }
	    *OpndTop -= ParaNum;

	    return  0;
	} //////////////////////////////////////////////////////////////////

	if( fFrTo.phuf != NULL )
	{
	    tdf = ((dFIELDWHENACTION *)lpOpnd[0].oval)->pSourceDfile;

	    if( tdf->rec_p <= 0 )
	    {
		*OpndTop -= ParaNum;
		return  0;
	    }

	    if( ( i = checkRecValid(tdf) ) != 0 ) {
		   ErrorSet.xERROR = iFailFunCall;
		   sprintf(ErrorSet.string, "calrec(%d),Field:%s", tdf->error,
			   tdf->field[i-1].field);
		   return  1;
	    }

	    if( toLockRec(tdf, tdf->rec_p) != 0 ) {
		   ErrorSet.xERROR = iFailFunCall;
		   sprintf(ErrorSet.string, "calrec(%d)", -1);
		   return  1;
	    }


	    fseek(fFrTo.phuf, -4, SEEK_CUR);

	    fwrite(&tdf, sizeof(dFILE *), 1, fFrTo.phuf);
	    fwrite(&(tdf->rec_p), sizeof(long), 1, fFrTo.phuf);
	    fwrite(tdf->rec_buf, tdf->rec_len, 1, fFrTo.phuf);

	    fwrite("\0\0\0\0", sizeof(dFILE *), 1, fFrTo.phuf);
	} else {

	    p = (dFIELDWHENACTION *)lpOpnd[0].oval;
	    if( (tdf = p->pTargetDfile) == NULL ) 	return( 1 );

	    if( tdf->rec_p <= 0 )
	    {
		*OpndTop -= ParaNum;
		return  0;
	    }

	    if( tdf != p->pSourceDfile ) {
		/*1998.5.28
		short  i;
		unsigned short j;
		dFILE  *sdf = p->pSourceDfile;
		char   buf[512];

		for( i = tdf->field_num-1;   i >= 0;   i-- ) {
		     get_fld(tdf, i, buf);
		     if( ltrim(buf)[0] != '\0' )	continue;
		     if( (j = GetFldid(sdf, tdf->field[i].field)) != 0xFFFF ) {
			if( tdf->field[i].fieldtype == 'M' ) {
			    sprintf(buf, "ILDIODBT.%03X", intOfThread);
			    //read the dbt into file
			    dbtToFile(sdf, j, buf);

			    //put_fld from file
			    dbtFromFile(tdf, i, buf);
			} else {
			    GetField(sdf, j, buf);
			    PutField(tdf, i, buf);
			}
		     }
		}*/
		tdf->rec_buf[0] = ' ';
		putrec( tdf );
	    } else {
	       if( put1rec( tdf ) == NULL ) {
		   ErrorSet.xERROR = iFailFunCall;
		   sprintf(ErrorSet.string, "calrec(%d)", tdf->error);
		   return  1;
		}
	    }
	}

    } /* end of if */

    *OpndTop -= ParaNum;    /* maintain the opnd stack */

    return( 0 );

} /* end of function _ASK_CalRec() */


/*------------
 !                      _ASK_AppRec()
 ! protocol: apprec(field)
 ! apprec() use for source
 !-----------------------------------------------------------------------*/
//#pragma argsused
short PD_style _ASK_AppRec( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState )
{
    dFILE *tdf;
    int    i;
extern WSToMT FromToStru fFrTo;

    if( *CurState == 0 || *CurState == LASTWORKTIMEOFACTION )
    {
	*OpndTop -= ParaNum;
	return  0;
    }

    if( xIsOpndField(&lpOpnd[0]) == 0 )
    {
	i = xGetOpndLong(&lpOpnd[0]) - 1;
	if( i < 0 || i >= fFrTo.cSouDbfNum ) {
		ErrorSet.xERROR = iRefTabelErr;
		sprintf(ErrorSet.string, "apprec(%d)", i);
		return  1;
	}

	tdf = fFrTo.cSouFName[i];
    } else {

	tdf = ((dFIELDWHENACTION *)lpOpnd[0].oval)->pTargetDfile;
	if( tdf == NULL )       return  1;
    }

#ifdef WSToMT
    //lock it to avoid other thread move to the end for appending
    wmtDbfLock(tdf);
#endif
    dseek(tdf, 0, dSEEK_END);

    if( fFrTo.phuf != NULL )
    {
	tdf->rec_buf[0] = '*';
	if( put1rec( tdf ) == NULL ) {

	    wmtDbfUnLock(tdf);

	    ErrorSet.xERROR = iFailFunCall;
	    sprintf(ErrorSet.string, "calrec(%d)", tdf->error);
	    return  1;
	}

	wmtDbfUnLock(tdf);

	tdf->rec_buf[0] = ' ';

	fseek(fFrTo.phuf, -4, SEEK_CUR);

	fwrite(&tdf, sizeof(dFILE *), 1, fFrTo.phuf);
	fwrite(&(tdf->rec_p), sizeof(long), 1, fFrTo.phuf);
	fwrite(tdf->rec_buf, tdf->rec_len, 1, fFrTo.phuf);

	fwrite("\0\0\0\0", sizeof(dFILE *), 1, fFrTo.phuf);
    } else {
	if( put1rec( tdf ) == NULL ) {

	    wmtDbfUnLock(tdf);

	    ErrorSet.xERROR = iFailWriteRec;
	    sprintf(ErrorSet.string, "calrec(%d)", tdf->error);
	    return  1;
	}
    }

#ifdef WSToMT
    wmtDbfUnLock(tdf);
#endif

    *OpndTop -= ParaNum;    /* maintain the opnd stack */

    return( 0 );

} /* end of function _ASK_AppRec() */


/*------------
 !                      _ASK_InitRec()
 ! protocol: initrec
 ! initrec(field)
 !-----------------------------------------------------------------------*/
//#pragma argsused
short PD_style _ASK_InitRec( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState )
{
    dFILE *tdf;
extern WSToMT FromToStru fFrTo;

    //if( ParaNum <= 0 )       return  1;
    /*if( xIsOpndField(&lpOpnd[0]) == 0 )
	return  1;

    tdf = ((dFIELDWHENACTION *)lpOpnd[0].oval)->pSourceDfile;
    */
    if( *CurState == 0 || *CurState == LASTWORKTIMEOFACTION )
    {
	*OpndTop -= ParaNum;
	return  0;
    }

    if( xIsOpndField(&lpOpnd[0]) == 0 )
    {
	int i;
	i = xGetOpndLong(&lpOpnd[0]) - 1;
	if( i < 0 || i >= fFrTo.cSouDbfNum ) {
		ErrorSet.xERROR = iRefTabelErr;
		strZcpy(ErrorSet.string,
			"reference one of source tables, but it doesn't exist", XES_LENGTH);
		return  1;
	}

	tdf = fFrTo.cSouFName[i];
    } else {

	tdf = fFrTo.targefile;
	if( tdf == NULL ) {
		ErrorSet.xERROR = iRefTabelErr;
		strZcpy(ErrorSet.string,
			"reference target table, but it doesn't exist", XES_LENGTH);
		return  1;
	}
    }

    NewRec( tdf );

    *OpndTop -= ParaNum;    /* maintain the opnd stack */

    return  0;

} /* end of function _ASK_InitRec() */



/*------------
 !                      _ASK_RecPack()
 ! protocol: recpack
 ! the operating target is the FROM
 !-----------------------------------------------------------------------*/
//#pragma argsused
short PD_style _ASK_RecPack( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState )
{
    int    i;
extern WSToMT FromToStru fFrTo;

    if( fFrTo.nGroupby == Asql_GROUPYES ) {
	ErrorSet.xERROR = iNoAction;          	/* no this action */
       *OpndTop -= ParaNum;    			/* maintain the opnd stack */
	return( 1 );
    }


    if( *CurState == 0 ) {
#ifdef WSToMT
      int    tryCount = 10;

asqlRecPackRetry:
      srand( (unsigned)time( NULL ) );
      for(i = 0;   i < fFrTo.cSouDbfNum;   i++) {
	if( wmtDbfTryLock(fFrTo.cSouFName[i]) )
	    break;
      }
      if( i < fFrTo.cSouDbfNum )
      { //leave the critical section to avoid dead lock
        if( --tryCount < 0 )
            return  -1;

        for(i--;  i >= 0;  i--) {
	    wmtDbfUnLock(fFrTo.cSouFName[i]);
        }

        Sleep(rand()%1000);

        goto asqlRecPackRetry;
      }
#endif
    } else if( *CurState > 0 ) {
       for(i = 0; i < fFrTo.cSouDbfNum;  i++)
	     recDelPack(fFrTo.cSouFName[i], NULL);
    } else {
#ifdef WSToMT
       for(i = 0; i < fFrTo.cSouDbfNum;  i++)
             wmtDbfUnLock(fFrTo.cSouFName[i]);
#endif
    }

    *OpndTop -= ParaNum;    /* maintain the opnd stack */

    return  0;

} //end of _ASK_RecPack()


/*------------
 !                      _ASK_Distinct()
 ! protocol: _ASK_Distinct
 ! the operating target is the FROM
 !-----------------------------------------------------------------------*/
//#pragma argsused
short PD_style _ASK_Distinct( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState )
{
    dFIELDWHENACTION *p;
    char   	      buf[256];
    dFIELD 	     *field;
extern WSToMT FromToStru fFrTo;
extern char  tmpPath[MAXPATH];		//asqlana.c

    if( *CurState == 0 ) {

	short	i;

	if( fFrTo.distinctBh != NULL ) {
	    *OpndTop -= ParaNum;    /* maintain the opnd stack */
	    return  0;
	}

	if( xIsOpndField( &lpOpnd[0] ) == 0 )
	{
	    i = (short)xGetOpndLong( &lpOpnd[0] );
	    if( i < 1 ) {
		return  1;
	    }
	} else {
	    i = -1;
	}

	sprintf(buf, "DT%03X%03X.NDX", (fFrTo.insideInt)++, intOfThread&0xFFF);
	makefilename(buf, tmpPath, buf);
	if( i < 1 ) {
	    p = (dFIELDWHENACTION *)lpOpnd[0].oval;
	    field=getFieldInfo(p->pSourceDfile, p->wSourceid);
	    fFrTo.distinctBh=BppTreeBuild(buf, (short)(field->fieldlen));
	} else {
	    fFrTo.distinctBh=BppTreeBuild(buf, i);
	}
    } else if( *CurState > 0 ) {
	if( xIsOpndField( &lpOpnd[0] ) == 0 )
	{
	    char *sz = xGetOpndString( &lpOpnd[1] );
	    strZcpy(buf, sz, 256);
	} else {
	    p = (dFIELDWHENACTION *)lpOpnd[0].oval;
	    get_fld(p->pSourceDfile, p->wSourceid, buf);
	}

	if( IndexSeek(fFrTo.distinctBh, buf) == LONG_MIN ) {
	    IndexRecIns(fFrTo.distinctBh, buf, LONG_MAX);
	} else {
	    return  2;
	}
    } else {
	/*sprintf(buf, "DIST_%03X.NDX", intOfThread&0xFFF);
	makefilename(buf, tmpPath, buf);
	*/
	strcpy(buf, fFrTo.distinctBh->ndxName);
	IndexDispose(fFrTo.distinctBh);
	fFrTo.distinctBh = NULL;
	unlink(buf);
    }

    return  0;

} //end of _ASK_Distinct()



/*------------
 !                      _ASK_InsRec()
 ! protocol: insrec
 ! use for source
 !-----------------------------------------------------------------------*/
//#pragma argsused
short PD_style _ASK_InsRec( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState )
{
    dFILE *tdf;
extern WSToMT FromToStru fFrTo;

    if( *CurState == 0 || *CurState == LASTWORKTIMEOFACTION )
    {
	*OpndTop -= ParaNum;
	return  0;
    }

    if( xIsOpndField(&lpOpnd[0]) == 0 )
    {
	int i;
	i = xGetOpndLong(&lpOpnd[0]) - 1;
	if( i < 0 || i >= fFrTo.cSouDbfNum )
		return  1;

	tdf = fFrTo.cSouFName[i];
    } else {
	tdf = ((dFIELDWHENACTION *)lpOpnd[0].oval)->pTargetDfile;
	if( tdf == NULL )       return  1;
    }

    if( fFrTo.phuf != NULL )
    {
	tdf->rec_buf[0] = '*';
	daddrec(tdf);
	tdf->rec_buf[0] = ' ';

	fseek(fFrTo.phuf, -4, SEEK_CUR);

	fwrite(&tdf, sizeof(dFILE *), 1, fFrTo.phuf);
	fwrite(&(tdf->rec_p), sizeof(long), 1, fFrTo.phuf);
	fwrite(tdf->rec_buf, tdf->rec_len, 1, fFrTo.phuf);

	fwrite("\0\0\0\0", sizeof(dFILE *), 1, fFrTo.phuf);
    } else {
	daddrec(tdf);
    }

    *OpndTop -= ParaNum;    /* maintain the opnd stack */

    return( 0 );

} /* end of function _ASK_InsRec() */


/*------------
 !                      _ASK_Tarray()
 ! protocol: tarray(array1, array2, calint, start1x, start1y, start2x, start2y, w, h)
 ! calint:
 ! 1	+
 ! 2	-
 ! 3	*
 ! 4	/
 ! 5	:=
 !-----------------------------------------------------------------------*/
// #pragma argsused /* Remed by NiuJingyu */
short PD_style _ASK_Tarray( OpndType *lpOpnd, short ParaNum, \
					     short *OpndTop, short *CurState )
{

    ArrayType *array1, *array2;
    int        calint, start1x, start1y, start2x, start2y, w, h;
    long       l;
    double     f;
    int        i, j;

    if( *CurState == 0 )
	return  0;

    if( lpOpnd[0].type == ARRAY_TYPE ) {
	array1 = (ArrayType *)*(long *)lpOpnd[0].oval;
    } else {
	return  1;
    }

    if( lpOpnd[1].type == ARRAY_TYPE ) {
	array2 = (ArrayType *)*(long *)lpOpnd[1].oval;
    } else {
	return  1;
    }

    calint = xGetOpndLong(&lpOpnd[2]);
    if( calint == LONG_MIN )
	return  1;

    start1x = xGetOpndLong(&lpOpnd[4]);
    if( start1x-- == LONG_MIN )
	return  1;

    start1y = xGetOpndLong(&lpOpnd[3]);
    if( start1y-- == LONG_MIN )
	return  1;

    start2x = xGetOpndLong(&lpOpnd[6]);
    if( start2x-- == LONG_MIN )
	return  1;

    start2y = xGetOpndLong(&lpOpnd[5]);
    if( start2y-- == LONG_MIN )
	return  1;

    w = xGetOpndLong(&lpOpnd[7]);
    if( w < 1 || start1x+w > array1->ArrayDim[1] || \
				start2x+w > array2->ArrayDim[1] ) {
	ErrorSet.xERROR = iDimError;       /* error dim type */
	return  1;
    }

    h = xGetOpndLong(&lpOpnd[8]);
    if( h < 1 || start1y+h > array1->ArrayDim[0] || \
				start2y+h > array2->ArrayDim[0] ) {
	ErrorSet.xERROR = iDimError;       /* error dim type */
	return  1;
    }

    switch( calint ) {
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	case 1:		//+
	    for(i = 0; i < h;  i++) {
		for(j = 0;  j < w;  j++ ) {
		   switch( array2->ElementType ) {
			case CHR_TYPE:
				l = *((char *)array2->ArrayMem + (start2x+j) + (start2y+i) * array2->ArrayDim[1]);
				goto noBreak1;
			case INT_TYPE:
				l = *(short *)((char *)array2->ArrayMem + 2 * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
				goto noBreak1;
			case LONG_TYPE:
				l = *(long *)((char *)array2->ArrayMem + 4 * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
noBreak1:
				switch( array1->ElementType ) {
				    case CHR_TYPE:
					*((char *)array1->ArrayMem + (start1x+j) + (start1y+i) * array1->ArrayDim[1])
									+= (char)l;
					break;
				    case INT_TYPE:
					*(short *)((char *)array1->ArrayMem + 2*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									+= (short)l;
					break;
				    case LONG_TYPE:
					*(long *)((char *)array1->ArrayMem + 4*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									+= (long)l;
					break;
				    case FLOAT_TYPE:
					*(double *)((char *)array1->ArrayMem + sizeof(double)*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									+= (double)l;
					break;
				    default:
					ErrorSet.xERROR = iDimError;       /* error dim type */
					return 1;
				} /* end of switch */
				break;
			case FLOAT_TYPE:
				f = *(double *)((char *)array2->ArrayMem + sizeof(double) * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
				switch( array1->ElementType ) {
				    case CHR_TYPE:
					*((char *)array1->ArrayMem + (start1x+j) + (start1y+i) * array1->ArrayDim[1])
									+= (char)f;
					break;
				    case INT_TYPE:
					*(short *)((char *)array1->ArrayMem + 2*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									+= (short)f;
					break;
				    case LONG_TYPE:
					*(long *)((char *)array1->ArrayMem + 4*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									+= (long)f;
					break;
				    case FLOAT_TYPE:
					*(double *)((char *)array1->ArrayMem + sizeof(double)*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									+= (double)f;
					break;
				    default:
					ErrorSet.xERROR = iDimError;       /* error dim type */
					return 1;
				} /* end of switch */
				break;
			default:
				ErrorSet.xERROR = iDimError;       /* error dim type */
				return 1;
		   } /* end of switch */
		} /* end of for(j) */
	    } // end of for(i)
	break;
	//------------------------------------------------------------------
	case 2:		//+
	    for(i = 0; i < h;  i++) {
		for(j = 0;  j < w;  j++ ) {
		   switch( array2->ElementType ) {
			case CHR_TYPE:
				l = *((char *)array2->ArrayMem + (start2x+j) + (start2y+i) * array2->ArrayDim[1]);
				goto noBreak2;
			case INT_TYPE:
				l = *(short *)((char *)array2->ArrayMem + 2 * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
				goto noBreak2;
			case LONG_TYPE:
				l = *(long *)((char *)array2->ArrayMem + 4 * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
noBreak2:
				switch( array1->ElementType ) {
				    case CHR_TYPE:
					*((char *)array1->ArrayMem + (start1x+j) + (start1y+i) * array1->ArrayDim[1])
									-= (char)l;
					break;
				    case INT_TYPE:
					*(short *)((char *)array1->ArrayMem + 2*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									-= (short)l;
					break;
				    case LONG_TYPE:
					*(long *)((char *)array1->ArrayMem + 4*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									-= l;
					break;
				    case FLOAT_TYPE:
					*(double *)((char *)array1->ArrayMem + sizeof(double)*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									-= (double)l;
					break;
				    default:
					ErrorSet.xERROR = iDimError;       /* error dim type */
					return 1;
				} /* end of switch */
				break;
			case FLOAT_TYPE:
				f = *(double *)((char *)array2->ArrayMem + sizeof(double) * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
				switch( array1->ElementType ) {
				    case CHR_TYPE:
					*((char *)array1->ArrayMem + (start1x+j) + (start1y+i) * array1->ArrayDim[1])
									-= (char)f;
					break;
				    case INT_TYPE:
					*(short *)((char *)array1->ArrayMem + 2*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									-= (short)f;
					break;
				    case LONG_TYPE:
					*(long *)((char *)array1->ArrayMem + 4*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									-= (long)f;
					break;
				    case FLOAT_TYPE:
					*(double *)((char *)array1->ArrayMem + sizeof(double)*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									-= f;
					break;
				    default:
					ErrorSet.xERROR = iDimError;       /* error dim type */
					return 1;
				} /* end of switch */
				break;
			default:
				ErrorSet.xERROR = iDimError;       /* error dim type */
				return 1;
		   } /* end of switch */
		} /* end of for(j) */
	    } // end of for(i)
	break;
	// ******************************************************************
	case 3:		//+
	    for(i = 0; i < h;  i++) {
		for(j = 0;  j < w;  j++ ) {
		   switch( array2->ElementType ) {
			case CHR_TYPE:
				l = *((char *)array2->ArrayMem + (start2x+j) + (start2y+i) * array2->ArrayDim[1]);
				goto noBreak3;
			case INT_TYPE:
				l = *(short *)((char *)array2->ArrayMem + 2 * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
				goto noBreak3;
			case LONG_TYPE:
				l = *(long *)((char *)array2->ArrayMem + 4 * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
noBreak3:
				switch( array1->ElementType ) {
				    case CHR_TYPE:
					*((char *)array1->ArrayMem + (start1x+j) + (start1y+i) * array1->ArrayDim[1])
									*= (char)l;
					break;
				    case INT_TYPE:
					*(short *)((char *)array1->ArrayMem + 2*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									*= (short)l;
					break;
				    case LONG_TYPE:
					*(long *)((char *)array1->ArrayMem + 4*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									*= l;
					break;
				    case FLOAT_TYPE:
					*(double *)((char *)array1->ArrayMem + sizeof(double)*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									*= (double)l;
					break;
				    default:
					ErrorSet.xERROR = iDimError;       /* error dim type */
					return 1;
				} /* end of switch */
				break;
			case FLOAT_TYPE:
				f = *(double *)((char *)array2->ArrayMem + sizeof(double) * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
				switch( array1->ElementType ) {
				    case CHR_TYPE:
					*((char *)array1->ArrayMem + (start1x+j) + (start1y+i) * array1->ArrayDim[1])
									*= (char)f;
					break;
				    case INT_TYPE:
					*(short *)((char *)array1->ArrayMem + 2*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									*= (short)f;
					break;
				    case LONG_TYPE:
					*(long *)((char *)array1->ArrayMem + 4*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									*= (long)f;
					break;
				    case FLOAT_TYPE:
					*(double *)((char *)array1->ArrayMem + sizeof(double)*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									*= f;
					break;
				    default:
					ErrorSet.xERROR = iDimError;       /* error dim type */
					return 1;
				} /* end of switch */
				break;
			default:
				ErrorSet.xERROR = iDimError;       /* error dim type */
				return 1;
		   } /* end of switch */
		} /* end of for(j) */
	    } // end of for(i)
	break;
	// //////////////////////////////////////////////////////////////////
	case 4:		//+
	    for(i = 0; i < h;  i++) {
		for(j = 0;  j < w;  j++ ) {
		   switch( array2->ElementType ) {
			case CHR_TYPE:
				l = *((char *)array2->ArrayMem + (start2x+j) + (start2y+i) * array2->ArrayDim[1]);
				goto noBreak4;
			case INT_TYPE:
				l = *(short *)((char *)array2->ArrayMem + 2 * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
				goto noBreak4;
			case LONG_TYPE:
				l = *(long *)((char *)array2->ArrayMem + 4 * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
noBreak4:
				switch( array1->ElementType ) {
				    case CHR_TYPE:
					*((char *)array1->ArrayMem + (start1x+j) + (start1y+i) * array1->ArrayDim[1])
									/= (char)l;
					break;
				    case INT_TYPE:
					*(short *)((char *)array1->ArrayMem + 2*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									/= (short)l;
					break;
				    case LONG_TYPE:
					*(long *)((char *)array1->ArrayMem + 4*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									/= l;
					break;
				    case FLOAT_TYPE:
					*(double *)((char *)array1->ArrayMem + sizeof(double)*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									/= (double)l;
					break;
				    default:
					ErrorSet.xERROR = iDimError;       /* error dim type */
					return 1;
				} /* end of switch */
				break;
			case FLOAT_TYPE:
				f = *(double *)((char *)array2->ArrayMem + sizeof(double) * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
				switch( array1->ElementType ) {
				    case CHR_TYPE:
					*((char *)array1->ArrayMem + (start1x+j) + (start1y+i) * array1->ArrayDim[1])
									/= (char)f;
					break;
				    case INT_TYPE:
					*(short *)((char *)array1->ArrayMem + 2*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									/= (short)f;
					break;
				    case LONG_TYPE:
					*(long *)((char *)array1->ArrayMem + 4*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									/= (long)f;
					break;
				    case FLOAT_TYPE:
					*(double *)((char *)array1->ArrayMem + sizeof(double)*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									/= f;
					break;
				    default:
					ErrorSet.xERROR = iDimError;       /* error dim type */
					return 1;
				} /* end of switch */
				break;
			default:
				ErrorSet.xERROR = iDimError;       /* error dim type */
				return 1;
		   } /* end of switch */
		} /* end of for(j) */
	    } // end of for(i)
	break;
	// := := := := := := := := := := := := := := := := := := := := := := := :=
	case 5:		//+
	    for(i = 0; i < h;  i++) {
		for(j = 0;  j < w;  j++ ) {
		   switch( array2->ElementType ) {
			case CHR_TYPE:
				l = *((char *)array2->ArrayMem + (start2x+j) + (start2y+i) * array2->ArrayDim[1]);
				goto noBreak5;
			case INT_TYPE:
				l = *(short *)((char *)array2->ArrayMem + 2 * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
				goto noBreak5;
			case LONG_TYPE:
				l = *(long *)((char *)array2->ArrayMem + 4 * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
noBreak5:
				switch( array1->ElementType ) {
				    case CHR_TYPE:
					*((char *)array1->ArrayMem + (start1x+j) + (start1y+i) * array1->ArrayDim[1])
									= (char)l;
					break;
				    case INT_TYPE:
					*(short *)((char *)array1->ArrayMem + 2*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									= (short)l;
					break;
				    case LONG_TYPE:
					*(long *)((char *)array1->ArrayMem + 4*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									= l;
					break;
				    case FLOAT_TYPE:
					*(double *)((char *)array1->ArrayMem + sizeof(double)*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									= (double)l;
					break;
				    default:
					ErrorSet.xERROR = iDimError;       /* error dim type */
					return 1;
				} /* end of switch */
				break;
			case FLOAT_TYPE:
				f = *(double *)((char *)array2->ArrayMem + sizeof(double) * ( (start2x+j) + (start2y+i) * array2->ArrayDim[1] ) );
				switch( array1->ElementType ) {
				    case CHR_TYPE:
					*((char *)array1->ArrayMem + (start1x+j) + (start1y+i) * array1->ArrayDim[1])
									= (char)f;
					break;
				    case INT_TYPE:
					*(short *)((char *)array1->ArrayMem + 2*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									= (short)f;
					break;
				    case LONG_TYPE:
					*(long *)((char *)array1->ArrayMem + 4*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									= (long)f;
					break;
				    case FLOAT_TYPE:
					*(double *)((char *)array1->ArrayMem + sizeof(double)*((start1x+j) + (start1y+i) * array1->ArrayDim[1]) )
									= f;
					break;
				    default:
					ErrorSet.xERROR = iDimError;       /* error dim type */
					return 1;
				} /* end of switch */
				break;
			default:
				ErrorSet.xERROR = iDimError;       /* error dim type */
				return 1;
		   } /* end of switch */
		} /* end of for(j) */
	    } // end of for(i)
	break;

    }

    *OpndTop -= ParaNum;                /* maintain the opnd stack */

    return 0;

} /* end of function _ASK_Tarray() */


/*------------
 !                      _ASK_DbtToFile()
 ! protocol: btofile(field, filename)
 !-----------------------------------------------------------------------*/
//#pragma argsused
short PD_style _ASK_DbtToFile( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState )
{
    dFILE *df;
    char   fileName[256];
    char  *sz;
    short  newid;
extern WSToMT FromToStru fFrTo;

    if( *CurState == 0 || *CurState == LASTWORKTIMEOFACTION )
    {
	*OpndTop -= ParaNum;
	return  0;
    }

    df = getRealDbfId(((dFIELDWHENACTION *)lpOpnd[0].oval)->wSourceid, &newid);
    //df = ((dFIELDWHENACTION *)lpOpnd[0].oval)->pSourceDfile;
    if( df == NULL )
	return  1;

    sz = xGetOpndString(&lpOpnd[1]);
    if( sz == NULL )
	return  1;

    if( sz[1] == ':' || sz[0] == '\\' ) {
	strZcpy(fileName, sz, 256);
    } else {
	if( *sz != '^' ) {
		strcpy(fileName, asqlEnv.szAsqlResultPath);
		strncat(fileName, sz, 256-strlen(fileName));
	} else {
		strcpy(fileName, asqlEnv.szAsqlFromPath);
		strncat(fileName, sz+1, 256-strlen(fileName));
	}
    }

    //if( dbtToFile(df, ((dFIELDWHENACTION *)lpOpnd[0].oval)->wSourceid, fileName) < 0 )
    if( dbtToFile(df, newid, fileName) < 0 )
    {
	strZcpy(ErrorSet.string, "btofile()", XES_LENGTH);
	return  1;
    }

    *OpndTop -= ParaNum;    /* maintain the opnd stack */

    return  0;

} /* end of function _ASK_DbtToFile() */


/*------------
 !                      _ASK_DbtFromFile()
 ! protocol: bfromfile(field, filename)
 !-----------------------------------------------------------------------*/
//#pragma argsused
short PD_style _ASK_DbtFromFile( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState )
{
    dFILE *df;
    char   fileName[256];
    char  *sz;
extern WSToMT FromToStru fFrTo;

    if( *CurState == 0 || *CurState == LASTWORKTIMEOFACTION )
    {
	*OpndTop -= ParaNum;
	return  0;
    }

    df = ((dFIELDWHENACTION *)lpOpnd[0].oval)->pSourceDfile;
    if( df == NULL )
	return  1;

    sz = xGetOpndString(&lpOpnd[1]);
    if( sz == NULL )
	return  1;

    if( sz[1] == ':' || sz[0] == '\\' ) {
	strZcpy(fileName, sz, 256);
    } else {
	if( *sz != '^' ) {
		strcpy(fileName, asqlEnv.szAsqlResultPath);
		strncat(fileName, sz, 256-strlen(fileName));
	} else {
		strcpy(fileName, asqlEnv.szAsqlFromPath);
		strncat(fileName, sz+1, 256-strlen(fileName));
	}
    }

    if( dbtFromFile(df, ((dFIELDWHENACTION *)lpOpnd[0].oval)->wSourceid, fileName) < 0 )
    {
	strZcpy(ErrorSet.string, "bfromfile()", XES_LENGTH);
	return  1;
    }

    *OpndTop -= ParaNum;    /* maintain the opnd stack */

    return  0;

} /* end of function _ASK_DbtFromFile() */


/*------------
 !                      _ASK_CallASQL()
 !-----------------------------------------------------------------------*/
short PD_style _ASK_CallASQL( OpndType *lpOpnd, short ParaNum, \
					short *OpndTop, short *CurState )
{
    char  *s, *asqlAsk;
    int   j;
    MSG   msg1;
extern WSToMT FromToStru fFrTo;
extern int asqlTaskThreadNum;	//in wmtasql.c

    //1999/7/20 Xilong Pei
    if( *CurState == 0 || *CurState == LASTWORKTIMEOFACTION )
    {
	*OpndTop -= ParaNum;
	return  0;
    }

    s = xGetOpndString(&lpOpnd[0]);
    if( s == NULL )
	return  1;

    asqlAsk = strdup(s);

    //callasql, inside task
    j = wmtAskQS(-1, asqlAsk, AsqlExprInMemory|Asql_USEENV, \
			      &asqlEnv, \
			      &(fFrTo.sHead),
			      &(fFrTo.nSybNum),
			      (EXCHNG_BUF_INFO *)wmtExbuf);
    if( j >= asqlTaskThreadNum ) {
        free(asqlAsk);
	return  1;
    }

    { //wait the end of this thread
	GetMessage(&msg1, NULL, ASQL_ENDTASK_MSG, ASQL_ENDTASK_MSG);
    }

    free(asqlAsk);
    lpOpnd[0].type = LONG_TYPE;
    *(long *)(lpOpnd[0].values) = msg1.wParam;

    *OpndTop -= ParaNum - 1;    /* maintain the opnd stack */

    return  0;

} /* end of function _ASK_CallASQL() */



/*------------
 !                      _ASK_SQL()
 !-----------------------------------------------------------------------*/
short PD_style _ASK_SQL( OpndType *lpOpnd, short ParaNum, \
					short *OpndTop, short *CurState )
{
    char  *szSQL = NULL, *sz;
    int   i, j;

    if( *CurState == 0 || *CurState == LASTWORKTIMEOFACTION )
    {
	*OpndTop -= ParaNum;
	return  0;
    }

    j = 0;
    for(i = 0;  i < ParaNum;  i++) {
	sz = xGetOpndString(&lpOpnd[i]);
	j += strlen(sz) + 2;

	if( szSQL == NULL )
	    szSQL = zeroMalloc(j);
	else
	    szSQL = realloc(szSQL, j);

	strcat(szSQL, sz);
    }

    if( szSQL != NULL ) {
	sz = odbcExecute(szSQL);
	free( szSQL );
    } else {
	sz = "szSQL";
    }

    if( sz != NULL ) {
	ErrorSet.xERROR = iOPrError;
	strZcpy(ErrorSet.string, sz, XES_LENGTH);
	*OpndTop -= ParaNum;
	return  1;

    }

    *OpndTop -= ParaNum;    /* maintain the opnd stack */

    return  0;

} /* end of function _ASK_SQL() */



/*------------
 !                      _ASK_RelSeek()
 !-----------------------------------------------------------------------*/
short PD_style _ASK_RelSeek( OpndType *lpOpnd, short ParaNum, \
					short *OpndTop, short *CurState )
{
extern WSToMT FromToStru fFrTo;
    long  l;
    bHEAD *bh;
    dFILE **sdf;

    if( *CurState <= 0 ) {
	*OpndTop -= ParaNum;
	return  0;
    }

    if( (l = xGetOpndLong(&lpOpnd[0]) ) == LONG_MIN ) {
	return ErrorSet.xERROR = iRelCntParaErr;
    }

    l--;

    if( l <= 0 || l >= fFrTo.cSouDbfNum ) {
	return ErrorSet.xERROR = iRelCntParaErr;
    }

    if( fFrTo.lRecNo[l] > 0 ) {
    	*OpndTop -= ParaNum;
	return  0;
    }

    bh = fFrTo.syncBh[l];
    if( bh == NULL ) {
	return ErrorSet.xERROR = iRelCntParaErr;
    }

    sdf = fFrTo.cSouFName;
    fFrTo.lRecNo[l] = IndexSeek(bh, fFrTo.szLocateStr[l]);
    if( fFrTo.lRecNo[l] > 0 ) {
	if( get1rec( sdf[l] ) == NULL ) {
		return  1;
	}

	if( sdf[l]->rec_buf[0] == '*' ) {
	    while( 1 ) {

	    //take the next record
	    if( fFrTo.szLocateStr[l][0] != '\xFF' )
		fFrTo.lRecNo[l] = IndexStrEqSkip(bh, fFrTo.szLocateStr[l], 1);
	    else
		fFrTo.lRecNo[l] = IndexSkip(bh, 1);

		if( fFrTo.lRecNo[l] > 0 ) {
		    if( get1rec( sdf[l] ) == NULL ) {
			return  1;
		    }

		    if( sdf[l]->rec_buf[0] != '*' )
			break;
		} else {
		    NewRec( sdf[l] );
		    break;
		}
	    } //end while(1)
	}
    } else {
	NewRec( sdf[l] );
    }

    *OpndTop -= ParaNum;
    return  0;

} /* end of function _ASK_RelSeek() */



/*------------
 !                      _ASK_AddVarArray()
 ! protocol: addvar(ARRAY_TYPE, 10.0)
 !-----------------------------------------------------------------------*/
// #pragma argsused /* Remed by NiuJingyu */
short PD_style _ASK_AddVarArray( OpndType *lpOpnd, short ParaNum, \
			    short *OpndTop, short *CurState )
{

    ArrayType *p;
    int	       itemSize;
    char       c;
    short      w;
    long       l;
    double     f;

    if( lpOpnd[0].type != ARRAY_TYPE ) {
	*OpndTop -= ParaNum;
	return  1;
    }

    p = (ArrayType *)*(long *)lpOpnd[0].oval;
    if( p->DimNum != 0 ) {
	*OpndTop -= ParaNum;
	return  1;
    }

    //first action and last action return directly
    if( *CurState <= 0 ) {
	*OpndTop -= ParaNum;
	return  0;
    }

    switch( p->ElementType ) {
	   case LONG_TYPE:      itemSize = sizeof(long);           break;
	   case FLOAT_TYPE:     itemSize = sizeof(double);         break;
	   case CHR_TYPE:       itemSize = sizeof(char);           break;
	   case INT_TYPE:       itemSize = sizeof(short);          break;
	   default:
				*OpndTop -= ParaNum;
				return  1;
    }

    if( p->MemSize <= itemSize * (p->ElementNum+1) ) {
	p->MemSize += 1024;
	p->ArrayMem = realloc(p->ArrayMem, p->MemSize);
	if( p->ArrayMem == NULL ) {
		*OpndTop -= ParaNum;
		return  1;
	}
    } //end of if

    switch( p->ElementType ) {
	   case LONG_TYPE:
		l = xGetOpndLong( &(lpOpnd[1]) );
		*((long *)(p->ArrayMem)+p->ElementNum) = l;
		break;

	   case FLOAT_TYPE:
		f = xGetOpndFloat( &(lpOpnd[1]) );
		*((double *)(p->ArrayMem)+p->ElementNum) = f;
		break;

	   case CHR_TYPE:
		c = xGetOpndChr( &(lpOpnd[1]) );
		*((char *)(p->ArrayMem)+p->ElementNum) = c;
		break;

	   case INT_TYPE:
		w = xGetOpndShort( &(lpOpnd[1]) );
		*((short *)(p->ArrayMem)+p->ElementNum) = w;
		break;
    }

    (p->ElementNum)++;

    *OpndTop -= ParaNum;
    return  0;

} /* end of function _ASK_AddVarArray() */



/*------------
 !                      _ASK_ClearVarArray()
 ! protocol: clearvar(ARRAY_TYPE)
 !-----------------------------------------------------------------------*/
// #pragma argsused /* Remed by NiuJingyu */
short PD_style _ASK_ClearVarArray( OpndType *lpOpnd, short ParaNum, \
			    short *OpndTop, short *CurState )
{

    ArrayType *p;

    if( lpOpnd[0].type != ARRAY_TYPE ) {
	*OpndTop -= ParaNum;
	return  1;
    }

    p = (ArrayType *)*(long *)lpOpnd[0].oval;
    if( p->DimNum != 0 ) {
	*OpndTop -= ParaNum;
	return  1;
    }

    //first time of action and last time of action
    //do nothing
    if( *CurState == 0 || *CurState != LASTWORKTIMEOFACTION ) {
	*OpndTop -= ParaNum;
	return  1;
    }

    p->ElementNum = 0;
    p->MemSize = 1024;
    p->ArrayMem = realloc(p->ArrayMem, p->MemSize);
    if( p->ArrayMem == NULL ) {
	*OpndTop -= ParaNum;
	return  1;
    }

    *OpndTop -= ParaNum;
    return  0;

} /* end of function _ASK_ClearVarArray() */





/****************************** askact2.c ***********************************/
