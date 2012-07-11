/****
*  mistring.H     Prototype and general define for MISTRING.C
*
*  Writen By Xilong Pei , 1991.10.15
****************************************************************************/

#ifndef _IncludedMISTRINGProgramHead_
#define _IncludedMISTRINGProgramHead_        "MSTR v1.00"

#define NORMAL_SUBSTR_LEN	64


char     *substr(char *source_string, short start_pos, short length);
unsigned char *csubstr(unsigned char *source_string, short start_pos, \
		       short length);
short    CcharTest(unsigned char *test_string, short test_byte);
char     *SubstrOutBlank(char *source_string, short start_pos, short length);
short    StrTrimLen( char *source_string );
char     *trim( char *s );
	/* cut the special character: " ", "\t", 
	 * at the head and middle of a string
	 */
char     *MakeName( char *Name, const char *ExtName, const short NameMaxLen );
	/* Make file name with ExtName, blank string return UNTITLED
	 */
unsigned char    *lrtrim( unsigned char *s );
	/* shrink the special character: " ", "\t", 0xA1A1(Chinese blank)
	 * at the head and tail of a string
	 */
unsigned char    *ltrim( unsigned char *s );
unsigned char    *rtrim( unsigned char *s );
unsigned char    *shrink( unsigned char *s );
	/* shrink the blank isspace
	*/
char    *TrimFileName( char *FileName );
	/*
	 */
unsigned char *strnchr(unsigned char *str, short strlen, unsigned char c, short n );
	/*get the place of char 'c' n times appeared in str
	*/
char *strrstr(char *s1,  char *s2, char *s3);
	/* replace string s2 with s3 in s1
	*/
char *strfill(char *s1, char *s2, short len);
	/*fill string s1 with s2 with 'len' length
	*/
char *WordBreak( char *string, int *num, char *word[], char *breakChar);
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

char **seperateStr( char *s, char seperator, char **seeds );
	/* seperate string s into pieces, such as:
	*  abc'def'asdafs' into abc   def   asdafs
	*  is seed is NULL, alloc the pointer memory, else use seeds
	*  to store it.
	*/
char *makefilename( char *result, char *path, char *filename );
char *makeTrimFilename( char *result, char *path, char *filename );
char *makeDefFilename( char *result, char *path, char *filename );

short isCspace( unsigned short i );

char *stoUpper( char *s );
char *stoLower( char *s );

char *changeFilenameExt( char *result, char *filename, char *newExt);

#endif

