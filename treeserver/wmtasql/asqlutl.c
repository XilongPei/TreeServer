/****************
* asqlutl.c
* copyright (c)
* author: Xilong Pei
* Function:
*     askLittleTaskGen(): write FromToCondition base program
*****************************************************************************/
#include <stdio.h>
#include <time.h>
#include "asqlana.h"
#include "asqlutl.h"

short askLittleTaskGen(char *fileName, char *szFrom, char *szTo, \
		       char *szCon, char *szAct)
{
    FILE *fp;
    //char *sz;	  // By NiuJingyu

    fp = fopen(fileName, "wt");
    if( fp == NULL )
    {  //ask file cannot be generate
	return  1;
    }

    if( fprintf(fp, "/*^-_-^*/\nFROM %s\nTO %s\nCONDITION\nBEGIN\n%s$%s\nEND\n", \
			    szFrom, szTo, szCon, szAct) < 0 )
    { //write error
	fclose(fp);
	return  2;
    }

    fclose(fp);

    return  0;

} //end of askLittleTaskGen(()


/****************
* asqlDbCommit()
*
* explain:
*     asqlDbCommit() is used only when the FROM is updating, so we lock
* them just. but this can hang up the threads!
****************************************************************************/
long asqlDbCommit( void )
{
    extern WSToMT FromToStru fFrTo;
    long   l = 0;
    int    i;
    dFILE  *df;
    int    tryCount = 10;

asqlDbCommitRetry:
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

        goto asqlDbCommitRetry;
    }

    fseek(fFrTo.phuf, 0, SEEK_SET);
    while( 1 )
    {
        fread(&df, sizeof(dFILE *), 1, fFrTo.phuf);
        if( df == NULL )
            break;

        fread(&(df->rec_p), sizeof(long), 1, fFrTo.phuf);
        fread(df->rec_buf, df->rec_len, 1, fFrTo.phuf);
	put1rec( df );
        l++;
    }
    dflush(df);

    for(i = 0;   i < fFrTo.cSouDbfNum;   i++) {
        wmtDbfUnLock(fFrTo.cSouFName[i]);
    }

    return  l;

} //end of asqlDbCommit()




/*==============
*                               getRealDbfId()
*===========================================================================*/
dFILE *getRealDbfId(short id, short *newid)
{
    int  i;
    extern WSToMT FromToStru fFrTo;

    for(i = 0;  i < fFrTo.cSouDbfNum;  i++)
    {
	if( id > fFrTo.cSouFName[i]->field_num )
	    id -= fFrTo.cSouFName[i]->field_num;
	else
	    break;
    }

    *newid = id;
    if( i < fFrTo.cSouDbfNum )
	return  fFrTo.cSouFName[i];

    return  NULL;

} //end of getRealDbfId()




/*==============
*                               getRealDbfIdByName()
*===========================================================================*/
dFILE *getRealDbfIdByName(char *fldname, short *newid)
{
    int  i;
    extern WSToMT FromToStru fFrTo;

    for(i = 0;  i < fFrTo.cSouDbfNum;  i++)
    {
	if( (*newid = GetFldid(fFrTo.cSouFName[i], fldname)) != 0xFFFF )
	    break;
    }

    if( i < fFrTo.cSouDbfNum )
	return  fFrTo.cSouFName[i];

    return  NULL;

} //end of getRealDbfIdByName()




/*************************** end of asqlutl.c *******************************/