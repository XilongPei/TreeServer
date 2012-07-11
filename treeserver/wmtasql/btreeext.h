/****************
* ^-_-^
* btreeext.h
* copyright (c) 1995, 1997
*****************************************************************************/

#ifndef _BTREEEXT_H_
#define _BTREEEXT_H_

#include "dio.h"

int keyFieldCmp(bHEAD *bh, char *fieldName);
short sKeyFieldCmp(bHEAD *bh, char **fieldName);
short keyFieldPartCmp(bHEAD *bh, char *fieldName);

#ifdef  __BTREEEXT_C__
_declspec(dllexport) bHEAD * IndexBuildKeyExpr(dFILE *df, \
					       char  *szExpr, \
					       short  wKeyLen, \
					       char  *ndxName);
#else
_declspec(dllimport) bHEAD * IndexBuildKeyExpr(dFILE *df, \
					       char  *szExpr, \
					       short  wKeyLen, \
					       char  *ndxName);
#endif

#endif