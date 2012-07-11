/****************
*  ParaBrk.C
*  author: Richard.J
*
*  copyright (c) Shanghai Tiedao University MIS Research, 1996
****************************************************************************/

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <memory.h>
#else
#include <mem.h>
#endif
#include "parabrk.h"

#define TEST
#define TASK_PARA_MAXLEN    516
#ifdef TEST

#include <stdio.h>
#endif

char **paraBreak( const char *str1, const char *str2, int *strNum )
{
	static char *str;
	int  i, trueStrLen = 0, atomLen = sizeof(char *), strLen, tmpNum;
	int  flag = 0, trueStrNum = 0;
	char   buf[TASK_PARA_MAXLEN], *p, *p1, *firstStr;
	long   address;

	p = p1 = buf;
	i = atomLen * (*strNum+1) + TASK_PARA_MAXLEN;
	str = (char *)malloc( i );
	if ( str == NULL ) return NULL;
	memset(str, 0, i);

	strcpy( buf, str1 );
	if ( str2 != NULL )     strcat( buf, str2 );
	firstStr = str + atomLen * (*strNum + 1);
	for ( i = 0; i < *strNum; i++ )
	{
		if ( *p == '`' ) {
			p++;
			for( p1 = p; *p1 != '\0' && *p1 != '`'; p1++ );
			if ( *p1 == '\0' )   flag = 1;
			else   *p1 = '\0';
			strLen = strlen( p );
		}
		else {
			for ( ; *p != '`'; p++ );
			*p = '\0';
			p++;
			strLen = atoi( p1 );
			p1 = p + strLen;
			if ( *p1 == '\0' )   flag = 1;
			else   *p1 = '\0';
		}
		p1++;
		address = (long)(str + atomLen * (*strNum + 1) + trueStrLen);
		memcpy( str + atomLen * i, &address, atomLen );
		strcpy( firstStr + trueStrLen, p );
		trueStrLen += strLen + 1;
		p = p1;
		trueStrNum++;
		if ( flag ) break;
	}

	tmpNum = i;
	if ( flag ) tmpNum++;
	i = (*strNum - trueStrNum) * atomLen;
	memmove( str + atomLen * (trueStrNum + 1), firstStr, i );
	p = ( char *)realloc(str, atomLen * (*strNum) + trueStrLen);
	*strNum = tmpNum;
	//if ( p == NULL ) return (char **)str;
	//else 			 return (char **)p;
	return (char **)p;
}

/*
void main( void )
{
	char **str;
	//char *str1 = "`asd`0``4`char``cha`6`kkkkkk", *str2;
	char str1[80], *str2;
	int i = 0, num = 20;

	printf( "Input the string:" );
	gets( str1 );

	str2 = NULL;
	str = (char **)paraBreak( str1, str2, &num );

	printf( "\nthe number of string: %d\n", num );
	for ( ; *(str + i) != NULL; )
	{
		puts( *(str + i ) );
		i++;
	}
	printf( "The Number of this string is : %d", num );

	free( (char *)str );
}
*/
