/*********
 * asql_xml.c
 *
 * copyright (c) Xilong Pei, 2001
 *               
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


DWORD charSrvWriteExchngBuf( char *buf, int *iBufLen, char c);

 /*------------
 !                      _ASK_ARRAY2XML()
 ! protocol: xarray2xml(Array, szFmtStr, szXslFileName, isZeroOut)
 !-----------------------------------------------------------------------*/
// #pragma argsused /* Remed by NiuJingyu */
short PD_style _ASK_Array2XML( OpndType *lpOpnd, short ParaNum, \
			    short *OpndTop, short *CurState )
{

    ArrayType *p;
    int	       ii;
    double     ff;
    int        i, j;
    char       buf[4096];
    char       szFmtStr[256];
    TS_COM_PROPS tscp;
    char       *s;
    int		isZeroOut;
    char	lpsz[4096];

    if( *CurState != ONE_MODALITY_ACTION ) {
	*OpndTop -= ParaNum;
	return  0;
    }

    if( lpOpnd[0].type != ARRAY_TYPE ) {
	*OpndTop -= ParaNum;
	return  1;
    }

    s = xGetOpndString(&lpOpnd[1]);
    if( *s != '\0' )
	strZcpy(szFmtStr, s, 256);
    else
	strcpy(szFmtStr, "%f");

    isZeroOut = xGetOpndLong(&lpOpnd[3]);


    //write out the head
    //-------------------------------------------------------------------------
    s = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\n";
    //memset(&tscp, 0, sizeof(TS_COM_PROPS));
    tscp.leftPacket = '\1';	//no use: there are any other packet to transmit
    tscp.endPacket = '\1';	//not end, DONNOT GIVE UP
    tscp.packetType = 'R';
    tscp.msgType = 'W';
    tscp.len = min(strlen(s), MAX_PKG_MSG_LEN);
    tscp.lp = (long)tscp.len;
    memcpy(lpsz, &tscp, sizeof(TS_COM_PROPS));
    memcpy(&lpsz[sizeof(TS_COM_PROPS)], s, tscp.len);
    if( SrvWriteExchngBuf((EXCHNG_BUF_INFO *)wmtExbuf, lpsz, \
					tscp.len+sizeof(TS_COM_PROPS)) != 0 )
    {
	//agent shell error
	*OpndTop -= ParaNum;    /* maintain the opnd stack */
	return  1;
    }

    //write out the XSL information
    //-------------------------------------------------------------------------
    s = xGetOpndString(&lpOpnd[2]);
    if( s != NULL && *s != '\0' ) {
	tscp.len = min(sprintf(buf, "<?xml-stylesheet type=\"text/xsl\" href=\"%s\" ?>\n", s), MAX_PKG_MSG_LEN);
	tscp.lp = (long)tscp.len;
	memcpy(lpsz, &tscp, sizeof(TS_COM_PROPS));
	memcpy(&lpsz[sizeof(TS_COM_PROPS)], buf, tscp.len);
	if( SrvWriteExchngBuf((EXCHNG_BUF_INFO *)wmtExbuf, lpsz, \
					tscp.len+sizeof(TS_COM_PROPS)) != 0 )
	{
	    //agent shell error
	    *OpndTop -= ParaNum;    /* maintain the opnd stack */
	    return  1;
	}
    }
    
    //write out the head of <ResultSet>
    //-------------------------------------------------------------------------
    s = "<ResultSet>\n";
    tscp.len = min(strlen(s), MAX_PKG_MSG_LEN);
    tscp.lp = (long)tscp.len;
    memcpy(lpsz, &tscp, sizeof(TS_COM_PROPS));
    memcpy(&lpsz[sizeof(TS_COM_PROPS)], s, tscp.len);
    if( SrvWriteExchngBuf((EXCHNG_BUF_INFO *)wmtExbuf, lpsz, \
					tscp.len+sizeof(TS_COM_PROPS)) != 0 )
    {
	//agent shell error
	*OpndTop -= ParaNum;    /* maintain the opnd stack */
	return  1;
    }
    

    p = (ArrayType *)*(long *)lpOpnd[0].oval;

    for( i = 0;  i < p->ArrayDim[0];  i++ ) {

	//write out the head of <row>
	//-------------------------------------------------------------------------
	ii = sprintf(buf, "<row id=\"Y%d\">\n", i+1);
	tscp.len = min(ii, MAX_PKG_MSG_LEN);
	tscp.lp = (long)tscp.len;
	memcpy(lpsz, &tscp, sizeof(TS_COM_PROPS));
	memcpy(&lpsz[sizeof(TS_COM_PROPS)], buf, tscp.len);
	if( SrvWriteExchngBuf((EXCHNG_BUF_INFO *)wmtExbuf, lpsz, \
					tscp.len+sizeof(TS_COM_PROPS)) != 0 )
	{
	    //agent shell error
	    *OpndTop -= ParaNum;    /* maintain the opnd stack */
	    return  1;
	}
		

	for( j = 0;  j < p->ArrayDim[1];  j++ ) {

		   buf[0] = '\0';
		   switch( p->ElementType ) {
			case CHR_TYPE:
				ii = (int)*((char *)p->ArrayMem + j + i * p->ArrayDim[1] );
				if( isZeroOut || ii != 0 )
				    sprintf(buf, "<col id=\"X%d\" value=\"%d\"/>\n", j+1 ,ii);
				break;
			case INT_TYPE:
				ii = (int)*(short *)( (char *)p->ArrayMem + 2 * ( j + i * p->ArrayDim[1] ) );
				if( isZeroOut || ii != 0 )
				    sprintf(buf, "<col id=\"X%d\" value=\"%d\"/>\n", j+1 ,ii);				 
				break;
			case LONG_TYPE:
				ii = *(long *)( (char *)p->ArrayMem + 4 * ( j + i * p->ArrayDim[1] ) );
				if( isZeroOut || ii != 0 )
				    sprintf(buf, "<col id=\"X%d\" value=\"%d\"/>\n", j+1 ,ii);				 
				break;
			case FLOAT_TYPE:
			    {
				char  buff[256];

				ff = *(double *)( (char *)p->ArrayMem + sizeof(double) * ( j + i * p->ArrayDim[1] ) );
				sprintf(buff, szFmtStr, ff);

				if( isZeroOut || ff > FLOAT_PRECISION || ff < -FLOAT_PRECISION )
				    sprintf(buf, "<col id=\"X%d\" value=\"%s\"/>\n", j+1, buff);
			    }
				break;
			default:
				ErrorSet.xERROR = iNoMatchArray;       /* error dim type */
				return 1;
		   } /* end of switch */

		    //write out the head of <row>
		    //-------------------------------------------------------------------------
		    tscp.len = min(strlen(buf), MAX_PKG_MSG_LEN);
		    tscp.lp = (long)tscp.len;
		    memcpy(lpsz, &tscp, sizeof(TS_COM_PROPS));
		    memcpy(&lpsz[sizeof(TS_COM_PROPS)], buf, tscp.len);
		    if( SrvWriteExchngBuf((EXCHNG_BUF_INFO *)wmtExbuf, lpsz, \
					tscp.len+sizeof(TS_COM_PROPS)) != 0 )
		    {
			//agent shell error
			*OpndTop -= ParaNum;    /* maintain the opnd stack */
			return  1;
		    }

	} /* end of for(j) */

	//write out the tail of </row>
	//-------------------------------------------------------------------------
	s = "</row>\n";
	tscp.len = min(strlen(s), MAX_PKG_MSG_LEN);
	tscp.lp = (long)tscp.len;
	memcpy(lpsz, &tscp, sizeof(TS_COM_PROPS));
	memcpy(&lpsz[sizeof(TS_COM_PROPS)], s, tscp.len);
	if( SrvWriteExchngBuf((EXCHNG_BUF_INFO *)wmtExbuf, lpsz, \
					tscp.len+sizeof(TS_COM_PROPS)) != 0 )
	{
	    //agent shell error
	    *OpndTop -= ParaNum;    /* maintain the opnd stack */
	    return  1;
	}

    } // end of for(i)

    //write out the tail of <ResultSet>
    //-------------------------------------------------------------------------
    s = "</ResultSet>\n";
    tscp.len = min(strlen(s), MAX_PKG_MSG_LEN);
    tscp.lp = (long)tscp.len;
    memcpy(lpsz, &tscp, sizeof(TS_COM_PROPS));
    memcpy(&lpsz[sizeof(TS_COM_PROPS)], s, tscp.len);
    if( SrvWriteExchngBuf((EXCHNG_BUF_INFO *)wmtExbuf, lpsz, \
					tscp.len+sizeof(TS_COM_PROPS)) != 0 )
    {
	//agent shell error
	*OpndTop -= ParaNum;    /* maintain the opnd stack */
	return  1;
    }

    *OpndTop -= ParaNum;

    return 0;

} /* end of function _ASK_Array2XML() */



