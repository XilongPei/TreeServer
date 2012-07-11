/***************************************************************************\
 * ASQLANA.C
 * ^-_-^
 *
 * Author: Xilong Pei    2001.8.4
\***************************************************************************/

#include "asqlana.h"
#include "xexp.h"
#include "strutl.h"

short addOneVarToSybTab(char *szTextName, char *sp, int iSzLen)
{
    SysVarOFunType  *SysVar;
    int		     i;
    char            *sz1;

    if( (fFrTo.sHead = (SysVarOFunType *)realloc( fFrTo.sHead, \
					(fFrTo.nSybNum+1) * \
					sizeof(SysVarOFunType) ) ) == NULL )
	return  FALSE;

    SysVar = fFrTo.sHead;
    strZcpy(SysVar[fFrTo.nSybNum].VarOFunName, szTextName, 32);
    
    SysVar[fFrTo.nSybNum].type = STRING_IDEN;
    SysVar[fFrTo.nSybNum].length = iSzLen;

    if( i < MAX_OPND_LENGTH ) {
	strZcpy(SysVar[fFrTo.nSybNum].values, sp, iSzLen);    
    } else {
	if( (sz1 = malloc(i+16)) == (long)NULL ) {
	    qsError = 1005;
	    return  FALSE;
	}
	*(long *)(SysVar[fFrTo.nSybNum].values) = (long)sz1;
	strZcpy(sz1, sp, iSzLen)
    }
    
    fFrTo.nSybNum++;

    return  TRUE;

} //addOneVarToSybTab()








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
	tscp.len = MAX_PKG_MSG_LEN;
	tscp.lp = MAX_PKG_MSG_LEN;
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