/************************************************************
*	IDBASE.EC
*		----Informix DataBase interface module
*
*	Copyright: MISR Shanghai Tiedao University, 1997.3
*
*	Author:    JingYuNiu
**************************************************************/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <ctype.h>

#include "idbase.h"
#include "sqlproto.h"
#include "datetime.h"
#include "value.h"

//#define DEBUG_INFO

$include sqlca;
$include login.h;
$include sqltypes.h;

static long	i_ErrCode = SQL_OK;
static int sqlIdTable[MAX_OPEN_TABLE];

long WINAPI OpenDataBase ( LPSTR DataBase, LPSTR UserName, LPSTR Password )
{
	#if defined(DEBUG_INFO)
	{
		char DebugInfo[80];
		wsprintf( (LPSTR)DebugInfo, "OpenDataBase" );
		MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
	}
	#endif

	InetLogin.cHost = -1;

	if( lstrlen( UserName ) != 0 )
	{
		lstrcpy( (LPSTR)InetLogin.User, UserName );
		InetLogin.cUser = lstrlen( UserName );
	}
	else
		InetLogin.cUser = -1;

	if( lstrlen( Password ) != 0 )
	{
		lstrcpy( (LPSTR)InetLogin.Pass, Password );
		InetLogin.cPass = lstrlen( Password );
	}
	else
	  InetLogin.cPass = -1;

	_iqdbase( DataBase, 0 );

	i_ErrCode = (long)sqlca.sqlcode;

	if ( sqlca.sqlcode != 0)
	{
		sqlexit();
		return ( (long)sqlca.sqlcode );
	}

	return SQL_OK;
}

BOOL WINAPI CloseDataBase ( void )
{
	#if defined(DEBUG_INFO)
	{
		char DebugInfo[80];
		wsprintf( (LPSTR)DebugInfo, "CloseDataBase" );
		MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
	}
	#endif

	_iqdbclose();
	sqlexit();
	i_ErrCode = SQL_OK;
	return TRUE;
}