/*------------
 !                      _ASK_ARRAY2HTML()
 ! protocol: xarray2html(Array, szFmtStr, szHtml, isZeroOut[, szFillStr])
 ! html $[1,2] ... $[2,3] ......$[1]......
 ! replace the $[1,2] with the value of $[1,2]
 !-----------------------------------------------------------------------*/
// #pragma argsused /* Remed by NiuJingyu */
short PD_style _ASK_Array2HTML( OpndType *lpOpnd, short ParaNum, \
			    short *OpndTop, short *CurState )
{

    ArrayType *p;
    int	       ii;
    double     ff;
    int        i, j;
    char       buf[4096];
    char       szFmtStr[256];
    char       *s;
    int		isZeroOut;
    char	lpsz[4096];
    int         iLpsz = 0;
    char       *szFillStr = NULL;
    int         slen;

    if( *CurState != ONE_MODALITY_ACTION ) {
	*OpndTop -= ParaNum;
	return  0;
    }

    if( lpOpnd[0].type != ARRAY_TYPE ) {
	*OpndTop -= ParaNum;
	return  1;
    }

    if( ParaNum > 4 ) {
	s = xGetOpndString(&lpOpnd[4]);
	if( s != NULL && *s != '\0' ) {
	    szFillStr = strdup(s);
	    if( szFillStr == NULL ) {
			strcpy(ErrorSet.string, "xarray2html()");
			ErrorSet.xERROR = iNoMem;
			*OpndTop -= ParaNum;		//maintain the opnd stack
			return  1;
	    }
	    slen = strlen(szFillStr);
	}
    }

    s = xGetOpndString(&lpOpnd[1]);
    if( *s != '\0' )
	strZcpy(szFmtStr, s, 256);
    else
	strcpy(szFmtStr, "%f");

    isZeroOut = xGetOpndLong(&lpOpnd[3]);
    p = (ArrayType *)*(long *)lpOpnd[0].oval;    

    //write out the HTML information
    //-------------------------------------------------------------------------
    s = xGetOpndString(&lpOpnd[2]);
    if( s != NULL && *s != '\0' ) {
	while( *s ) {

	    //here we can check the next char of s, for it can be accessed
	    //even it is '\0'
	    if( *s != '$' || *(s+1) != '[' ) {
		if( charSrvWriteExchngBuf( lpsz, &iLpsz, *s) != 0 )
		{
		    *OpndTop -= ParaNum;    /* maintain the opnd stack */
		    return  1;
		}
		s++;
	    } else {
		
		//skip $[
		s++;
		s++;

		//test wether there is string to use
		//if there is ',' before ']'
		if( szFillStr != NULL || isalpha(*s) ) {
		    char *sp = s;

		    while( *sp ) {
			if( *sp == ',' || *sp == ']' ) {
				break;
			}
			sp++;
		    } //end of while

		    if( *sp == ']' ) {

		       int  k;

		       k = sp - s + 1;
		       if( k >= 4096 )
			       k = 4096;

		       strZcpy(buf, s, k);
		       trim(buf);


		       if( stricmp(buf, "xml") == 0 ) {	
			  //export the XML result
/////////////////////////////////////////////////////////////////////////////////////
{  //end of write XML
    char	*sz;
    int	         ii, ij;

    //write out the head
    //-------------------------------------------------------------------------
    sz = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\n";
    for(i = 0;   sz[i] != '\0';   i++) {
	if( charSrvWriteExchngBuf( lpsz, &iLpsz, sz[i]) != 0 )
	{
	    if( szFillStr != NULL )
		free( szFillStr );
	    *OpndTop -= ParaNum;    /* maintain the opnd stack */
	    return  1;
	}
    }

    //write out the head of <ResultSet>
    //-------------------------------------------------------------------------
    sz = "<ResultSet>\n";
    for(i = 0;   sz[i] != '\0';   i++) {
	if( charSrvWriteExchngBuf( lpsz, &iLpsz, sz[i]) != 0 )
	{
	    if( szFillStr != NULL )
		free( szFillStr );
	    *OpndTop -= ParaNum;    /* maintain the opnd stack */
	    return  1;
	}
    }
    
    for( ij = 0;  ij < p->ArrayDim[0];  ij++ ) {

	//write out the head of <row>
	//-------------------------------------------------------------------------
	ii = sprintf(buf, "<row id=\"Y%d\">\n", i+1);
	for(i = 0;   buf[i] != '\0';   i++) {
	    if( charSrvWriteExchngBuf( lpsz, &iLpsz, buf[i]) != 0 )
	    {
		if( szFillStr != NULL )
			free( szFillStr );
		*OpndTop -= ParaNum;    /* maintain the opnd stack */
		return  1;
	    }
	}

	for( j = 0;  j < p->ArrayDim[1];  j++ ) {

		   buf[0] = '\0';
		   switch( p->ElementType ) {
			case CHR_TYPE:
				ii = (int)*((char *)p->ArrayMem + j + ij * p->ArrayDim[1] );
				sprintf(buf, "<col id=\"X%d\" value=\"%d\"/>\n", j+1 ,ii);
				break;
			case INT_TYPE:
				ii = (int)*(short *)( (char *)p->ArrayMem + 2 * ( j + ij * p->ArrayDim[1] ) );
				sprintf(buf, "<col id=\"X%d\" value=\"%d\"/>\n", j+1 ,ii);				 
				break;
			case LONG_TYPE:
				ii = *(long *)( (char *)p->ArrayMem + 4 * ( j + ij * p->ArrayDim[1] ) );
				sprintf(buf, "<col id=\"X%d\" value=\"%d\"/>\n", j+1 ,ii);				 
				break;
			case FLOAT_TYPE:
				ff = *(double *)( (char *)p->ArrayMem + sizeof(double) * ( j + ij * p->ArrayDim[1] ) );
				sprintf(buf, "<col id=\"X%d\" value=\"%.4f\"/>\n", j+1, ff);
				break;
			default:
				ErrorSet.xERROR = iNoMatchArray;       /* error dim type */
				return 1;
		   } /* end of switch */

		    //write out the head of <row>
		    //-------------------------------------------------------------------------
		    for(i = 0;   buf[i] != '\0';   i++) {
			if( charSrvWriteExchngBuf( lpsz, &iLpsz, buf[i]) != 0 )
			{
			    if( szFillStr != NULL )
				free( szFillStr );
			    *OpndTop -= ParaNum;    /* maintain the opnd stack */
			    return  1;
			}
		    }
	} /* end of for(j) */

	//write out the tail of </row>
	//-------------------------------------------------------------------------
	sz = "</row>\n";
	for(i = 0;   sz[i] != '\0';   i++) {
	    if( charSrvWriteExchngBuf( lpsz, &iLpsz, sz[i]) != 0 )
	    {
		if( szFillStr != NULL )
			free( szFillStr );
		*OpndTop -= ParaNum;    /* maintain the opnd stack */
		return  1;
	    }
	}

    } // end of for(i)

    //write out the tail of <ResultSet>
    //-------------------------------------------------------------------------
    sz = "</ResultSet>\n";
    for(i = 0;   sz[i] != '\0';   i++) {
	if( charSrvWriteExchngBuf( lpsz, &iLpsz, sz[i]) != 0 )
	{
	    if( szFillStr != NULL )
		free( szFillStr );
	    *OpndTop -= ParaNum;    /* maintain the opnd stack */
	    return  1;
	}
    }

} //end of write XML

    buf[0] = '\0';

/////////////////////////////////////////////////////////////////////////////////////
		       } else {
			  sscanf(s, "%d", &i);
			  if( i <= 0 ) {
				if( szFillStr != NULL )
				    free( szFillStr );
				sprintf(ErrorSet.string, "xarray2html()ÖÐ×Ö·û´®Ìî³ä->[%d]", i);
				ErrorSet.xERROR = iErrArrayUse;	//access array overload their dim
				*OpndTop -= ParaNum;		//maintain the opnd stack
				return  1;
			  }

			  buf[0] = '\0';
			  sp = strnchr(szFillStr, slen, *szFillStr, i);
			  if( sp != NULL ) {
			    sp++;
			    for(i = 0;  *sp != '\0' && i < 4095;    sp++,  i++ ) {
				if( *sp == *szFillStr ) {
					break;
				}
				buf[i] = *sp;
			    }
			    buf[i] = '\0';
			  }
		       } //end of else
			
		       goto XARRAY2HTML_SKIP;

		    }
		}

		


		sscanf(s, "%d,%d", &i, &j);
		if( i > p->ArrayDim[0] || j > p->ArrayDim[1] || i <= 0 || j <= 0 ) {
			if( szFillStr != NULL )
				free( szFillStr );
			sprintf(ErrorSet.string, "->[%d,%d]", i, j);
			ErrorSet.xERROR = iErrArrayUse;	//access array overload their dim
			*OpndTop -= ParaNum;		//maintain the opnd stack
			return  1;
		}

		i--;
		j--;
		
		buf[0] = '\0';
		switch( p->ElementType ) {
			case CHR_TYPE:
				ii = (int)*((char *)p->ArrayMem + j + i * p->ArrayDim[1] );
				if( ii != 0 )
				    sprintf(buf, "%d", ii);
				else
				    if( isZeroOut == 2 )
					strcpy(buf, "&nbsp;");
				    else if( isZeroOut == 1 )
					strcpy(buf, "0");
				break;
			case INT_TYPE:
				ii = (int)*(short *)( (char *)p->ArrayMem + 2 * ( j + i * p->ArrayDim[1] ) );
				if( ii != 0 )
				    sprintf(buf, "%d", ii);				 
				else
				    if( isZeroOut == 2 )
					strcpy(buf, "&nbsp;");
				    else if( isZeroOut == 1 )
					strcpy(buf, "0");
				break;
			case LONG_TYPE:
				ii = *(long *)( (char *)p->ArrayMem + 4 * ( j + i * p->ArrayDim[1] ) );
				if( ii != 0 )
				    sprintf(buf, "%d", ii);				 
				else
				    if( isZeroOut == 2 )
					strcpy(buf, "&nbsp;");
				    else if( isZeroOut == 1 )
					strcpy(buf, "0");
				break;
			case FLOAT_TYPE:
			    {
				char  buff[256];

				ff = *(double *)( (char *)p->ArrayMem + sizeof(double) * ( j + i * p->ArrayDim[1] ) );
				sprintf(buff, szFmtStr, ff);

				if( ff > FLOAT_PRECISION || ff < -FLOAT_PRECISION )
				    sprintf(buf, "%s", buff);
				else
				    if( isZeroOut == 2 )
					strcpy(buf, "&nbsp;");
				    else if( isZeroOut == 1 )
					sprintf(buf, "%s", buff);
				}
				break;
			default:
				if( szFillStr != NULL )
					free( szFillStr );
				ErrorSet.xERROR = iNoMatchArray;       /* error dim type */
				return 1;
		} /* end of switch */

XARRAY2HTML_SKIP:

		for(i = 0;   buf[i] != '\0';   i++) {
		    if( charSrvWriteExchngBuf( lpsz, &iLpsz, buf[i]) != 0 )
		    {
			if( szFillStr != NULL )
				free( szFillStr );
			*OpndTop -= ParaNum;    /* maintain the opnd stack */
			return  1;
		    }
		}

		while( *s ) {
		    if( *s == ']' ) {
			s++;
			break;
		    }
		    s++;
		} //end of while

	    } //end of else
	} //end of while
	
	if( charSrvWriteExchngBuf( lpsz, &iLpsz, 0) != 0 )
	{
	    if( szFillStr != NULL )
		free( szFillStr );
	    *OpndTop -= ParaNum;    /* maintain the opnd stack */
	    return  1;
	}

    } else {
	if( szFillStr != NULL )
		free( szFillStr );
	ErrorSet.xERROR = iNoMatchFun;
	return 1;
    }

    if( szFillStr != NULL )
	free( szFillStr );
    *OpndTop -= ParaNum;

    return 0;

} /* end of function _ASK_Array2HTML() */






