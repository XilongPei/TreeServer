/***************
 * ^-_-^
 * xexp.c module for MIS expression deal.
 *
 * Writen by Tong Wang 1990.5-10
 *      +Rewriten by Xilong Pei 1991.12-1993
 *      +change MidCodeType size of little memory require 1995.11.24
 *      +add ? : process
 *      +support 4096B MFIELD_IDEN for xcomp(), store_g(), xGetOpndString()
 *		 1998.2.23
 *      +xSelfBaseVar for SysVar, xSelfBaseVar, UserDefVar. 1998.5.11
 *	+indirect store variable support: the length in variable table is
 *	 less than 0.	1998.7.23
 *      +change BITL(<<) into MOD(%) 1998.11.23
 *
 * For dBASE-Version
 *
 * Learn:
 *    1.in C we need not pay attention to the change of a memory in realloc
 *    for it will copy them all the time.
 *    2.in C, switch-case statements will be translate into SKIP-TABLE
 *
 * Rem:
 *    1.MidCodeType->oval point to item which won't vary size,
 *      oval=>STRING_TYPE: CONST STRING
 *      values=>STRING_TYPE: VARIABLE STRING
 *      add_g, _xSubstr() will make their result in values
 *    2.type whose values[] will be MAX_OPND_LENGTH:
 *      IDEN_TYPE
 *    3.if a STRING is point by oval, it is CONST STRING, IN OPND
 *      oval JUST point to MidCodeType pointer's values
 *    4.correct an error: "string'string" 1999.3.25
 *    5.support _int64 2000.3.3
 *    6 add value check in and_g(), or_g(), not_g()  2000.5.6
 *
 * Copyright (c) Shanghai Tiedao University 1990-1993,96
 *               CRSC 1998
 *		 Shanghai Withub Vision Software Co., Ltd 1999-2000
 ****************************************************************************/

#define INrEXPRANALYSIS         "XEXP 2.10"

#define __FOR_WINDOWS__

/*#ifndef __LARGE__
    #error Please compile the xexp.c module in large module
#endif
*/
#ifdef _MSC_VER
#include <windows.h>
#endif

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
#ifdef __FOR_WINDOWS__
#include <windows.h>
#endif
#ifdef __BORLANDC__
#include <alloc.h>
#endif

#include "mistring.h"
#include "mistools.h"
#include "dio.h"
#include "xexp.h"
#include "btree.h"
#include "strutl.h"
#include "wst2mt.h"
#include "ascii_s.h"
#include "charutl.h"
#include "dir.h"
#include "diodbt.h"
#include "asqlutl.h"

/*****
#include "bugstat.h"
*****/
#define USE_NEW_WRITECODE
#define XEXP_ACTION_SERVICE


WSToMT ErrorType ErrorSet = {0, ""};
static char *szXexpOptrStr = "|| && !  =  == >  <  >= <= != +  -  *  /  << >> &  |"
		       "  ~  ^  (  )  f( f) -D := ,  ?  L: A[ A] ED ";

#ifdef __FOR_WINDOWS__
    HINSTANCE 	hLibXexp = 0;
#endif

WSToMT static OpndType near opnd[ MAX_OPND_NUMBER ];                 /* ������ => �ṹ */
WSToMT static OptrType near optr[ MAX_OPND_NUMBER ] = { END_TYPE };  /* ������ => ���� */

WSToMT short opnd_top = 0;	//point to current opnd
WSToMT short optr_top = 0;

//set the following 2 variable for CalExpr call CalExpr itself
WSToMT short optrStackFund = 0;
WSToMT short opndStackFund = 0;

WSToMT static unsigned char *ps;	  /* point to the opnd[opnd_top].values */
WSToMT static OpndType *dp;              /* point to the opnd[opnd] 		*/

WSToMT static unsigned char *_xEXP_result_mem = NULL;


//2000.8.10
WSToMT char cEscapeXexp = '\\';


static MidCodeType myEndType_MidCodeType = {END_TYPE, 0, NULL, ""};

static char * near PD_style iden_recognize(char *buffer_pointer, MidCodeType **mpp);
static char * near PD_style SpecialFieldRecognize(char *buffer_pointer, MidCodeType **mpp);
static char * near PD_style string_recognize(char *buffer_pointer, MidCodeType **mpp);
static char * near PD_style digit_recognize(char *buffer_pointer, MidCodeType *m_c_pointer);
static char * near PD_style fun_recognize(char *buffer_pointer, MidCodeType *m_c_pointer);
static char * near PD_style array_recognize(char *buffer_pointer, MidCodeType *m_c_pointer);
static short near PD_style not_g( void );
static short near PD_style and_g( void );
static short near PD_style or_g( void );
static short near PD_style xcomp( void );
static short near PD_style absxcomp( void );
static short near PD_style neg_g( MidCodeType *m_c_pointer );
static short near PD_style qustn_g( MidCodeType **m_c_pointer );
static short near PD_style add_g( void );
static short near PD_style minus_g( void );
static short near PD_style multiple_g( void );
static short near PD_style divide_g( void );
static short near PD_style mod_g( void );
static short near PD_style store_g( void );
static short near PD_style array_g( MidCodeType * );

/************** define for simple remember *********************************/

#define   __SetOpndFloatType       opnd[opnd_top].type = FLOAT_TYPE; \
				   opnd[opnd_top].length = sizeof(double)
#define   __SetOpnd0FloatType      opnd0->type = FLOAT_TYPE; \
				   opnd0->length = sizeof(double)
#define   __SetOpndChrType         opnd[opnd_top].type = CHR_TYPE; \
				   opnd[opnd_top].length = 1
#define   __SetOpnd0ChrType        opnd0->type = CHR_TYPE; \
				   opnd0->length = 1
#define   __SetOpndIntType         opnd[opnd_top].type = INT_TYPE; \
				   opnd[opnd_top].length = 2
#define   __SetOpnd0IntType        opnd0->type = INT_TYPE; \
				   opnd0->length = 2
#define   __SetOpndLongType        opnd[opnd_top].type = LONG_TYPE; \
				   opnd[opnd_top].length = 4
#define   __SetOpnd0LongType       opnd0->type = LONG_TYPE; \
				   opnd0->length = 4
#define   __SetOpndInt64Type       opnd[opnd_top].type = INT64_TYPE; \
				   opnd[opnd_top].length = 8
#define   __SetOpnd0Int64Type      opnd0->type = INT64_TYPE; \
				   opnd0->length = 8
#define   __SetOpndDateType        opnd[opnd_top].type = DATE_TYPE; \
				   opnd->length = 8
#define   __SetOpnd0DateType       opnd0->type = DATE_TYPE

#ifdef __cplusplus

  short max (short value1, short value2);

  short max(short value1, short value2)
  {
     return ( (value1 > value2) ? value1 : value2);
  }

#endif

/*********
 *              WordAnalyse()
 ****************************************************************************/
_declspec(dllexport) MidCodeType * PD_style WordAnalyse( unsigned char *buffer )
{
     MidCodeType   *m_c_head, *m_c_pointer, *p, *mpLast = NULL;
     unsigned char *buffer_pointer;
     char           success = 1;
     char           neg_flag = 1;
     unsigned char  c;
     signed short   quotoCnt;

     if( buffer == NULL )       return( NULL );

     ErrorSet.xERROR = XexpOK;

     buffer_pointer = buffer;
     neg_flag = 1;
     success = 1;
     quotoCnt = 0;

     //first time, the last node is itself
     if( (m_c_pointer = \
		    (MidCodeType *)malloc( sizeof(MidCodeType) )) == NULL ) {
	ErrorSet.xERROR = iNoMem;    // not enough memory
	return  NULL;
     }
     m_c_pointer->next = NULL;
     m_c_head = NULL;

     while( *buffer_pointer != '\0' ) {
	c = *buffer_pointer;
	if( asc_isalpha[c] )  c = 'A';
	else if( isdigit( c ) )        c = '0';
	/*else if( !isspace(c) ) {
		//2001.9.7
		//error process, not support iden
		ErrorSet.xERROR = iBadExp;
		strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
		success = 0;
		break;
	}*/

	switch( c ) {
	    case 'A':
		buffer_pointer = iden_recognize(buffer_pointer, &m_c_pointer);
		if ( buffer_pointer == NULL ) {
			
			//this needn't, for iden_recognize() return NULL only when 
			//m_c_pointer hasn't realloced:

			//if( mpLast != NULL )
			//  mpLast->next = m_c_pointer;

			success = 0;
			break;
		}
		while( isspace(*buffer_pointer) )
			buffer_pointer++;
		if( *buffer_pointer == '(' ) {
		    
		    if( mpLast != NULL && mpLast->type > INSIDE_TYPE ) {
			
			//2000.5.21
			mpLast->next = m_c_pointer;
			
			ErrorSet.xERROR = iToLittOPr;
			strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
			success = 0;
			break;
		    }

		    buffer_pointer = fun_recognize(buffer_pointer, m_c_pointer);
		    if( buffer_pointer == NULL ) {
			
			//set the link
			if( mpLast != NULL )
			    mpLast->next = m_c_pointer;
			else
			    m_c_head = m_c_pointer;
			
			success = 0;
		    }
		    neg_flag = 1;
		} else if ( *buffer_pointer == '[' ) {
		    if( mpLast != NULL && mpLast->type > INSIDE_TYPE ) {
			
			mpLast->next = m_c_pointer;
			
			ErrorSet.xERROR = iToLittOPr;
			strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
			success = 0;
			break;
		    }

		    buffer_pointer = array_recognize(buffer_pointer, m_c_pointer);
		    if( buffer_pointer == NULL ) {
			
			//set the link
			if( mpLast != NULL )
			    mpLast->next = m_c_pointer;
			else
			    m_c_head = m_c_pointer;
			
			success = 0;
		    }
		    neg_flag = 1;
		} else {
		    neg_flag = 0;

		    if( stricmp(m_c_pointer->values, "AND") == 0 )
		    {
			/*this check will do when calculate the expression
			if( mpLast != NULL && mpLast->type > INSIDE_TYPE && mpLast->type != NEG_TYPE ) {
				ErrorSet.xERROR = iToLittOPr;
				strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
				success = 0;
				break;
			}*/
			m_c_pointer->type = AND_TYPE;
			break;
		    } else if( stricmp(m_c_pointer->values, "OR") == 0 )
		    {
			m_c_pointer->type = OR_TYPE;
			break;
		    } else if( stricmp(m_c_pointer->values, "NOT") == 0 )
			m_c_pointer->type = NOT_TYPE;
			break;
		    }

		    if( mpLast != NULL && mpLast->type > INSIDE_TYPE && mpLast->type < ONE_PARA_OPTR_TYPE)
		    {	//data type
			//
			if( mpLast->type == VARADDR_TYPE ) {
			    m_c_pointer->type = VIDEN_TYPE;
			} else {
			
			    mpLast->next = m_c_pointer;

			    ErrorSet.xERROR = iToLittOPr;
			    strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
			    success = 0;
			}
			break;
		    }

		break;

	    case '0':
	    case '.':
		if( mpLast != NULL && mpLast->type > INSIDE_TYPE && mpLast->type != NEG_TYPE ) {
		    ErrorSet.xERROR = iToLittOPr;
		    strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
		    success = 0;
		    break;
		}

		if( ( buffer_pointer = digit_recognize(buffer_pointer, \
						    m_c_pointer) ) == NULL ) {
			success = 0;
			break;
		}
		neg_flag = 0;
		break;
	    case '@':
		if( mpLast != NULL && mpLast->type > INSIDE_TYPE ) {
		    ErrorSet.xERROR = iToLittOPr;
		    strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
		    success = 0;
		    break;
		}

		if( ( buffer_pointer = SpecialFieldRecognize(buffer_pointer, \
						  &m_c_pointer) ) == NULL ) {
			success = 0;
		} else {
			neg_flag = 0;
		}
		break;
	    case '"' :
		if( mpLast != NULL && mpLast->type > INSIDE_TYPE ) {
		    ErrorSet.xERROR = iToLittOPr;
		    strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
		    success = 0;
		    break;
		}

		buffer_pointer = string_recognize(buffer_pointer+1,&m_c_pointer);
		if ( buffer_pointer == NULL )  success = 0;
		else {
			neg_flag = 0;
		}
		break;

	    case '\'':
		if( mpLast != NULL && mpLast->type > INSIDE_TYPE ) {
		    ErrorSet.xERROR = iToLittOPr;
		    strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
		    success = 0;
		    break;
		}

		buffer_pointer = strGetChar( ++buffer_pointer, &c );
		if( *buffer_pointer++ != '\'' ) {
		      ErrorSet.xERROR = iSymbolUsedError;
		      strcpy(ErrorSet.string, "\'");
		      success = 0;
		} else {
		      m_c_pointer->values[0] = c; 	//get the char

		      m_c_pointer->type = CHR_TYPE;
		      m_c_pointer->length = 1;
		      neg_flag = 0;
		}

		/*old method
		if( *(buffer_pointer+2) == '\'' ) {

		      buffer_pointer++;   			//skip to char
		      m_c_pointer->values[0] = *buffer_pointer; //get the char
		      buffer_pointer += 2;                      //skip char'

		      m_c_pointer->type = CHR_TYPE;
		      m_c_pointer->length = 1;
		      neg_flag = 0;
		} else {
		      ErrorSet.xERROR = iSymbolUsedError;
		      strcpy(ErrorSet.string, "\'");
		      success = 0;
		};
		*/
		break;

	    case '|':
		buffer_pointer++;
		if( *buffer_pointer == '|' ) {
			m_c_pointer->type = OR_TYPE;
			buffer_pointer++;
		} else  m_c_pointer->type = BITOR_TYPE;
		neg_flag = 0;
		break;

	    case '&':
		buffer_pointer++;
		if( *buffer_pointer == '&' ) {
			m_c_pointer->type = AND_TYPE;
			buffer_pointer++;
		} else {
			if( neg_flag == 0 ) {
				m_c_pointer->type = BITAND_TYPE;
			} else {
				m_c_pointer->type = VARADDR_TYPE;
				//m_c_pointer->length = '&';
			}
		}
		neg_flag = 0;
		break;
	    case '!':
		buffer_pointer++;
		switch( *buffer_pointer ) {
		    case '=':  m_c_pointer->type = NEQ_TYPE;
			       buffer_pointer++;
			       //2000.4.30
			       neg_flag = 1;			break;
		    case '>':  m_c_pointer->type = LE_TYPE;
			       buffer_pointer++;
			       //2000.4.30
			       neg_flag = 1;			break;
		    case '<':  m_c_pointer->type = GE_TYPE;
			       buffer_pointer++;
			       //2000.4.30
			       neg_flag = 1;			break;
		    default :  m_c_pointer->type = NOT_TYPE;
			       //2000.4.30
			       neg_flag = 0;			break;
		}
		//2000.4.30
		// NOT boolean,  cannot be NOT -boolean
		//neg_flag = 1;
		break;
	    case '=':
		buffer_pointer++;
		switch( *buffer_pointer ) {
		    case '=':  m_c_pointer->type = ABSOLUTE_EQ;
			       buffer_pointer++;                break;
		    case '>':  m_c_pointer->type = GE_TYPE;
			       buffer_pointer++;                break;
		    case '<':  m_c_pointer->type = LE_TYPE;
			       buffer_pointer++;                break;
		    default :  m_c_pointer->type = EQ_TYPE;
		}
		neg_flag = 1;
		break;
	    case ':':
		buffer_pointer++;
		switch( *buffer_pointer ) {
		    case '=':
                    {
                        if( mpLast == NULL ) {
			    success = 0;
			    ErrorSet.xERROR = iOPrError;
			    strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
			    break;
			}

			if( mpLast->type == IDEN_TYPE ) {
			    mpLast->type = VIDEN_TYPE;
			} else {
			    if( mpLast->type != ARRAYE_TYPE && \
				mpLast->type != FIELD_IDEN ) {
				success = 0;
				ErrorSet.xERROR = iOPrError;
				strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
				break;
			    }
			}

                        m_c_pointer->type = STORE_TYPE;
			buffer_pointer++;
                    }
		    break;
		    default :
		    {
			//short quoto = 0;
			MidCodeType *mp[MAX_OPND_NUMBER];
			short 	     itop = -1;
			int	     ifound;
			MidCodeType  mpBlank;

			mpBlank.type = 0;

			m_c_pointer->type = LABEL_TYPE;
			m_c_pointer->length = ':';
			for( p = m_c_head; p != m_c_pointer; p = p->next ) {
			   switch( p->type ) {
				case QUSTN_TYPE:
					//hasnot assigned jmp position
					if( *(long *)(p->values) == (long)NULL ) {
						mp[++itop] = p;
					}
					break;
				case LEFT_TYPE:
					mp[++itop] = p;
					//quoto++;
					break;
				case RIGHT_TYPE:
				        ifound = 0;
					for( ;  itop >= 0;  itop-- )
					{ //popup the oper
					    if( mp[itop]->type == LEFT_TYPE ) {
						ifound = 1;
						mp[itop] = &mpBlank;
						itop--;
						break;
					    }
					}
					//if( itop < 0 )
					if( ifound == 0 )
					{ //quoto error
						FreeCode( m_c_head );
						ErrorSet.xERROR = iOPrError;
						strcpy(ErrorSet.string, "()");
						return  NULL;
					}
					//quoto--;
					break;
			   }
			} //end of for

			if( itop >= 0 && mp[itop]->type == QUSTN_TYPE ) {
				*(long *)(mp[itop]->values) = (long)m_c_pointer;
			} else {
				FreeCode( m_c_head );
				ErrorSet.xERROR = iOPrError;
				strcpy(ErrorSet.string, ":");
				return  NULL;
			}

			/*
			if( quoto == 0 && mp != NULL ) {
				*(long *)(mp->values) = (long)m_c_pointer;
			} else {
				FreeCode( m_c_head );
				ErrorSet.xERROR = iOPrError;
				strcpy(ErrorSet.string, ":");
				return  NULL;
			}
			*/
		    }
		}
		neg_flag = 1;
		break;
	    case '>':
		buffer_pointer++;
		switch( *buffer_pointer ) {
		    case '>':  m_c_pointer->type = BITR_TYPE;
			       buffer_pointer++;                break;
		    case '<':  m_c_pointer->type = NEQ_TYPE;
			       buffer_pointer++;                break;
		    case '=':  m_c_pointer->type = GE_TYPE;
			       buffer_pointer++;                break;
		    default :  m_c_pointer->type = GT_TYPE;
		}
		neg_flag = 1;
		break;
	    case '<' :
		buffer_pointer++;
		switch( *buffer_pointer ) {
		    case '>':  m_c_pointer->type = NEQ_TYPE;
			       buffer_pointer++;                break;
		    case '<':  m_c_pointer->type = BITR_TYPE;
			       buffer_pointer++;                break;
		    case '=':  m_c_pointer->type = LE_TYPE;
			       buffer_pointer++;                break;
		    default :  m_c_pointer->type = LT_TYPE;
		}
		neg_flag = 1;
		break;
	    case '+': m_c_pointer->type = ADD_TYPE;  neg_flag = 0;
			       buffer_pointer++;                break;
	    case '-': if( neg_flag == 0 )  m_c_pointer->type = SUB_TYPE;
		      else {
				m_c_pointer->type = NEG_TYPE;
				//m_c_pointer->length = '-';
		      }
		      neg_flag = 0;
		      buffer_pointer++;
		      break;
	    case '*': m_c_pointer->type = MUL_TYPE;    neg_flag = 0;
		      buffer_pointer++;                break;
	    case '/': 
		      //2001.9.7
		      //an expression can ended with //
		      //it is an remark, such as:
		      //a+b //this is a remark for this expression
		      buffer_pointer++;
		      if( *buffer_pointer == '/' ) {
		          *buffer_pointer = '\0';
			  continue;
		      }

		      m_c_pointer->type = DIV_TYPE;    
		      neg_flag = 0;
		      break;

		      /*m_c_pointer->type = DIV_TYPE;    neg_flag = 0;
		      buffer_pointer++;                break;
		      */
	    case '%': m_c_pointer->type = MOD_TYPE;    neg_flag = 0;
		      buffer_pointer++;                break;
	    case '(': m_c_pointer->type = LEFT_TYPE;   neg_flag = 1;
		      buffer_pointer++;		       quotoCnt++;
		      break;
	    case ')': m_c_pointer->type = RIGHT_TYPE;  neg_flag = 0;
		      buffer_pointer++;		       quotoCnt--;
		      if( quotoCnt < 0 ) {
			  success = 0;
			  ErrorSet.xERROR = iCircleNoMatch;
			  strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
		      }
		      break;
	    case ',':
		      if( mpLast == NULL ) {
			  success = 0;
			  ErrorSet.xERROR = iToLittOPD;
			  strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
			  break;
		      }
		      if( mpLast->type == PARAL_TYPE ) {
			  success = 0;
			  ErrorSet.xERROR = iToLittOPD;
			  strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
			  break;
		      }
		      m_c_pointer->type = PARAL_TYPE;  neg_flag = 1;
		      m_c_pointer->length = *buffer_pointer++;
		      break;
	    case ';':
		      m_c_pointer->type = LABEL_TYPE;  neg_flag = 1;
		      m_c_pointer->length = *buffer_pointer++;
		      break;
	    case '?': {
		      m_c_pointer->type = QUSTN_TYPE;  neg_flag = 1;
		      *(long *)m_c_pointer->values = (long)NULL;
		      buffer_pointer++;
		      }
		      break;
	    case '\1':
		      if( mpLast == NULL ) {
			  success = 0;
			  ErrorSet.xERROR = iToLittOPD;
			  strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
			  break;
		      }
		      if( mpLast->type == PARAL_TYPE ) {
			  success = 0;
			  ErrorSet.xERROR = iToLittOPD;
			  strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
			  break;
		      }
		      m_c_pointer->type = FUNE_TYPE; neg_flag = 0;
		      *buffer_pointer++ = ')';	      break;
	    case ']' : //the ARRAYE_TYPE will store the array information
		      if( mpLast == NULL ) {
			  success = 0;
			  ErrorSet.xERROR = iToLittOPD;
			  strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
			  break;
		      }
		      if( mpLast->type == PARAL_TYPE ) {
			  success = 0;
			  ErrorSet.xERROR = iToLittOPD;
			  strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
			  break;
		      }
		      if( (m_c_pointer = realloc(m_c_pointer, \
				sizeof(MidCodeType)-\
				MAX_ATOM_LENGTH+MAX_OPND_LENGTH) ) == NULL ) {
			   ErrorSet.xERROR = iNoMem;
			   success = 0;
			   break;
		      }
		      m_c_pointer->type = ARRAYE_TYPE; neg_flag=0;
		      buffer_pointer++;                break;
	    case ' ' :
	    case '\t':
	    case '\n':
	    case '\r':
			buffer_pointer++;
			continue;       /* no pay attention character */
	    default  :  success = 0;
			ErrorSet.xERROR = iSymbolUsedError;
			strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
			FreeCode( m_c_head );
			return( NULL );
	}
	if( success == 1 ) {

	    if( ( m_c_pointer->next = \
		(MidCodeType *)malloc( sizeof( MidCodeType ) ) ) == NULL ) {
		FreeCode( m_c_head );
		ErrorSet.xERROR = iNoMem;
		return( NULL );
	    }

	    if( m_c_head == NULL ) {
		mpLast = m_c_head = m_c_pointer;
	    } else {
		mpLast->next = m_c_pointer;
	    }

	    mpLast = m_c_pointer;
	    m_c_pointer = m_c_pointer->next;
	    m_c_pointer->next = NULL;

	} else	break;

     } //end of while

     m_c_pointer->type = END_TYPE;

     if( quotoCnt > 0 ) {
	ErrorSet.xERROR = iCircleNoMatch;
	strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
	FreeCode( m_c_head );
	return  NULL;
     }
     if( success != 0 )  {
	if( m_c_head == NULL )
		return  m_c_pointer;
	return  m_c_head;
     }

     FreeCode( m_c_head );
     return  NULL;

} /*  end of WordAnalyse()  */