dFILE *WINAPI sqlOpen ( sqlName, sqlStatement )
LPSTR sqlName;
$char *sqlStatement;
{
	struct sqlda far *Desc = (struct sqlda *)NULL;
	struct sqlvar_struct far	*col;
	int 	*FieldPos;
	dFILE	*Table;
	int  	i, size, sqlIdNumber, DataBufSize, sqlDataBufSize;
	$char   Query_Id[IDLEN+1], sqlCursor[CURSORNAMELEN+1];

	#if defined(DEBUG_INFO)
	{
		char DebugInfo[80];
		wsprintf( (LPSTR)DebugInfo, "sqlOpen" );
		MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
	}
	#endif

	if( ( sqlIdNumber = GetSql_ID() ) == -1 )
	{
		i_ErrCode = NO_SQL_HANDLE;
		return NULL;
	}

	wsprintf( (LPSTR)Query_Id, "ID%d", sqlIdNumber );
	wsprintf( (LPSTR)sqlCursor, "CS%d", sqlIdNumber );

	#if defined(DEBUG_INFO)
	{
		char DebugInfo[80];
		wsprintf( (LPSTR)DebugInfo, "sql_ID: %s\nsqlCursor: %s", Query_Id, sqlCursor );
		MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
	}
	#endif

	$PREPARE $Query_Id FROM $sqlStatement;
	i_ErrCode = (long)sqlca.sqlcode;
	if( sqlca.sqlcode != 0 )
		return( NULL );

	$DESCRIBE $Query_Id INTO Desc;
	i_ErrCode = (long)sqlca.sqlcode;
	if( sqlca.sqlcode != 0 )
		return( NULL );

	#if defined(DEBUG_INFO)
	{
		char DebugInfo[80];
		wsprintf( (LPSTR)DebugInfo, "DescAddress: %ld", (long)Desc );
		MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
	}
	#endif

	if( ( Table = (dFILE *)malloc( sizeof(dFILE) ) ) == NULL )
	{
		i_ErrCode = ALLOC_MEM_ERROR;
		return( NULL );
	}

	memset( Table, 0, sizeof(dFILE) );
	lstrcpy( (LPSTR)Table->name, (LPCSTR)sqlName );
	lstrcpy( (LPSTR)Table->sql_id, (LPCSTR)Query_Id );
	lstrcpy( (LPSTR)Table->sqlCursor, (LPCSTR)sqlCursor );
	Table->sqlDesc = Desc;
	Table->dbf_flag = SQL_TABLE;
	Table->field_num = Desc->sqld;
	Table->rec_no = 1;
	Table->rec_p = 1;

	if( ( Table->field = malloc( (size_t) (Table->field_num+1)*sizeof(dFIELD) ) ) == NULL )
	{
		free( Table );
		i_ErrCode = ALLOC_MEM_ERROR;
		return( NULL );
	}

	if( ( FieldPos = (int *)malloc( (Table->field_num+1)*sizeof(int) ) ) == NULL )
	{
		free( Table->field );
		free( Table );
		i_ErrCode = ALLOC_MEM_ERROR;
		return( NULL );
	}

	ReType( Desc, Table->field, FieldPos, &sqlDataBufSize, &DataBufSize );
	#if defined(DEBUG_INFO)
	{
		char DebugInfo[80];
		wsprintf( (LPSTR)DebugInfo, "sqlDataBufSize: %d\nDataBufSize: %d\n",
					sqlDataBufSize, DataBufSize );
		MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
	}
	#endif

	Table->rec_len = DataBufSize;

	if( ( Table->rec_buf = (LPSTR)malloc( DataBufSize ) ) == NULL )
	{
		free( FieldPos );
		free( Table->field );
		free( Table );
		i_ErrCode = ALLOC_MEM_ERROR;
		return( NULL );
	}

	if( ( Table->lpSqlDataBuf = (LPSTR)malloc( sqlDataBufSize ) ) == NULL )
	{
		free( Table->rec_buf );
		free( FieldPos );
		free( Table->field );
		free( Table );
		i_ErrCode = ALLOC_MEM_ERROR;
		return( NULL );
	}

	if( ( Table->fld_id = (short *)malloc( Table->field_num * sizeof( short ) ) ) == NULL )
	{
		free( Table->lpSqlDataBuf );
		free( Table->rec_buf );
		free( FieldPos );
		free( Table->field );
		free( Table );
		i_ErrCode = ALLOC_MEM_ERROR;
		return( NULL );
	}

	size = 0;
	for( col = Desc->sqlvar, i = 0; i < Desc->sqld; col++, i++ )
	{
		size = rtypalign( size, col->sqltype );
		col->sqldata = Table->lpSqlDataBuf + size;
		#if defined(DEBUG_INFO)
		{
			char DebugInfo[80];
			wsprintf( (LPSTR)DebugInfo, "size: %d\nAddress: %ld", size, (long)col->sqldata );
			MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
		}
		#endif
		size += col->sqllen;
		if( col->sqltype != CDECIMALTYPE )
			size ++;

		Table->field[i].fieldstart = Table->rec_buf + FieldPos[i];
		Table->fld_id[i] = i;
	}

	free( FieldPos );

	$DECLARE $sqlCursor SCROLL CURSOR FOR $Query_Id;

	i_ErrCode = (long)sqlca.sqlcode;
	if( sqlca.sqlcode != 0 )
	{
		free( Table->fld_id );
		free( Table->lpSqlDataBuf );
		free( Table->rec_buf );
		free( Table->field );
		free( Table );
		return( NULL );
	}

	$OPEN $sqlCursor;
	i_ErrCode = (long)sqlca.sqlcode;
	if( sqlca.sqlcode != 0 )
	{
		free( Table->fld_id );
		free( Table->lpSqlDataBuf );
		free( Table->rec_buf );
		free( Table->field );
		free( Table );
		return( NULL );
	}

	return Table;
}

