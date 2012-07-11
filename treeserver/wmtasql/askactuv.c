/*********
 * askactuv.c
 * copyright (c) EastUnion Computer Service Ltd., Co. 1995
 * author:  Xilong Pei
 ****************************************************************************/

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

#include "dir.h"
#include "mistring.h"
#include "mistools.h"
#include "strutl.h"
#include "dio.h"
#include "xexp.h"
#include "btree.h"
#include "asqlana.h"

#ifdef __N_C_S
#include "msgbox.h"
#include "uvutl.h"
#endif

/*------------
 !                       _ASK_ErrorStamp
 ! protocol:  errStamp(char*)
 !-----------------------------------------------------------------------*/
//#pragma argsused
short PD_style _ASK_ErrorStamp( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState )
{
    if( *CurState == 0 || *CurState == LASTWORKTIMEOFACTION ) {
	*OpndTop -= ParaNum;
	return  0;
    }

#ifdef __N_C_S
    char  *p;
    char  buf[512];
    unsigned short i, len;

    if( *CurState == 0 || *CurState == LASTWORKTIMEOFACTION ) {
	*OpndTop -= ParaNum - 1;    
	return  0;
    }

    buf[0] = '\0';
    for(i = 0, len = 0;  i < ParaNum;  i++ ) {
	p = lpOpnd[i].oval;
	if( lpOpnd[i].type != STRING_TYPE ) {

	    dFIELDWHENACTION *dd;

	    dd = (dFIELDWHENACTION *)p;
	    strncat(buf,substr(dd->pSourceStart,0,lpOpnd[i].length),500-len);
	} else if( p != NULL ) {
	    if( lpOpnd[i].length >= MAX_OPND_LENGTH ) {
		p = (char *)*(long *)p;
		strncat(buf, p, 500-len);
	    } else {
		strncat(buf, p, 500-len);
	    }
	} else {
	    if( lpOpnd[i].length >= MAX_OPND_LENGTH ) {
		p = (char *)*(long *)lpOpnd[i].values;
		strncat(buf, p, 500-len);
		// free them, for str+str alloc memory
		free(p);
	    } else {
		strncat(buf,lpOpnd[i].values, 500-len);
	    }
	}
	len += lpOpnd[i].length;
	buf[len] = '\0';
    }

    setMBName("~Y~Í£Ö¹", cmOK);
    setMBName("~N~¼ÌÐø", cmCancel);
    i = mMessageBox(buf, mfInformation|mfOKCancel);
    setMBName(NULL, -1);
    if( i != cmCancel ) {
	errorStamp = 1;
    } else {
	*OpndTop -= ParaNum - 1;    /* maintain the opnd stack */
	return  0;
    }

#else
    errorStamp = (char)xGetOpndLong(&lpOpnd[0]);
#endif    
    lpOpnd[0].type = LONG_TYPE;
    *(long *)(lpOpnd[0].values) = LONG_MAX;

    *OpndTop -= ParaNum - 1;    /* maintain the opnd stack */

    return  0;

} /* end of function _ASK_ErrorStamp() */



/*------------
 !                       _ASK_Print
 ! protocol:  print(fmtFile, fun, hzth)
 !-----------------------------------------------------------------------*/
//#pragma argsused
short PD_style _ASK_Print( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState )
{
    char  *s;
    char  fmtFile[MAXPATH];
    char  funSel, hzthFlag;

    //use substr to hold the temp string
    if( lpOpnd[0].type >= FIELD_IDEN && lpOpnd[0].type <= SFIELD_IDEN ) {
	s = substr((char *)*(long *)lpOpnd[0].oval, 0, lpOpnd[0].length);
    } else {
	if( lpOpnd[0].length >= MAX_OPND_LENGTH ) {
		if( lpOpnd[0].oval == NULL ) {
			s = substr((char *)*(long *)lpOpnd[0].values,\
							0, lpOpnd[0].length );
			free((char *)*(long *)lpOpnd[0].values);
		} else {
			s = substr(lpOpnd[0].oval,0,lpOpnd[0].length);
		}
	} else {
		if( lpOpnd[0].oval == NULL ) {
			s = lpOpnd[0].values;
		} else {
			s = (char *)lpOpnd[0].oval;
		}
	}
    }
    strZcpy(fmtFile, s, MAXPATH);

    if( ParaNum < 2 ) {
	funSel = 'F';
	goto  ASKACT_PRINT_JMP;
    }

    if( lpOpnd[1].type >= FIELD_IDEN && lpOpnd[1].type <= SFIELD_IDEN ) {
	s = substr((char *)*(long *)lpOpnd[1].oval, 0, lpOpnd[1].length);
    } else {
	if( lpOpnd[1].length >= MAX_OPND_LENGTH ) {
		if( lpOpnd[1].oval == NULL ) {
			s = substr((char *)*(long *)lpOpnd[1].values,\
							0, lpOpnd[1].length );
			free((char *)*(long *)lpOpnd[1].values);
		} else {
			s = substr(lpOpnd[1].oval,0,lpOpnd[1].length);
		}
	} else {
		if( lpOpnd[1].oval == NULL ) {
			s = lpOpnd[1].values;
		} else {
			s = (char *)lpOpnd[1].oval;
		}
	}
    }
    funSel = toupper(s[0]);

    if( ParaNum < 3 ) {
	hzthFlag = 1;
	goto  ASKACT_PRINT_JMP;
    }

    if( lpOpnd[2].type >= FIELD_IDEN && lpOpnd[2].type <= SFIELD_IDEN ) {
	s = substr((char *)*(long *)lpOpnd[2].oval, 0, lpOpnd[2].length);
    } else {
	if( lpOpnd[2].length >= MAX_OPND_LENGTH ) {
		if( lpOpnd[2].oval == NULL ) {
			s = substr((char *)*(long *)lpOpnd[2].values,\
							0, lpOpnd[2].length );
			free((char *)*(long *)lpOpnd[2].values);
		} else {
			s = substr(lpOpnd[2].oval,0,lpOpnd[2].length);
		}
	} else {
		if( lpOpnd[2].oval == NULL ) {
			s = lpOpnd[2].values;
		} else {
			s = (char *)lpOpnd[2].oval;
		}
	}
    }

    hzthFlag = s[0];
    if( hzthFlag == 'Y' || hzthFlag == 'y' || hzthFlag == '1' )
	 hzthFlag = 1;
    else hzthFlag = 0;

ASKACT_PRINT_JMP:
    if( funSel == 'S' )
		s = "ASQLPRNT.TSK";
    else        s = NULL;

#ifdef __N_C_S
    L_Print(fmtFile, funSel, hzthFlag, s);
#else
    //do nothing now, we will develop print service
#endif

    *OpndTop -= ParaNum;                // maintain the opnd stack
    return  0;

} // end of function _ASK_Print()
