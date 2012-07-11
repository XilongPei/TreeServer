/****************
*  		password.c
***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "password.h"

char *EncryptPasswd( char *passwd )
{
    char *s;
    short i;
    static char secretStr[] = {8, 6, 1, 9, 4, 7, 11, 15, 2, 3, 9, 13, \
			       11, 9, 7, 3};

    if( passwd == NULL ) 	return  NULL;

    s = passwd;

    i = 0;
    while( *s ) {
	if( i )	*s = toupper( *s );
	else	*s = tolower( *s );
	i = 1 - i;
	s++;
    }

    s = passwd;

    i = 0;
    while( *s ) {
	*s -= secretStr[i];
	if( i++ >= 16 ) 	i = 0;
	s++;
    }

    return  passwd;

} // end of function EncryptPasswd()