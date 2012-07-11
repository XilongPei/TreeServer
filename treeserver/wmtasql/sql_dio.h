/*****************
 * SQL_DIO.H
 * a common interface between DIO and other datasource,
 * such as:informix Esql/C
 *         Borland BDE
 * designed at: 1998.4.16
 ***************************************************************************/

#ifndef __SQL_DIO_H_
#define __SQL_DIO_H_

#include "dio.h"

//dbf-type -> sql Table
#define SQL_TABLE	0

_declspec(dllimport) long TOpenDataBase (
	LPSTR DateBase,
	LPSTR UserName,
	LPSTR Password );

_declspec(dllimport) BOOL TCloseDataBase ( void );

_declspec(dllimport) dFILE *TsqlOpen (
	LPSTR sqlName,
	char *sqlStatement );

_declspec(dllimport) BOOL TsqlClose (
	 dFILE *Table );

_declspec(dllimport) BOOL TsqlSeek (
	dFILE *Table,
	long pos,
	int FromWhere );

_declspec(dllimport) BOOL TsqlEOF (
	dFILE *Table );

_declspec(dllimport) LPSTR TsqlGetRow (
	dFILE *Table );

_declspec(dllimport) long TgetSqlErrCode( void );
_declspec(dllimport) void TresetSqlErrCode( void );

#endif