BOOL WINAPI sqlClose ( dFILE *Table )
{
	$char	sqlCursor[CURSORNAMELEN+1];

	if( Table == NULL )
		return TRUE;

	#if defined(DEBUG_INFO)
	{
		char DebugInfo[80];
		wsprintf( (LPSTR)DebugInfo, "sqlClose" );
		MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
	}
	#endif
	lstrcpy( (LPSTR)sqlCursor, (LPCSTR)Table->sqlCursor );
	$CLOSE $sqlCursor;

	free( Table->lpSqlDataBuf );
	free( Table->rec_buf );
	free( Table->field );
	free( Table );

	FreeSql_ID( atoi( &sqlCursor[2]) );

	return TRUE;
}

BOOL sqlSeek ( dFILE *Table, long pos, int FromWhere )   /* pos start from 0 to Table->rec_num-1 */
{
	if( Table == NULL )
		return FALSE;

	switch( FromWhere )
	{
		case dSEEK_SET :
			Table->rec_p = pos + 1;
			break;

		case dSEEK_CUR :
			Table->rec_p += pos;
			break;

		case dSEEK_END :
			Table->rec_p = Table->rec_num + pos + 1;
			break;

		default : return  FALSE;
	}
	return TRUE;
}

BOOL sqlEOF ( dFILE *Table )
{
	$char	sqlCursor[CURSORNAMELEN+1];
	struct  sqlda far *Desc;
	BOOL	EOF_Status = FALSE;

	if( Table == NULL )
		return TRUE;

	lstrcpy( (LPSTR)sqlCursor, (LPCSTR)Table->sqlCursor );
	Desc = Table->sqlDesc;

	$FETCH NEXT $sqlCursor USING DESCRIPTOR Desc;

	i_ErrCode = (long)sqlca.sqlcode;
	if( sqlca.sqlcode != 0 )
	{
		if( sqlca.sqlcode == 100 )
			EOF_Status = TRUE;
	}

	$FETCH PRIOR $sqlCursor USING DESCRIPTOR Desc;
	i_ErrCode = (long)sqlca.sqlcode;

	return EOF_Status;
}

LPSTR WINAPI sqlGetRow ( dFILE *Table )
{
	$char	sqlCursor[CURSORNAMELEN+1];
	struct	sqlvar_struct far 	*col;
	struct  sqlda far *Desc;
	char	DecStr[STRSZ], DateStr[DATELEN+1], TmpBuf[256];
	long 	i;
	$long int pos;
	int 	FieldLen;

	#if defined(DEBUG_INFO)
	{
		char DebugInfo[80];
		wsprintf( (LPSTR)DebugInfo, "sqlGetRow" );
		MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
	}
	#endif

	lstrcpy( (LPSTR)sqlCursor, (LPCSTR)Table->sqlCursor );
	Desc = Table->sqlDesc;
	pos = Table->rec_p;

	#if defined(DEBUG_INFO)
	{
		char DebugInfo[80];
		wsprintf( (LPSTR)DebugInfo, "DescAddress: %ld", (long)Desc );
		MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
		wsprintf( (LPSTR)DebugInfo, "sqlCursor: %s", sqlCursor );
		MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
	}
	#endif

	$FETCH ABSOLUTE $pos $sqlCursor USING DESCRIPTOR Desc;

	i_ErrCode = (long)sqlca.sqlcode;
	if( sqlca.sqlcode != 0 )
		return( NULL );

	Table->rec_no = Table->rec_p;

	for( col = Table->sqlDesc->sqlvar, i = 0;  i < Table->sqlDesc->sqld;  col++, i++ )
	{
		FieldLen = Table->field[i].fieldlen;

		switch( Table->field[i].fieldtype )
		{
			case 'N':
				if ( (int) (Table->field[i].fielddec) != 0 ) {
					dectoasc( (_LPDECIMAL)col->sqldata, DecStr, STRSZ, -1 );
					lstrcpyn( Table->field[i].fieldstart, DecStr, FieldLen+1 );
				}
				else {
					ltoa( (long)col->sqldata, DecStr, 10 );
					lstrcpyn( Table->field[i].fieldstart, DecStr, FieldLen+1 );
				}
				break;

			case 'C':
				//bycopy( col->sqldata, &DataBuf[FieldStart], FieldLen+1 );
				lstrcpyn( Table->field[i].fieldstart, (LPSTR)col->sqldata, FieldLen+1 );
				break;

			case 'D':
				if( lstrlen( col->sqldata ) != 0 )
				{
					bycopy( (LPSTR)&(col->sqldata[6]), (LPSTR)DateStr, 4 );
					bycopy( (LPSTR)col->sqldata, (LPSTR)&DateStr[4], 2 );
					bycopy( (LPSTR)&(col->sqldata[3]), (LPSTR)&DateStr[6], 2 );
					DateStr[8] = '\x0';
				}
				else
					lstrcpy( DateStr, "" );
				lstrcpyn( Table->field[i].fieldstart, DateStr, FieldLen+1 );
				break;
		}

		#if defined(DEBUG_INFO)
		{
			char DebugInfo[80];
			wsprintf( (LPSTR)DebugInfo, "Field[i]: %s", (LPSTR)&DataBuf[FieldStart] );
			MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
		}
		#endif
	}

	return Table->rec_buf;
}

