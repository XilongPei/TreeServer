/****************
*				strutl.c
****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define __IN_STRUTL_C_

#include "strutl.h"

_declspec(dllexport) char *strToSourceC( char *t, char *s, char turnChar )
{
    char *sz = t;

    while( *s != '\0' ) {
	if( *s == turnChar ) {
		*sz++ = turnChar;
		*sz++ = turnChar;
	} else {
		*sz++ = *s;
	}
	s++;
    } // end of while
    *sz = '\0';

    return  t;

}  // end of function strToSourceC()

/****************
*				strtostr()
****************************************************************************/
_declspec(dllexport) char *strtostr(char *dest, char *s, short start, short len)
{
    short i;

    i = strlen( s );
    if( len > i )	len = i;

    i = strlen( dest );
    if( i < start + len )	dest[start + len] = '\0';

    return  (char *)memcpy(dest+start, s, len);

} // end of strtostr()


/****************
*				strModStr()
****************************************************************************/
_declspec(dllexport) char *strModStr(char *dest, char *s, short start, short len)
{
    unsigned short i;

    if( dest == NULL || s == NULL || len <= 0 || start < 0 )	return  NULL;

    i = strlen( dest );

    // if the dest string is not longer enough, add ' ' at tail
    if( i < start ) {
	memset(dest+i, ' ', start-i);
    }
    memcpy(dest+start, s, len);

    if( i < start + len ) 	dest[start+len] = '\0';

    return  dest;

} // end of strModStr()


/****************
*				lstrModStr()
****************************************************************************/
_declspec(dllexport) char *lstrModStr(char *dest, char *s, short start, short len)
{
    unsigned short i;

    if( dest == NULL || s == NULL )	return  NULL;

    i = strlen( dest );
    // if the dest string is not longer enough, add ' ' at tail
    if( i < start ) {
	memset(dest+i, '*', start-i);
    }
    memcpy(dest+start, s+start, len);

    if( i < start + len ) 	dest[start+len] = '\0';
    for( start--;   start >= 0;   start-- ) {
	if( dest[start] == '*' )	dest[start] = s[start];
    }

    return  dest;

} // end of lstrModStr()


/****************
*				strrchrCnt()
****************************************************************************/
_declspec(dllexport) unsigned short strrchrCnt(const char *s, char c)
{
    unsigned short i;
    char           *sz;

    if( ( i = strlen( s ) ) <= 0 )	return  0;

    sz = (char *)s + i - 1;

    i = 0;
    while( sz != s && *sz == c ) {
	sz--;
	i++;
    }

    return  i;

} // end of strrchrCnt()


/****************
*				strGcmp()
****************************************************************************/
_declspec(dllexport) short strGcmp(const char *s1, const char *s2)
{
    short i, len;

    len = strlen( s1 );

    for( i = 0;  i < len;  i++ )
    {
	if( s1[i] != s2[i] && s1[i] != '*' || s2[i] != '*' ) 	break;
    }

    return  s1[i] - s2[i];

} // end of function strGcmp()

/****************
*				strModReplace()
* Function:
*     replace a string s with sMod but the char is '*' in sMod
****************************************************************************/
_declspec(dllexport) char *strModReplace( char *s, const char *sMod )
{
    short i, len, sLen;

    sLen = strlen(s);
    len = max( sLen, (short)strlen(sMod) );

    for( i = 0;  i < len && sMod[i] != '\0';  i++ )
    {
	if( sMod[i] != '*' || i > sLen )	s[i] = sMod[i];
    }

    s[len] = '\0';

    return  s;

} // end of function strModReplace()


/****************
*				strNcpy()
* Function:
****************************************************************************/
_declspec(dllexport) char *strZcpy(char *dest, const char *src, size_t maxlen)
{
    maxlen--;
    dest[maxlen] = '\0';
    return  strncpy(dest, src, maxlen);
}


/****************
*				textBreak)
* Function:
****************************************************************************/
_declspec(dllexport) char *textBreak(char *text, short lineLen)
{
    char  *retTxt;
    short i, thisLen, retLen, textLen, maxRetLen;

    if( text == NULL )	return  NULL;
    textLen = strlen(text);
    if( textLen <= 0 ) {		// the text is too long or 0 char
	return  NULL;
    }

    maxRetLen = textLen+ textLen/lineLen + 128;
    retTxt = malloc(maxRetLen + 16);
    if( retTxt == NULL )
	return  NULL;

    thisLen = retLen = i = 0;
    while( 1 ) {

	if( thisLen + lineLen < textLen ) {
		// get the break position
		for(i = thisLen+lineLen;   i >= thisLen;  i-- ) {
			if( strchr(BLANK_CHARS, text[i]) != NULL )
				break;
		}
		if( i < thisLen )	i = thisLen + lineLen - 2;

		//copy the text out
		for( ;  thisLen <= i;  thisLen++ ) {
			retTxt[retLen++] = text[thisLen];
		}
		retTxt[retLen++] = '\\';
		retTxt[retLen++] = '\x0D';
		retTxt[retLen++] = '\x0A';
		if( retLen >= maxRetLen ) {
			free(retTxt);
			return  NULL;
		}
	} else {
		for( ;  thisLen < textLen;  thisLen++ ) {
			retTxt[retLen++] = text[thisLen];
		}
		retTxt[retLen] = '\0';
		break;
	}
    }

    return  retTxt;

} // end of textBreak()


