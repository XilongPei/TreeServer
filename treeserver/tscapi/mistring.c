/*************
 *
 * string utility
 *
 * Writen by Xilong Pei   Sep. 12  1989
 *
 **************************************************************************/

#define __MISTRING_H_

#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif

#include "mistring.h"
#include "dir.h"
#include "wst2mt.h"

WSToMT static char *substring=NULL;
WSToMT static char subStaticStr[NORMAL_SUBSTR_LEN];

/*==============
 *  function substr
 *========================================================================*/

char *substr(char *source_string, int start_pos, int length)
{
   int i, source_len;
   char  *szResult;

   if( length < NORMAL_SUBSTR_LEN ) {
	szResult = subStaticStr;
   } else {
	if( length == SHRT_MAX ) {
		if( substring != NULL ) 	free( substring );
		return  substring = NULL;
	}
	if( (szResult = substring = (char *)realloc(substring, length+1)) == NULL )
		return NULL;
   }
   if( source_string != NULL )
	source_len = strlen( source_string );
   else
	source_len = 0;

   for(i=0; i<length; i++)
	 if( start_pos < source_len )
	   szResult[i] = source_string[start_pos++];
	 else
	   szResult[i] = ' ';

   szResult[i] = '\0';

   return  szResult;

} // end of substr()


/*============
 *  function csubstr get a sub-string from a Chinese string.
 *
 *=========================================================================*/

_declspec(dllexport) unsigned char *csubstr(unsigned char *source_string, \
		       short start_pos, \
		       short length)
{
   short i, source_len, chinese_mark;
   unsigned char *szResult;

   if( source_string != NULL )
	source_len = strlen( (char *)source_string );
   else
	source_len = 0;

   if( length < NORMAL_SUBSTR_LEN ) {
	szResult = subStaticStr;
   } else {
	if( length == SHRT_MAX ) {
		if( substring != NULL ) 	free( substring );
		return  substring = NULL;
	}
        if( (szResult = substring = (unsigned char *)realloc(substring, length+1)) == NULL )
		return  NULL;
   }

   if( CcharTest(source_string, start_pos) == 2 )     start_pos--;

   chinese_mark = 0;

   for(i=0; i<length; i++) {

	if( start_pos < source_len ) {
		if( chinese_mark == 1 )         chinese_mark = 2;
		else {
			if( source_string[start_pos] >= 0x80 ) chinese_mark = 1;
			else    chinese_mark = 0;
		}
		szResult[i] = source_string[start_pos++];
	}
	else
		szResult[i] = ' ';

   }

   if( chinese_mark == 1 )      szResult[i-1] = ' ';

   szResult[i] = '\0';

   return  szResult;
}



/*==================
 * function to test whether a char in a string is a chinese character:
 * the first char of chinese return 1, the second return 2
 * others 0; ( the chinese character's inside code is two bytes code
 * and the first bytes must bigger than 0x80
 * chinese_character_test, if test_byte is bigger than the length of string
 * return the last char's condition;
 *=========================================================================*/

_declspec(dllexport) short CcharTest(unsigned char *test_string, short test_byte)
{
   short chinese_mark;

   chinese_mark = 0;

   if( test_string == NULL )    return( 3 );

   test_byte++;
   while( ( *test_string ) && ( test_byte > 0 ) ) {

	test_byte--;

	if( chinese_mark == 1 )         chinese_mark = 2;
	else {
		if( *test_string >= 0x80 ) chinese_mark = 1;
		else    chinese_mark = 0;
	}

	test_string++;

   }

   if( test_byte )      return( 4 );      /* over the end 0 */

   return( chinese_mark );

}


/*
----------------------------------------------------------------------------
			SubstrOutBlank()
---------------------------------------------------------------------------*/
_declspec(dllexport) char *SubstrOutBlank(char *source_string, int start_pos, int length)
{
   int    i, source_len;
   char  *szResult;

   if( length < NORMAL_SUBSTR_LEN ) {
	szResult = subStaticStr;
   } else {
	if( length == SHRT_MAX ) {
		if( substring != NULL ) 	free( substring );
		return  substring = NULL;
	}
	if( (szResult = substring = (char *)realloc(substring, length+1)) == NULL )
		return NULL;
   }

   if( source_string != NULL )
	source_len = strlen( source_string );
   else
	source_len = 0;

   for(i = 0;   i < length && start_pos < source_len;  i++)
	szResult[i] = source_string[start_pos++];

   szResult[i] = '\0';

   return  szResult;

} //end of SubstrOutBlank()


/*==========
 *  string length function
 *  Input: string.
 *  Output: it return an odd integer.
 *==========================================================================*/

