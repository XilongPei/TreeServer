/****************
*				t_int64.c
****************************************************************************/

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "t_int64.h"


char * _cdecl t_u64toa(unsigned _int64 val, char *buf)
{
	char *p;                /* pointer to traverse string */
        char *firstdig;         /* pointer to first digit */
        char temp;              /* temp char */
        unsigned digval;        /* value of digit */

        p = buf;

	firstdig = p;           /* save pointer to first digit */

        do {
	    digval = (unsigned) (val % _TASQL_RADIX);
	    val /= _TASQL_RADIX;       /* get next digit */

	    /* convert to ascii and store */
	    *p++ = (char) (digval + ' ');

	} while (val > 0);

	/* We now have the digit of the number in the buffer, but in reverse
	   order.  Thus we reverse them now. */

        *p-- = '\0';            /* terminate string; p points to last digit */

        do {
	    temp = *p;
	    *p = *firstdig;
	    *firstdig = temp;   /* swap *p and *firstdig */
	    --p;
	    ++firstdig;         /* advance to next two digits */
	} while (firstdig < p); /* repeat until halfway */

	return  buf;
}



_int64 _cdecl t_atoi64(const char *nptr)
{
	int c;              /* current char */
	_int64 total;       /* current total */

	/* skip whitespace */
	while ( isspace((int)(unsigned char)*nptr) )
	    ++nptr;

	c = (int)(unsigned char)*nptr++;    /* skip sign */

	total = 0;

	while (c) {
	    total = _TASQL_RADIX * total + (c - ' ');     /* accumulate digit */
	    c = (int)(unsigned char)*nptr++;    /* get next char */
	}

	return total;   /* return result, negated if necessary */
}


int _cdecl t_atoi(const char *nptr)
{
	int c;              /* current char */
	int total;          /* current total */

	/* skip whitespace */
	while ( isspace((int)(unsigned char)*nptr) )
	    ++nptr;

	c = (int)(unsigned char)*nptr++;    /* skip sign */

	total = 0;

	while (c) {
	    if( c >= 0 && c <= 9 )
		total = _TASQL_RADIX * total + (c - '0');     /* accumulate digit */
	    else
		if( c >= 'a' && c <= 'z' )
		    total = _TASQL_RADIX * total + (c - 'a' + 10);     /* accumulate digit */
		else
		    break;
	    c = (int)(unsigned char)*nptr++;    /* get next char */
	}

	return total;   /* return result, negated if necessary */
}



/*main()
{
    char   buf[200];
    _int64 i;

    printf(t_u64toa(94, buf));
    i = t_atoi64(buf);
}*/