/*********
 *    ARGUMENTS : *M_C_POINTER     ( a struct pointer       )
 *                *tb              ( a dbase struct pointer )
 ****************************************************************************/
_declspec(dllexport) MidCodeType * PD_style SymbolRegister( MidCodeType *m_c_head, \
			     dFILE *tb, \
			     SysVarOFunType *UserSymbol, \
			     unsigned short  SymbolNum, \
			     SYSDEFINETYPE  *DefineTable, \
			     unsigned short DefineNum )
{
     int    i, j;
     void  *p;
     SysVarOFunType *vp;
     short  success= 1;
     char  *sz;

     //short ArrayStack[_ArrayOrFunNestDeep_];
     //short ArrayParaStack[_ArrayOrFunNestDeep_];
	 long ArrayStack[_ArrayOrFunNestDeep_];
	 long ArrayParaStack[_ArrayOrFunNestDeep_];

     long  FunStack[_ArrayOrFunNestDeep_];
     long  FunParaStack[_ArrayOrFunNestDeep_];
	 //short FunParaStack[_ArrayOrFunNestDeep_];

     short ArrayDeep = -1, FunDeep = -1;
     char  buf[ MAX_OPND_LENGTH ];

 __try {
     ErrorSet.xERROR = XexpOK;
     actionFunRefered = 0;
     /* this line to avoid NULL pointer use of UserSymbol */
     /*1998.5.11
     if( UserSymbol == NULL )   SymbolNum = 0;
     else {
	if( SymbolNum <= 0 )
		for( SymbolNum = 0; UserSymbol[SymbolNum++].type != 0; );
     };
     */

/* Pointer the new position */

     if( DefineTable == NULL )  DefineNum = 0;
     else {
	if( DefineNum <= 0 )
		for( DefineNum = 0; *(char *)DefineTable[DefineNum++].DefineName != 0; );
     }

     ArrayDeep = FunDeep = -1;

     while( m_c_head != NULL && success == 1 ) {
	p = NULL;
	switch( m_c_head->type ) {
	   case IDEN_TYPE:
	   case VIDEN_TYPE:
		sz = m_c_head->values;
		// look whether it is in UserSymbol
		for(i = 0;  i < SymbolNum && /*!IsBadReadPtr( UserSymbol[i].VarOFunName, 32 ) &&\*/
				strnicmp(sz, \
				UserSymbol[i].VarOFunName,32) != 0; i++ );

		/*if( IsBadReadPtr( UserSymbol[i].VarOFunName, 32 ) ) {
			Sleep( 0 );
			MessageBox( 0, "HELLO", "", MB_OK );
		}*/

		if( i < SymbolNum ) {

			if( UserSymbol[i].length < 0 )
			{ //indirect store variable
			    *(long *)m_c_head->values = *(long *)UserSymbol[i].values;
			    m_c_head->length = -UserSymbol[i].length;
			} else {
			    *(long *)m_c_head->values = (long)UserSymbol[i].values;
			    m_c_head->length = UserSymbol[i].length;
			}

			if( m_c_head->type == VIDEN_TYPE )
			    m_c_head->type = (UserSymbol[i].type|0x1000);
			else
			    m_c_head->type = UserSymbol[i].type;
			break;
		}

		//support ASQLANA.c askQS()
		//whether it is a user defined base variable 1998.5.11
		for(i = 0;  i < nSelfBaseVar && strnicmp(sz, \
				     xSelfBaseVar[i].VarOFunName,32) != 0; i++ );
		if( i < nSelfBaseVar ) {
			if( xSelfBaseVar[i].length < 0 )
			{ //indirect store variable
			    *(long *)(m_c_head->values) = *(long *)xSelfBaseVar[i].values;
			    m_c_head->length = -xSelfBaseVar[i].length;
			} else {
			    *(long *)(m_c_head->values) = (long)xSelfBaseVar[i].values;
			    m_c_head->length = xSelfBaseVar[i].length;
			}
			if( m_c_head->type == VIDEN_TYPE )
			    m_c_head->type = (xSelfBaseVar[i].type|0x1000);
			else
			    m_c_head->type = xSelfBaseVar[i].type;
			break;
		}


		// look whether it is a sys variable
		for(i = 0;  i < SysVarNum && strnicmp(sz, \
					SysVar[i].VarOFunName,32) != 0; i++ );
		if( i < SysVarNum ) {
			if( m_c_head->type == VIDEN_TYPE ) {
			    if( i < SysVarAsqlUserReadonlyNum ) {
				ErrorSet.xERROR =iAsqlUserReadonly;
				strZcpy(ErrorSet.string, sz, XES_LENGTH);
				return  m_c_head;
			    } else {
				m_c_head->type = (SysVar[i].type|0x1000);
			    }
			} else {
			    m_c_head->type = SysVar[i].type;
			}

			*(long *)(m_c_head->values) = (long)SysVar[i].values;
			m_c_head->length = SysVar[i].length;
			break;
		}

		//continue search for field
		if( tb == NULL ) {
			ErrorSet.xERROR =iUndefineVar;
			strcpy(ErrorSet.string, m_c_head->values);
			return  m_c_head;
		}

	   case FIELD_IDEN:
		memcpy(buf, m_c_head->values, MAX_OPND_LENGTH);

		if( tb == NULL ) {
			ErrorSet.xERROR = iUseTabel;       // tb shouldn't be NULL
			return  m_c_head;
		}

		if( (p = strchr(buf, '.')) != NULL ) {
			*(char *)p = '\0';
		}

		/* get the field position in i */
		if( (*(SPECIALFIELDSTRUCT *)buf).Mark == '@' ) {
			i = (*(SPECIALFIELDSTRUCT *)buf).FieldNo - 1;
			if( i < 0 || i >= tb->field_num ) {
			    ErrorSet.xERROR = iNoField;
			    return  m_c_head;
			}

			strcpy(buf, tb->field[i].field);
		} else {

			//find symbol in source
			if( p != NULL ) {
			    if( isdigit( *((char *)p+1) ) )
			    {
				register char c;
				dFIELD        *fld = tb->field;

				c = *buf;
				if( c >= 'a' && c <= 'z' ) {
				    c += ('A' - 'a');
				}

				for( i = 0; i < tb->field_num; i++ ) {

				    if( fld[i].field[0] != c )
					continue;

				    if( stricmp(fld[i].field, buf) == 0 ) break;
				}
			    } else {
				//field is as tablename.fieldname type
				//and this kind of fieldname can appeares
				//in more dbf ASQL
				register char c;
				dFIELD        *fld = tb->field;

				*(char *)p = '.';

				c = *buf;
				if( c >= 'a' && c <= 'z' ) {
				    c += ('A' - 'a');
				}

				for( i = 0; i < tb->field_num; i++ ) {

				    if( fld[i].field[0] != c )
					continue;

				     if( stricmp(fld[i].field, buf) == 0 )
					break;
				}
				if( i >= tb->field_num ) {
				    sz = (char *)p+1;
				    for( i = 0; i < tb->field_num; i++ )
				     if( stricmp(tb->field[i].field, sz) == 0 )
				     {
					 char buff[32];
					 char *sz;

					 sz = tb->field[i].field;
					 sz += strlen(sz)+1;

					 if( *sz != '\0' ) {
						*(char *)p = '\0';
						strZcpy(buff, buf, 32);
						*(char *)p = '.';
						if( stricmp(buff, sz) == 0 )
							break;
					 } else {
						break;
					 }
				     }
				}
			    }
			} else {
			    register char c;
			    dFIELD        *fld = tb->field;

			    c = *buf;
			    if( c >= 'a' && c <= 'z' ) {
				c += ('A' - 'a');
			    }

			    for( i = 0; i < tb->field_num; i++ ) {

				if( fld[i].field[0] != c )
					continue;

				if( stricmp(fld[i].field, buf) == 0 ) break;
			    }
			}

			if( i >= tb->field_num ) {
			    if( askTdf == NULL )
			    {//in action expression, field can be new appeared
				ErrorSet.xERROR = iNoField;
				return  m_c_head;
			    }
			}
			memcpy(buf, m_c_head->values, MAX_OPND_LENGTH);
		} /* end of find field in source */

		  /* end of if m_c_head->type == FIELD_IDEN
		   * i now stored the field order, m_c_head->values has the
		   * complete described of field
		   */
		/*++++++++++ union use by IDEN_TYPE & FIELD_IDEN ++++++++++*/

		//find field in target
		vp = NULL;
		if( askTdf != NULL ) {
			/* action expression, register more information.
			 * now the alias of field is usefull
			 */
		  if( i < tb->field_num ) {
		      (*(dFIELDWHENACTION *)m_c_head->values).pSourceStart = tb->field[i].fieldstart;
		      (*(dFIELDWHENACTION *)m_c_head->values).wSourceid = i;
		      (*(dFIELDWHENACTION *)m_c_head->values).pSourceDfile =  tb;
		  } else
		  { //give it a really value to avoid program dump. the result is random
		      if( (m_c_head->next != NULL) && (m_c_head->next->type == STORE_TYPE) ) {
			(*(dFIELDWHENACTION *)m_c_head->values).pSourceStart = tb->field[0].fieldstart;
			(*(dFIELDWHENACTION *)m_c_head->values).wSourceid = 0xFFFF;
			(*(dFIELDWHENACTION *)m_c_head->values).pSourceDfile =  NULL;
		      } else {
			ErrorSet.xERROR = iNoField;
			strZcpy(ErrorSet.string, m_c_head->values, MAX_OPND_LENGTH);
			return  m_c_head;
		      }

		  }

		  // vp is ASSIGN value here !
		  if( p != NULL )
		  { //it is a field type, but it has symbol require
		     char *sp;

		     if( *((char *)p+1) == '0' ) {
			 sp = (char *)p + 2;
		     } else {
			 *(char *)p = '.';
			 sp = buf;
		     }
		     if( ( vp = SymbolTableSearch(sp, UserSymbol, SymbolNum ) ) == NULL ) {

			//support ASQLANA.c askQS()
			//whether it is a user defined base variable 2000.4.20
			if( ( vp = SymbolTableSearch(sp, xSelfBaseVar, nSelfBaseVar ) ) != NULL ) {
			    (*(dFIELDWHENACTION *)m_c_head->values).ResultMem = vp->values;
			} else {
			    (*(dFIELDWHENACTION *)m_c_head->values).ResultMem = NULL;
			}
			/*this is not an error
			  for the table.field can be writen by users himself
			  ^-_-^ 1998.5.28
				ErrorSet.xERROR = iActionParaError;
				return  m_c_head;
			*/
		     } else {
			(*(dFIELDWHENACTION *)m_c_head->values).ResultMem = \
								vp->values;
		     }

		     //1998.9.3
		     //*(char *)p = '\0';
		  }

		  if( askTdf == (dFILE *)1 )
		  { //it is action expression, but to is not assigned
			if( p != NULL ) {
			    (*(dFIELDWHENACTION *)m_c_head->values).pTargetStart = tb->field[i].fieldstart;
			    (*(dFIELDWHENACTION *)m_c_head->values).wTargetid = i;
			    (*(dFIELDWHENACTION *)m_c_head->values).pTargetDfile = tb;
			} else {
			    (*(dFIELDWHENACTION *)m_c_head->values).pTargetStart = NULL;
			    (*(dFIELDWHENACTION *)m_c_head->values).pTargetDfile = NULL;
			    //p = NULL;
			}
			goto TO_IS_NULL;
		  }

		  if( p == NULL )
		  { //we will treat 'db.field' as no target field 1998.5.28
		      for(j = 0; j < askTdf->field_num && \
			     stricmp(buf, askTdf->field[j].field) !=0; j++);
		  }

		  if( p != NULL || j >= askTdf->field_num )
		  {//doesn't appeared in target dbf
			if( i >= tb->field_num )
			{ //doesn't appeared in source dbf?
				ErrorSet.xERROR = iNoDBF;
				strZcpy(m_c_head->values, buf, MAX_OPND_LENGTH);
				return  m_c_head;
			}
			j = 0;	//give it a random value and continue
				//POINT TO SOURCE 1998.5.28
				//this store statement is no use, perhaps
		      (*(dFIELDWHENACTION *)m_c_head->values).pTargetStart = tb->field[i].fieldstart;
		      (*(dFIELDWHENACTION *)m_c_head->values).wTargetid = i;
		      (*(dFIELDWHENACTION *)m_c_head->values).pTargetDfile = tb;
		  } else {
		      // j now stores the target position
		      (*(dFIELDWHENACTION *)m_c_head->values).pTargetStart = askTdf->field[j].fieldstart;
		      (*(dFIELDWHENACTION *)m_c_head->values).wTargetid = j;
		      (*(dFIELDWHENACTION *)m_c_head->values).pTargetDfile = askTdf;
		  }
		} else /* condition expression */ {
			(*(dFIELDWHENACTION *)m_c_head->values).pSourceStart = tb->field[i].fieldstart;
			(*(dFIELDWHENACTION *)m_c_head->values).wSourceid = i;
			(*(dFIELDWHENACTION *)m_c_head->values).pSourceDfile =  tb;

			//1998.9.1
			(*(dFIELDWHENACTION *)m_c_head->values).pTargetStart = tb->field[i].fieldstart;
			(*(dFIELDWHENACTION *)m_c_head->values).wTargetid = i;
			(*(dFIELDWHENACTION *)m_c_head->values).pTargetDfile =  tb;
		}

		if( i >= tb->field_num )
		{ //appear in target dbf but not in source dbf
			m_c_head->length = (short)(askTdf->field[j].fieldlen);
			switch( askTdf->field[j].fieldtype ) {
			case 'C':

				if( vp != NULL )    vp->type = STRING_IDEN;
				// register the symbol type

				m_c_head->type = CFIELD_IDEN;
				break;
			case 'N':
				if( askTdf->field[j].fielddec == 0 ) {
					m_c_head->type = NFIELD_IDEN;
					if( vp != NULL )    vp->type = LONG_IDEN;
				} else {
					m_c_head->type = FFIELD_IDEN;
					if( vp != NULL )    vp->type = FLOAT_IDEN;
				}
				break;
			case 'D':
				if( vp != NULL )    vp->type = DATE_IDEN;
				m_c_head->type = DFIELD_IDEN;
				break;
			case 'M':
			case 'G':
				if( vp != NULL )    vp->type = STRING_IDEN;
				m_c_head->type = MFIELD_IDEN;
				break;
			case 'L':
				if( vp != NULL )    vp->type = STRING_IDEN;
				m_c_head->type = LFIELD_IDEN;
				break;
			default:
				ErrorSet.xERROR = iNoThisDataType;     /* not support data type */
				return  m_c_head;
			} // end of switch
		break;
		}

TO_IS_NULL:		// goto here

		m_c_head->length = (short)(tb->field[i].fieldlen);
		switch( tb->field[i].fieldtype ) {
			case 'C':

				if( vp != NULL )    vp->type = STRING_IDEN;
				// register the symbol type

				m_c_head->type = CFIELD_IDEN;
				break;
			case 'N':
				if( tb->field[i].fielddec == 0 ) {
					m_c_head->type = NFIELD_IDEN;
					if( vp != NULL )    vp->type = LONG_IDEN;
				} else {
					m_c_head->type = FFIELD_IDEN;
					if( vp != NULL )    vp->type = FLOAT_IDEN;
				}
				break;
			case 'D':
				if( vp != NULL )    vp->type = DATE_IDEN;
				m_c_head->type = DFIELD_IDEN;
				break;
			case 'M':
			case 'G':
				if( vp != NULL )    vp->type = STRING_IDEN;
				m_c_head->type = MFIELD_IDEN;
				break;
			case 'L':
				if( vp != NULL )    vp->type = STRING_IDEN;
				m_c_head->type = LFIELD_IDEN;
				break;
			default:
				ErrorSet.xERROR = iNoThisDataType;     /* not support data type */
				return  m_c_head;
		} // end of switch

		break;
	   case ARRAYB_TYPE:
			/* look whether it is in UserSymbol. algorithm: Half*/
			for(i = 0 ;  i < SymbolNum && \
				strnicmp(m_c_head->values, \
				UserSymbol[i].VarOFunName,32) != 0; i++ );
			if( i < SymbolNum ) {
				ArrayStack[++ArrayDeep] = i;
				ArrayParaStack[ArrayDeep] = m_c_head->length;
#ifdef XexpDebugOn
				if( ArrayDeep >= _ArrayOrFunNestDeep_ - 1 ) {
					ErrorSet.xERROR = iNestToDepth;
					break;
				}
#endif
			} /* end of if */
			break;
	   case ARRAYE_TYPE:
			if( ArrayDeep >= 0 ) {
				memcpy(m_c_head->values, \
				   UserSymbol[ArrayStack[ArrayDeep]].values,\
				   MAX_OPND_LENGTH );
				m_c_head->length = ArrayParaStack[ArrayDeep--];
			} else {
				success = 0;
				ErrorSet.xERROR =iNoMatchArray ;
				return  m_c_head;
			}
			break;
	   case FUNB_TYPE:
			/* look whether it is in UserSymbol. algorithm: Half*/
			/*old version  1998.2.22
			for(i = 0 ;  i < SysFunNum && \
				stricmp(m_c_head->values, \
				       SysFun[i].VarOFunName) != 0; i++ );
			// if it is a system function call, register it
			if( i < SysFunNum ) {
			*/
			j = toupper(m_c_head->values[0]) - 'A';
			i = SysFunHash[j];
			j = SysFunHash[j+1];
			for( ;  i < j && strnicmp(m_c_head->values, \
				     SysFun[i].VarOFunName,32) != 0;   i++ );
			// if it is a system function call, register it
			if( i < j ) {
#ifdef XexpDebugOn
				if( FunDeep >= _ArrayOrFunNestDeep_ - 1 ) {
					ErrorSet.xERROR = iNestToDepth;  /* nest too deep */
					return  m_c_head;
				}
#endif
				if( m_c_head->length >= SysFun[i].varnum1 && \
						    m_c_head->length <= SysFun[i].varnum2 )
				{
				    FunStack[++FunDeep] = i;
				    FunParaStack[FunDeep] = m_c_head->length;
				    break;
				} else {
				    ErrorSet.xERROR = iFailFunCall;  //para error
				    strZcpy(ErrorSet.string, m_c_head->values, MAX_OPND_LENGTH);
				    return  m_c_head;
				}

			}
			if( askTdf != NULL ) {
				/*old version 1998.2.22
				for(i = 0;  i < AskActionFunNum && \
					stricmp(m_c_head->values, \
					AskActionTable[i].VarOFunName) != 0; i++ );
				if( i < AskActionFunNum ) {
				*/
				j = toupper(m_c_head->values[0]) - 'A';
				i = ActFunHash[j];
				j = ActFunHash[j+1];
				for( ;  i < j && strnicmp(m_c_head->values, \
					AskActionTable[i].VarOFunName,32) != 0; i++ );
				if( i < j ) {
#ifdef xEXPDEBUG
					if( FunDeep >= _ArrayOrFunNestDeep_ - 1 ) {
						ErrorSet.xERROR = iNestToDepth;
						return  m_c_head;
					}
#endif
					if( m_c_head->length >= AskActionTable[i].varnum1 && \
						m_c_head->length <= AskActionTable[i].varnum2 )
					{
					    FunStack[++FunDeep] = -i-1;
					    FunParaStack[FunDeep] = m_c_head->length;

					    actionFunRefered = 1;

					    break;
					} else {
					    ErrorSet.xERROR = iFailFunCall;  //para error
					    strZcpy(ErrorSet.string, m_c_head->values, MAX_OPND_LENGTH);
					    return  m_c_head;
					}
				}
			} /* end of if */

#ifdef __FOR_WINDOWS__
			//user gives function with *.DLL
			if( hLibXexp == NULL ) {
				strZcpy(ErrorSet.string, m_c_head->values, MAX_OPND_LENGTH);
				ErrorSet.xERROR = iNoAction; /* no this action */
				return  m_c_head;
			}

			/*
			if( ( hLibXexp = LoadLibrary( "isql.dll" ) ) == NULL )
			{
				MessageBox( NULL, "Can not load isql.dll", "ERROR", MB_OK | MB_ICONSTOP );
				return  FALSE;
			}
			FreeLibrary( hLibXexp );
			*/

			{
			    FARPROC	isqlFunc;
			    if( (isqlFunc = GetProcAddress(hLibXexp, \
					     m_c_head->values) ) == NULL ) {
				strZcpy(ErrorSet.string, m_c_head->values, MAX_OPND_LENGTH);
				ErrorSet.xERROR = iNoAction; /* no this action */
				return  m_c_head;
			    }
			    FunStack[++FunDeep] = (long)isqlFunc;
			    FunParaStack[FunDeep] = m_c_head->length;
			}
#endif
			break;
	   case FUNE_TYPE:
			if( FunDeep >= 0 ) {
#ifdef __FOR_WINDOWS__
				if( labs(FunStack[FunDeep]) > 4096 ) {
				     *(long *)m_c_head->values = \
							FunStack[FunDeep];
				} else
#endif
				if( FunStack[FunDeep] >= 0 ) {
				     *(long *)m_c_head->values = \
					  (long)SysFun[FunStack[FunDeep]].FarProc;
				} else {
				     *(long *)m_c_head->values = \
					  (long)AskActionTable[-1-FunStack[FunDeep]].FarProc;
				}
				m_c_head->length = FunParaStack[FunDeep--];
			} else {
				success = 0;
				ErrorSet.xERROR = iNoMatchFun ;
				return( m_c_head );
			}

	}				 /* end of switch m_c_head->type */
	m_c_head = m_c_head->next;
     }     				 /* end of while */

 }
 __except ( EXCEPTION_EXECUTE_HANDLER  ) {
	 MessageBox( 0, "Some Exception ocurred.", "Exception", MB_OK | MB_ICONINFORMATION );
	 return  NULL;
 }
     return  NULL;

}					 /* end of switch_field */

/*********
 *                      CalExpr()
 **************************************************************************/
