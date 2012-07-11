/****
*  mistring.H     Prototype and general define for MISTRING.C
*
*  Writen By Xilong Pei , 1991.10.15
****************************************************************************/

#ifndef _IncludedMISTRINGProgramHead_
#define _IncludedMISTRINGProgramHead_        "MSTR v1.00"

#define NORMAL_SUBSTR_LEN	4096

#ifdef __MISTRING_H_
_declspec(dllexport) char     *substr(char *source_string, int start_pos, int length);
_declspec(dllexport) unsigned char *csubstr(unsigned char *source_string, short start_pos, \
		       short length);
_declspec(dllexport) short    CcharTest(unsigned char *test_string, short test_byte);
_declspec(dllexport) char     *SubstrOutBlank(char *source_string, int start_pos, int length);
_declspec(dllexport) short    StrTrimLen( char *source_string );
_declspec(dllexport) char     *trim( char *s );
	/* cut the special character: " ", "\t",
	 * at the head and middle of a string
	 */
_declspec(dllexport) char     *MakeName( char *Name, const char *ExtName, const short NameMaxLen );
	/* Make file name with ExtName, blank string return UNTITLED
	 */
_declspec(dllexport) unsigned char    *lrtrim( unsigned char *s );
	/* shrink the special character: " ", "\t", 0xA1A1(Chinese blank)
	 * at the head and tail of a string
	 */
_declspec(dllexport) unsigned char    *ltrim( unsigned char *s );
_declspec(dllexport) unsigned char    *rtrim( unsigned char *s );
_declspec(dllexport) unsigned char    *shrink( unsigned char *s );
	/* shrink the blank isspace
	*/
_declspec(dllexport) char    *TrimFileName( char *FileName );
	/*
	 */
_declspec(dllexport) unsigned char *strnchr(unsigned char *str, int strlen, unsigned char c, int n );
	/*get the place of char 'c' n times appeared in str
	*/
_declspec(dllexport) char *strrstr(char *s1,  char *s2, char *s3);
	/* replace string s2 with s3 in s1
	*/
_declspec(dllexport) char *strfill(char *s1, char *s2, short len);
	/*fill string s1 with s2 with 'len' length
	*/
_declspec(dllexport) char *WordBreak( char *string, int *num, char *word[], char *breakChar);
	/****************
	* A powerful wordbreak function to break a string to many word,
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
	*/

_declspec(dllexport) char **seperateStr( char *s, char seperator, char **seeds );
	/* seperate string s into pieces, such as:
	*  abc'def'asdafs' into abc   def   asdafs
	*  is seed is NULL, alloc the pointer memory, else use seeds
	*  to store it.
	*/
_declspec(dllexport) char *makefilename( char *result, char *path, char *filename );
_declspec(dllexport) char *makeTrimFilename( char *result, char *path, char *filename );
_declspec(dllexport) char *makeDefFilename( char *result, char *path, char *filename );
_declspec(dllexport) short isCspace( unsigned short i );
_declspec(dllexport) char *stoUpper( char *s );
_declspec(dllexport) char *stoLower( char *s );
_declspec(dllexport) char *changeFilenameExt( char *result, char *filename, char *newExt);
_declspec(dllexport) char *subcopy(char *source_string, int start_pos, int length);
#else
_declspec(dllimport) char     *substr(char *source_string, int start_pos, int length);
_declspec(dllimport) unsigned char *csubstr(unsigned char *source_string, short start_pos, \
		       short length);
_declspec(dllimport) short    CcharTest(unsigned char *test_string, short test_byte);
_declspec(dllimport) char     *SubstrOutBlank(char *source_string, int start_pos, int length);
_declspec(dllimport) short    StrTrimLen( char *source_string );
_declspec(dllimport) char     *trim( char *s );
	/* cut the special character: " ", "\t",
	 * at the head and middle of a string
	 */
_declspec(dllimport) char     *MakeName( char *Name, const char *ExtName, const short NameMaxLen );
	/* Make file name with ExtName, blank string return UNTITLED
	 */
_declspec(dllimport) unsigned char    *lrtrim( unsigned char *s );
	/* shrink the special character: " ", "\t", 0xA1A1(Chinese blank)
	 * at the head and tail of a string
	 */
_declspec(dllimport) unsigned char    *ltrim( unsigned char *s );
_declspec(dllimport) unsigned char    *rtrim( unsigned char *s );
_declspec(dllimport) unsigned char    *shrink( unsigned char *s );
	/* shrink the blank isspace
	*/
_declspec(dllimport) char    *TrimFileName( char *FileName );
	/*
	 */
_declspec(dllimport) unsigned char *strnchr(unsigned char *str, int strlen, unsigned char c, int n );
	/*get the place of char 'c' n times appeared in str
	*/
_declspec(dllimport) char *strrstr(char *s1,  char *s2, char *s3);
	/* replace string s2 with s3 in s1
	*/
_declspec(dllimport) char *strfill(char *s1, char *s2, short len);
	/*fill string s1 with s2 with 'len' length
	*/
_declspec(dllimport) char *WordBreak( char *string, int *num, char *word[], char *breakChar);
	/****************
	* A powerful wordbreak function to break a string to many word,
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
	*/

_declspec(dllimport) char **seperateStr( char *s, char seperator, char **seeds );
	/* seperate string s into pieces, such as:
	*  abc'def'asdafs' into abc   def   asdafs
	*  is seed is NULL, alloc the pointer memory, else use seeds
	*  to store it.
	*/
_declspec(dllimport) char *makefilename( char *result, char *path, char *filename );
_declspec(dllimport) char *makeTrimFilename( char *result, char *path, char *filename );
_declspec(dllimport) char *makeDefFilename( char *result, char *path, char *filename );
_declspec(dllimport) short isCspace( unsigned short i );
_declspec(dllimport) char *stoUpper( char *s );
_declspec(dllimport) char *stoLower( char *s );
_declspec(dllimport) char *changeFilenameExt( char *result, char *filename, char *newExt);
_declspec(dllimport) char *subcopy(char *source_string, int start_pos, int length);
#endif

#endif

