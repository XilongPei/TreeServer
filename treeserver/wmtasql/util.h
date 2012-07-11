#ifndef _INCLUDE_UTIL_
#define _INCLUDE_UTIL_ 
#include <stdio.h>
#include <stdlib.h>	//for max and min

typedef unsigned short ushort ;
typedef unsigned char  uchar  ;
#define EOS '\0'
//chinese character bit mask
#define CCBITMASK  0x80
#define loByte(w)    (((uchar *)&w)[0])
#define hiByte(w)    (((uchar *)&w)[1])

typedef enum {
	False , True
}Boolean ;

typedef struct pkgPoint{
	short x;
	short y;
}Point;
#define Size Point

typedef struct pkgRect {
	Point a ;
	Point b ;
}Rect ;
Rect initRect( short , short ,short , short );


//short  max(short , short );
//short  min(short , short );

void fexpand( char * );

char hotKey( const char *s );
ushort ctrlToArrow( ushort );
char getAltChar( ushort keyCode );
ushort getAltCode( char ch );

ushort historyCount( uchar id );
const char *historyStr( uchar id, short index );
void historyAdd( uchar id, const char * );

int cstrlen( const char * );

typedef struct pkgView View ;

void *sendMessage( View *receiver, ushort what, ushort command, void *infoPtr );
Boolean lowMemory();

char *newStr( const char * );

Boolean driveValid( char drive );

Boolean isDir( const char *str );

Boolean pathValid( const char *path );

Boolean validFileName( const char *fileName );

void getCurDir( char *dir );

Boolean isWild( const char *f );

ushort ctrlToArrow(ushort keyCode);

void readBytes(FILE *is , char *buf , ushort size);
void writeBytes(FILE *os , char *buf , ushort size);
void putLong(long val ,FILE *os);
long getLong( FILE *is );

#endif  // __UTIL_H