_declspec(dllexport) long PD_style CalExpr( MidCodeType *m_c_pointer )
{
    static signed char priority[32][32] = {
/*********|| &&  !  = ==  >  <  >= <= != +  -  *  /  %  >>  &  | ~  ^  (  )  f( f) -D := ,  ?  L: A[ A] ED **/
/********* 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 **/
/* 0 ||*/{ 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 2, 2, 2, 0,90, 2},
/* 1 &&*/{ 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 2, 2, 3, 0,90, 2},
/* 2 ! */{ 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 2, 2, 2, 0,90, 2},
/* 3 = */{ 2, 2, 2, 90,90,90,90,90,90,90,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 2, 2, 2, 0,90, 2},
/* 4 ==*/{ 2, 2, 2, 90,90,90,90,90,90,90,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 2, 2, 2, 0,90, 2},
/* 5 > */{ 2, 2, 2, 90,90,90,90,90,90,90,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 2, 2, 2, 0,90, 2},
/* 6 < */{ 2, 2, 2, 90,90,90,90,90,90,90,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 2, 2, 2, 0,90, 2},
/* 7 >=*/{ 2, 2, 2, 90,90,90,90,90,90,90,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 2, 2, 2, 0,90, 2},
/* 8 <=*/{ 2, 2, 2, 90,90,90,90,90,90,90,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 2, 2, 2, 0,90, 2},
/* 9 !=*/{ 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 8, 8, 2, 0, 0, 2, 0, 2, 2, 0, 2, 2, 2, 0,90, 2},
/*10 + */{ 2, 2, 90,2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 2, 2, 2, 0,90, 2},
/*11 - */{ 2, 2, 90,2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 2, 2, 2, 0,90, 2},
/*12 * */{ 2, 2, 90,2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 2, 2, 2, 0,90, 2},
/*13 / */{ 2, 2, 90,2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 2, 2, 2, 0,90, 2},
/*14 % */{ 2, 2, 90,2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 2, 2, 2, 0,90, 2},
/*15 >>*/{ 2, 2, 90,2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 2, 2, 2, 0,90, 2},
/*16 & */{ 2, 2, 90,2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 2, 0, 2, 2, 2, 2, 0,90, 2},
/*17 | */{ 2, 2, 90,2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 2, 0, 2, 2, 2, 2, 0,90, 2},
/*18 ~ */{ 2, 2, 90,2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 2, 0, 2, 2, 2, 2, 0,90, 2},
/*********|| &&  !  = ==  >  <  >= <= != +  -  *  /  << >>  &  | ~  ^  (  )  f( f) -D := ,  ?  L: A[ A] ED **/
/********* 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 **/
/*19 ^ */{ 0, 0, 90,2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 2, 2, 0, 0, 0, 2, 0, 2, 0, 2, 2, 2, 2, 0,90, 2},
/*20 ( */{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 3, 9, 5, 0,90, 2},
/*21 ) */{ 90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,1, 2},
/*22 f(*/{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 3, 9, 5, 0,90, 2},
/*23 f)*/{ 90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,1, 2},
/*24 -D*/{ 2, 2, 90,2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 90,0, 90,0, 90,3, 2, 2, 0, 2, 2},
/*25 :=*/{ 2, 2, 90,2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 2, 2, 2, 0, 10,2},
/*26 , */{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10,10,10,10,0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 10,10,2, 2, 0,90, 2},
/*27 ? */{ 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,90, 9,90, 9, 9, 9, 0, 9, 9,90, 9},
/*28 L:*/{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1},
/*29 A[*/{ 90,90,90,90,90,90,90,90,90,90,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,10, 0,13, 0, 0, 3, 0, 2, 0, 4, 2},
/*30 A]*/{ 90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90},
/*31 ED*/{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,90, 0, 0,10, 9, 5, 0, 90,1}
};
//every expression will stop while it meets ':'
//a+b?c will make the result of a+b to LOGIC_TYPE

     short success = 1;               /* 1: success   0: error code  */
     short itype;			// temp for type
	 long  ilength;			// temp fot length
     //short ilength;			// temp fot length

     short i;
     char  *svalues;			// temp for values
//     MidCodeType  *mcpQustnRem;

     //REMARK this, for the if statement will do this
     //if( m_c_pointer == NULL )        return  LONG_MAX;

     optr[ optr_top = optrStackFund ] = END_TYPE;
     opnd_top = opndStackFund-1;

     updateFlag = 0;

     *(long *)&ErrorSet = 0;

     while( success == 1 ) {
	if( m_c_pointer != NULL ) {
		itype = m_c_pointer->type;
		ilength = m_c_pointer->length;
		svalues = m_c_pointer->values;
	} else {
		itype = END_TYPE;
	}

	// const or var
	if( itype >= FIELD_IDEN && itype <= SFIELD_IDEN || \
						      itype == STRING_TYPE) {
		      /*case CFIELD_IDEN:       // 1062
			case NFIELD_IDEN:       // 1057
			case FFIELD_IDEN:       // 1058
			case LFIELD_IDEN:       // 1059
			case DFIELD_IDEN:       // 1060
			case MFIELD_IDEN:       // 1061
			case SFIELD_IDEN:       // 1063
			case STRING_TYPE  	// 1068: const string
			*/
		//ps = ( dp = &opnd[++opnd_top] )->values;
		dp = &opnd[++opnd_top];
		dp->type = itype;
		dp->length = ilength;
		dp->oval = svalues;
		m_c_pointer = m_c_pointer->next;
		continue;
	}

	if( itype >= IDEN_TYPE /*&& itype <= ARRAY_TYPE )  || \
			itype == NEG_TYPE*/ ) {
		ps = ( dp = &opnd[++opnd_top] )->values;
		dp->type = itype;
		dp->oval = NULL;
		switch( itype ) {
			//case STRING_TYPE:       // 1068: constr string
			case ARRAY_TYPE:        /* 1077 */

				dp->length = ilength;
				dp->oval = svalues;
//				memcpy( dp, m_c_pointer, sizeof(OpndType) );
// we use the upper memcpy for data store
//				opnd[opnd_top].type = m_c_pointer->type;
//				opnd[opnd_top].length = m_c_pointer->length;
				break;
			case STRING_IDEN:       /* 1051 */
				if( ( dp->length = ilength ) < MAX_OPND_LENGTH ) {
				    strZcpy(ps, (char *)*(long *)*(long *)svalues, ilength+1);
				} else {
				    dp->length = strlen( (char *)*(long *)*(long *)svalues );
				    if( dp->length < MAX_OPND_LENGTH ) {
					strZcpy(ps, (char *)*(long *)*(long *)svalues, dp->length+1);
				    } else {
					*(long *)ps = (long)strdup( (char *)*(long *)*(long *)svalues );
				    }
				}
				dp->oval = NULL;
				dp->type = STRING_TYPE;
				break;
                        case VSTRING_IDEN:
                                *(long *)ps = *(long *)svalues;
				dp->length = ilength;
                                break;
			case CHR_IDEN:          /* 1052 */
				*(char *)ps = *(char *)*(long *)svalues;
				dp->type = CHR_TYPE;
				break;
			case VCHR_IDEN:
				*(long *)ps = *(long *)svalues;
				dp->length = ilength;
				break;
			case INT_IDEN:          /* 1053 */
				*(short *)ps = *(short *)*(long *)svalues;
				dp->type = INT_TYPE;
				break;
			case VINT_IDEN:
				*(long *)ps = *(long *)svalues;
				dp->length = ilength;
				break;
			case LONG_IDEN:         /* 1054 */
				*(long *)ps = *(long *)*(long *)svalues;
				dp->type = LONG_TYPE;
				break;
			case VLONG_IDEN:
				*(long *)ps = *(long *)svalues;
				dp->length = ilength;
				break;
			case FLOAT_IDEN:        /* 1055 */
				*(double *)ps = *(double *)*(long *)svalues;
				dp->type = FLOAT_TYPE;
				break;
			case VFLOAT_IDEN:
                                *(long *)ps = *(long *)svalues;
				dp->length = ilength;
				break;
			case DATE_IDEN:
				strZcpy(ps, (char *)*(long *)svalues, 9);
				dp->type = DATE_TYPE;
				dp->length = 8;
				break;
                        case VDATE_IDEN:
				*(long *)ps = *(long *)svalues;
				dp->length = ilength;
				break;
			case CHR_TYPE:          /* 1064 */
				*(long *)ps = *(char *)svalues;
				break;
			case INT_TYPE:          /* 1065 */
				*(long *)ps = *(short *)svalues;
				break;
			case LONG_TYPE:          /* 1066 */
				*(long *)ps = *(long *)svalues;
				break;
			case FLOAT_TYPE:         /* 1067 */
				*(double *)ps = *(double *)svalues;
				break;
			case DATE_TYPE:
				strZcpy(ps, svalues, 9);
				dp->length = 8;
				break;
			case NEG_TYPE:
				m_c_pointer = m_c_pointer->next;
				if( neg_g( m_c_pointer ) != 0 )      success = 0;
				break;
			case VARADDR_TYPE:
				if( m_c_pointer->next->type != ARRAYB_TYPE )
					opnd_top--;
				break;
			case INT64_IDEN:         /* 1074 */
				*(long *)ps = *(long *)*(long *)svalues;
				dp->type = LONG_TYPE;
				break;
			case VINT64_IDEN:
				*(_int64 *)ps = *(_int64 *)svalues;
				dp->length = ilength;
				break;
			default:
				ErrorSet.xERROR = iInVaildOpndType;         /* opnd type error */
				success = 0;
		} /* end of switch */
		m_c_pointer = m_c_pointer->next;
	} else {

	    if( itype == QUSTN_TYPE && optr_top <= optrStackFund ) {
		//m_c_pointer = mcpQustnRem;
		if( qustn_g( &m_c_pointer ) ) {
			success = 0;
		}
		continue;
	    }

	    ps = ( dp = &opnd[opnd_top] )->values;
//	    col_priorityis itype now
	    switch( priority[ optr[optr_top] ][ itype ] ) {
		case 0: /* push the optr into optr_strack if the priority
			 * is equal. for example: .and. meet .or., now we
			 * cannot first calculate .and.
			 */
//			if( itype == QUSTN_TYPE )
//				mcpQustnRem = m_c_pointer;

			optr[ ++optr_top ] = itype;
			m_c_pointer = m_c_pointer->next;
			break;

		case 1: /* Only the .(. meet .). or the end of expr. found
			 * can appear this priority. Now the stack top has
			 * .(.
			 */
			if ( itype == RIGHT_TYPE ) {     /* .). */
				optr_top--;
				m_c_pointer = m_c_pointer->next;
			} else if( opnd_top == opndStackFund ) {
				  /* Only logic value is expressed with short
				   * so we can return the logic value
				   * directly for speed, else the calculated
				   * value and type is stored in struct
				   * OpndType
				   */
				  switch( opnd[ opndStackFund ].type ) {
					case LONG_TYPE:
						return( *(long *)opnd[opndStackFund].values );
					case STRING_TYPE:
					case NFIELD_IDEN:    /**************/
					case FFIELD_IDEN:    /* the fields */
							     /* iden has   */
					case DFIELD_IDEN:    /* the same   */
					//case MFIELD_IDEN:  /* storage    */
					case CFIELD_IDEN:    /* method     */
					case SFIELD_IDEN:    /**************/
					case MFIELD_IDEN:
					{
					    char *sz;

					    sz = xGetOpndString(&opnd[opndStackFund]);
					    _xEXP_result_mem = realloc(_xEXP_result_mem, strlen(sz)+1);
					    strcpy(_xEXP_result_mem, sz);
					    return  (long)_xEXP_result_mem;
					}
					break;
					case DATE_TYPE:
					case FLOAT_TYPE:
						return( (long)opnd[opndStackFund].values );
					case CHR_TYPE:
						return( *(char *)opnd[opndStackFund].values );
					case INT_TYPE:
						//opnd[opndStackFund].values[2] = 0;
						//opnd[opndStackFund].values[3] = 0;
						return( *(short *)opnd[opndStackFund].values );
					case LOGIC_TYPE:
						//opnd[opndStackFund].values[2] = 0;
						//opnd[opndStackFund].values[3] = 0;
						return( *(short *)opnd[opndStackFund].values );
					case LFIELD_IDEN:
					{
					    char *sz;

					    sz = xGetOpndString(&opnd[opndStackFund]);
					    if( *sz == 'T' )
						return *(short *)opnd[opndStackFund].values = 1;
					    else
						return *(short *)opnd[opndStackFund].values = 0;
					}
					default:
						return  (long)opnd[opndStackFund].values;    //type error not mentioned
				  }
			       } else {
				  if( opnd_top <= opndStackFund ) {
					opnd_top = opndStackFund;
					opnd[opndStackFund].type = LOGIC_TYPE;
					return( *(short *)opnd[opndStackFund].values = 1 );
				  }

				  for(i=opndStackFund;  i<opnd_top;  i++) {
				      if( opnd[i].type == STRING_TYPE && \
						opnd[i].length >= MAX_OPND_LENGTH ) {
					  free((char *)*(long *)opnd[i].values);
				      }

				  }

				  opnd[opndStackFund].type = LONG_TYPE;
				  ErrorSet.xERROR = iToManyOPD; // TOO MANY OPND IN GRAMMAR !!!
				  return  *(long *)opnd[opndStackFund].values = LONG_MAX;
			       }
			break;

		case 2: /* optr pop optr_strack */
			switch( optr[ optr_top ] ) {
			   case OR_TYPE:
				if( or_g() < 0 )	success = 0;
				break;
			   case AND_TYPE:
				if( and_g() < 0 ) 	success = 0;
				break;
			   case NOT_TYPE:
				if( not_g() < 0 )	success = 0;
				break;
			   case ABSOLUTE_EQ:
				if( absxcomp() < -1 ) 	success = 0;
				if( *(short *)ps == 0 )
					*(short *)ps = 1;
				else    *(short *)ps = 0;
				break;
			   case EQ_TYPE:
				if( xcomp() < -1 ) 	success = 0;
				if( *(short *)ps == 0 )
					*(short *)ps = 1;
				else    *(short *)ps = 0;
				break;
			   case GT_TYPE:
				if( xcomp() < -1 ) 	success = 0;
				if( *(short *)ps > 0 )
					*(short *)ps = 1;
				else    *(short *)ps = 0;
				break;
			   case LT_TYPE:
				if( xcomp() < -1 ) 	success = 0;
				if( *(short *)ps < 0 )
					*(short *)ps = 1;
				else    *(short *)ps = 0;
				break;
			   case GE_TYPE:
				if( xcomp() < -1 ) 	success = 0;
				if( *(short *)ps >= 0 )
					*(short *)ps = 1;
				else    *(short *)ps = 0;
				break;
			   case LE_TYPE:
				if( xcomp() < -1 ) 	success = 0;
				if( *(short *)ps <= 0 )
					*(short *)ps = 1;
				else    *(short *)ps = 0;
				break;
			   case NEQ_TYPE:
				if( xcomp() < -1 ) 	success = 0;
				if( *(short *)ps != 0 )
					*(short *)ps = 1;
				else    *(short *)ps = 0;
				break;
			   case ADD_TYPE:
			   {
				if( add_g() != 0 ) 	success = 0;
			   }
			   break;
			   case SUB_TYPE:
			   {
				if( minus_g() != 0 )	success = 0;
			   }
			   break;
			   case MUL_TYPE:
			   {
				if( multiple_g() != 0 )	success = 0;
			   }
			   break;
			   case DIV_TYPE:
			   {
				if( divide_g() != 0 )	success = 0;
			   }
			   break;
			   case MOD_TYPE:
			   {
				if( mod_g() != 0 )	success = 0;
			   }
			   break;
			   case STORE_TYPE:
			   {
				if( store_g() != 0 ) 	success = 0;
			   }
			   break;
			   case LEFT_TYPE:              break;
			   case RIGHT_TYPE:             break;
			}
			if( opnd_top < 0 ) {
			   ErrorSet.xERROR = iToLittOPD; //TOO LITTER OPND IN GRAMMAR !!!
			   success = 0;
			}
			if( --optr_top < 0 ) {
			   ErrorSet.xERROR = iToLittOPr; //TOO LITTER OPTR IN GRAMMAR !!!
			   success = 0;
			}
			break;

		case 3:                 // eat the operator and do nothing
			m_c_pointer = m_c_pointer->next;
			break;

		case 4:                 // array call, A[ meet A]
			if( array_g( m_c_pointer ) )
				success = 0;
			m_c_pointer = m_c_pointer->next;    /* eat the ] */
			optr_top--;
			break;

		case 5:
			// a ? b : c ; d ? e : f
			//       ^     ^
			//       +-----+
			// if there is no ';' found, m_c_pointer->type = END_TYPE;
			// else m_c_pointer will be d
			//
			if( m_c_pointer->length == ':' ) {
			    MidCodeType  *mc;
			    int		  founded;
			    int           brackets;

			    for(i=opndStackFund;  i<opnd_top;  i++) {
				if( opnd[i].type == STRING_TYPE && \
					opnd[i].length >= MAX_OPND_LENGTH ) {
				    free((char *)*(long *)opnd[i].values);
				}
			    }

			    //search for ';' optr
			    founded = 0;
			    brackets = 0;
			    mc = m_c_pointer->next;
			    while( mc && mc->type != END_TYPE )
			    {
				if( mc->type == LEFT_TYPE )
				    brackets++;

				if( mc->type == RIGHT_TYPE ) {
				    if( brackets <= 0 ) {
					founded = 2;
					break;
				    }
				    brackets--;
				}

				if( mc->type == LABEL_TYPE && mc->length == ';' ) {
					founded = 1;
					break;
				}
				mc = mc->next;
			    }

			    if( founded ) {
				if( founded == 2 ) {
				    m_c_pointer = mc;
				} else if( mc->next->type != END_TYPE )
				{
				   //found a ';'
				   //clear the opnd stack and prepare to the
				   //next part of statement

				    opnd_top = opndStackFund-1;
				    m_c_pointer = mc->next;
				} else {
				    m_c_pointer = &myEndType_MidCodeType;
				}
			    } else {
				//opnd_top = opndStackFund;
				m_c_pointer = &myEndType_MidCodeType;
			    }
			    break;
			}

			// a ; b ; c ; d
			// Initialize the calculate environment after a
			// is finished
			m_c_pointer = m_c_pointer->next;
			if( m_c_pointer->type != END_TYPE )
			{ //if it is not the end of this expression
			    for(i=opndStackFund;  i<opnd_top;  i++) {
				if( opnd[i].type == STRING_TYPE && \
					opnd[i].length >= MAX_OPND_LENGTH ) {
				    free((char *)*(long *)opnd[i].values);
				}
			    }
			    opnd_top = opndStackFund-1;
			}
			break;

		case 8:                 // function call
			if( askTdf != NULL ) {
				/* action function call */
/*			    if( ilength < 0 )
			    { //action expression*/
				i=(short)((ACTIONPFD)*(long *)svalues)(\
				  &opnd[opnd_top - ilength + 1], \
				  (short)ilength, &opnd_top, \
				  ASKACTIONCURRENTSTATEshort );
				if( i ) {
				   switch( i ) {
				     case 2:
					return  LONG_MAX-17;
				     default:
					success = 0;
					if( ErrorSet.xERROR == 0 )
					    ErrorSet.xERROR = iFailFunCall;
					break;
				  }
				}
/*			    } else
			    { //condition expression will be run only when
			      //the askTdf is not initial
			      //!!this is wrong, for the expression will use
			      //the result type
				if( *ASKACTIONCURRENTSTATEshort > 0 ) {
				  if( (short)((PFD)*(long *)svalues)(\
					&opnd[opnd_top - ilength + 1], \
					ilength, &opnd_top) ) {
					success = 0;
					ErrorSet.xERROR = iFailFunCall;
				  }
				}
			    }*/
			} else {
			    if( (short)((PFD)*(long *)svalues)(\
				  &opnd[opnd_top - ilength + 1], \
				  (short)ilength, &opnd_top) ) {
				success = 0;
				if( ErrorSet.xERROR == 0 )
				    ErrorSet.xERROR = iFailFunCall;
			    }
			}
			m_c_pointer = m_c_pointer->next;
			optr_top--;
			break;
		case 9: 		// '?' operator meet others
//			m_c_pointer = mcpQustnRem;
			qustn_g( &m_c_pointer );
			/*if( --optr_top < 0 ) {
			   ErrorSet.xERROR = iToLittOPr; //TOO LITTER OPTR IN GRAMMAR !!!
			   success = 0;
			}*/
			break;
		case 10: /*error operation should not meet for example [ meet )*/
		case 13: /*inside error*/
		case 99:
		default:
			success = 0;
			ErrorSet.xERROR = iOPrNoMatch ; // not matched
			strZcpy(ErrorSet.string, &szXexpOptrStr[itype*3], 4);
			break;
	    } //end of switch
	} //end of else
     } //end of while

     return  LONG_MAX;

} //end of CalExpr()


/*********
 *                      GetCurrentResult()
 **************************************************************************/
_declspec(dllexport) long PD_style GetCurrentResult( void )
{

    if( opnd_top != 0 ) return( LONG_MAX );

/* last operation is error */

    switch( opnd[ 0 ].type ) {
	    case STRING_TYPE:
		 if( opnd[ 0 ].length >= MAX_OPND_LENGTH )
					return( (long)_xEXP_result_mem );
		 return( (long)opnd[ 0 ].values );
	    case DATE_TYPE:
	    case FLOAT_TYPE:
		 return( (long)opnd[ 0 ].values );
	    case CHR_TYPE:
		 return( *(char *)opnd[ 0 ].values );
	    case INT_TYPE:
		 return( *(short *)opnd[ 0 ].values );
	    case LOGIC_TYPE:
		 return( *(short *)opnd[ 0 ].values );
	    case LONG_TYPE:
		 return( *(long *)opnd[ 0 ].values );
    }
    return( (long)opnd[ 0 ].values );    //type error not mentioned

} /* end of function GetCurrentResult() */



/*********
 *                      GetCurrentResult()
 **************************************************************************/
_declspec(dllexport) short PD_style GetCurrentResultType( void )
{
    if( opnd_top != 0 ) return  1;

    return  opnd[ 0 ].type;

} /* end of function GetCurrentResultType() */



/*********
 *                      GetUpdataFlag()
 **************************************************************************/
_declspec(dllexport) short PD_style GetUpdateFlag( void )
{

    return  updateFlag;

} /* end of function GetUpdataFlag() */



/*--------------
 !    ARGUMENTS : *BUFFER_POINTER ( a char string pointer )
 !                *M_C_POINTER    ( a struct pointer      )
 !    RETURN    :  a char string pointer
 !------------------------------------------------------------------------*/
static char * near PD_style iden_recognize(char *buffer_pointer, MidCodeType **mpp)
{
     short   	 i, type;
     char    	 *s;
     MidCodeType *mp;

     s = buffer_pointer;
     for( i = 1;   \
	  asc_isalnum[(unsigned)(*buffer_pointer)];
	  /*isalnum( *buffer_pointer ) || *buffer_pointer == '_';*/
	  buffer_pointer++, i++);

     if( *buffer_pointer == '.' ) {   /* identify the field name, look the */
				      /* identified as dbf name.           */
	buffer_pointer++;
	i++;
	//while( isalnum( *buffer_pointer ) || *buffer_pointer == '_' ) {
	while( asc_isalnum[(unsigned)(*buffer_pointer)] ) {
		i++;
		buffer_pointer++;
	}
	type = FIELD_IDEN;
     } else     type = IDEN_TYPE;

     //i++ for hold tail 0
     if( i > MAX_OPND_LENGTH ) //   The more char of buffer was cutted !
	i = MAX_OPND_LENGTH;

     if( (mp = (MidCodeType *)realloc(*mpp, sizeof(MidCodeType)-\
				MAX_ATOM_LENGTH+MAX_OPND_LENGTH) ) == NULL ) {
	return  NULL;
     }
     *mpp = mp;
     mp->type = type;

     strZcpy(mp->values, s, i);

     return  buffer_pointer;

} // end of iden_recoginze()



/*--------------
 !    ARGUMENTS : *BUFFER_POINTER ( a char string pointer )
 !                *M_C_POINTER    ( a struct pointer      )
 !    RETURN    :  a char string pointer
 !------------------------------------------------------------------------*/
static char * near PD_style SpecialFieldRecognize(char *buffer_pointer, MidCodeType **mpp)
{
     short i = 0;
     unsigned char  temp[dMAXDECLEN+1];
     MidCodeType *mp;

     if( (mp = (MidCodeType *)realloc(*mpp, sizeof(MidCodeType)-\
				MAX_ATOM_LENGTH+MAX_OPND_LENGTH) ) == NULL ) {
	return  NULL;
     }
     *mpp = mp;

     buffer_pointer++;				/* skip the @ mark */

     while( isdigit(*buffer_pointer)&&i<dMAXDECLEN )    temp[i++]  = *buffer_pointer++;
     temp[i] = '\0';

     mp->type = FIELD_IDEN;
     (*(SPECIALFIELDSTRUCT *)mp->values).Mark = '@';
     (*(SPECIALFIELDSTRUCT *)mp->values).IntervalChar = '\0';
     if( *buffer_pointer == '.' ) {
	   (*(SPECIALFIELDSTRUCT *)mp->values).LibNo = atoi( temp );
	   i = 0;
	   while( isdigit( *buffer_pointer ) ) temp[i++]  = *buffer_pointer++;
	   temp[i] = '\0';
	   (*(SPECIALFIELDSTRUCT *)mp->values).FieldNo = atoi( temp );
     } else {
	   (*(SPECIALFIELDSTRUCT *)mp->values).LibNo = 0;
	   (*(SPECIALFIELDSTRUCT *)mp->values).FieldNo = atoi( temp );
     }

     return  buffer_pointer;

} //end of iden_recoginze()