LPSTR WINAPI sqlGetField ( dFILE *Table,	int Field_ID, LPSTR FieldDataBuf )
{
	dFIELD  *Field;
	int 	FieldLen;

	#if defined(DEBUG_INFO)
	{
		char DebugInfo[80];
		wsprintf( (LPSTR)DebugInfo, "sqlGetRow" );
		MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
	}
	#endif

	if( FieldDataBuf == NULL )
	{
		i_ErrCode = NULL_POINTER;
		return NULL;
	}

	FieldLen = Table->field[Field_ID].fieldlen;

	lstrcpyn( FieldDataBuf, Table->field[Field_ID].fieldstart, FieldLen+1 );

	return FieldDataBuf;
}

/*********************** Unexport Functions ********************************/

BOOL WINAPI ReType( struct sqlda far *Desc, dFIELD far *Field, int *FieldPos,
					int *sqlDataBufSize, int *DataBufSize )
{
	int	i, size = 0, pos = 0;
	struct sqlvar_struct 	*col;

	#if defined(DEBUG_INFO)
	{
		char DebugInfo[80];
		wsprintf( (LPSTR)DebugInfo, "ReType" );
		MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
	}
	#endif

	*sqlDataBufSize = 0;
	*DataBufSize = 0;
	for( col = Desc->sqlvar, i = 0;   i < Desc->sqld;   col++, i++ )
	{
		lstrcpyn( Field[i].field, col->sqlname, FIELDNAMELEN );
		Field[i].field[FIELDNAMELEN] = '\0';

		switch( col->sqltype )
		{
			case SQLSMFLOAT:
			case SQLFLOAT:
			case SQLMONEY:
			case SQLDECIMAL:
				//Field[i].fieldtype = DBFDEC;
				size = rtypalign(size, CDECIMALTYPE);
				col->sqltype = CDECIMALTYPE;
				col->sqllen = rtypmsize(CDECIMALTYPE, 0 )+1;
				size += col->sqllen + 1;
				Field[i].fieldlen = 20;
				Field[i].fielddec = 2;
				Field[i].fieldtype = 'N';
				FieldPos[i] = pos;
				pos += Field[i].fieldlen;
				pos++;
				break;

			case SQLINT:
			case SQLSMINT:
				//Field[i].fieldtype = DBFINT;
				size = rtypalign( size, CFIXCHARTYPE );
				col->sqllen = rtypwidth(col->sqltype, col->sqllen)+1;
				col->sqltype = CFIXCHARTYPE;
				size += col->sqllen + 1;
				Field[i].fieldlen = 10;
				Field[i].fielddec = 0;
				Field[i].fieldtype = 'N';
				FieldPos[i] = pos;
				pos += Field[i].fieldlen;
				pos++;
				break;

			case SQLDATE:
				//Field[i].fieldtype = DBFDATE;
				size = rtypalign( size, CFIXCHARTYPE );
				col->sqllen = rtypwidth( col->sqltype, col->sqllen)+1;
				col->sqltype = CFIXCHARTYPE;
				size += col->sqllen + 1;
				Field[i].fieldlen = 8;
				Field[i].fieldtype = 'D';
				FieldPos[i] = pos;
				pos += Field[i].fieldlen;
				pos++;
				break;

			default:
				//Field[i].fieldtype = DBFCHAR;
				size = rtypalign( size, CFIXCHARTYPE );
				col->sqllen = rtypwidth( col->sqltype, col->sqllen)+1;
				col->sqltype = CFIXCHARTYPE;
				size += col->sqllen + 1;
				Field[i].fieldlen = col->sqllen - 1;
				Field[i].fieldtype = 'C';
				FieldPos[i] = pos;
				pos += Field[i].fieldlen;
				pos++;
				break;
		}

		#if defined(DEBUG_INFO)
		{
			char DebugInfo[256];
			wsprintf( (LPSTR)DebugInfo, "Name: %s\nType: %d\nLen: %d\nDecLen: %d\npos: %d",
				Field[i].field, Field[i].fieldtype, Field[i].fieldlen,
				Field[i].fielddec, Field[i].fieldstart );
			MessageBox( NULL, DebugInfo, "DebugInfo", MB_OK );
		}
		#endif
	}
	*DataBufSize = pos;
	*sqlDataBufSize = size;

	Field[i].field[0] = '\0';

	return TRUE;
}