//
DWORD charSrvWriteExchngBuf( char *buf, int *iBufLen, char c)
{
    char	 lpsz[4096];
    TS_COM_PROPS tscp;
    DWORD        dw;

    if( *iBufLen < MAX_PKG_MSG_LEN && c != 0 ) {
        buf[*iBufLen] = c;
	(*iBufLen)++;
    } else {
	tscp.leftPacket = '\1';	//no use: there are any other packet to transmit
	tscp.endPacket = '\1';	//not end, DONNOT GIVE UP
	tscp.packetType = 'R';
	tscp.msgType = 'W';

	if( c == '\0' ) {
	    tscp.len = min(*iBufLen, MAX_PKG_MSG_LEN);
	    tscp.lp = tscp.len;
	} else {
	    tscp.len = MAX_PKG_MSG_LEN;
	    tscp.lp = MAX_PKG_MSG_LEN;
	}
	memcpy(lpsz, &tscp, sizeof(TS_COM_PROPS));
	memcpy(&lpsz[sizeof(TS_COM_PROPS)], buf, tscp.len);
	dw = SrvWriteExchngBuf((EXCHNG_BUF_INFO *)wmtExbuf, lpsz, tscp.len+sizeof(TS_COM_PROPS));
	if( dw != 0 ) {
	    return  dw;
	}

        buf[0] = c;
	*iBufLen = 1;
    }

    return  0;

} //charSrvWriteExchngBuf()


/****************************** asql_xml.c ***********************************/