/*------------
 !    ARGUMENTS : *BUFFER_POINTER ( a char string pointer )
 !                *M_C_PINTER    ( a struct pointer )
 !    RETURN    :  a char string pointer
 !-----------------------------------------------------------------------*/
static char * near PD_style string_recognize(char *buffer_pointer, MidCodeType **mpp)
{
     short 	 i;
     char    	 *s;
     MidCodeType *mp;

     s = buffer_pointer;
     i = 1;     	//for hold tail '\0'
     while( *buffer_pointer != '"' ) {

	   if( *buffer_pointer == '\0' || *buffer_pointer == '\n' || \
						*buffer_pointer == '\r' ) {	//string not end
		ErrorSet.xERROR = iQuotoErr;
		strZcpy(ErrorSet.string, s, MAX_OPND_LENGTH);
		return  NULL;
	   }

	   if( *buffer_pointer == '\\' ) {
		buffer_pointer++;
		i++;			//if the char can't be escaped?
	   }
	   i++;
	   buffer_pointer++;
     }

     if( i > MAX_ATOM_LENGTH ) {
	if( (mp = (MidCodeType *)realloc(*mpp, sizeof(MidCodeType)-\
					MAX_ATOM_LENGTH+i) ) == NULL ) {
	    ErrorSet.xERROR = iNoMem;
	    strZcpy(ErrorSet.string, s, MAX_OPND_LENGTH);
	    return  NULL;
	}
	*mpp = mp;
     } else {
	mp = *mpp;
     }

     // skip the '"' and check
     if( *(buffer_pointer++) != '"' ) {
	mp->length = 0;
	ErrorSet.xERROR = iQuotoErr;
	strZcpy(ErrorSet.string, s, MAX_OPND_LENGTH);
	return  NULL;
     }

     cnStrToStr(mp->values, s, cEscapeXexp, i);
     mp->length = i-1;   		//not with tail 0
     mp->type = STRING_TYPE;

     return buffer_pointer;

} //end of string_recoginze()



/*-------------
 !    ARGUMENTS : *BUFFER_POINTER ( a char string pointer )
 !                *M_C_POINTER    ( a struct pointer )
 !    RETURN    :  a char string pointer
 !    DESCRIPTION: the digit should be described like:
 !                 [{+|-}][digits][ {.digits][{d|D|e|E}[sign]digits |
 !                      l|L } ]
 !------------------------------------------------------------------------*/
static char * near PD_style digit_recognize(char *buffer_pointer, MidCodeType *m_c_pointer)
{
     short   i=0;            /*  temp var          */
     unsigned char  temp[dMAXDECLEN+1];       /*  aid buffer block  */

     while( isdigit( *buffer_pointer ) )    temp[i++]  = *buffer_pointer++;

     switch( *buffer_pointer ) {
	case '.':
	case 'E':
	case 'e':
	case 'D':
	case 'd':
	   if( *buffer_pointer == '.' ) {
		temp[i++]  = *buffer_pointer++;               /*  skip '.' */
		while( isdigit( *buffer_pointer ) ) temp[i++]  = *buffer_pointer++;
	   }  else      goto DigitRecognize_JMP;
	   if( *buffer_pointer == 'e' || *buffer_pointer == 'E' || \
	       *buffer_pointer == 'd' || *buffer_pointer == 'D' ) {
DigitRecognize_JMP:
		temp[i++] = *buffer_pointer++;
		temp[i++] = *buffer_pointer++;
		while( isdigit( *buffer_pointer ) )
					temp[i++] = *buffer_pointer++;
	   }
	   temp[i] = '\0';
	   m_c_pointer->type = FLOAT_TYPE;
	   m_c_pointer->length = sizeof(double);
	   *(double *)m_c_pointer->values = atof( temp );
	   break;
	case 'T':
	case 't':
	   buffer_pointer++;
	   m_c_pointer->type = DATE_TYPE;
	   m_c_pointer->length = 8;
	   temp[i] = '\0';
	   strncpy(m_c_pointer->values, temp, 8);
	   break;
	case 'L':
	case 'l':
	   buffer_pointer++;
	default:
	   temp[ i ] = '\0';

	   if( strlen(temp) <= _MAX_LONG_NUM_WIDTH )
	   { //2147483647~-2147483648
	     //123456789A  123456789A

		m_c_pointer->type = LONG_TYPE;
		m_c_pointer->length = 4;
		*(long *)m_c_pointer->values = atol( temp );
	   } else {
		m_c_pointer->type = INT64_TYPE;
		m_c_pointer->length = 8;
		*(_int64 *)m_c_pointer->values = _atoi64( temp );
	   }

	   break;
     }

     return  buffer_pointer;

} // end of digit_recognize()


/*----------
 !    ARGUMENTS : *BUFFER_POINTER ( a char string pointer  )
 !                *M_C_PINTER     ( a struct pointer       )
 !    RETURN    :  a char string pointer
 !    DESCRIPTION :  NONE
 !    GLOBAL VARS     :
 !    GLOBAL FUNCTION :  FUNCTION()
 ------------------------------------------------------------------------*/
static char * near PD_style fun_recognize(char *buffer_pointer, MidCodeType *m_c_pointer)
{
     unsigned char  *temp;
     short          i;
     short          SquareBrackets, quoto, para, quoto1;

     buffer_pointer++;          /*   skip '('   */

     m_c_pointer->type = FUNB_TYPE;

     temp = buffer_pointer;
     i = quoto = quoto1 = 0;     //i remember the circle brackets nummber
     SquareBrackets = 0;
     m_c_pointer->length = 0;
     para = 0;

     while( *temp && i >= 0 )
	switch( *temp++ ) {
	    case '\\':  para = 1;
			temp++;     /* escape the character */
			break;

	    case '(':   para = 1;
			if( quoto == 0 && quoto1 == 0 )
				i++;
			break;

	    case ')':   if( quoto == 0 && quoto1 == 0 )
				i--;
			break;

	    case '[':   para = 1;
			if( quoto == 0 && quoto1 == 0 )
				SquareBrackets++;
			break;

	    case ']':   if( quoto == 0 && quoto1 == 0 )
				SquareBrackets--;
			break;

	    case '"':
			para = 1;
			if( quoto1 == 0 )
			{  //'"'
				quoto = (unsigned char)1 - quoto;
			}
			break;

	    case '\'':
			para = 1;
			if( quoto == 0 )
			{ // "string'string" 1999.3.25
			  //        |
			  //        +----- not single char
				quoto1 = (unsigned char)1 - quoto1;
			}
			break;

	    case ',':   if( para != 1 ) {
				ErrorSet.xERROR = iUnExpectEnd;
				return( NULL );
			}
			if( quoto == 0 && quoto1 == 0 && i == 0 && SquareBrackets == 0 )
				m_c_pointer->length++;
				/* para in brackets is one para */
			break;

	    case ';':   if( quoto == 0 && quoto1 == 0 )
			{ //
			  // fun(abc;def)
			  //
				ErrorSet.xERROR = iBadUseOfSemicolon;
				strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
				return( NULL );
			}
			break;

	    case ' ':
	    case '\t':  break;
	    default:    para = 1;
	} 				/* end of switch( *temp ) */

     if( *temp == '\0' && i >= 0 ) {
	ErrorSet.xERROR = iUnExpectEnd;                         // expression has no end
	return( NULL );
     }
     temp--;
     *temp = '\1';
     m_c_pointer->length += para;

     return  buffer_pointer;

} //end of fun_recognize()


/*----------
 !    ARGUMENTS  : *BUFFER_POINTER ( a char string pointer  )
 !                *M_C_PINTER     ( a struct pointer       )
 !    RETURN     : a char string pointer
 !    DESCRIPTION: recognize the array described in a string
 !    GLOBAL VARS:
 ------------------------------------------------------------------------*/
static char * near PD_style array_recognize( char *buffer_pointer, MidCodeType *m_c_pointer )
{
     unsigned char  *temp;
     short 	     i;
     short 	     SquareBrackets, quoto, quoto1, para;

     buffer_pointer++;          /*   skip '['   */
     m_c_pointer->type = ARRAYB_TYPE;

     temp = buffer_pointer;
     SquareBrackets = i = quoto = quoto1 = 0;
     m_c_pointer->length = 0;
     para = 0;

     while( *temp && SquareBrackets >= 0 )
	switch( *temp++ ) {
	    case '\\':  para = 1;
			temp++;        	     /* escape this character*/ break;
	    case '(':   para = 1;
			if( quoto == 0 )        i++;                    break;
	    case ')':   if( quoto == 0 )        i--;                    break;
	    case '[':   para = 1;
			if( quoto == 0 )        SquareBrackets++;       break;
	    case ']':   if( quoto == 0 )        SquareBrackets--;       break;

	    case '"':
			para = 1;
			if( quoto1 == 0 )
			{  //'"'
				quoto = (unsigned char)1 - quoto;
			}
			break;

	    case '\'':
			para = 1;
			if( quoto == 0 )
			{ // "string'string"
			  //        |
			  //        +----- not single char
				quoto1 = (unsigned char)1 - quoto1;
			}
			break;

	    case ',':   if( para != 1 ) {
				ErrorSet.xERROR = iUnExpectEnd;
				return( NULL );
			}
			if( quoto == 0 && quoto1 == 0 && i == 0 && SquareBrackets == 0 )
				m_c_pointer->length++;
				/* para in brackets is one para */
			break;

	    case ';':   if( quoto == 0 && quoto1 == 0 )
			{ //
			  // fun(abc;def)
			  //
				ErrorSet.xERROR = iBadUseOfSemicolon;
				strZcpy(ErrorSet.string, buffer_pointer, XES_LENGTH);
				return( NULL );
			}
			break;

	    case ' ':
	    case '\t':  break;
	    default:    para = 1;
	}
     if( SquareBrackets >= 0 &&  *temp == '\0' ) {
	ErrorSet.xERROR = iUnExpectEnd;         // expression has no end
	return( NULL );
     }
     m_c_pointer->length += para;

     return( buffer_pointer );

} /* end of array_recognize */


/*------------
 !    ARGUMENTS : opnd_top (  a opnd stack pointer )
 !    RETURN    :  a opnd stack pointer
 !    DESCRIPTION :  NONE
 !    GLOBAL VARS     :  OPND[]
 !    GLOBAL FUNCTION :  NONE
 ---------------------------------------------------------------------------*/
static short near PD_style not_g( void )
{
// this algorithm is for multi values login
#ifdef XLMultiValueLogin
    register short iTemp;

    iTemp = *(short *)ps;
    if ( dp->type == LOGIC_TYPE ) {
	if( iTemp <= 1 )
		return  *(short *)ps = 1 - iTemp;
	else {
		if( ( iTemp = iTemp * iTemp + 1 ) > 10000 ) {
			return *(short *)ps = 0;
		}
		return  *(short *)ps = iTemp;
	}
    }
    ErrorSet.xERROR = iOPrError;
    strcpy(ErrorSet.string, "!");
    return( -iOPrError );
#else
//2000.5.6 xGetOpndShort() first to check data type
    short  w;

    w = xGetOpndShort( dp );
    dp->type = LOGIC_TYPE;
    if( w != 0 ) {
	if( w == SHRT_MIN ) {
	    ErrorSet.xERROR = iOPrError;
	    strcpy(ErrorSet.string, "!");
	    return  -iOPrError;
	}
	return  *(short *)ps = 0;
    }
    return  *(short *)ps = 1;
/*
    if( dp->type == LOGIC_TYPE ) {
	if( *(short *)ps != 0 ) {
		return  *(short *)ps = 0;
	}
	return  *(short *)ps = 1;
    }
    ErrorSet.xERROR = iOPrError;
    strcpy(ErrorSet.string, "!");
    return( -iOPrError );
*/
#endif

} /* end of not_g */

/*-------------
 !    ARGUMENTS : opnd_top (  a opnd stack pointer )
 !    RETURN    :  a opnd stack pointer
 !    DESCRIPTION :  NONE
 !    GLOBAL VARS     :  OPND[]
 !    GLOBAL FUNCTION :  NONE
 !    BUILD DATA : 05/03/91
 !    MODIFIED   :
 !------------------------------------------------------------------------*/
static short near PD_style and_g( void )
{
    short  w1, w2;

    opnd_top--;

    w1 = xGetOpndShort( dp );
    w2 = xGetOpndShort( &opnd[opnd_top] );
    if( w1 == SHRT_MIN || w2 == SHRT_MIN ) {
        ErrorSet.xERROR = iOPrError;
        strcpy(ErrorSet.string, "!");
        return  -iOPrError;
    }

     opnd[opnd_top].type = LOGIC_TYPE;
     return  *(short *)(opnd[opnd_top].values) = w1 && w2;
			

/*2000.5.6
     opnd[opnd_top].type = LOGIC_TYPE;
     return  *(short *)(opnd[opnd_top].values) = \
			(*(short *)(opnd[opnd_top].values) && *(short *)ps);

     // dp now is opnd[opnd_top+1]  ps is opnd[opnd_top+1].values

     if( (dp->type==LOGIC_TYPE) && (opnd[opnd_top].type==LOGIC_TYPE) ) {
//		opnd[opnd_top].type = LOGIC_TYPE;

		return( *(short *)(opnd[opnd_top].values) *= *(short *)ps );

     } else {
	  ErrorSet.xERROR = iOPrError;
	  strcpy( ErrorSet.string, "&&" );
     };
     return( -iOPrError );
*/
} /* end of and_g */



/*----------
 !    ARGUMENTS : opnd_top (  a opnd stack pointer )
 !    RETURN    :  a opnd stack pointer
 !    DESCRIPTION :  NONE
 !    GLOBAL VARS     :  OPND[]
 !    GLOBAL FUNCTION :  NONE
 -------------------------------------------------------------------------*/
static short near PD_style or_g( void )
{
// this algorithm is for multi values login
#ifdef XLMultiValueLogin
     register short iTemp, jTemp, temp_int;
     OpndType *sdp;

     jTemp = *(short *)ps;
     iTemp = *(short *)(sdp = &opnd[--opnd_top])->values;
     if( (dp->type==LOGIC_TYPE) && (sdp->type==LOGIC_TYPE) ) {
//		sdp->type = LOGIC_TYPE;
		if( ( temp_int = iTemp + jTemp ) == 0 ) {
			return  *(short *)sdp->values = 0;
		}

		if( ( iTemp *= jTemp ) == 1 ) {
			return  *(short *)sdp->values = 1;
		}

		return *(short *)sdp->values = temp_int / (iTemp + 1);

     }
     ErrorSet.xERROR = iOPrError;
     strcpy( ErrorSet.string, "||");
     return( -iOPrError);
#else
    short  w1, w2;

    opnd_top--;

    w1 = xGetOpndShort( dp );
    w2 = xGetOpndShort( &opnd[opnd_top] );
    if( w1 == SHRT_MIN || w2 == SHRT_MIN ) {
        ErrorSet.xERROR = iOPrError;
        strcpy(ErrorSet.string, "!");
        return  -iOPrError;
    }

    opnd[opnd_top].type = LOGIC_TYPE;
    return  *(short *)(opnd[opnd_top].values) = w1 || w2;

/*2000.5.6
    opnd[opnd_top].type = LOGIC_TYPE;
    return  *(short *)(opnd[opnd_top].values) = \
			(*(short *)(opnd[opnd_top].values) || *(short *)ps);


     // dp now is opnd[opnd_top+1]  ps is opnd[opnd_top+1].values

     if( (dp->type==LOGIC_TYPE) && (opnd[opnd_top].type==LOGIC_TYPE) ) {
//		opnd[opnd_top].type = LOGIC_TYPE;

		//////////////
		if( (*(short *)(opnd[opnd_top].values)) || (*(short *)ps) ) {
			return  *(short *)(opnd[opnd_top].values) = 1;
		}
		return  *(short *)(opnd[opnd_top].values) = 0;
     } else {
	  ErrorSet.xERROR = iOPrError;
	  strcpy( ErrorSet.string, "||" );
     };
     return( -iOPrError );
 */
#endif

} /* end of or_g  */



/*---------------
 !    ARGUMENTS : opnd_top (  a opnd stack pointer )
 !    RETURN    : 1, bigger; -1, less; 0 equal, LESS than -1: ERROR
 !    DESCRIPTION : compare with calculate.
 !    GLOBAL VARS     :  OPND[]
 !------------------------------------------------------------------------*/
static short near PD_style xcomp( void )
{
     long 		temp_long, temp_long2;
     double 		temp_double, temp_double2;
     int   		change_type;       /* 0: long 1:double 2: string 3: date */
     int   		i;
     OpndType *opnd0;
     char  		buf1[4096], buf2[4096];
     dFIELDWHENACTION 	*p;

     opnd0 = &opnd[ --opnd_top ];
     change_type = 0;

     switch( dp->type ) {
		case STRING_TYPE:
			if( dp->oval != NULL ) {
				temp_long = (long)(dp->oval);
			} else {
				if( dp->length < MAX_OPND_LENGTH )
				    temp_long = (long)ps;
				else
				    temp_long = *(long *)ps;
			}
			change_type = 2;
			break;
		case CFIELD_IDEN:
		case LFIELD_IDEN:
			temp_long = *(long *)dp->oval;
			change_type = 2;
			break;
		case CHR_TYPE:
			temp_long = *(char *)ps;
			break;
		case INT_TYPE:
			temp_long = *(short *)ps;
			break;
		case LONG_TYPE:
			temp_long = *(long *)ps;
			break;
		case FLOAT_TYPE:
			temp_double = *(double *)ps;
			change_type = 1;
			break;
		case DATE_TYPE:
			temp_long = (long)ps;
			change_type = 3;
			break;
		case DFIELD_IDEN:
			temp_long = *(long *)dp->oval;
			change_type = 3;
			break;
		case NFIELD_IDEN:       /* number Field */
			temp_long = atol(subcopy((char *)*(long *)dp->oval, 0, dp->length));
			break;
		case FFIELD_IDEN:       /* float Field */
			temp_double = atof(subcopy((char *)*(long *)dp->oval, 0, \
									dp->length));
			change_type = 1;
			break;
		case MFIELD_IDEN:
			p = (dFIELDWHENACTION *)dp->oval;

			buf1[0] = '\x8';         	//8 blocks is need
			GetField(p->pSourceDfile, p->wSourceid, buf1);
			temp_long = (long)buf1;
			change_type = 2;
			break;
		default:
			ErrorSet.xERROR = iInVaildOpndType;         //* type error

			return  -iInVaildOpndType;
     }
     switch( opnd0->type ) {
	   case STRING_TYPE:

		if( opnd0->oval == NULL ) {
			if( opnd0->length < MAX_OPND_LENGTH )
				temp_long2 = (long)(opnd0->values);
			else	temp_long2 = *(long *)(opnd0->values);
		} else  temp_long2 = (long)(opnd0->oval);

/*		if( opnd0->length < MAX_OPND_LENGTH )
			(char *)temp_long2 = opnd0->values;
		else    temp_long2 = *(long *)opnd0->values;
*/		goto XCOMP_JMP3;
	   case CFIELD_IDEN:
	   case LFIELD_IDEN:
/*		if( opnd0->oval == NULL )	temp_long2 = *(long *)(opnd0->values);
		else			*/
		temp_long2 = *(long *)opnd0->oval;
		goto XCOMP_JMP3;
	   case MFIELD_IDEN:
		p = (dFIELDWHENACTION *)opnd0->oval;

		buf2[0] = '\x8';         	//8 blocks is need
		GetField(p->pSourceDfile, p->wSourceid, buf2);
		temp_long2 = (long)buf2;
XCOMP_JMP3:
		if( change_type == 2 ) {
		   goto  XCOMP_Q_JMP1;
		}

		//2001.7.10 Xilong Pei
		//support date compare with string
		if( change_type == 3 ) {
		   goto  XCOMP_Q_JMP1;
		}

		switch( change_type ) {
		   //case 2:	break;
		   case 0:      temp_long -= atol((char *)temp_long2);
				goto XCOMP_RET;
		   case 1:      temp_double -= atof((char *)temp_long2);
				goto XCOMP_RET;
		   default:
			ErrorSet.xERROR = iTypeNoCompt;         /* type not comparable */
			return  -iTypeNoCompt;
		}

XCOMP_Q_JMP1:

/* this the first version. chenge to the following for speed.
		i = 0;
		change_type = opnd0->length;
		while( i < change_type ) {
			if( *(char *)temp_long == *(char *)temp_long2  || \
				*(char *)temp_long == '*' || \
				*(char *)temp_long2 == '*' ) {
					i++;
					temp_long++;
					temp_long2++;
			} else 		break;
		}
*/

		change_type = opnd0->length;
		for(i = 0;  i < change_type;  i++ ) {

			register char   c1, c2;

			c1 = ((char *)temp_long)[i];
			c2 = ((char *)temp_long2)[i];

			if( ( c1 != c2  && c1 != '*' && c2 != '*') || \
				(c1 == '\0') || \
				(c2 == '\0') \
			) {
			    break;
			}
		}

		if( change_type /*opnd0->length*/ >= MAX_OPND_LENGTH && opnd0->type == STRING_TYPE )
		{
			if( opnd0->oval == NULL )
				free((char *)*(long *)(opnd0->values));
		}
		if( dp->length >= MAX_OPND_LENGTH && dp->type == STRING_TYPE )
		{
			if( dp->oval == NULL )
				free( (char *)*(long *)ps );
		}

		opnd0->type = LOGIC_TYPE;

		ps = opnd0->values;
		// change this data for the CalExpr will use it as a new one
		if( i >= change_type /*opnd0->length*/ || i >= dp->length )
			return( *(short *)ps = 0 );

		i = ((char *)temp_long2)[i] - ((char *)temp_long)[i];
		if( i < -1 )
		   return  	*(short *)ps	= -1;
		return  	*(short *)ps	= i;

	   case CHR_TYPE:
		temp_long2 = *(char *)opnd0->values;
		switch( change_type ) {
		   case 0:      temp_long -= temp_long2;      break;
		   case 1:      temp_double -= temp_long2;    break;
		   case 2:	change_type = 0;
				temp_long = atol((char *)temp_long) - temp_long2;
				break;
		   default:
			    ErrorSet.xERROR =iInVaildOpndType ;
			    return  -iInVaildOpndType;
		}
		break;
	   case INT_TYPE:
		temp_long2 = *(short *)opnd0->values;
		switch( change_type ) {
		   case 0:      temp_long -= temp_long2;    break;
		   case 1:      temp_double -= temp_long2;  break;
		   case 2:	change_type = 0;
				temp_long = atol((char *)temp_long) - temp_long2;
				break;
		   default:
				ErrorSet.xERROR = iInVaildOpndType;
				return  -iInVaildOpndType;
		}
		break;
	   case LONG_TYPE:
		temp_long2 = *(long *)opnd0->values;
		goto XCOMP_JMP1;
	   case NFIELD_IDEN:
		temp_long2 = atol( subcopy( (char *)*(long *)opnd0->oval, \
					   0, opnd0->length ) );
XCOMP_JMP1:
		switch( change_type ) {
		   case 0:      temp_long -= temp_long2;    break;
		   case 1:      temp_double -= temp_long2;  break;
		   case 2:	change_type = 0;
				temp_long = atol((char *)temp_long) - temp_long2;
				break;
		   default:     ErrorSet.xERROR = iInVaildOpndType;
				return  -iInVaildOpndType;
		}
		break;
	   case FLOAT_TYPE:
		temp_double2 = *(double *)opnd0->values;
		goto XCOMP_JMP2;
	   case FFIELD_IDEN:
		temp_double2 = atof( subcopy((char *)*(long *)opnd0->oval,\
				       0, opnd0->length ) );
XCOMP_JMP2:
		switch( change_type ) {
		   case 0:	temp_double = temp_long-temp_double2;	break;
		   case 1:	temp_double -= temp_double2;		break;
		   case 2:	temp_double = atof((char *)temp_long) - temp_double2;
				break;
		}
		change_type = 1;
		break;
	   case DATE_TYPE:
		switch( change_type ) {
		   case 3:
		   case 2:
			temp_long = DateMinusToLong( (char *)temp_long, \
						     opnd0->values );
			change_type = 0;
			break;
		   default:     ErrorSet.xERROR = iInVaildOpndType;             return( -iInVaildOpndType);
		}
		break;
	   case DFIELD_IDEN:
		switch( change_type ) {
		   case 3:

                   //2000.7.29
                   case 2:
			temp_long = DateMinusToLong( (char *)temp_long, \
					(char *)*(long *)opnd0->oval );
			change_type = 0;
			break;
		   default:     ErrorSet.xERROR = iInVaildOpndType;             return( -iInVaildOpndType);
		}
		break;

	   default:
		ErrorSet.xERROR = iInVaildOpndType;
		return  (-iInVaildOpndType);
     }

XCOMP_RET:
     opnd0->type = LOGIC_TYPE;
     ps = opnd0->values;
     // change this data for the CalExpr will use it as a new one
     switch( change_type ) {
	   case 0:
		if( temp_long > 0 )
			return( *(short *)ps = -1 );
		if( temp_long < 0 )
			return( *(short *)ps = 1 );
		return( *(short *)ps = 0 );
	   case 1:
		if( temp_double > FLOAT_PRECISION )
			return( *(short *)ps = -1 );
		if( temp_double < -FLOAT_PRECISION )
			return( *(short *)ps = 1 );
		return( *(short *)ps = 0 );
	   default:
		ErrorSet.xERROR = iInternalError ; /* inside error */
		return( -iInternalError);
     }

} /* end of xcomp */