int WINAPI GetSql_ID ( void )
{
	int i;

	for( i = 0; i < MAX_OPEN_TABLE; i++ )
		if( sqlIdTable[i] == 0 )
		{
			sqlIdTable[i] = 1;
			return i;
		}

	return -1;
}

void WINAPI FreeSql_ID ( int IDNumber )
{
	sqlIdTable[IDNumber] = 0;
}


long getI_ErrCode( void )
{
    return  i_ErrCode;
}

void resetI_ErrCode( void )
{
    i_ErrCode = SQL_OK;
}

/*
#include <conio.h>
void main()
{
	dFILE *Table;
	char FieldDataBuf[256];
	int i, j;

	if( OpenDataBase( "ward@OH", "informix", "stu1996" ) != 0 )
	{
		printf( "Open database error(sqlErrorNO: %d)\n", i_ErrCode );
		return;
	}
	printf( "Open database\n" );

	if( ( Table = sqlOpen( "Test", "select * from sub0" ) ) == NULL )
	{
		printf( "Open table error(sqlErrorNO: %d)\n", i_ErrCode );
		return;
	}
	printf( "Open table\n" );

	j=0;

	do {
		printf( "\n=*=*=*=*=*=*=*=*=*=*=*=RECORD %d=*=*=*=*=*=*=*=*=*=*=*=\n", j+1 );
		sqlSeek( Table, j, dSEEK_SET );
		sqlGetRow( Table );
		for( i = 0; i < Table->field_num; i++ )
		{
			sqlGetField( Table, i, (LPSTR)FieldDataBuf );
			printf( "%s : %s\n", Table->field[i].field, FieldDataBuf );
			if( i != 0 && i % 20 == 0 )
			{
				printf( "Press any key to continue...\n" );
				if( getch() == 'q' )
					goto end;
			}
		}
		j++;
		printf( "Press any key to continue...\n" );
		if( getch() == 'q' )
			goto end;
	} while( i_ErrCode != 100 );

end:
	if( sqlEOF( Table ) == FALSE )
		printf( "Not end of view.\n" );
	else
		printf( "End of view.\n" );

	sqlClose( Table );
	printf( "\nClose Table\n" );

	CloseDataBase( );
	printf( "Close Database\n" );
}
*/