_declspec(dllexport) short StrTrimLen( char *source_string )
{
  short source_string_len;
  short i;

  source_string_len = strlen(source_string);

  for(i=source_string_len; i>=0 && source_string[i]==' '; i--)
	;

  return( (i%2==0) ? i : i+1 );

}


/*
----------------------------------------------------------------------------
			trim()
---------------------------------------------------------------------------*/
_declspec(dllexport) char *trim( char *s )
{
    char *t, *w;

    w = s;
    t = s;
    while( *w != '\0' ) {
	if( isCspace( *w ) ) w++;
	else	break;
    }
    while( *w != '\0' ) {
	if( isCspace( *w ) == 0 ) *t++ = *w++;
	else	break;
    }
    *t = '\0';
    return( s );
}


/*
-----------------------------------------------------------------------
			     MakeName()
-----------------------------------------------------------------------*/
_declspec(dllexport) char *MakeName( char *Name, const char *ExtName, const short NameMaxLen )
{
    char *Temp;
    char path[_MAX_PATH];

    if( Name == NULL || ExtName == NULL || strlen( ExtName ) > 3 || \
	    strchr( ExtName, '.' ) != NULL )
	    return( NULL );

    if( Name[0] == '\0' ) {
	strcpy(path, "UNTITLED.");
	strcat(path, ExtName);
	strncpy(Name, path, NameMaxLen);
	Name[NameMaxLen - 1] = '\0';
    } else {
	if( ( Temp = strrchr( trim( Name ), '\\' ) ) == NULL )     Temp = Name;
	if( strlen( Temp ) < 1 ) {
		if( (short)strlen( Name ) + 12 >= NameMaxLen )    return( NULL );
		strcat(Temp, "UNTITLED.");
		strcat(Temp, ExtName);
	}
	if( ( strrchr( Temp, '.' ) ) == NULL ) {
		if( strlen( Name ) <= NameMaxLen - strlen( ExtName ) )
		    strcat( strcat(Name, "." ), ExtName );
	}
    }

    _fullpath(path, Name, NameMaxLen );

    return strcpy( Name, path );

} /* end of function MakeName */


/*
----------------------------------------------------------------------------
			lrtrim()
---------------------------------------------------------------------------*/
_declspec(dllexport) unsigned char *lrtrim( unsigned char *s )
{
    register unsigned char *w;

    if( s == NULL )	return 	NULL;

    w = s + strlen( (char *)s ) - 1;

    while( w >= s ) {                        /* shrink end space */
	if( isCspace( *w ) )    w--;
	else if( *(unsigned short *)(w-1) == 0xA1A1 ) {  w--;    w--; }
	     else break;
    }
    *(++w) = '\0';
    w = s;
    while( *w != '\0' ) {       /* skip head space */
	if( isCspace( *w ) )    w++;
	else if( *(unsigned short *)w == 0xA1A1 ) {  w++;    w++; }
	     else break;
    }
    return (unsigned char *)strcpy( (char *)s, (char *)w );
}


/*
----------------------------------------------------------------------------
			ltrim()
---------------------------------------------------------------------------*/
_declspec(dllexport) unsigned char *ltrim( unsigned char *s )
{
    register unsigned char *w;

    if( s == NULL )	return 	NULL;

    w = s;
    while( *w != '\0' ) {       /* skip head space */
	if( isCspace( *w ) )    w++;
	else if( *(unsigned short *)w == 0xA1A1 ) {  w++;    w++; }
	     else break;
    }
    return (unsigned char *)strcpy( (char *)s, (char *)w );
}


/*
----------------------------------------------------------------------------
			rtrim()
---------------------------------------------------------------------------*/
_declspec(dllexport) unsigned char *rtrim( unsigned char *s )
{
    register unsigned char *w;

    if( s == NULL )	return 	NULL;

    w = s + strlen( (char *)s ) - 1;

    while( w >= s ) {                        // shrink end space
	if( isCspace( *w ) )    w--;
	else if( *(unsigned short *)(w-1) == 0xA1A1 ) {  w--;    w--; }
	     else break;
    }
    *(++w) = '\0';

    return s;
}


/*
----------------------------------------------------------------------------
			shrink()
---------------------------------------------------------------------------*/
_declspec(dllexport) unsigned char *shrink( unsigned char *s )
{
    register unsigned char *w, *wf;

    if( s == NULL )	return 	NULL;

    wf = w = s;


    while( *wf != '\0' ) {
	if( isCspace( *wf ) )    wf++;
	else if( *(unsigned short *)(wf) == 0xA1A1 ) {  wf++;    wf++; }
	     else *w++ = *wf++;
    }
    *w = '\0';

    return s;
}


/*---------------------------------------------------------------------------
 !                           TrimFileName()
 *--------------------------------------------------------------------------*/