/*---------------
 !    ARGUMENTS : opnd_top (  a opnd stack pointer )
 !    RETURN    : 1, bigger; -1, less; 0 equal
 !    DESCRIPTION : compare with calculate.
 !    GLOBAL VARS     :  OPND[]
 !------------------------------------------------------------------------*/
static short near PD_style absxcomp( void )
{
     long temp_long, temp_long2;
     double temp_double, temp_double2;
     int    change_type;       /* 0: long 1:double 2: string 3: date */
     int    i;
     OpndType *opnd0;

     opnd0 = &opnd[ --opnd_top ];
     change_type = 0;

     switch( dp->type ) {
		case STRING_TYPE:
			if( dp->oval != NULL ) {
				temp_long = (long)(dp->oval);
			} else {
				if( dp->length < MAX_OPND_LENGTH )
					temp_long = (long)ps;
				else
					temp_long = *(long *)ps;
			}
			change_type = 2;
			break;
		case CFIELD_IDEN:
			temp_long = *(long *)dp->oval;
			change_type = 2;
			break;
		case CHR_TYPE:
			temp_long = *(char *)ps;
			break;
		case INT_TYPE:
			temp_long = *(short *)ps;
			break;
		case LONG_TYPE:
			temp_long = *(long *)ps;
			break;
		case FLOAT_TYPE:
			temp_double = *(double *)ps;
			change_type = 1;
			break;
		case DATE_TYPE:
			temp_long = (long)ps;
			change_type = 3;
			break;
		case DFIELD_IDEN:
			temp_long = *(long *)dp->oval;
			change_type = 3;
			break;
		case NFIELD_IDEN:       /* number Field */
			temp_long = atol(subcopy((char *)*(long *)dp->oval, 0, dp->length));
			break;
		case FFIELD_IDEN:       /* float Field */
			temp_double = atof(subcopy((char *)*(long *)dp->oval, 0, \
									dp->length));
			change_type = 1;
			break;
		default:
			ErrorSet.xERROR = iInVaildOpndType;         //* type error

			return  -iInVaildOpndType;
     }
     switch( opnd0->type ) {
	   case STRING_TYPE:

		if( opnd0->oval == NULL ) {
			if( opnd0->length < MAX_OPND_LENGTH )
				temp_long2 = (long)(opnd0->values);
			else	temp_long2 = *(long *)(opnd0->values);
		} else  temp_long2 = (long)(opnd0->oval);

/*		if( opnd0->length < MAX_OPND_LENGTH )
			(char *)temp_long2 = opnd0->values;
		else    temp_long2 = *(long *)opnd0->values;
*/		goto XCOMP_JMP3;
	   case CFIELD_IDEN:
/*		if( opnd0->oval == NULL )	temp_long2 = *(long *)(opnd0->values);
		else			*/
		temp_long2 = *(long *)opnd0->oval;
XCOMP_JMP3:
		if( change_type == 2 ) {
		   goto  XCOMP_Q_JMP2;
		}

		switch( change_type ) {
		   //case 2:	break;
		   case 0:      temp_long -= atol((char *)temp_long2);
				goto XCOMP_RET;
		   case 1:      temp_double -= atof((char *)temp_long2);
				goto XCOMP_RET;
		   default:
			ErrorSet.xERROR = iTypeNoCompt;         /* type not comparable */
			return  -iTypeNoCompt;
		}

XCOMP_Q_JMP2:

/* this the first version. chenge to the following for speed.
		i = 0;
		change_type = opnd0->length;
		while( i < change_type ) {
			if( *(char *)temp_long == *(char *)temp_long2  || \
				*(char *)temp_long == '*' || \
				*(char *)temp_long2 == '*' ) {
					i++;
					temp_long++;
					temp_long2++;
			} else 		break;
		}
*/              change_type = opnd0->length;
		for(i = 0;  i < change_type;  i++ ) {

		    register char   c1, c2;

		    c1 = ((char *)temp_long)[i];
		    c2 = ((char *)temp_long2)[i];

		    if( c1 != c2 || c1 == '\0' || c2 == '\0' ) {
			break;
		    }
		}

		if( change_type /*opnd0->length*/ >= MAX_OPND_LENGTH && opnd0->type == STRING_TYPE )
		{
			if( opnd0->oval == NULL )
				free((char *)*(long *)(opnd0->values));
		}
		if( dp->length >= MAX_OPND_LENGTH && dp->type == STRING_TYPE )
		{
			if( dp->oval == NULL )
				free( (char *)*(long *)ps );
		}

		opnd0->type = LOGIC_TYPE;

		ps = opnd0->values;
		// change this data for the CalExpr will use it as a new one
		if( i >= change_type /*opnd0->length*/ && i >= dp->length )
			return( *(short *)ps = 0 );

		i = ((char *)temp_long2)[i] - ((char *)temp_long)[i];
		if( i < -1 )
		   return  	*(short *)ps	= -1;
		return  	*(short *)ps	= i;

	   case CHR_TYPE:
		temp_long2 = *(char *)opnd0->values;
		switch( change_type ) {
		   case 0:      temp_long -= temp_long2;      break;
		   case 1:      temp_double -= temp_long2;    break;
		   case 2:	change_type = 0;
				temp_long = atol((char *)temp_long) - temp_long2;
				break;
		   default:
			    ErrorSet.xERROR =iInVaildOpndType ;
			    return  -iInVaildOpndType;
		}
		break;
	   case INT_TYPE:
		temp_long2 = *(short *)opnd0->values;
		switch( change_type ) {
		   case 0:      temp_long -= temp_long2;    break;
		   case 1:      temp_double -= temp_long2;  break;
		   case 2:	change_type = 0;
				temp_long = atol((char *)temp_long) - temp_long2;
				break;
		   default:
				ErrorSet.xERROR = iInVaildOpndType;
				return  -iInVaildOpndType;
		}
		break;
	   case LONG_TYPE:
		temp_long2 = *(long *)opnd0->values;
		goto XCOMP_JMP1;
	   case NFIELD_IDEN:
		temp_long2 = atol( subcopy( (char *)*(long *)opnd0->oval, \
					   0, opnd0->length ) );
XCOMP_JMP1:
		switch( change_type ) {
		   case 0:      temp_long -= temp_long2;    break;
		   case 1:      temp_double -= temp_long2;  break;
		   case 2:	change_type = 0;
				temp_long = atol((char *)temp_long) - temp_long2;
				break;
		   default:     ErrorSet.xERROR = iInVaildOpndType;
				return  -iInVaildOpndType;
		}
		break;
	   case FLOAT_TYPE:
		temp_double2 = *(double *)opnd0->values;
		goto XCOMP_JMP2;
	   case FFIELD_IDEN:
		temp_double2 = atof( subcopy((char *)*(long *)opnd0->oval,\
				       0, opnd0->length ) );
XCOMP_JMP2:
		switch( change_type ) {
		   case 0:	temp_double = temp_long-temp_double2;	break;
		   case 1:	temp_double -= temp_double2;		break;
		   case 2:	temp_double = atof((char *)temp_long) - temp_double2;
				break;
		}
		change_type = 1;
		break;
	   case DATE_TYPE:
		switch( change_type ) {
		   case 3:
			temp_long = DateMinusToLong( (char *)temp_long, \
						     opnd0->values );
			change_type = 0;
			break;
		   default:     ErrorSet.xERROR = iInVaildOpndType;             return( -iInVaildOpndType);
		}
		break;
	   case DFIELD_IDEN:
		switch( change_type ) {
		   case 3:
			temp_long = DateMinusToLong( (char *)temp_long, \
					(char *)*(long *)opnd0->oval );
			change_type = 0;
			break;
		   default:     ErrorSet.xERROR = iInVaildOpndType;             return( -iInVaildOpndType);
		}
		break;
	   default:
		ErrorSet.xERROR = iInVaildOpndType;
		return  (-iInVaildOpndType);
     }

XCOMP_RET:
     opnd0->type = LOGIC_TYPE;
     ps = opnd0->values;
     // change this data for the CalExpr will use it as a new one
     switch( change_type ) {
	   case 0:
		if( temp_long > 0 )
			return( *(short *)ps = -1 );
		if( temp_long < 0 )
			return( *(short *)ps = 1 );
		return( *(short *)ps = 0 );
	   case 1:
		if( temp_double > FLOAT_PRECISION )
			return( *(short *)ps = -1 );
		if( temp_double < -FLOAT_PRECISION )
			return( *(short *)ps = 1 );
		return( *(short *)ps = 0 );
	   default:
		ErrorSet.xERROR = iInternalError ; /* inside error */
		return( -iInternalError);
     }

} /* end of xcomp */


/*-------------------------------------------------------------------------
 !              neg_g()
 !------------------------------------------------------------------------*/
static short near PD_style neg_g( MidCodeType *m_c_pointer )
{
    switch( m_c_pointer->type  ) {
	 case CHR_TYPE:
		*(char *)ps = - *(char *)m_c_pointer->values;
		dp->type = CHR_TYPE;
		break;
	 case INT_TYPE:
		*(short *)ps = - *(short *)m_c_pointer->values;
			 dp->type = INT_TYPE;
		break;
	 case LONG_TYPE:
		*(long *)ps = - *(long *)m_c_pointer->values;
		dp->type = LONG_TYPE;
		break;
	 case FLOAT_TYPE:
		*(double *)ps = - *(double *)m_c_pointer->values;
		dp->type = FLOAT_TYPE;
		break;
	 case CHR_IDEN:
		*(char *)ps = - *(char *)*(long *)m_c_pointer->values;
		dp->type = CHR_TYPE;
		break;
	 case INT_IDEN:
		*(short *)ps = - *(short *)*(long *)m_c_pointer->values;
			 dp->type = INT_TYPE;
		break;
	 case LONG_IDEN:
		*(long *)ps = - *(long *)*(long *)m_c_pointer->values;
		dp->type = LONG_TYPE;
		break;
	 case FLOAT_IDEN:
		*(double *)ps = - *(double *)*(long *)m_c_pointer->values;
		dp->type = FLOAT_TYPE;
		break;
	 case NFIELD_IDEN:
		*(long *)ps = - atol( subcopy( \
				(char *)*(long *)m_c_pointer->values, 0, \
				m_c_pointer->length ));
		dp->type = LONG_TYPE;
		break;
	 case FFIELD_IDEN:
		*(double *)ps = - atof(\
				subcopy( (char *)*(long *)m_c_pointer->values, 0, \
				m_c_pointer->length ) );
		dp->type = FLOAT_TYPE;
		break;
	 default:
		ErrorSet.xERROR = iOPrError;
		strcpy(ErrorSet.string, "-");
		return  -iOPrError;       /* varable cannot be neged */
    }

    return( 0 );  /* TRUE */

} /* end of neg_g */


/*-------------------------------------------------------------------------
 !              qustn_g()
 !------------------------------------------------------------------------*/
static short near PD_style qustn_g( MidCodeType **m_c_pointer )
{
    --opnd_top;

    //comment the LOGIC_TYPE check will allowed value data type to be
    //use as LOGIC_TYPE
    if( /*dp->type == LOGIC_TYPE && */*(short *)ps == 0 ) {
	// FALSE ? expr : expr	-> goto
	// a ? b : c
	// if c not exist, *m_c_pointer will be NULL
	// this will call the expression return the result of a: FALSE
	*m_c_pointer = (MidCodeType *)*(long *)((*m_c_pointer)->values);
	if( *m_c_pointer != NULL ) {
		if( (*m_c_pointer)->type == LABEL_TYPE && \
					     (*m_c_pointer)->length != ';' )
			*m_c_pointer = (*m_c_pointer)->next;
	} /*
	else {
		ErrorSet.xERROR = iBadExp;
		strcpy(ErrorSet.string, ":");
		return  -iBadExp;
	}*/
    } else {
	*m_c_pointer = (*m_c_pointer)->next;
    }

    return  0;

} //end of qustn_g()


/*------------------------------------------------------------------------
 !                      add_g
 ! sucess: return 0;
 !------------------------------------------------------------------------*/
