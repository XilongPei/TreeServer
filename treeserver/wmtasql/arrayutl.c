/****
*  arrayutl.H
*
*  Writen By Xilong Pei , 1995.5.4
****************************************************************************/

#include <LIMITS.H>

#include "arrayutl.h"

/****************
*                 		arrayUnion()
* num not included the end mark SHRT_MIN
****************************************************************************/
short *arrayUnion(short *art, short *ars, short num)
{
    int i, j;

    for( i = 0;  ars[i] != SHRT_MIN;  i++ ) {
	for( j = 0;  art[j] != SHRT_MIN;  j++ ) {
		if( ars[i] == art[j] ) 	break;
	}

	// if not found the string
	if( art[j] == SHRT_MIN ) {
		if( j >= num )	return  art;
		art[j] = ars[i];
		art[j+1] = SHRT_MIN;
	}
    } // end of for

    return  art;

} // end of function arrayUnion()


/****************
*                 		iarrayShort()
* find a number in array
****************************************************************************/
short iarrayShort( short *array, short num, short digit )
{
    short  i;

    if( digit < num && array[digit] == digit ) {
	return  digit;
    }

    for( i = 0;  i < num;  i++ ) {
	if( array[i] == digit ) {
		return  i;
	}
    }

    return  -1;

} // end of iarrayShort()


/*

main()
{
	short ip1[]={1, 2, 3, SHRT_MIN, 0, 0, 0, 0};
	short ip2[]={2, SHRT_MIN};

	arrayUnion(ip1, ip2, 7);
}

*/