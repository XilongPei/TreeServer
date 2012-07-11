/****************
* str_gram.c
* 1999.5.19
*
* ^-_-^ Xilong Pei
****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "strutl.h"

//
// locate a keyword in buf
//
// return the position
//
//
char *locateKeywordInBuf(char *sz, char *szkey, char *szFollowKey, int ibufsize)
{
    int		   i;
    char           buf[128];
    char           *sp;

    while( 1 ) {
	while( *sz == ' ' || *sz == '\t' )
	    sz++;

	if( *sz == '\0' )
	    return  NULL;

	//try to get a word in a line
	i = strcspn(sz, " \t\n\r");
	strZcpy(buf, sz, min(i+1,32));

	if( stricmp(buf, szkey) == 0 )
	{
	    //get the end of line as szFollowKey

	    sp = sz;

	    // akeyword\0
	    // ^.......^
	    // sz      sz+i

	    sz += i;
	    while( *sz == ' ' || *sz == '\t' )
		sz++;
	    i = strcspn(sz, "\n\r");
	    strZcpy(szFollowKey, sz, min(ibufsize, i+1));

	    return  sp;
	}

	sz += i;

	//skip to next line
	while( *sz != '\n' && *sz != '\r' && *sz )
	    sz++;
	while( *sz == '\n' || *sz == '\r' )
	    sz++;
    }

    return  NULL;

} //end of locateKeywordInBuf()