_declspec(dllexport) char *cnStrToStr(char *t, char *s, char turnChar, short size)
{
    short i = 0;

    size--;		//for hold tail 0
    while( *s != '\0' && *s != '"' && i < size ) {
	if( *s == turnChar ) {
	   switch( *(++s) ) {
		case 't':
			t[i++] = '\t';
			break;
		case 'n':
			t[i++] = '\n';
			break;
		case 'F':
			t[i++] = '\xFF';
			break;
		case '"':
			t[i++] = '"';
			break;

		case '\0':
			t[i] = '\0';
			return  t;

		case 'x':
		case 'X':
		{
			int j, num = 0;

			s++;
			for(j = 0;  j < 2 && *s;  j++ ) {
			    *s = toupper(*s);
			    if( *s >= 'A' && *s <= 'Z' )
				num = num * 16 + (*s - 'A' + 10 );
			    else if( isdigit(*s) )
				num = num * 16 + (*s - '0') ;
			    s++;
			}

			t[i++] = num;
			continue;
		}
		break;

		case '0':	case '1':	case '2':
		case '3':	case '4':	case '5':
		case '6':	case '7':	case '8':
		case '9':
		{
			int j, num = 0;

			for(j = 0;  j < 3 && *s;  j++ ) {
			    if( isdigit(*s) ) {
				num = num * 10 + (*s - '0') ;
			    } else {
				break;
			    }
			    s++;
			}

			t[i++] = (unsigned char)num;
			continue;
		}
		break;

		default:
			t[i++] = *s;
			break;
	   }
	} else {
		t[i++] = *s;
	}
	s++;
    } // end of while

    t[i] = '\0';

    return  t;

}  // end of function strToSourceC()


_declspec(dllexport) char *eraseQuotation(char *s, char cQuotation)
{
    int i;
    if( s == NULL )
	return  NULL;

    i = strlen(s);
    if( s[0] == cQuotation && s[i-1] == cQuotation ) {
	i -= 2;
	memmove(s, s+1, i);
	s[i] = '\0';
    }
    return  s;

} //end of eraseQuotation()


/*main()
{	char *s = "I will have a baby";
	char buf[80];

	strcpy(buf, "**");
	strModStr(buf, s, 3, strlen(s));
	strtostr(s, "can ", 2, 4);

void main( void )
{
    char *text = "the people's republic of China";
    char *s;

    s = textBreak(text, 7);
    puts(s);
}
} */


/****************
*				strNputStr
*
* str: datasource
* len: mem size
* c  : seperate char
* n  : the n times appearences
* szstr : data
****************************************************************************/
_declspec(dllexport) unsigned char *strNputStr(unsigned char *str, int len, \
					unsigned char c, int n, char *szstr )
{
    int   i, i1;

    if( n < 0 )
	return  NULL;
    if( n == 0 )
	return  str;

    i = i1 = 0;

    while( str[i] && i < len ) {
	if( str[ i ] == c ) {
		if( --n == 0 )          break;
		if( n == 1 ) {
			i1 = i;
		}
	}
	i++;
    }

    if( n != 0 ) {
	while( i < len && --n ) {
	    i1 = i;
	    str[ i++ ] = c;
	}
	strncpy(&str[i1+1], szstr, len-i1);
	str[len-1] = '\0';
    } else {
	char *sm;

	//
	//a,b,c,d,e,f,g,h,i,j
	//     ^ ^
	//     | +------ i
	//     +-------- i1
	sm = strdup( &str[i] );
	if( sm == NULL )
	    return  NULL;

	if( i1 != 0 )
	    str[i1++] = c;

	strncpy(&str[i1], szstr, len-i1);
	str[len-1] = '\0';

	i = i1 + strlen(szstr);
	if( len > i )
	    strncpy(str, sm, len-i);

	free(sm);
    }

    return  str;

} //end of strNputStr()

/*
main()
{
    char str[4096] = "pei,shanghai,a";

    strNputStr(str, 4096, ',', 5, "szstr" );
    printf(str);
}*/