static short near PD_style add_g( void )
{
     long   temp_long, temp_long2;
     double temp_double, temp_double2;
     _int64 temp_int64;
     
     int    change_type;         //0: long 1:double 2: string 3: date
     //short  change_type;         //0: long 1:double 2: string 3: date
				 //4: _int64
     OpndType *opnd0;

     opnd0 = &opnd[--opnd_top];
     change_type = 0;
     switch( dp->type ) {
		case CHR_TYPE:
			temp_long = *(char *)ps;
			break;
		case INT_TYPE:
			temp_long = *(short *)ps;
			break;
		case FLOAT_TYPE:
			temp_double = *(double *)ps;
			change_type = 1;
			break;
		case DATE_TYPE:
			temp_long = (long)ps;
			change_type = 3;
			break;
		case LONG_TYPE:
			temp_long = *(long *)ps;
			break;
		case DFIELD_IDEN:
			temp_long = *(long *)dp->oval;
			change_type = 3;
			break;
		case STRING_TYPE:
		case CFIELD_IDEN:
			change_type = 2;
			break;
		case NFIELD_IDEN:       /* number Field */
			if( dp->length <= _MAX_LONG_NUM_WIDTH )
			{
			    temp_long = atol(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			} else {
			    temp_int64 = _atoi64(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			    change_type = 4;
			}
			break;
		case FFIELD_IDEN:       /* float Field */
			temp_double = atof(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			change_type = 1;
			break;

		case INT64_TYPE:
			change_type = 4;
			temp_int64 = *(_int64 *)ps;
			break;

		default:
			ErrorSet.xERROR = iBadUseOPr;         //type error
			return  -iBadUseOPr;
     }
     switch( opnd0->type ) {
	   case CHR_TYPE:
		temp_long2 = *(char *)opnd0->values;
		switch( change_type ) {
		   case 0:  *(long *)opnd0->values = temp_long + temp_long2;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
						   temp_double + temp_long2;
			    __SetOpnd0FloatType;
			    break;
		   case 3:  DateAddToLong((char *)temp_long, \
					temp_long2, opnd0->values);
			    __SetOpnd0DateType;
			    break;
		   case 4:  *(_int64 *)opnd0->values = \
						   temp_int64 + temp_long2;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			     ErrorSet.xERROR = iInVaildOpndType;     return  -10004;
		}
		break;
	   case INT_TYPE:
		temp_long2 = *(short *)opnd0->values;
		switch( change_type ) {
		   case 0:  *(long *)opnd0->values = temp_long + temp_long2;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
						    temp_double + temp_long2;
			    __SetOpnd0FloatType;
			    break;
		   case 3:  DateAddToLong((char *)temp_long, \
					temp_long2, opnd0->values);
			    __SetOpnd0DateType;
			    break;
		   case 4:  *(_int64 *)opnd0->values = \
						   temp_int64 + temp_long2;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			    ErrorSet.xERROR = iInVaildOpndType;                 return( -10004 );
		}
		break;
	   case LONG_TYPE:
		temp_long2 = *(long *)opnd0->values;
		goto ADD_JMP1;
	   case NFIELD_IDEN:
		temp_long2 = atol( subcopy( (char *)*(long *)opnd0->oval, \
					  0, opnd0->length ) );
ADD_JMP1:
		switch( change_type ) {
		   case 0:  *(long *)opnd0->values = temp_long + temp_long2;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
						   temp_double + temp_long2;
			    __SetOpnd0FloatType;
			    break;
		   case 3:  DateAddToLong((char *)temp_long, \
					temp_long2, opnd0->values);
			    __SetOpnd0DateType;
			    break;
		   case 4:  *(_int64 *)opnd0->values = \
						   temp_int64 + temp_long2;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			      ErrorSet.xERROR = iInVaildOpndType;      return -10004;
		}
		break;
	   case FLOAT_TYPE:
		temp_double2 = *(double *)opnd0->values;
		goto ADD_JMP2;
	   case FFIELD_IDEN:
		temp_double2 = atof( subcopy((char *)*(long *)(opnd0->oval), 0, \
						   opnd0->length ) );
ADD_JMP2:
		__SetOpnd0FloatType;
		if( change_type == 0 )       temp_double = temp_long;
		*(double *)opnd0->values = temp_double + temp_double2;
		break;
	   case DATE_TYPE:
		switch( change_type ) {
		   case 0:  DateAddToLong(opnd0->values, \
					   temp_long, opnd0->values);
			    __SetOpnd0DateType;
			    break;
		   default:
			    ErrorSet.xERROR = iInVaildOpndType;       return -10004;
		}
		break;
	   case DFIELD_IDEN:
		switch( change_type ) {
		   case 0:  DateAddToLong(subcopy((char *)*(long *)opnd0->oval, 0, 8),\
					   temp_long, opnd0->values);
			    __SetOpnd0DateType;
			    break;
		   default:
			     ErrorSet.xERROR = iInVaildOpndType;                 return( -10004 );
		}
		break;
	   case STRING_TYPE:
	   case CFIELD_IDEN:
	   {
		char *sp;

		if( change_type != 2 ) {
			ErrorSet.xERROR = iBadUseOPr;  //type not comparable
			return -10004;
		}
		change_type = dp->length + opnd0->length;

		//deal the result memory
		if( change_type >= MAX_OPND_LENGTH ) {

		    sp = malloc(change_type+1);

		    if( sp == NULL ) {
			ErrorSet.xERROR = iNoMem;  //Mem error
			return( -10004 );
		    }
		    if( opnd0->type == CFIELD_IDEN ) {
			strZcpy(sp, (char *)*(long *)(opnd0->oval), opnd0->length+1);
		    } else {
			if( opnd0->oval == NULL ) {
			    if( opnd0->length >= MAX_OPND_LENGTH ) {
				strZcpy(sp, (char *)*(long *)(opnd0->values), opnd0->length+1);
				free((char *)*(long *)(opnd0->values));
			    } else  strZcpy(sp, opnd0->values, opnd0->length+1);
			} else {
			    strZcpy(sp, opnd0->oval, opnd0->length+1);
			}
		    }
		    *(long *)(opnd0->values) = (long)sp;
		} else
		{ //Now, It is sure that  opnd0->length < MAX_OPND_LENGTH
		    sp = opnd0->values;
		    if( opnd0->type == CFIELD_IDEN ) {
			strZcpy(sp, (char *)*(long *)(opnd0->oval), opnd0->length+1);
		    } else {
			if( opnd0->oval != NULL ) {
			    strZcpy(sp, opnd0->oval, opnd0->length+1);
			} else {
			    strZcpy(sp, opnd0->values, opnd0->length+1);
			}
		    }
		}

		opnd0->length = change_type;
		opnd0->oval   = NULL;

		if( dp->type == CFIELD_IDEN ) {
		    strncat(sp, (char *)*(long *)(dp->oval), dp->length);
		} else {
		    if( dp->oval == NULL ) {
			if( dp->length >= MAX_OPND_LENGTH ) {
				strncat(sp, (char *)*(long *)ps, dp->length);
				free( (char *)*(long *)ps );
			} else  strncat(sp, ps, dp->length);
		    } else {
			strncat(sp, dp->oval, dp->length);
		    }
		}
		opnd0->type = STRING_TYPE;
	   }
		break;
	   default:
		ErrorSet.xERROR = iInVaildOpndType;
		return( -10003 );
     }

     dp->oval = NULL;
     return  0;

} /* end of add_g() */


/*------------------------------------------------------------------------
 !                      minus_g
 ! sucess: return 0;
 !------------------------------------------------------------------------*/
static short near PD_style minus_g( void )
{
     long   temp_long, temp_long2;
     double temp_double, temp_double2;
     _int64 temp_int64;
     short change_type;         // 0: long 1:double 2: string.error! 3: date
				// 4: _int64
     OpndType *opnd0;

     opnd0 = &opnd[--opnd_top];
     change_type = 0;
     switch( dp->type ) {
		case CHR_TYPE:
			temp_long = *(char *)ps;
			break;
		case INT_TYPE:
			temp_long = *(short *)ps;
			break;
		case LONG_TYPE:
			temp_long = *(long *)ps;
			break;
		case FLOAT_TYPE:
			temp_double = *(double *)ps;
			change_type = 1;
			break;
		case DATE_TYPE:
			temp_long = (long)ps;
			change_type = 3;
			break;
		case DFIELD_IDEN:
			temp_long = *(long *)dp->oval;
			change_type = 3;
			break;
		case NFIELD_IDEN:       /* number Field */
			/*temp_long = atol(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));*/
			if( dp->length <= _MAX_LONG_NUM_WIDTH )
			{
			    temp_long = atol(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			} else {
			    temp_int64 = _atoi64(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			    change_type = 4;
			}
			break;

		case FFIELD_IDEN:       /* float Field */
			temp_double = atof(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			change_type = 1;
			break;

		case INT64_TYPE:
			change_type = 4;
			temp_int64 = *(_int64 *)ps;
			break;

		default:
			ErrorSet.xERROR = iBadUseOPr;         //* type error
			return( -10003 );
     }

     switch( opnd0->type ) {
	   case CHR_TYPE:
		temp_long2 = *(char *)opnd0->values;
		switch( change_type ) {
		   case 0:  *(long *)opnd0->values = \
						temp_long2 - temp_long;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
						temp_long2 - temp_double;
			    __SetOpnd0FloatType;
			    break;
		   case 4:  *(_int64 *)opnd0->values = \
						temp_long2 - temp_int64;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			    ErrorSet.xERROR = iInVaildOpndType;     return( -10004 );
		}
		break;
	   case INT_TYPE:
		temp_long2 = *(short *)opnd0->values;
		switch( change_type ) {
		   case 0:  *(long *)opnd0->values = \
					       temp_long2 - temp_long;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
					       temp_long2 - temp_double;
			    __SetOpnd0FloatType;
			    break;
		   case 4:  *(_int64 *)opnd0->values = \
						temp_long2 - temp_int64;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			     ErrorSet.xERROR = iInVaildOpndType;             return( -10004 );
		}
		break;
	   case LONG_TYPE:
		temp_long2 = *(long *)opnd0->values;
		goto MINUS_JMP1;
	   case NFIELD_IDEN:
		temp_long2 = atol( subcopy( (char *)*(long *)opnd0->oval, \
					  0, opnd0->length ) );
MINUS_JMP1:
		switch( change_type ) {
		   case 0:  *(long *)opnd0->values = \
					      temp_long2 - temp_long;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
					      temp_long2 - temp_double;
			    __SetOpnd0FloatType;
			    break;
		   case 4:  *(_int64 *)opnd0->values = \
						temp_long2 - temp_int64;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			    ErrorSet.xERROR = iInVaildOpndType;             return( -10004 );
		}
		break;
	   case FLOAT_TYPE:
		temp_double2 = *(double *)opnd0->values;
		goto MINUS_JMP2;
	   case FFIELD_IDEN:
		temp_double2 = atof( subcopy((char *)*(long *)opnd0->oval,\
				       0, opnd0->length ) );
MINUS_JMP2:
		if( change_type == 0 )       temp_double = temp_long;
		*(double *)opnd0->values = temp_double2 - temp_double;
		__SetOpnd0FloatType;
		break;
	   case DATE_TYPE:
		switch( change_type ) {
		   case 0:  DateAddToLong(opnd0->values, \
					   -temp_long, opnd0->values);
			    __SetOpnd0DateType;
			    break;
		   case 3:  *(long *)opnd0->values = \
					   DateMinusToLong(opnd0->values, \
					   (char *)temp_long);
			    __SetOpnd0LongType;
			    break;
		   default:
			    ErrorSet.xERROR = iInVaildOpndType;                 return( -10004 );
		}
		break;
	   case DFIELD_IDEN:
		switch( change_type ) {
		   case 0:  DateAddToLong((char *)*(long *)opnd0->oval, \
					   -temp_long, opnd0->values);
			    __SetOpnd0DateType;
			    break;
		   case 3:  *(long *)opnd0->values = \
				DateMinusToLong((char *)*(long *)opnd0->oval, \
					   (char *)temp_long);
			    __SetOpnd0LongType;
			    break;
		   default:
			    ErrorSet.xERROR = iInVaildOpndType;                 return( -10004 );
		}
		break;
	   default:
		ErrorSet.xERROR = iInVaildOpndType;
		return( -10003 );
     }

     return  0;

} /* end of minus_g() */


/*------------------------------------------------------------------------
 !                      multiple_g
 ! sucess: return 0;
 !------------------------------------------------------------------------*/
static short near PD_style multiple_g( void )
{
     long temp_long, temp_long2;
     double temp_double, temp_double2;
     _int64 temp_int64;
     short  change_type;         //0: long 1:double 2: string 3: date
				 //4: _int64
     OpndType *opnd0;

     opnd0 = &opnd[--opnd_top];
     change_type = 0;
     switch( dp->type ) {
		case CHR_TYPE:
			temp_long = *(char *)ps;
			break;
		case INT_TYPE:
			temp_long = *(short *)ps;
			break;
		case LONG_TYPE:
			temp_long = *(long *)ps;
			break;
		case FLOAT_TYPE:
			temp_double = *(double *)ps;
			change_type = 1;
			break;
		case NFIELD_IDEN:       /* number Field */
			/*temp_long = atol(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			*/
			if( dp->length <= _MAX_LONG_NUM_WIDTH )
			{
			    temp_long = atol(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			} else {
			    temp_int64 = _atoi64(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			    change_type = 4;
			}
			break;
		case FFIELD_IDEN:       /* float Field */
			change_type = 1;
			temp_double = atof(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			break;

		case INT64_TYPE:
			change_type = 4;
			temp_int64 = *(_int64 *)ps;
			break;

		default:
			ErrorSet.xERROR = iBadUseOPr;         //* type error
			return( -10003 );
     }
     switch( opnd0->type ) {
	   case CHR_TYPE:
		temp_long2 = *(char *)opnd0->values;
		switch( change_type ) {
		   case 0:  *(long *)opnd0->values = \
					      temp_long * temp_long2;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
						temp_double * temp_long2;
			    __SetOpnd0FloatType;
			    break;

		   case 4:  *(_int64 *)opnd0->values = \
						temp_int64 * temp_long2;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			    ErrorSet.xERROR = iInVaildOpndType;     return( -10004 );
		}
		break;
	   case INT_TYPE:
		temp_long2 = *(short *)opnd0->values;
		switch( change_type ) {
		   case 0:  *(long *)opnd0->values = \
					      temp_long * temp_long2;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
					      temp_double * temp_long2;
			    __SetOpnd0FloatType;
			    break;
		   case 4:  *(_int64 *)opnd0->values = \
						temp_int64 * temp_long2;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			    ErrorSet.xERROR = iInVaildOpndType;             return( -10004 );
		}
		break;
	   case LONG_TYPE:
		temp_long2 = *(long *)opnd0->values;
		goto MULTIPLE_JMP1;
	   case NFIELD_IDEN:
		temp_long2 = atol( subcopy( (char *)*(long *)opnd0->oval, \
					  0, opnd0->length ) );
MULTIPLE_JMP1:
		switch( change_type ) {
		   case 0:  *(long *)opnd0->values = \
					      temp_long * temp_long2;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
						     temp_double * temp_long2;
			    __SetOpnd0FloatType;
			    break;
		   case 4:  *(_int64 *)opnd0->values = \
						temp_int64 * temp_long2;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			    ErrorSet.xERROR = iInVaildOpndType;             return( -10004 );
		}
		break;
	   case FLOAT_TYPE:
		temp_double2 = *(double *)opnd0->values;
		goto MULTIPLE_JMP2;
	   case FFIELD_IDEN:
		temp_double2 = atof( subcopy((char *)*(long *)opnd0->oval,\
					      0, opnd0->length ) );
MULTIPLE_JMP2:
		if( change_type == 0 )       temp_double = temp_long;
		*(double *)opnd0->values = \
					     temp_double * temp_double2;
		__SetOpnd0FloatType;
		break;
	   default:
		ErrorSet.xERROR = iInVaildOpndType;
		return( -10003 );
     }

     return  0;

} /* end of multiple_g */


/*------------------------------------------------------------------------
 !                      divide_g
 ! sucess: return 0;
 !------------------------------------------------------------------------*/
static short near PD_style divide_g( void )
{
     long temp_long, temp_long2;
     double temp_double, temp_double2;
     _int64 temp_int64;
     short  change_type;         //0: long 1:double 2: string 3: date
				 //4: _int64
     OpndType *opnd0;

     opnd0 = &opnd[--opnd_top];
     change_type = 0;
     switch( dp->type ) {
		case CHR_TYPE:
			temp_long = *(char *)ps;
			break;
		case INT_TYPE:
			temp_long = *(short *)ps;
			break;
		case LONG_TYPE:
			temp_long = *(long *)ps;
			break;
		case FLOAT_TYPE:
			temp_double = *(double *)ps;
			change_type = 1;
			break;
		case NFIELD_IDEN:       /* number Field */
			/*temp_long = atol(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			*/
			if( dp->length <= _MAX_LONG_NUM_WIDTH )
			{
			    temp_long = atol(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			} else {
			    temp_int64 = _atoi64(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			    change_type = 4;
			}
			break;

		case FFIELD_IDEN:       /* float Field */
			change_type = 1;
			temp_double = atof(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			break;

		case INT64_TYPE:
			change_type = 4;
			temp_int64 = *(_int64 *)ps;
			break;

		default:
			ErrorSet.xERROR = iBadUseOPr;         //* type error
			return  -iBadUseOPr;
     }
     /*change this design in Nanchang Xilong 1995.09.23
     if( ( change_type == 0 && temp_long == 0 ) || \
		( change_type == 2 && temp_double == 0.0 ) ) {
	ErrorSet.xERROR = iMathError;                 // divide by zero
	ErrorSet.xchar = '/';
	return( -10001 );
     }*/
     switch( opnd0->type ) {
	   case CHR_TYPE:
		temp_long2 = *(char *)opnd0->values;
		switch( change_type ) {
		   case 0:
			   if( temp_long == 0 ) {
				*(long *)opnd0->values = 0;
				__SetOpnd0LongType;
				break;
			   }
			   *(long *)opnd0->values = \
						temp_long2 / temp_long;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
						temp_long2 / (temp_double+FLOAT_PRECISION);
			    __SetOpnd0FloatType;
			    break;
		   case 4:  *(_int64 *)opnd0->values = \
						temp_long2 / temp_int64;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			    ErrorSet.xERROR = iInVaildOpndType;     return( -10004 );
		}
		break;
	   case INT_TYPE:
		temp_long2 = *(short *)opnd0->values;
		switch( change_type ) {
		   case 0:
			    if( temp_long == 0 ) {
				*(long *)opnd0->values = 0;
				__SetOpnd0LongType;
				break;
			    }
			    *(long *)opnd0->values = \
					      temp_long2 / temp_long;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
					      temp_long2 / (temp_double+FLOAT_PRECISION);
			    __SetOpnd0FloatType;
			    break;
		   case 4:  *(_int64 *)opnd0->values = \
						temp_long2 / temp_int64;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			    ErrorSet.xERROR = iInVaildOpndType;             return( -10004 );
		}
		break;
	   case LONG_TYPE:
		temp_long2 = *(long *)opnd0->values;
		goto DIVIDE_JMP1;
	   case NFIELD_IDEN:
		temp_long2 = atol( subcopy( (char *)*(long *)opnd0->oval, 0, \
					opnd0->length ) );
DIVIDE_JMP1:
		switch( change_type ) {
		   case 0:
			    if( temp_long == 0 ) {
				*(long *)opnd0->values = 0;
				__SetOpnd0LongType;
				break;
			    }
			    *(long *)opnd0->values = \
					      temp_long2 / temp_long;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
					      temp_long2 / (temp_double+FLOAT_PRECISION);
			    __SetOpnd0FloatType;
			    break;
		   case 4:  *(_int64 *)opnd0->values = \
						temp_long2 / temp_int64;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			     ErrorSet.xERROR = iInVaildOpndType;             return( -10004 );
		}
		break;
	   case FLOAT_TYPE:
		temp_double2 = *(double *)opnd0->values;
		goto DIVIDE_JMP2;
	   case FFIELD_IDEN:
		temp_double2 = atof( subcopy((char *)*(long *)opnd0->oval,\
					      0, opnd0->length ) );
DIVIDE_JMP2:
		if( change_type == 0 )
			temp_double = temp_long;

		*(double *)opnd0->values =  temp_double2/(temp_double+FLOAT_PRECISION);
		__SetOpnd0FloatType;
		break;
	   default:
		ErrorSet.xERROR = iInVaildOpndType;
		return  -iInVaildOpndType;
     }

     return  0;

} /* end of divide_g() */



/*------------------------------------------------------------------------
 !                      mod_g
 ! sucess: return 0;
 !------------------------------------------------------------------------*/
static short near PD_style mod_g( void )
{
     long   temp_long, temp_long2;
     double temp_double, temp_double2;
     _int64 temp_int64;
     short  change_type;         //0: long 1:double 2: string 3: date
				 //4: _int64
     OpndType *opnd0;

     opnd0 = &opnd[--opnd_top];
     change_type = 0;
     switch( dp->type ) {
		case CHR_TYPE:
			temp_long = *(char *)ps;
			break;
		case INT_TYPE:
			temp_long = *(short *)ps;
			break;
		case LONG_TYPE:
			temp_long = *(long *)ps;
			break;
		case FLOAT_TYPE:
			temp_double = *(double *)ps;
			change_type = 1;
			break;
		case NFIELD_IDEN:       /* number Field */
			/*temp_long = atol(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			*/
			if( dp->length <= _MAX_LONG_NUM_WIDTH )
			{
			    temp_long = atol(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			} else {
			    temp_int64 = _atoi64(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			    change_type = 4;
			}
			break;
		case FFIELD_IDEN:       /* float Field */
			change_type = 1;
			temp_double = atof(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			break;

		case INT64_TYPE:
			change_type = 4;
			temp_int64 = *(_int64 *)ps;
			break;

		default:
			ErrorSet.xERROR = iBadUseOPr;         //* type error
			return  -iBadUseOPr;
     }
     /*change this design in Nanchang Xilong 1995.09.23
     if( ( change_type == 0 && temp_long == 0 ) || \
		( change_type == 2 && temp_double == 0.0 ) ) {
	ErrorSet.xERROR = iMathError;                 // divide by zero
	ErrorSet.xchar = '/';
	return( -10001 );
     }*/
     switch( opnd0->type ) {
	   case CHR_TYPE:
		temp_long2 = *(char *)opnd0->values;
		switch( change_type ) {
		   case 0:
			   if( temp_long == 0 ) {
				*(long *)opnd0->values = 0;
				__SetOpnd0LongType;
				break;
			   }
			   *(long *)opnd0->values = \
					      temp_long2 % temp_long;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
					      temp_long2 % ((long)(temp_double+FLOAT_PRECISION));
			    __SetOpnd0FloatType;
			    break;
		   case 4:  *(_int64 *)opnd0->values = \
						temp_long2 % temp_int64;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			    ErrorSet.xERROR = iInVaildOpndType;     return( -10004 );
		}
		break;
	   case INT_TYPE:
		temp_long2 = *(short *)opnd0->values;
		switch( change_type ) {
		   case 0:
			    if( temp_long == 0 ) {
				*(long *)opnd0->values = 0;
				__SetOpnd0LongType;
				break;
			    }
			    *(long *)opnd0->values = \
					      temp_long2 % temp_long;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
					      temp_long2 % ((long)(temp_double+FLOAT_PRECISION));
			    __SetOpnd0FloatType;
			    break;
		   case 4:  *(_int64 *)opnd0->values = \
						temp_long2 % temp_int64;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			    ErrorSet.xERROR = iInVaildOpndType;             return( -10004 );
		}
		break;
	   case LONG_TYPE:
		temp_long2 = *(long *)opnd0->values;
		goto DIVIDE_JMP1;
	   case NFIELD_IDEN:
		temp_long2 = atol( subcopy( (char *)*(long *)opnd0->oval, 0, \
					opnd0->length ) );
DIVIDE_JMP1:
		switch( change_type ) {
		   case 0:
			    if( temp_long == 0 ) {
				*(long *)opnd0->values = 0;
				__SetOpnd0LongType;
				break;
			    }
			    *(long *)opnd0->values = \
					      temp_long2 % temp_long;
			    __SetOpnd0LongType;
			    break;
		   case 1:  *(double *)opnd0->values = \
					      temp_long2 % ((long)(temp_double+FLOAT_PRECISION));
			    __SetOpnd0FloatType;
			    break;
		   case 4:  *(_int64 *)opnd0->values = \
						temp_long2 % temp_int64;
			    __SetOpnd0Int64Type;
			    break;
		   default:
			     ErrorSet.xERROR = iInVaildOpndType;             return( -10004 );
		}
		break;
	   case FLOAT_TYPE:
		temp_double2 = *(double *)opnd0->values;
		goto DIVIDE_JMP2;
	   case FFIELD_IDEN:
		temp_double2 = atof( subcopy((char *)*(long *)opnd0->oval,\
					      0, opnd0->length ) );
DIVIDE_JMP2:
		if( change_type == 0 )       temp_double = temp_long;
		*(double *)opnd0->values = (double)((long)temp_double2 % (long)((temp_double+FLOAT_PRECISION)));
		__SetOpnd0FloatType;
		break;
	   default:
		ErrorSet.xERROR = iInVaildOpndType;
		return  -iInVaildOpndType;
     }

     return  0;

} /* end of mod_g */



/*------------------------------------------------------------------------
 !                      store_g
 ! sucess: return 0;
 ! store_g() is an action function, insted
 !------------------------------------------------------------------------*/
static short near PD_style store_g( void )
{
    dFIELDWHENACTION *p, *p1;
    long 	l = 0;
    double	f;

    if( askTdf != NULL && ( *ASKACTIONCURRENTSTATEshort == 0 || \
		     *ASKACTIONCURRENTSTATEshort == LASTWORKTIMEOFACTION ) ) {
	opnd_top--;
	return  0;
    }

    updateFlag = 1;
    switch( opnd[ --opnd_top ].type ) {

#ifdef XEXP_ACTION_SERVICE

	/* only the action expression can have store_g appeared */
	case NFIELD_IDEN:       /* 1057 */
	{
	    switch( dp->type ) {
		case CHR_TYPE:
			l = (long)*(char *)ps;
			break;
		case INT_TYPE:
			l = (long)*(short *)ps;
			break;
		case LONG_TYPE:
			l = *(long *)ps;
			break;
		case FLOAT_TYPE:
			l = (long)*(double *)ps;
			break;
		case NFIELD_IDEN:       /* 1057 */
		case LFIELD_IDEN:       /* 1059 */
		{
			char buf[256];

			p1 = (dFIELDWHENACTION *)(dp->oval);
			get_fld(p1->pSourceDfile, p1->wSourceid, buf);
			l = atol( buf );
			break;
		}
		case FFIELD_IDEN:       /* 1058 */
		case SFIELD_IDEN:       /* 1063 */
		{
			char buf[256];

			p1 = (dFIELDWHENACTION *)(dp->oval);
			get_fld(p1->pSourceDfile, p1->wSourceid, buf);
			l = (long)atof( buf );
			break;
		}
	      /*case DFIELD_IDEN:       // 1060
		case MFIELD_IDEN:       // 1061
		case CFIELD_IDEN:       // 1062
		*/
		default:
			ErrorSet.xERROR = iBadUseOPr;         /* type error */
			strcpy( ErrorSet.string,":=");

			return( -10003 );
	    }
	    p = (dFIELDWHENACTION *)opnd[ opnd_top ].oval;
	    if( p->pTargetDfile == NULL ) {
		ErrorSet.xERROR = iNoTargetField;
		strcpy(ErrorSet.string, ":=");
		return  -1;
	    }
	    PutField(p->pTargetDfile, p->wTargetid, &l);
	}
	break;
	case FFIELD_IDEN:       /* 1058 */
	{
	    switch( dp->type ) {
		case CHR_TYPE:
			f = *(char *)ps;
			break;
		case INT_TYPE:
			f = *(short *)ps;
			break;
		case LONG_TYPE:
			f = *(long *)ps;
			break;
		case FLOAT_TYPE:
			f = *(double *)ps;
			break;
		case NFIELD_IDEN:       /* 1057 */
		case LFIELD_IDEN:       /* 1059 */
		{
			char buf[256];

			p1 = (dFIELDWHENACTION *)(dp->oval);
			get_fld(p1->pSourceDfile, p1->wSourceid, buf);
			f = (double)atol( buf );
			break;
		}
		case FFIELD_IDEN:       /* 1058 */
		case SFIELD_IDEN:       /* 1063 */
		{
			char buf[256];

			p1 = (dFIELDWHENACTION *)(dp->oval);
			get_fld(p1->pSourceDfile, p1->wSourceid, buf);
			f = atof( buf );
			break;
		}
	      /*case DFIELD_IDEN:       // 1060
		case MFIELD_IDEN:       // 1061
		case CFIELD_IDEN:       // 1062
		*/
		default:
			ErrorSet.xERROR = iBadUseOPr;         /* type error */
			strcpy( ErrorSet.string,":=");

			return( -10003 );
	    }
	    p = (dFIELDWHENACTION *)opnd[ opnd_top ].oval;
		if( p->pTargetDfile == NULL ) {
			return  -1;
	    }
	    PutField(p->pTargetDfile, p->wTargetid, &f);
	}
	break;
	case CFIELD_IDEN:       /* 1063 */
	case LFIELD_IDEN:
	{
	    char *sz;

	    p = (dFIELDWHENACTION *)opnd[ opnd_top ].oval;
	    if( p->pTargetDfile == NULL ) {
		ErrorSet.xERROR = iNoTargetField;
		strcpy(ErrorSet.string, ":=");
		return  -1;
	    }

	    sz = xGetOpndString(dp);
	    if( sz == NULL ) {
		ErrorSet.xERROR = iBadUseOPr;         /* type error */
		strcpy( ErrorSet.string,":=");
		return  -iBadUseOPr;
	    }

	    PutField(p->pTargetDfile, p->wTargetid, sz);
#ifdef MMMM
	    switch( dp->type ) {
		case STRING_TYPE:
		{
		     if( dp->oval != NULL )
		     { //const string
			PutField(p->pTargetDfile, p->wTargetid, dp->oval);
		     } else {
			if( dp->length < MAX_OPND_LENGTH )
				PutField(p->pTargetDfile, p->wTargetid, ps);
			else {  PutField(p->pTargetDfile, p->wTargetid, (char *)*(long *)ps);
				free((char *)*(long *)ps);
			}
		     }
		}
		break;
		case NFIELD_IDEN:       /* 1057 */
		case FFIELD_IDEN:       /* 1058 */
		case LFIELD_IDEN:       /* 1059 */
		case DFIELD_IDEN:       /* 1060 */
		case CFIELD_IDEN:       /* 1062 */
		case SFIELD_IDEN:       /* 1063 */
		{
			char buf[256];
			p = (dFIELDWHENACTION *)opnd[opnd_top].oval;
			if( p->pTargetDfile == NULL ) {
				ErrorSet.xERROR = iNoTargetField;
				strcpy(ErrorSet.string, ":=");
				return  -1;
			}
			p1 = (dFIELDWHENACTION *)(dp->oval);

			/*if( p1->wSourceid == 0xFFFF )
			{
			    ErrorSet.xERROR = iNoField;
			    strcpy(ErrorSet.string, ":=");
			    return  -1;
			}*/

			GetField(p1->pSourceDfile, p1->wSourceid, buf);
			PutField(p->pTargetDfile, p->wTargetid, buf);
		}
		return  0;
		case MFIELD_IDEN:       /* 1061 */
		{
			char *sp;
			p = (dFIELDWHENACTION *)opnd[opnd_top].oval;
			if( p->pTargetDfile == NULL ) {
				ErrorSet.xERROR = iNoTargetField;
				strcpy(ErrorSet.string, ":=");
				return  -1;
			}
			p1 = (dFIELDWHENACTION *)(dp->oval);

			/*if( p1->wSourceid == 0xFFFF )
			{
			    ErrorSet.xERROR = iNoField;
			    strcpy(ErrorSet.string, ":=");
			    return  -1;
			}*/

			sp = SubstrOutBlank("", 0, 4096);	//alloc memory
			sp[0] = '\x8';         		//8 blocks is need
			GetField(p1->pSourceDfile, p1->wSourceid, sp);
			PutField(p->pTargetDfile, p->wTargetid, sp);
		}
		return  0;
		default:
			ErrorSet.xERROR = iBadUseOPr;         /* type error */
			strcpy( ErrorSet.string,":=");

			return (-iBadUseOPr);
	    } /* end of switch */
#endif
	    /*// this is depend on the pointer 0:short is wrong
	     *(unsigned long *)opnd[opnd_top].values = p->wTargetid;
	    __SetOpndIntType;*/
	    break;
	}
	case MFIELD_IDEN:       /* 1063 */
	{
	    p = (dFIELDWHENACTION *)opnd[ opnd_top ].oval;
	    if( p->pTargetDfile == NULL ) {
		ErrorSet.xERROR = iNoTargetField;
		strcpy(ErrorSet.string, ":=");
		return  -1;
	    }
	    switch( dp->type ) {
		case STRING_TYPE:
		{
		     dBITMEMO  *bMemo;

		    bMemo = (dBITMEMO *)malloc(sizeof(DBTBLOCK) + dp->length + 32);
		    if( bMemo == NULL ) {
			ErrorSet.xERROR = iNoMem;
			strcpy( ErrorSet.string,":=");

			return  -iNoMem;
		    }
		    bMemo->MemoMark = dBITMEMOMemoMark;
		    bMemo->MemoLen = dp->length+1;
		    bMemo->MemoTime = time(NULL);

		     if( dp->oval != NULL )
		     { //const string
			memcpy(&bMemo[1], dp->oval, bMemo->MemoLen);
			//PutField(p->pTargetDfile, p->wTargetid, dp->oval);
		     } else {
			if( dp->length < MAX_OPND_LENGTH ) {
			     memcpy(&bMemo[1], ps, bMemo->MemoLen);
			     //PutField(p->pTargetDfile, p->wTargetid, ps);
			} else {
			     //PutField(p->pTargetDfile, p->wTargetid, (char *)*(long *)ps);
			     memcpy(&bMemo[1], (char *)*(long *)ps, bMemo->MemoLen);
			     free((char *)*(long *)ps);
			}
		     }

		     if( PutField(p->pTargetDfile, p->wTargetid, bMemo) != 0 )
		     {
			ErrorSet.xERROR = iNoField;
			strcpy( ErrorSet.string,":=");

			return  -iNoMem;
		     }

		     free(bMemo);
		}
		break;

		case ARRAY_TYPE:
		{
		    ArrayType *pArray;
		    dBITMEMO  *bMemo;

		    pArray = (ArrayType *)*(long *)dp->oval;
		    bMemo = (dBITMEMO *)malloc(sizeof(DBTBLOCK) + pArray->MemSize);
		    if( bMemo == NULL ) {
			ErrorSet.xERROR = iNoMem;
			strcpy( ErrorSet.string,":=");

			return  -iNoMem;
		    }
		    bMemo->MemoMark = dBITMEMOMemoMark;
		    bMemo->MemoLen = pArray->MemSize;
		    bMemo->MemoTime = time(NULL);
		    memcpy(&bMemo[1], pArray->ArrayMem, pArray->MemSize);
		    PutField(p->pTargetDfile, p->wTargetid, bMemo);
		    free(bMemo);
		}
		break;

		case NFIELD_IDEN:       /* 1057 */
		case FFIELD_IDEN:       /* 1058 */
		case LFIELD_IDEN:       /* 1059 */
		case DFIELD_IDEN:       /* 1060 */
		case CFIELD_IDEN:       /* 1062 */
		case SFIELD_IDEN:       /* 1063 */
		{
			char buf[256];
			p = (dFIELDWHENACTION *)opnd[opnd_top].oval;
			if( p->pTargetDfile == NULL ) {
			    ErrorSet.xERROR = iBadUseOPr;         /* type error */
			    strcpy( ErrorSet.string,":=");

			    return( -10003 );
			}
			p1 = (dFIELDWHENACTION *)(dp->oval);

			/*if( p1->wSourceid == 0xFFFF )
			{
			    ErrorSet.xERROR = iNoField;
			    strcpy(ErrorSet.string, ":=");
			    return  -1;
			}*/

			GetField(p1->pSourceDfile, p1->wSourceid, buf);
			PutField(p->pTargetDfile, p->wTargetid, buf);
		}
			return  0;
		case MFIELD_IDEN:       /* 1061 */
		{
			char        buf[260];
			extern char tmpPath[MAXPATH];
			dFILE 	    *df;
			short       newid;

			p = (dFIELDWHENACTION *)opnd[opnd_top].oval;
			if( p->pTargetDfile == NULL ) {
				ErrorSet.xERROR = iNoTargetField;
				strcpy(ErrorSet.string, ":=");
				return  -1;
			}
			p1 = (dFIELDWHENACTION *)(dp->oval);

			sprintf(buf, "ILDIODBT.%03X", intOfThread&0xFFF);
			makefilename(buf, tmpPath, buf);

			//read the dbt into file
			df = getRealDbfId(p1->wSourceid, &newid);

			//dbtToFile(p1->pSourceDfile, p1->wSourceid, buf);
			dbtToFile(df, newid, buf);

			//put_fld from file
			dbtFromFile(p->pTargetDfile, p->wTargetid, buf);

		    /*
			char *sp;
			p = (dFIELDWHENACTION *)opnd[opnd_top].oval;
			if( p->pTargetDfile == NULL ) {
				ErrorSet.xERROR = iNoTargetField;
				strcpy(ErrorSet.string, ":=");
				return  -1;
			}
			p1 = (dFIELDWHENACTION *)(dp->oval);
			*/

			/*if( p1->wSourceid == 0xFFFF )
			{
			    ErrorSet.xERROR = iNoField;
			    strcpy(ErrorSet.string, ":=");
			    return  -1;
			}*/

			/*
			sp = SubstrOutBlank("", 0, 4096);	//alloc memory
			sp[0] = '\x8';         	//8 blocks is need
			GetField(p1->pSourceDfile, p1->wSourceid, sp);
			PutField(p->pTargetDfile, p->wTargetid, sp);
			*/
		}
		return  0;
		default:
			ErrorSet.xERROR = iBadUseOPr;         /* type error */
			strcpy( ErrorSet.string,":=");

			return  (-iBadUseOPr);
	    } /* end of switch */

	    /*// this is depend on the pointer 0:short is wrong
	     *(unsigned long *)opnd[opnd_top].values = p->wTargetid;
	    __SetOpndIntType;*/
	    break;
	}

	case DFIELD_IDEN:       /* 1064 date field*/
	{
	    p = (dFIELDWHENACTION *)opnd[ opnd_top ].oval;
	    if( p->pTargetDfile == NULL ) {
		ErrorSet.xERROR = iNoTargetField;
		strcpy(ErrorSet.string, ":=");
		return  -1;
	    }
	    switch( dp->type ) {
		case DATE_TYPE:
		{
		    PutField(p->pTargetDfile, p->wTargetid, subcopy(ps,0,8));
		}
		break;
		case STRING_TYPE:
		{
		     if( dp->oval != NULL )
		     { //const string
			PutField(p->pTargetDfile, p->wTargetid, subcopy(dp->oval,0,8));
		     } else {
			if( dp->length < MAX_OPND_LENGTH )
				PutField(p->pTargetDfile, p->wTargetid, subcopy(ps,0,8));
			else {  PutField(p->pTargetDfile, p->wTargetid, subcopy((char *)*(long *)ps,0,8));
				free((char *)*(long *)ps);
			}
		     }
		}
		break;
		case NFIELD_IDEN:       /* 1057 */
		case FFIELD_IDEN:       /* 1058 */
		case LFIELD_IDEN:       /* 1059 */
		case DFIELD_IDEN:       /* 1060 */
		case CFIELD_IDEN:       /* 1062 */
		case SFIELD_IDEN:       /* 1063 */
		{
			char buf[256];
			p = (dFIELDWHENACTION *)opnd[opnd_top].oval;
			if( p->pTargetDfile == NULL ) {
				ErrorSet.xERROR = iNoTargetField;
				strcpy(ErrorSet.string, ":=");
				return  -1;
			}
			p1 = (dFIELDWHENACTION *)(dp->oval);

			/*if( p1->wSourceid == 0xFFFF )
			{
			    ErrorSet.xERROR = iNoField;
			    strcpy(ErrorSet.string, ":=");
			    return  -1;
			}*/

			GetField(p1->pSourceDfile, p1->wSourceid, buf);
			PutField(p->pTargetDfile, p->wTargetid, subcopy(buf,0,8));
		}
			return  0;
		case MFIELD_IDEN:       /* 1061 */
		{
			char *sp;
			p = (dFIELDWHENACTION *)opnd[opnd_top].oval;
			if( p->pTargetDfile == NULL ) {
				ErrorSet.xERROR = iNoTargetField;
				strcpy(ErrorSet.string, ":=");
				return  -1;
			}
			p1 = (dFIELDWHENACTION *)(dp->oval);

			/*if( p1->wSourceid == 0xFFFF )
			{
			    ErrorSet.xERROR = iNoField;
			    strcpy(ErrorSet.string, ":=");
			    return  -1;
			}*/

			sp = SubstrOutBlank("", 0, 4096);	//alloc memory
			sp[0] = '\x8';         		//8 blocks is need
			GetField(p1->pSourceDfile, p1->wSourceid, sp);
			PutField(p->pTargetDfile, p->wTargetid, subcopy(sp,0,8));
		}
		return  0;
		default:
			ErrorSet.xERROR = iBadUseOPr;         /* type error */
			strcpy( ErrorSet.string,":=");

			return  -iBadUseOPr;
	    } /* end of switch */

	    /*// this is depend on the pointer 0:short is wrong
	     *(unsigned long *)opnd[opnd_top].values = p->wTargetid;
	    __SetOpndIntType;*/
	    break;
	}

	case ARRAY_TYPE:
	{
	    switch( dp->type ) {
		case ARRAY_TYPE:
		{
		    ArrayType *pArray1, *pArray2;
		    pArray1 = (ArrayType *)*(long *)dp->oval;
		    pArray2 = (ArrayType *)*(long *)opnd[ opnd_top ].oval;
		    memcpy(pArray2->ArrayMem, pArray1->ArrayMem, pArray2->MemSize);
		}
		break;
		case MFIELD_IDEN:
		{
		    ArrayType *pArray;
		    dBITMEMO  *bMemo;

		    p = (dFIELDWHENACTION *)dp->oval;
		    if( p->pSourceDfile == NULL ) {
			return  -1;
		    }

		    pArray = (ArrayType *)*(long *)opnd[opnd_top].oval;
		    bMemo = (dBITMEMO *)malloc(sizeof(DBTBLOCK) + pArray->MemSize);
		    if( bMemo == NULL ) {
			ErrorSet.xERROR = iNoMem;
			strcpy( ErrorSet.string,":=");

			return  -iNoMem;
		    }
		    *(unsigned char *)bMemo=(sizeof(DBTBLOCK)+DBTBLOCKSIZE-1+\
						pArray->MemSize) / DBTBLOCKSIZE;
		    GetField(p->pSourceDfile, p->wSourceid, bMemo);
		    memcpy(pArray->ArrayMem, &bMemo[1], pArray->MemSize);
		    free(bMemo);
		}
		break;
		default:
		    ErrorSet.xERROR = iBadUseOPr;         /* type error */
		    strcpy( ErrorSet.string,":=");
		    return  (-iBadUseOPr);
	    }
	}
	break;

#endif
	case VINT_IDEN:         /////////////////////////////////////////////
	{
	    switch( dp->type ) {
		case CHR_TYPE:
			*(short *)*(long *)opnd[opnd_top].values = *(char *)ps;
			break;
		case INT_TYPE:
			*(short *)*(long *)opnd[opnd_top].values = *(short *)ps;
			break;
		case LONG_TYPE:
			*(short *)*(long *)opnd[opnd_top].values = (short)*(long *)ps;
			break;
		case FLOAT_TYPE:
			*(short *)*(long *)opnd[opnd_top].values = (short)*(double *)ps;
			break;
		case NFIELD_IDEN:       /* number Field */
			*(short *)*(long *)opnd[opnd_top].values = \
				     (short)atol(subcopy((char *)*(long *)dp->oval, 0, \
				     dp->length));
			break;
		case FFIELD_IDEN:       /* float Field */
			*(short *)*(long *)opnd[opnd_top].values = \
				(short)atof(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			break;
		case STRING_TYPE:
			if( dp->oval == NULL ) {
			   if( dp->length < MAX_OPND_LENGTH ) {
				*(short *)*(long *)opnd[opnd_top].values = \
								    atoi(ps);
			   } else {
				*(short *)*(long *)opnd[opnd_top].values = \
						    atoi((char *)*(long *)ps);
			   }
			} else {
				*(short *)*(long *)opnd[opnd_top].values = \
							     atoi(dp->oval);
			}
			break;
		default:
			ErrorSet.xERROR = iBadUseOPr;         /* type error */
			strcpy( ErrorSet.string,":=");

			return  -iBadUseOPr;
	    } /* end of switch */

	    *(short *)opnd[opnd_top].values = \
				*(short *)*(long *)opnd[opnd_top].values;
	    __SetOpndIntType;
	}
	break;
	case VSTRING_IDEN:	/////////////////////////////////////////////
	{
	    char *sz, *sz1;
	    int  ispace = opnd[opnd_top].length;

	    if( ispace < MAX_OPND_LENGTH ) {
		sz = (char *)*(long *)opnd[opnd_top].values;
	    } else {
		sz = (char *)*(long *)*(long *)opnd[opnd_top].values;
	    }

	    sz1 = xGetOpndString(dp);
	    if( sz1 == NULL ) {
		ErrorSet.xERROR = iBadUseOPr;         /* type error */
		strcpy( ErrorSet.string,":=");
		return  -iBadUseOPr;
	    }
	    strZcpy(sz, sz1, ispace);

	    //SetOpndStringType
	    ///////////////////
	    if( strlen(sz1) < MAX_OPND_LENGTH ) {
		strcpy(opnd[opnd_top].values, sz1);
	    } else {
		*(long *)opnd[opnd_top].values = (long)strdup( sz1 );
	    }
	    dp->oval = NULL;
	    dp->type = STRING_TYPE;
	}
	break;

	case VDATE_IDEN:        /////////////////////////////////////////////
	{
	    switch( dp->type ) {
		case INT_TYPE:
		{
			char buf[32];

			itoa(*(short *)ps, buf, 10);
			strZcpy( (char *)*(long *)opnd[opnd_top].values, buf, 9);
		}
		break;
		case LONG_TYPE:
		{
			char buf[32];

			ltoa(*(short *)ps, buf, 10);
			strZcpy((char *)*(long *)opnd[opnd_top].values, buf, 9);
		}
		break;
		case STRING_TYPE:
			if( dp->oval == NULL ) {
				if( dp->length < MAX_OPND_LENGTH ) {
				strZcpy((char *)*(long *)opnd[opnd_top].values, \
							ps, 9);
			   } else {
				strZcpy((char *)*(long *)opnd[opnd_top].values, \
				    (char *)*(long *)ps, 9);
				free((char *)*(long *)ps);
			   }
			} else {
				strZcpy((char *)*(long *)opnd[opnd_top].values, \
							dp->oval, 9);
			}
		break;
		case DATE_TYPE:
			if( dp->oval == NULL ) {
				strZcpy((char *)*(long *)opnd[opnd_top].values, \
							ps, 9);
			} else {
				strZcpy((char *)*(long *)opnd[opnd_top].values, \
							dp->oval, 9);
			}
		break;
		default:
			ErrorSet.xERROR = iBadUseOPr;         /* type error */
			strcpy( ErrorSet.string,":=");
			return  -iBadUseOPr;
	    }

	    //SetOpndDateType
	    strcpy(opnd[opnd_top].values, (char *)*(long *)opnd[opnd_top].values);
	    __SetOpndDateType;
	}
	break;

	case VCHR_IDEN:         /////////////////////////////////////////////
	{
	    switch( dp->type ) {
		case CHR_TYPE:
			*(char *)*(long *)opnd[opnd_top].values = *(char *)ps;
			break;
		case INT_TYPE:
			*(char *)*(long *)opnd[opnd_top].values = *(char *)ps;
			break;
		case LONG_TYPE:
			*(char *)*(long *)opnd[opnd_top].values = (char)*(long *)ps;
			break;
		case FLOAT_TYPE:
			*(char *)*(long *)opnd[opnd_top].values = (char)*(double *)ps;
			break;
		case NFIELD_IDEN:       /* number Field */
			*(char *)*(long *)opnd[opnd_top].values = \
				     (char)atoi(subcopy((char *)*(long *)dp->oval, 0, \
				     dp->length));
			break;
		case FFIELD_IDEN:       /* float Field */
			*(char *)*(long *)opnd[opnd_top].values = \
				(char)atof(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			break;
		case STRING_TYPE:
			if( dp->oval == NULL ) {
			   if( dp->length < MAX_OPND_LENGTH ) {
				*(char *)*(long *)opnd[opnd_top].values = \
								    (char)atoi(ps);
			   } else {
				*(char *)*(long *)opnd[opnd_top].values = \
						(char)atoi((char *)*(long *)ps);
				free((char *)*(long *)ps);
			   }
			} else {
			   *(char *)*(long *)opnd[opnd_top].values = \
							     (char)atoi(dp->oval);
			}
			break;
		default:
			ErrorSet.xERROR = iBadUseOPr;         /* type error */
			strcpy( ErrorSet.string,":=");

			return  (-iBadUseOPr);
	    } /* end of switch */

	    *(char *)opnd[opnd_top].values = \
				*(char *)*(long *)opnd[opnd_top].values;
	    __SetOpndChrType;
	}
	break;
	case VLONG_IDEN:        /////////////////////////////////////////////
	{
	    switch( dp->type ) {
		case CHR_TYPE:
			*(long *)*(long *)opnd[opnd_top].values = (long)*(char *)ps;
			break;
		case INT_TYPE:
			*(long *)*(long *)opnd[opnd_top].values = (long)*(short *)ps;
			break;
		case LONG_TYPE:
			*(long *)*(long *)opnd[opnd_top].values = (long)*(long *)ps;
			break;
		case FLOAT_TYPE:
			*(long *)*(long *)opnd[opnd_top].values = (long)*(double *)ps;
			break;
		case NFIELD_IDEN:       /* number Field */
			*(long *)*(long *)opnd[opnd_top].values = \
				     atol(subcopy((char *)*(long *)dp->oval, 0, \
				     dp->length));
			break;
		case FFIELD_IDEN:       /* float Field */
			*(long *)*(long *)opnd[opnd_top].values = \
				(long)atof(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			break;
		case STRING_TYPE:
			if( dp->oval == NULL ) {
			   if( dp->length < MAX_OPND_LENGTH ) {
				*(long *)*(long *)opnd[opnd_top].values = \
								    atol(ps);
			   } else {
				*(long *)*(long *)opnd[opnd_top].values = \
							atol((char *)*(long *)ps);
				free((char *)*(long *)ps);
			   }
			} else {
			   *(long *)*(long *)opnd[opnd_top].values = \
							     atol(dp->oval);
			}
			break;
		default:
			ErrorSet.xERROR = iBadUseOPr;         /* type error */
			strcpy( ErrorSet.string,":=");

			return  (-iBadUseOPr);
	    } /* end of switch */

	    *(long *)opnd[opnd_top].values = \
				*(long *)*(long *)opnd[opnd_top].values;
	    __SetOpndLongType;
	}
	break;
	case VFLOAT_IDEN:
	{
	    switch( dp->type ) {
		case CHR_TYPE:
			*(double *)*(long *)opnd[opnd_top].values = (double)*(char *)ps;
			break;
		case INT_TYPE:
			*(double *)*(long *)opnd[opnd_top].values = (double)*(short *)ps;
			break;
		case LONG_TYPE:
			*(double *)*(long *)opnd[opnd_top].values = (double)*(long *)ps;
			break;
		case FLOAT_TYPE:
			*(double *)*(long *)opnd[opnd_top].values = *(double *)ps;
			break;
		case NFIELD_IDEN:       /* number Field */
			*(double *)*(long *)opnd[opnd_top].values = \
				     (double)atol(subcopy((char *)*(long *)dp->oval, 0, \
				     dp->length));
			break;
		case FFIELD_IDEN:       /* float Field */
			*(double *)*(long *)opnd[opnd_top].values = \
				atof(subcopy((char *)*(long *)dp->oval, 0, \
					dp->length));
			break;
		case STRING_TYPE:
			if( dp->oval == NULL ) {
			   if( dp->length < MAX_OPND_LENGTH ) {
				*(double *)*(long *)opnd[opnd_top].values = \
								    atof(ps);
			   } else {
				*(double *)*(long *)opnd[opnd_top].values = \
							atof((char *)*(long *)ps);
				free((char *)*(long *)ps);
			   }
			} else {
				*(double *)*(long *)opnd[opnd_top].values = \
							     atof(dp->oval);
			}
			break;
		default:
			ErrorSet.xERROR = iBadUseOPr;         /* type error */
			strcpy( ErrorSet.string,":=");

			return  (-iBadUseOPr);
	    } /* end of switch */

	    *(double *)opnd[opnd_top].values = \
				*(double *)*(long *)opnd[opnd_top].values;
	    __SetOpndFloatType;
	}
	break;
	default:
	{
		ErrorSet.xERROR = iBadUseOPr;         /* type error */
		strcpy( ErrorSet.string,":=");
		return  (-iBadUseOPr);
	}
     }

     return  0;

} /* end of store_g */


/*------------------------------------------------------------------------
 !                      array_g
 ! Prompt: the array dimmention should be one of CHAR, INT, LONG type, the
 !         array's dimmension is limmited by define MAXARRAYDIM
 ! sucess: return 0;
 !------------------------------------------------------------------------*/
static short near PD_style array_g( MidCodeType *m_c_pointer )
{
     int    i, j;
     short  adr_mark=0;
     void  *p;
     short  element_size = 0;
     long   l, ll;
     int    storeOpFollowed;

     switch( (*(ArrayType *)m_c_pointer->values).ElementType ) {
	  case STRING_TYPE:
		element_size = (short)(*(ArrayType *)m_c_pointer->values).\
					      ArrayDim[m_c_pointer->length ];
		break;
	  case CHR_TYPE:     element_size = 1;      break;
	  case INT_TYPE:     element_size = 2;      break;
	  case LONG_TYPE:    element_size = 4;      break;
	  case FLOAT_TYPE:   element_size = sizeof(double);     break;
	  case STRING_IDEN:  element_size = 4;
	  default:           ErrorSet.xERROR = iNoMatchArray;        return( 1 );
     }

     //
     //now, the opnd is like this:
     // [&]	    <----- opnd_top - m_c_pointer->length => i
     // [opnd1]
     // [opnd2]	    <----- opnd_top
     //

     if( m_c_pointer->next != NULL && m_c_pointer->next->type == STORE_TYPE )
	storeOpFollowed = 1;
     else
	storeOpFollowed = 0;

     p = NULL;
     j = 1;
     i = opnd_top - m_c_pointer->length;
     if( i >= 0 ) {
	if( opnd[i].type == VARADDR_TYPE ) {
		adr_mark = 1;
	}
     }

     for( i++;  i < opnd_top;  i++, j++ ) {
	  switch( opnd[i].type ) {
	      case CHR_TYPE:
		  p = (void *)(( (long)p + *(char *)opnd[i].values - 1 ) * \
			(short)(*(ArrayType *)m_c_pointer->values).ArrayDim[ j ]);
		  break;
	      case INT_TYPE:
		  p = (void *)(( (long)p + *(short *)opnd[i].values - 1 ) * \
			(short)(*(ArrayType *)m_c_pointer->values).ArrayDim[ j ]);
		  break;
	      case LONG_TYPE:
		  p = (void *)(( (long)p + *(long *)opnd[i].values - 1 ) * \
			(short)(*(ArrayType *)m_c_pointer->values).ArrayDim[ j ]);
		  break;
	      default:
		  ErrorSet.xERROR = iDimError;       // error dim type
		  return( 1 );
	  }
     }

     //last opnd value
     l = (long)(*(ArrayType *)m_c_pointer->values).ArrayMem;
     switch( opnd[i].type ) {
	  case CHR_TYPE:
		p = (void *)(( (long)p + *(char *)opnd[i].values - 1 ) * \
                                                          element_size + l);
		break;
	  case INT_TYPE:
		p = (void *)(( (long)p + *(short *)opnd[i].values - 1 ) * \
                                                          element_size + l);
		break;
	  case LONG_TYPE:
		p = (void *)(( (long)p + *(long *)opnd[i].values - 1 ) * \
                                                          element_size + l);

		break;
	  default:
		ErrorSet.xERROR = iDimError;                 /* error dim type  */
		return( 1 );
     }

     ll = (long)p - l;
     if( ll < 0 || ll >= (long)(*(ArrayType *)m_c_pointer->values).MemSize ) {
        ErrorSet.xERROR = iDimError;                 /* error dim type  */
	return( 1 );
     }

     if( storeOpFollowed || adr_mark == 1 ) {
	
	if( adr_mark == 1 )
	    opnd_top -= (short)m_c_pointer->length;
	else
	    opnd_top -= (short)m_c_pointer->length - 1;

	*(long *)opnd[opnd_top].values = (long)p;
	switch( (*(ArrayType *)m_c_pointer->values).ElementType ) {
	   case CHR_TYPE:
		opnd[opnd_top].type = VCHR_IDEN;
		opnd[opnd_top].length = sizeof( char );
		break;
	   case INT_TYPE:
		opnd[opnd_top].type = VINT_IDEN;
		opnd[opnd_top].length = sizeof( short );
		break;
	   case LONG_TYPE:
		opnd[opnd_top].type = VLONG_IDEN;
		opnd[opnd_top].length = sizeof( long );
		break;
	   case FLOAT_TYPE:
		opnd[opnd_top].type = VFLOAT_IDEN;
		opnd[opnd_top].length = sizeof( double );
		break;
	   default:
		ErrorSet.xERROR = iDimError;       // error dim type
		return( 1 );
	} /* end of switch */
	return( 0 );
     }

     opnd_top -= m_c_pointer->length - 1;
     opnd[opnd_top].type = (*(ArrayType *)m_c_pointer->values).ElementType;
     switch( (*(ArrayType *)m_c_pointer->values).ElementType ) {
	   case CHR_TYPE:
		    *(char *)opnd[opnd_top].values = *(char *)p;  break;
	   case INT_TYPE:
		    *(short *)opnd[opnd_top].values = *(short *)p;    break;
	   case LONG_TYPE:
		    *(long *)opnd[opnd_top].values = *(long *)p;  break;
	   case FLOAT_TYPE:
		    *(double *)opnd[opnd_top].values = \
					     *(double *)p;   break;
	   case STRING_IDEN:
		    opnd[opnd_top].type = STRING_TYPE;
		    if( ( opnd[opnd_top].length = strlen( (char *)*(long *)p ) ) \
							< MAX_OPND_LENGTH )
			   strcpy((char *)opnd[opnd_top].values,(char *)*(long *)p );
		    else {      /* alloc a block of space for we will
				 * release the space when we finished using
				 * it, but we dare not to release the
				 * variable space**************************/
			   *(long *)opnd[opnd_top].values = \
							(long)strdup( (char *)*(long *)p );
		    }
		    break;
	   case STRING_TYPE:
		    if( ( opnd[opnd_top].length = strlen( (char *)p ) ) \
							< MAX_OPND_LENGTH )
			   strcpy((char *)opnd[opnd_top].values,(char *)p );
		    else
			   *(long *)opnd[opnd_top].values = \
							(long)strdup( (char *)p );
		    break;
	   default:
		    ErrorSet.xERROR = iDimError;       // error dim type
		    return( 1 );
     } /* end of switch */

     return  0;

} /* end of array_g */


/*=========================================================================
 !              FreeCode()
 *========================================================================*/
_declspec(dllexport) short PD_style FreeCode( MidCodeType *m_c_head )
{
     MidCodeType *p;

     while( m_c_head != NULL ) {
	p = m_c_head->next;
	free(m_c_head);
	m_c_head = p;
     }

    return  0;

}

/*********
 *                      ReadCode()
 **************************************************************************/
MidCodeType * PD_style ReadCode(FILE *CodeFileHandle)
{
     MidCodeType *p, *mp, *m_c_head, *m_c_pointer;
     short 	node_num;
     char  	buf[OPND_STATIC_LENGTH];

#ifndef USE_NEW_WRITECODE
     char  	bufOld[OPND_STATIC_LENGTH];
struct tagOldMidCodeType {
	short    type;
	unsigned  char  values[ MAX_OPND_LENGTH ];
	short   length;
	struct tagMidCodeType   *next;
} *oldMc = (struct tagOldMidCodeType *)bufOld;
#endif

     if( CodeFileHandle == NULL ) {
	ErrorSet.xERROR = iFileRead;     // file cannot be read
	return( NULL );
     }

     m_c_head = NULL;
     fread(&node_num, sizeof(short), 1, CodeFileHandle);    // reserve space for node num
     if( node_num == 0 ) {
	return  NULL;
     }

     mp = (MidCodeType *)buf;

     do {

	short i;

	if( (p = (MidCodeType *)malloc(sizeof(MidCodeType))) == NULL ) {
		FreeCode(m_c_head);
		return  NULL;
	}

#ifndef USE_NEW_WRITECODE
	fread( bufOld, OPND_STATIC_LENGTH, 1, CodeFileHandle);
	//turn old to new
	mp->type = oldMc->type;
	memcpy(mp->values, oldMc->values, MAX_OPND_LENGTH);
	mp->length = oldMc->length;
#else
	fread( buf, OPND_STATIC_LENGTH, 1, CodeFileHandle);
#endif
	switch( mp->type ) {
	    case STRING_TYPE:
		i = mp->length+sizeof(MidCodeType)-MAX_ATOM_LENGTH;
		p = realloc(p, i);
		if( p == NULL ) {
			FreeCode(m_c_head);
			return  NULL;
		}
		if( mp->length >= MAX_OPND_LENGTH ) {
			fseek(CodeFileHandle, 0-OPND_STATIC_LENGTH, SEEK_CUR);
			fread( p, i, 1, CodeFileHandle);
		} else {
			memcpy(p, buf, i);
		}
	    break;
	    case IDEN_TYPE:
	    case STRING_IDEN:       /* 1051 */
	    case CHR_IDEN:          /* 1052 */
	    case INT_IDEN:          /* 1053 */
	    case LONG_IDEN:         /* 1054 */
	    case FLOAT_IDEN:        /* 1055 */
	    case DATE_IDEN:
	    case NFIELD_IDEN:       /* 1057 */
	    case FFIELD_IDEN:       /* 1058 */
	    case LFIELD_IDEN:       /* 1059 */
	    case DFIELD_IDEN:       /* 1060 */
	    case MFIELD_IDEN:       /* 1061 */
	    case CFIELD_IDEN:       /* 1062 */
	    case SFIELD_IDEN:       /* 1063 */
	    case ARRAY_TYPE:        /* 1077 */
	    case ARRAYB_TYPE:
	    case FUNB_TYPE:
		p = realloc(p, OPND_STATIC_LENGTH);
		if( p == NULL ) {
			FreeCode(m_c_head);
			return  NULL;
		}
		memcpy(p, buf, OPND_STATIC_LENGTH);
	    break;
	    case INT_TYPE:          /* 1065 */
		memcpy(p, buf, sizeof(MidCodeType)-MAX_ATOM_LENGTH+sizeof(short));
	    break;
	    case LONG_TYPE:          /* 1066 */
		memcpy(p, buf, sizeof(MidCodeType)-MAX_ATOM_LENGTH+sizeof(long));
	    break;
	    case FLOAT_TYPE:         /* 1067 */
		memcpy(p, buf, sizeof(MidCodeType)-MAX_ATOM_LENGTH+sizeof(double));
	    break;
	    case DATE_TYPE:
		memcpy(p, buf, sizeof(MidCodeType));
	    break;

	    case QUSTN_TYPE:
		memcpy(p, buf, sizeof(MidCodeType));
		*(long *)(p->values) = (long)NULL;
	    break;
	    case LABEL_TYPE:
	    {
		MidCodeType *pp;

		memcpy(p, buf, sizeof(MidCodeType));
		for( pp = m_c_head; pp != NULL; pp = pp->next ) {
		   if( pp->type == QUSTN_TYPE ) {
			//hasnot assigned jmp position
			if( *(long *)(pp->values) == (long)NULL ) {
				*(long *)(pp->values) = (long)p;
				break;
			}
		   } //end of if
		} //end of for
	    }
	    break;

//	    case CHR_TYPE:          // 1064
	    default:
		memcpy(p, buf, sizeof(MidCodeType));
	    break;
	}

	p->next = NULL;
	if( m_c_head == NULL ) {
		m_c_pointer = m_c_head = p;
	} else {
		m_c_pointer->next = p;
		m_c_pointer = p;
	}

	node_num--;

	if( p->type == END_TYPE )
		break;

     } while( p != NULL && node_num > 0 );

     if( node_num > 0 ) {
	ErrorSet.xERROR = iUnExpectEnd;    // expression has no end
	FreeCode(m_c_head);
	return  NULL;
     }

     return  m_c_head;

}


/*********
 *                      WriteCode()
 * this function should compatible with old version, so I have to use this
 * algorithm
 **************************************************************************/
short PD_style WriteCode( MidCodeType *m_c_head, FILE *CodeFileHandle )
{
     char  buf[OPND_STATIC_LENGTH];
     MidCodeType *mp;
     short node_num;
     short AdjustString;

     if( CodeFileHandle == NULL ) {
	ErrorSet.xERROR = iFileWrite;     // file cannot be writen
	return  -1;
     }

     mp = (MidCodeType *)buf;
     node_num = 1;
     AdjustString = 0;

     fwrite( &buf, sizeof(short), 1, CodeFileHandle );        // reserve space for node num
/*
     write( CodeFileHandle, &node_num, sizeof(short) );
*/
     while( m_c_head != NULL ) {
	memcpy(mp, m_c_head, OPND_STATIC_LENGTH);

	if( mp->next != NULL )
		mp->next = (MidCodeType *)(node_num * OPND_STATIC_LENGTH + AdjustString);

	node_num++;
	if( mp->type == STRING_TYPE && mp->length >= MAX_OPND_LENGTH ) {
		AdjustString += mp->length - MAX_OPND_LENGTH;
		fwrite(&buf, OPND_STATIC_LENGTH+mp->length - MAX_OPND_LENGTH, 1, CodeFileHandle);
	} else {
		fwrite(&buf, OPND_STATIC_LENGTH, 1, CodeFileHandle);
	}
	m_c_head = m_c_head->next;
     }
     node_num--;

     fseek( CodeFileHandle, 0L-sizeof(short)-node_num*OPND_STATIC_LENGTH-AdjustString, \
								SEEK_CUR);
     fwrite( &node_num, sizeof(short), 1, CodeFileHandle );
     fseek(CodeFileHandle, node_num*OPND_STATIC_LENGTH+AdjustString, SEEK_CUR);

     return  sizeof(short) + node_num*sizeof(MidCodeType);

}

/*
----------------------------------------------------------------------------
	       GeterrorNo()
--------------------------------------------------------------------------*/
short PD_style GeterrorNo( void )
{
	return  ErrorSet.xERROR;
}

/*
----------------------------------------------------------------------------
	       ClearError()
--------------------------------------------------------------------------*/
void  PD_style ClearError( void )
{
       ErrorSet.xERROR = XexpOK;
}


/*
----------------------------------------------------------------------------
	       GetErrorMes()
--------------------------------------------------------------------------*/
char *PD_style GetErrorMes(short ErrorNo)
{
    WSToMT static char szErrorMes[256];

    if( ErrorNo >= iXEXPErrorTail || ErrorNo < 0 )	ErrorNo = 0;
    sprintf(szErrorMes, "%s {%s}", XexpErrorMessage[ErrorNo],ErrorSet.string);
    return  szErrorMes;
}


/*
----------------------------------------------------------------------------
!!                      SymbolTableSearch()
----------------------------------------------------------------------------*/
SysVarOFunType * PD_style SymbolTableSearch( const unsigned char *Key, \
				   SysVarOFunType *Base, \
				   short BaseKeyNum )
{

#ifdef SYMBOLTABLESORTED

    short Low, Mid, CompValue;

    Low = 0;

    while( Low < BaseKeyNum ) {
	Mid = ( Low + BaseNum ) / 2;
	if( CompValue = stricmp(Key, Base[Mid]->VarOFunName) == 0 )
		return( &Base[Mid] );
	if( CompValue > 0 )     Low = Mid;
	else                    BaseKeyNum = Mid;
    }

    return( NULL );

#else

    short i;

    for( i = 0;  i < BaseKeyNum && stricmp(Key, Base[i].VarOFunName) != 0; i++ );

    if( i >= BaseKeyNum )        return( NULL );
    return( &Base[i] );

#endif

} /* end of SymbolTableSearch() */




/*+++++++++++++++++++* xexp functions for users *++++++++++++++++++++++++*/

short PD_style _TypeAlign( OpndType *lpOpnd, short ParaNum, short AlignType )
{
    int i;

    switch( AlignType ) {
	case LONG_TYPE:
	case CHR_TYPE:
	case INT_TYPE:
	case NFIELD_IDEN:
		for( i = 0;  i < ParaNum;   i++ ) {
		    switch( lpOpnd[i].type ) {
			case CHR_TYPE:
				*(long *)lpOpnd[i].values = *(char *)lpOpnd[i].values;
				break;
			case INT_TYPE:
				*(long *)lpOpnd[i].values = *(short *)lpOpnd[i].values;
				break;
			case NFIELD_IDEN:
				*(long *)lpOpnd[i].values = atol( subcopy( (char *)*(long *)lpOpnd[i].oval, 0, \
								lpOpnd[i].length ) );
				break;
			case FFIELD_IDEN:
				*(long *)lpOpnd[i].values = (long)atof( subcopy((char *)*(long *)lpOpnd[i].oval, 0, \
								lpOpnd[i].length ) );
				break;
			case LONG_TYPE:
				break;
			case DFIELD_IDEN:
			case CFIELD_IDEN:
				*(long *)lpOpnd[i].values = DateMinusToLong( subcopy((char *)*(long *)lpOpnd[i].oval, 0, \
								lpOpnd[i].length ), "" );
				break;
			case DATE_TYPE:
				*(long *)lpOpnd[i].values = DateMinusToLong( subcopy(lpOpnd[i].values, 0, \
								lpOpnd[i].length ), "" );
				break;
			default:
				ErrorSet.xERROR = iTypeNoCompt;     /* type not comptible */
				return( 1 );
		    }
		    lpOpnd[i].type = LONG_TYPE;
		    lpOpnd[i].length = sizeof(long);
		}  /* end of for */
		break;
	case DATE_TYPE:
	case DFIELD_IDEN:
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
				*(long *)lpOpnd[i].values = atol( subcopy( (char *)*(long *)lpOpnd[i].oval, 0, \
								lpOpnd[i].length ) );
				break;
			case FFIELD_IDEN:
				*(long *)lpOpnd[i].values = (long)atof( subcopy((char *)*(long *)lpOpnd[i].oval, 0, \
								lpOpnd[i].length ) );
				break;
			case LONG_TYPE:
			case LONG_IDEN:
				break;
			case DFIELD_IDEN:
			case CFIELD_IDEN:
				*(long *)lpOpnd[i].values = DateMinusToLong( subcopy((char *)*(long *)lpOpnd[i].oval, 0, \
								lpOpnd[i].length ), "" );
				break;
			case DATE_TYPE:
				*(long *)lpOpnd[i].values = DateMinusToLong( subcopy(lpOpnd[i].values, 0, \
								lpOpnd[i].length ), "" );
				break;
			default:
				ErrorSet.xERROR = iTypeNoCompt;     /* type not comptible */
				return( 1 );
		    }
		    lpOpnd[i].type = DATE_TYPE;	//date_long_type
		    lpOpnd[i].length = sizeof(long);
		}  /* end of for */
		break;
	case FLOAT_TYPE:
	case FFIELD_IDEN:
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
				*(double *)lpOpnd[i].values = atol( subcopy( (char *)*(long *)lpOpnd[i].oval, 0, \
								lpOpnd[i].length ) );
				break;
			case FFIELD_IDEN:
				*(double *)lpOpnd[i].values = atof( subcopy((char *)*(long *)lpOpnd[i].oval, 0, \
								lpOpnd[i].length ) );
				break;
			case FLOAT_TYPE:
			case FLOAT_IDEN:
				break;
			case DFIELD_IDEN:
				*(double *)lpOpnd[i].values = (double)DateMinusToLong( subcopy((char *)*(long *)lpOpnd[i].oval, 0, \
								lpOpnd[i].length ), "" );
				break;
			case DATE_TYPE:
				*(double *)lpOpnd[i].values = (double)DateMinusToLong( subcopy(lpOpnd[i].values, 0, \
								lpOpnd[i].length ), "" );
				break;
			default:
				ErrorSet.xERROR = iTypeNoCompt;
				return( 1 );
		    }
		    opnd[i].type = FLOAT_TYPE;
		    opnd[i].length = sizeof(double);
		}  /* end of for */
    }
    return( 0 );
} /* end of function TypeAlign */


long PD_style xGetOpndLong(OpndType *lpOpnd)
{
    switch( lpOpnd->type ) {
	case CHR_TYPE:
	case CHR_IDEN:
		return  (long)*(char *)lpOpnd->values;
		break;
	case LONG_TYPE:
	case LONG_IDEN:
		return  *(long *)lpOpnd->values;
		break;
	case INT_TYPE:
	case INT_IDEN:
	case LOGIC_TYPE:
		return  (long)*(short *)lpOpnd->values;
		break;
	case FLOAT_TYPE:
	case FLOAT_IDEN:
		return  (long)*(double *)lpOpnd->values;
		break;
	case NFIELD_IDEN:
		return  atol( subcopy((char *)*(long *)lpOpnd->oval, 0, lpOpnd->length) );
		break;
	case FFIELD_IDEN:
		return  (long)atof( subcopy((char *)*(long *)lpOpnd->oval, 0, lpOpnd->length) );
		break;

	//2000.6.6
	default:
	    {
		char buf[MAX_OPND_LENGTH+2];
		
		strZcpy(buf, lpOpnd->values, MAX_OPND_LENGTH);
		return  atol( buf );
	    }
    }

    ErrorSet.xERROR = iTypeNoCompt;
    return LONG_MIN;

} //xGetOpndLong()


short PD_style xGetOpndShort(OpndType *lpOpnd)
{
    switch( lpOpnd->type ) {
	case CHR_TYPE:
	case CHR_IDEN:
		return  (short)*(char *)lpOpnd->values;
		break;
	case LONG_TYPE:
	case LONG_IDEN:
		return  (short)*(long *)lpOpnd->values;
		break;
	case INT_TYPE:
	case INT_IDEN:
	case LOGIC_TYPE:
		return  *(short *)lpOpnd->values;
		break;
	case FLOAT_TYPE:
	case FLOAT_IDEN:
		return  (short)*(double *)lpOpnd->values;
		break;
	case NFIELD_IDEN:
		return  (short)atoi( subcopy((char *)*(long *)lpOpnd->oval, 0, lpOpnd->length) );
		break;
	case FFIELD_IDEN:
		return  (short)atof( subcopy((char *)*(long *)lpOpnd->oval, 0, lpOpnd->length) );
		break;
    
	//2000.6.6
	default:
	    {
		char buf[MAX_OPND_LENGTH+2];
		
		strZcpy(buf, lpOpnd->values, MAX_OPND_LENGTH);
		return  (short)atoi( buf );
	    }
    }

    ErrorSet.xERROR = iTypeNoCompt;
    return SHRT_MIN;

} //xGetOpndLong()



double PD_style xGetOpndFloat(OpndType *lpOpnd)
{
    switch( lpOpnd->type ) {
	case CHR_TYPE:
	case CHR_IDEN:
		return  (double)*(char *)lpOpnd->values;
		break;
	case LONG_TYPE:
	case LONG_IDEN:
		return  (double)*(long *)lpOpnd->values;
		break;
	case INT_TYPE:
	case INT_IDEN:
	case LOGIC_TYPE:
		return  (double)*(short *)lpOpnd->values;
		break;
	case FLOAT_TYPE:
	case FLOAT_IDEN:
		return  *(double *)lpOpnd->values;
		break;
	case NFIELD_IDEN:
		return  (double)atol( subcopy((char *)*(long *)lpOpnd->oval, 0, lpOpnd->length) );
		break;
	case FFIELD_IDEN:
		return  (double)atof( subcopy((char *)*(long *)lpOpnd->oval, 0, lpOpnd->length) );
		break;

	//2000.6.6
	default:
	    {
		char buf[MAX_OPND_LENGTH+2];
		
		strZcpy(buf, lpOpnd->values, MAX_OPND_LENGTH);
		return  atof( buf );
	    }
    }

    ErrorSet.xERROR = iTypeNoCompt;
    return  (double)(long)LONG_MIN;

} //xGetOpndFloat()



char * PD_style xGetOpndString(OpndType *lpOpnd)
{
    char  *s;
    char  buf[256];

    //use substr to hold the temp string
    if( lpOpnd->type >= FIELD_IDEN && lpOpnd->type <= SFIELD_IDEN ) {
	if( lpOpnd->type == MFIELD_IDEN ) {
	    dFIELDWHENACTION *p;
	    p = (dFIELDWHENACTION *)lpOpnd->oval;

	    if( p->pSourceDfile == NULL ) {
		s = "";
	    } else {
		s = SubstrOutBlank("", 0, 4096);	//alloc memory
		s[0] = '\x8';         	//8 blocks is need
		GetField(p->pSourceDfile, p->wSourceid, s);
	    }
	} else {
	    s = rtrim(subcopy((char *)*(long *)lpOpnd->oval, 0, lpOpnd->length));
	}
    } else {
	if( lpOpnd->type != STRING_TYPE )
	{  //type error
	    ErrorSet.xERROR = 1;
	    switch( lpOpnd->type ) {
		case CHR_TYPE:
		case CHR_IDEN:
		case LONG_TYPE:
		case LONG_IDEN:
			sprintf(buf, "%ld", *(long *)lpOpnd->values);
			break;
		case INT_TYPE:
		case INT_IDEN:
			sprintf(buf, "%d", *(short *)lpOpnd->values);
			break;
		case FLOAT_TYPE:
		case FLOAT_IDEN:
                        sprintf(buf, "%.2f", *(double *)lpOpnd->values);
			break;
		case DATE_TYPE:
			if( lpOpnd->oval == NULL ) {
				strZcpy(buf, lpOpnd->values, 9);
			} else {
				strZcpy(buf, lpOpnd->oval, 9);
			}
			break;
		default:
			return  NULL;

	    }
	    s = subcopy(buf, 0, (short)strlen(buf));
	    return  s;
	}

	if( lpOpnd->length >= MAX_OPND_LENGTH ) {
		if( lpOpnd->oval == NULL ) {
			s = SubstrOutBlank((char *)*(long *)lpOpnd->values,\
							0, lpOpnd->length );
			free((char *)*(long *)lpOpnd->values);
		} else {
			s = SubstrOutBlank(lpOpnd->oval,0,lpOpnd->length);
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


char PD_style xGetOpndChr(OpndType *lpOpnd)
{
    switch( lpOpnd->type ) {
	/*case CHR_TYPE:
	case CHR_IDEN:
	case LONG_TYPE:
	case LONG_IDEN:
	case INT_TYPE:
	case INT_IDEN:
	case LOGIC_TYPE:
		return  lpOpnd->values[0];
		break;
	*/
	case FLOAT_TYPE:
	case FLOAT_IDEN:
		return  (char)*(double *)lpOpnd->values;
		break;
	case NFIELD_IDEN:
		return  (char)atol( subcopy((char *)*(long *)lpOpnd->oval, 0, lpOpnd->length) );
		break;
	case FFIELD_IDEN:
		return  (char)atof( subcopy((char *)*(long *)lpOpnd->oval, 0, lpOpnd->length) );
		break;

	//2000.6.6
	default:
	    	return  lpOpnd->values[0];
    }

    ErrorSet.xERROR = iTypeNoCompt;
    return '\0';

} //xGetOpndChr()


/*********
 *                      xIsOpndField()
 **************************************************************************/
int PD_style xIsOpndField(OpndType *lpOpnd)
{
    if( lpOpnd->type < FIELD_IDEN || lpOpnd->type > SFIELD_IDEN )
	return  0;

    return  1;

} //xIsOpndField()


/*********
 *                      saveXexpEnv()
 **************************************************************************/
void saveXexpEnv( XEXP_ENV *xenv )
{
    xenv->optrStackFund = optrStackFund;
    xenv->opndStackFund = opndStackFund;
    xenv->optr_top = optr_top;
    xenv->opnd_top = opnd_top;

    optrStackFund = optr_top;
    opndStackFund = opnd_top;
}


/*********
 *                      restoreXexpEnv()
 **************************************************************************/
void restoreXexpEnv( XEXP_ENV *xenv )
{
    optrStackFund = xenv->optrStackFund;
    opndStackFund = xenv->opndStackFund;
    optr_top = xenv->optr_top;
    opnd_top = xenv->opnd_top;
}


#undef INrEXPRANALYSIS

/**************************** end of xexp.c ******************************/