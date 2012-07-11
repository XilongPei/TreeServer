{*****************
 * SQL_DIO.PAS
 * comes from sql_dio.h
 * a common interface between DIO and other datasource,
 * such as:informix Esql/C
 *         Borland BDE
 * designed at: 1998.4.16
 ***************************************************************************}

uses Windows;

//dbf-type -> sql Table
consts
 SQL_TABLE = 0;

function OpenDataBase(DateBase,UserName,Password:PCHAR):LONGINT;
function CloseDataBase():BOOL;
function sqlOpen (sqlName:PCHAR; sqlStatement:PCHAR): PCHAR;
function sqlClose(Table:PCHAR)LBOOL;
function sqlSeek(Table:PCHAR; pos:LONGINT; FromWhere:Integer):BOOL
function sqlEOF(Table:PCHAR):BOOL;
function sqlGetRow(Table:PCHAR):PCHAR;
function getSqlErrCode():Integer;
function resetSqlErrCode():Integer;
