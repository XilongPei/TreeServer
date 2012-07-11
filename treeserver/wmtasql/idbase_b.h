#if !defined(__IDBASE_H)
#define __IDBASE_H

#include <windows.h>
#ifndef _IncludedDIOProgramHead_
    #include "dio.h"
#endif

#define SQL_TABLE	0	/* dbf-type -> sql Table  */

#ifndef NAMELEN
   #define NAMELEN      256      /* Table name length */
#endif

#ifndef FIELDNAMELEN
   #define FIELDNAMELEN 32
#endif

#define DBFCHAR 	0
#define DBFINT 		1
#define DBFDEC 		2
#define DBFDATE 	3

#define MAX_OPEN_TABLE  20

#define	SQL_OK			0
#define ALLOC_MEM_ERROR		1
#define NULL_POINTER		2
#define NO_SQL_HANDLE           5

#define STRSZ 	256
#define DATELEN 8

#ifndef __dFIELD_
#define __dFIELD_
typedef struct {
	unsigned char 	field[FIELDNAMELEN+1];
	unsigned char 	fieldtype;
	unsigned char 	fieldlen;
	unsigned char 	fielddec;
	LPSTR		fieldstart;
} dFIELD;
#endif

_declspec(dllexport) long   TOpenDataBase (
	LPSTR DateBase,
	LPSTR UserName,
	LPSTR Password );

_declspec(dllexport) BOOL   TCloseDataBase ( void );

_declspec(dllexport) dFILE *  TsqlOpen (
	LPSTR sqlName,
	char *sqlStatement );

_declspec(dllexport) BOOL   TsqlClose (
	 dFILE *Table );

_declspec(dllexport) BOOL   TsqlSeek (
	dFILE *Table,
	long pos,
	int FromWhere );

_declspec(dllexport) BOOL   TsqlEOF (
	dFILE *Table );

_declspec(dllexport) LPSTR   TsqlGetRow (
	dFILE *Table );

LPSTR   sqlGetField (
	dFILE *Table,
	int Field_ID,
	LPSTR FieldDataBuf );

/* Unexport Functions */
BOOL   ReType (
	void *Desc,
	dFIELD far *Field,
	int *FieldPos,
	int *sqlDataBufSize,
	int *DataBufSize );

int   GetSql_ID ( void );

void   FreeSql_ID (
	int IDNumber );

_declspec(dllexport) long  TgetSqlErrCode( void );
_declspec(dllexport) void  TresetSqlErrCode( void );

#endif // __IDBASE_H
