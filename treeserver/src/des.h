/******************************************************************************
*
*                             DES.H   V1.0
*
*
* Author        :-
* System        :-
* Last modified :-
* Card type     :-
*
* Copyright     :
* notice        : All rights reserved.
******************************************************************************/

#ifndef __DES_H__
#define __DES_H__

#include <windows.h>

/*
#ifdef __DES_C__
_declspec(dllexport) void DES(LPCSTR key, LPCSTR text, LPBYTE mtext);

_declspec(dllexport) void _DES(LPCSTR key, LPCSTR text, LPBYTE mtext);
#else
_declspec(dllimport) void DES(LPCSTR key, LPCSTR text, LPBYTE mtext);

_declspec(dllimport) void _DES(LPCSTR key, LPCSTR text, LPBYTE mtext);
#endif
*/

void DES(LPCSTR key, LPCSTR text, LPBYTE mtext);
void _DES(LPCSTR key, LPCSTR text, LPBYTE mtext);


#endif