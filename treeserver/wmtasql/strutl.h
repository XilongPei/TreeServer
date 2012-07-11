/****************
*				strutl.h
****************************************************************************/

#ifndef __STRUTL_H__
#define __STRUTL_H__

#define  BLANK_CHARS	" \\()|+-*/"

#ifdef __IN_STRUTL_C_
_declspec(dllexport) char *strToSourceC( char *t, char *s, char turnChar );
_declspec(dllexport) char *cnStrToStr(char *t, char *s, char turnChar, short size);
_declspec(dllexport) char *strtostr(char *dest, char *s, short start, short len);
_declspec(dllexport) char *strModStr(char *dest, char *s, short start, short len);
_declspec(dllexport) char *lstrModStr(char *dest, char *s, short start, short len);
_declspec(dllexport) unsigned short strrchrCnt(const char *s, char c);
_declspec(dllexport) short strGcmp(const char *s1, const char *s2);
_declspec(dllexport) char *strModReplace( char *s, const char *sMod );
_declspec(dllexport) char *strZcpy(char *dest, const char *src, size_t maxlen);
_declspec(dllexport) char *textBreak(char *text, short lineLen);
_declspec(dllexport) char *eraseQuotation(char *s, char cQuotation);
_declspec(dllexport) unsigned char *strNputStr(unsigned char *str, int len, \
					unsigned char c, int n, char *szstr );
#else
_declspec(dllimport) char *strToSourceC( char *t, char *s, char turnChar );
_declspec(dllimport) char *cnStrToStr(char *t, char *s, char turnChar, short size);
_declspec(dllimport) char *strtostr(char *dest, char *s, short start, short len);
_declspec(dllimport) char *strModStr(char *dest, char *s, short start, short len);
_declspec(dllimport) char *lstrModStr(char *dest, char *s, short start, short len);
_declspec(dllimport) unsigned short strrchrCnt(const char *s, char c);
_declspec(dllimport) short strGcmp(const char *s1, const char *s2);
_declspec(dllimport) char *strModReplace( char *s, const char *sMod );
_declspec(dllimport) char *strZcpy(char *dest, const char *src, size_t maxlen);
_declspec(dllimport) char *textBreak(char *text, short lineLen);
_declspec(dllimport) char *eraseQuotation(char *s, char cQuotation);
_declspec(dllimport) unsigned char *strNputStr(unsigned char *str, int len, \
					unsigned char c, int n, char *szstr );
#endif

#endif