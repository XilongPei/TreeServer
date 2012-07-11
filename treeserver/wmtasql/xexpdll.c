/*****************
 * xexpdll.c
 *
 *
 * copyright (c) Shanghai Tiedao University 1998
 *               CRSC 1998
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
#include <time.h>

#include "asqlxexp.h"

//public function protocol
char *substr(char *source_string, int start_pos, int length);
short  cdecl _TypeAlign( OpndType *lpOpnd, short ParaNum, short AlignType );
long  cdecl xGetOpndLong(OpndType *lpOpnd);
double cdecl xGetOpndFloat(OpndType *lpOpnd);
char * cdecl xGetOpndString(OpndType *lpOpnd);

short  cdecl _TypeAlign( OpndType *lpOpnd, short ParaNum, short AlignType )
{
    register short i;

    switch( AlignType ) {
	case LONG_TYPE:
		for( i = 0;  i < ParaNum;   i++ ) {
		    switch( lpOpnd[i].type ) {
			case CHR_TYPE:
			case CHR_IDEN:
				*(long *)lpOpnd[i].values = *(char *)lpOpnd[i].values;
				break;
			case INT_TYPE:
			case INT_IDEN:
				*(long *)lpOpnd[i].values = *(short *)lpOpnd[i].values;
				break;
			case NFIELD_IDEN:
				*(long *)lpOpnd[i].values = atol( substr( lpOpnd[i].oval, 0, \
								lpOpnd[i].length ) );
				break;
			case FFIELD_IDEN:
				*(long *)lpOpnd[i].values = (long)atof( substr(lpOpnd[i].oval, 0, \
								lpOpnd[i].length ) );
				break;
			case LONG_TYPE:
			case LONG_IDEN:
				break;
			default:
				//ErrorSet.xERROR = iTypeNoCompt;     /* type not comptible */
				return( 1 );
		    }
		    lpOpnd[i].type = LONG_TYPE;
		    lpOpnd[i].length = 4;
		}  /* end of for */
		break;
	case FLOAT_TYPE:
		for( i = 0;  i < ParaNum;   i++ ) {
		    switch( lpOpnd[i].type ) {
			case CHR_TYPE:
			case CHR_IDEN:
				*(double *)lpOpnd[i].values = *(char *)lpOpnd[i].values;
				break;
			case INT_TYPE:
			case INT_IDEN:
				*(double *)lpOpnd[i].values = *(short *)lpOpnd[i].values;
				break;
			case LONG_TYPE:
			case LONG_IDEN:
				*(double *)lpOpnd[i].values = *(long *)lpOpnd[i].values;
				break;
			case NFIELD_IDEN:
				*(double *)lpOpnd[i].values = atol( substr( lpOpnd[i].oval, 0, \
								lpOpnd[i].length ) );
				break;
			case FFIELD_IDEN:
				*(double *)lpOpnd[i].values = atof( substr(lpOpnd[i].oval, 0, \
								lpOpnd[i].length ) );
				break;
			case FLOAT_TYPE:
			case FLOAT_IDEN:
				break;
			default:
				//ErrorSet.xERROR = 1;
				return( 1 );
		    }
		    lpOpnd[i].type = FLOAT_TYPE;
		    lpOpnd[i].length = sizeof(double);
		}  /* end of for */
    }
    return( 0 );
} /* end of function TypeAlign */


long  cdecl xGetOpndLong(OpndType *lpOpnd)
{
    switch( lpOpnd->type ) {
	case CHR_TYPE:
	case CHR_IDEN:
	case LONG_TYPE:
	case LONG_IDEN:
		return  *(long *)lpOpnd->values;
		break;
	case INT_TYPE:
	case INT_IDEN:
		return  (long)*(short *)lpOpnd->values;
		break;
	case NFIELD_IDEN:
		return  atol( substr(lpOpnd->oval, 0, lpOpnd->length) );
		break;
	case FFIELD_IDEN:
		return  (long)atof( substr(lpOpnd->oval, 0, lpOpnd->length) );
		break;
    }

    //ErrorSet.xERROR = iTypeNoCompt;
    return LONG_MIN;

} //xGetOpndLong()


