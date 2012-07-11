/****
*  charutl.c
*
*  Writen By Xilong Pei , 1998
****************************************************************************/

//#define __CHARUTL__TEST__

#include <ctype.h>
#include "charutl.h"


/****************
* '\xFF'
*****************************************************************************/
char *strGetChar( char *s, char *cp )
{
    int num = 0;
    int i;

    if( *s != '\\' ) {
	*cp = *s++;
	return  s;
    }

    *s = toupper( *(++s) );
    if( *s == 'X' ) {
	s++;
	for(i = 0;  i < 2 && *s;  i++ ) {
	    *s = toupper(*s);
	    if( *s >= 'A' && *s <= 'Z' )
		num = num * 16 + (*s - 'A' + 10 );
	    else if( isdigit(*s) )
		num = num * 16 + (*s - '0') ;
	    s++;
	}
    } else {
	for(i = 0;  i < 3 && *s;  i++ ) {
	    if( isdigit(*s) ) {
		num = num * 10 + (*s - '0') ;
	    } else {
		break;
	    }
	    s++;
	}
    }
    *cp = num;

    return s;

}

#ifdef __CHARUTL__TEST__
main()
{
    char *s = "\\x31Aasd";
    char *sz;
    char  c;

    sz = strGetChar(s, &c );

    printf("%c %s\n", c, sz);

}
#endif