/****************
*				uvutl.h
* copyright (c) 1994
****************************************************************************/

#ifndef __UVUTL_H__
#define __UVUTL_H__

void groupInsert( Group *p, View *v1, ... );
ushort mMessageBox( char *msg, ushort aOptions );
void mMessageBar( char *msg );
void cursorBack( void );
void defView(View *aView);
long uvDupFile(char *sfile, char *tfile);
void popupSysMenu( void );

#endif
