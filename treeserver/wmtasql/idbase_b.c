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

#include "idbase_b.h"

_declspec(dllexport) long TOpenDataBase ( LPSTR DataBase, LPSTR UserName, LPSTR Password )
{
	return SQL_OK;
}

_declspec(dllexport) BOOL TCloseDataBase ( void )
{
	return TRUE;
}

_declspec(dllexport) dFILE *TsqlOpen ( LPSTR sqlName, char *sqlStatement )
{
	return NULL;
}

_declspec(dllexport) BOOL TsqlClose ( dFILE *Table )
{
	return TRUE;
}

_declspec(dllexport) BOOL TsqlSeek ( dFILE *Table, long pos, int FromWhere )   /* pos start from 0 to Table->rec_num-1 */
{
	return TRUE;
}

_declspec(dllexport) BOOL TsqlEOF ( dFILE *Table )
{
	return 1;
}

_declspec(dllexport) LPSTR TsqlGetRow ( dFILE *Table )
{
	return NULL;
}

_declspec(dllexport) long TgetSqlErrCode( void )
{
    return  1;
}
_declspec(dllexport) void  TresetSqlErrCode( void )
{
}


LPSTR sqlGetField ( dFILE *Table, int Field_ID, LPSTR FieldDataBuf )
{
	return NULL;
}


/*********************** Unexport Functions ********************************/

BOOL ReType( struct sqlda far *Desc, dFIELD far *Field, int *FieldPos,
					int *sqlDataBufSize, int *DataBufSize )
{
	return TRUE;
}

int GetSql_ID ( void )
{
	return -1;
}

void FreeSql_ID ( int IDNumber )
{
}


long getI_ErrCode( void )
{
	return 0;
}

void resetI_ErrCode( void )
{
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