_declspec(dllexport) char *TrimFileName( char *FileName )
{
    char drive[MAXDRIVE];
    char dir[MAXDIR];
    WSToMT static char file[MAXFILE];
    char ext[MAXEXT];

    if( FileName == NULL )      return  NULL;

    _splitpath(FileName, drive, dir, file, ext);

    return file;

} /* end of function TrimFileName() */



/****************
* find the char appear place of n times
****************************************************************************/
_declspec(dllexport) unsigned char *strnchr(unsigned char *str, int strlen, unsigned char c, int n )
{
    int i = 0;

    while( i < strlen ) {
	if( str[ i ] == c ) {
		if( --n == 0 )          break;
	}
	i++;
    }

    if( n == 0 )        return &str[i];

    return  NULL;

} // end of function strnchr()



/****************
* replace s2 with s3 in s1
****************************************************************************/
_declspec(dllexport) char *strrstr(char *s1,  char *s2, char *s3)
{
    char *sz = s1;
    int  i;

    char *s;

    s = strstr(s1, s2);

    if( s == NULL )     return  NULL;

    do {
	// abcdddefg replace ddd with aaabcd
	i = abs(strlen(s3)-strlen(s2));
	memmove(s+strlen(s3), s+strlen(s2), i);
	memcpy(s, s3, strlen(s3));
	*(s1 + strlen(s1) + strlen(s3) - strlen(s2)) = '\0';
	s1 = s + strlen(s3) - strlen(s2);
	s = strstr(s1, s2);
    } while( s != NULL );

    return  sz;

} // end of function strrstr()



/****************
* build s1 with s2
****************************************************************************/
_declspec(dllexport) char *strfill(char *s1, char *s2, short len)
{
    char *sz = s1;
    char *sm = s2;

    while( len > 0 ) {
	*s1++ = *s2++;
	if( *s2 == '\0' )       s2 = sm;
	len--;
    }

    *s1 = '\0';

    return sz;

} // end of function strfill


/****************
*     A powerful wordbreak function to break a string to many word,
*  copyed from MXSTRING.CPP
*
*  input:
*        string: string to break
*     breakchar: break chars to break string, such as ".,`{}*&^%$#@!"
*
*  output:
*     num  : return the word num
*     word : return the wordptr
*   return : first word ptr;
****************************************************************************/
_declspec(dllexport) char *WordBreak( char *string, int *num, char *word[], char *breakChar)
{
    char *rp;

    if ( string == NULL || word == NULL ) return NULL;
    *num = 0;
    rp = string;

    while ( *rp == ' ' || *rp == '\t' || strchr( breakChar, *rp ))
       {
       rp++; // skip Top space
       if ( *rp == '\0' ) break;
       }

    word[*num] = rp;

    while ( rp && *rp != 0 )
    {
	while( *rp != ' ' && (!strchr( breakChar, *rp )) && *rp ) rp++;

	if ( *rp )
	   {
	   *rp = '\0';
	   rp++;
	   }

	if ( *rp && strchr( breakChar, *rp )!=NULL ) rp++;

	while( *rp == ' ' && *rp ) rp++; // skip blanks

	if ( *rp && strchr( breakChar, *rp )!=NULL ) rp++;

	word[++(*num)] = rp;
     }
    return word[0];
}


/****************
* seperate string s into pieces, such as:
*  abc'def'asdafs' into abc   def   asdafs
*  is seed is NULL, alloc the pointer memory, else use seeds
*  to store it.
****************************************************************************/
_declspec(dllexport) char **seperateStr( char *s, char seperator, char **seeds )
{
	short count;
	char *sz;

	if( seeds == NULL ) {
		count = 2;
		sz = s;
		while( *sz ) {
			if( *sz++ == seperator )        count++;
		}
		if( (seeds = calloc(count, sizeof(char *))) == NULL ) {
			return  NULL;
		}
	} // end of for

	count = 0;
	seeds[count] = s;
	while( *s ) {
		if( *s == seperator )   {
			*s = '\0';
			seeds[++count] = ++s;
		} else {
			s++;
		}
	} // end of while
	seeds[++count] = NULL;

	return  seeds;

} // end of function seperateStr()


/****************
*   make filename with  pathname and filename
****************************************************************************/
_declspec(dllexport) char *makefilename( char *result, char *path, char *filename )
{
   char buf[MAXPATH];
   char file0[MAXFILE];
   char drive[MAXDRIVE];
   char dir[MAXDIR];
   char file[MAXFILE];
   char ext[MAXEXT];
   char ext0[MAXEXT];

   if( result == NULL || path == NULL || filename == NULL ) {
	return  NULL;
   }

   _splitpath(filename, drive, dir, file, ext);
   if( *drive != '\0' || *dir != '\0' ) {
	return  strcpy(result, filename);
   }

   strcpy(file0, file);
   strcpy(ext0, ext);
   _splitpath(path, drive, dir, file, ext);

   _makepath(buf, drive, dir, file0, ext0);

   return  strcpy( result, buf );

} // end of function makefilename()