double cdecl xGetOpndFloat(OpndType *lpOpnd)
{
    switch( lpOpnd->type ) {
	case CHR_TYPE:
	case CHR_IDEN:
	case LONG_TYPE:
	case LONG_IDEN:
		return  (float)*(long *)lpOpnd->values;
		break;
	case INT_TYPE:
	case INT_IDEN:
		return  (float)*(short *)lpOpnd->values;
		break;
	case NFIELD_IDEN:
		return  (float)atol( substr(lpOpnd->oval, 0, lpOpnd->length) );
		break;
	case FFIELD_IDEN:
		return  (float)atof( substr(lpOpnd->oval, 0, lpOpnd->length) );
		break;
    }

    //ErrorSet.xERROR = iTypeNoCompt;
    return  (float)(long)LONG_MIN;

} //xGetOpndFloat()



char * cdecl xGetOpndString(OpndType *lpOpnd)
{
    char  *s;

    //use substr to hold the temp string
    if( lpOpnd->type >= FIELD_IDEN && lpOpnd[0].type <= SFIELD_IDEN ) {
	s = substr((char *)*(long *)lpOpnd->oval, 0, lpOpnd->length);
    } else {
	if( lpOpnd->length >= MAX_OPND_LENGTH ) {
		if( lpOpnd->oval == NULL ) {
			s = substr((char *)*(long *)lpOpnd->values,\
							0, lpOpnd->length );
			free((char *)*(long *)lpOpnd->values);
		} else {
			s = substr(lpOpnd->oval,0,lpOpnd->length);
		}
	} else {
		if( lpOpnd->oval == NULL ) {
			s = lpOpnd->values;
		} else {
			s = (char *)lpOpnd->oval;
		}
	}
    }

    return  s;


} //xGetOpndString()


static char *substring=NULL;
static char subStaticStr[4096];

/*==============
 *  function substr
 *==========================================================================*/
char *substr(char *source_string, int start_pos, int length)
{
   int i, source_len;
   char  *szResult;

   if( length < 4096 ) {
	szResult = subStaticStr;
   } else {
	if( length == SHRT_MAX ) {
		if( substring != NULL ) 	free( substring );
		return  substring = NULL;
	}
	if( (szResult = substring = (char *)realloc(substring, length+1)) == NULL )
		return NULL;
   }
   if( source_string != NULL )
	source_len = strlen( source_string );
   else
	source_len = 0;

   for(i=0; i<length; i++)
	 if( start_pos < source_len )
	   szResult[i] = source_string[start_pos++];
	 else
	   szResult[i] = ' ';

   szResult[i] = '\0';

   return  szResult;

} // end of substr()



/*------------
 !                      xexpdlltest()
 ! export function for asql call
 !-----------------------------------------------------------------------*/
//#pragma argsused
short cdecl _declspec(dllexport) xexpdlltest( OpndType *lpOpnd, short ParaNum, short *OpndTop, \
		short *CurState )
// OpndType *lpOpnd;               pointer of opnd stack
// short ParaNum;                  parameter number for this action
// short *OpndTop;                 system opnd top
// short *CurState;                the action sequence working state
{
    char  parameter[256];
    short sysFunNo;
    char  *sz;

    if( ParaNum < 2 )
	return  1;

    sz = xGetOpndString(&lpOpnd[0]);
    MessageBox(NULL, sz, "hello", MB_OK);

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

    if( lpOpnd[1].length >= MAX_OPND_LENGTH ) {
	    if( lpOpnd[1].oval == NULL ) {
                // temp string
                strncpy(parameter, (char *)*(long *)lpOpnd[1].values, 255);
		free( (char *)*(long *)lpOpnd[1].values );
	    } else {
		strncpy(parameter, (char *)*(long *)lpOpnd[1].oval, 255 );
	    }
    } else {
	    if( lpOpnd[1].oval == NULL ) {
		strncpy(parameter, (char *)lpOpnd[1].values, 255 );
	    } else {
		strncpy(parameter, lpOpnd[1].oval, 255 );
	    }
    }
    parameter[255] = '\0';

    switch( sysFunNo ) {
            case 1:
                break;
    }

    *OpndTop -= ParaNum;    /* maintain the opnd stack */

    return( 0 );

} /* end of function xexpdlltest() */


/*************************< end of xexpdll.c >*******************************/