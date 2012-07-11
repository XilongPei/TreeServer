/****************
* dbftotal.h
* 1998.12.24
*****************************************************************************/

#ifndef __DBFTOTAL_H__
#define __DBFTOTAL_H__

#ifdef __DBFTOTAL_C__
_declspec(dllexport) int dbftotal(char *szDft, char *szDfs);
#else
_declspec(dllimport) int dbftotal(char *szDft, char *szDfs);
#endif

#endif