/****************
*   make filename with  pathname and filename
****************************************************************************/
_declspec(dllexport) char *makeTrimFilename( char *result, char *path, char *filename )
{
   char buf[MAXPATH];
   char file0[MAXFILE];
   char drive[MAXDRIVE];
   char dir[MAXDIR];
   char file[MAXFILE];
   char ext[MAXEXT];
   char ext0[MAXEXT];

   if( result == NULL || path == NULL || filename == NULL ) {
	return  NULL;
   }

   _splitpath(filename, drive, dir, file, ext);

   strcpy(file0, file);
   strcpy(ext0, ext);
   _splitpath(path, drive, dir, file, ext);

   _makepath(buf, drive, dir, file0, ext0);

   return  strcpy( result, buf );

} // end of function makeTrimFilename()



/****************
*   make filename with  pathname and filename
****************************************************************************/
_declspec(dllexport) char *makeDefFilename( char *result, char *path, char *filename )
{
   char file0[MAXFILE];
   char drive[MAXDRIVE];
   char dir[MAXDIR];
   char file[MAXFILE];
   char ext[MAXEXT];
   char ext0[MAXEXT];

   if( result == NULL || path == NULL || filename == NULL ) {
	return  NULL;
   }

   _splitpath(filename, drive, dir, file, ext);

   strcpy(file0, file);
   strcpy(ext0, ext);
   _splitpath(path, drive, dir, file, ext);

   if( file[0] == '\0' && ext[0] == '\0' ) {
	_makepath(result, drive, dir, file0, ext0);
   } else {
	strcpy(result, path);
   }

   return  result;

} // end of function makeDefFilename()


/*
----------------------------------------------------------------------------
				isCspace()
---------------------------------------------------------------------------*/
_declspec(dllexport) short isCspace( unsigned short i )
{
    switch( i ) {
	case ' ':
	case '\t':
	case '\n':
	case '\r':
	case 0xA1A1:
		return  1;
    }

    return  0;

} // end of function isCspace()




/****************
*   			stoUpper()
****************************************************************************/
_declspec(dllexport) char *stoUpper( char *s )
{
    char *sz = s;
    char c;

    while( *sz )
    {
	c = *sz;
	if( c >= 'a' && c <= 'z' )
		*sz = c + ('A' - 'a');
	sz++;
    }

    return  s;

} // end of stoUpper()



/****************
*   			stoLower()
****************************************************************************/
_declspec(dllexport) char *stoLower( char *s )
{
    char *sz = s;
    char c;

    while( *sz )
    {
	c = *sz;
	if( c >= 'A' && c <= 'Z' )
		*sz = c + ('a' - 'A');
	sz++;
    }

    return  s;

} // end of stoLower()


/****************
*   change filename's ext
****************************************************************************/
_declspec(dllexport) char *changeFilenameExt( char *result, char *filename, char *newExt)
{
   char drive[MAXDRIVE];
   char dir[MAXDIR];
   char file[MAXFILE];
   char ext[MAXEXT];

   if( result == NULL || filename == NULL || newExt == NULL ) {
	return  NULL;
   }

   _splitpath(filename, drive, dir, file, ext);
   _makepath(result, drive, dir, file, newExt);

   return  result;

} // end of function makefilename()


/*==============
 *  function subcopy
 *
 * ! I am sure that the source is suit for this copy
 *========================================================================*/
char *subcopy(char *source_string, int start_pos, int length)
{
   int 	  i;
   char  *szResult;

   if( source_string == NULL )
       return  NULL;

   if( length < NORMAL_SUBSTR_LEN ) {
	szResult = subStaticStr;
   } else {
	if( length == SHRT_MAX ) {
		if( substring != NULL ) 	free( substring );
		return  substring = NULL;
	}
	if( (szResult = substring = (char *)realloc(substring, length+1)) == NULL )
		return NULL;
   }

   for(i=0; i<length; i++)
	   szResult[i] = source_string[start_pos++];

   szResult[i] = '\0';

   return  szResult;

} // end of subcopy()


/*
int main(int argc, char *argv[] )
{
	char s[80] = {"c:\\tg\\sub0.dbf"};
	char s1[80] = {"tgview.vew"};
	char s2[80];

	makefilename( s2, s, s1 );
	return  printf("%s\n", s2);

} // end of test main()

********/



/*                  end of string utility  software                       */
														       
