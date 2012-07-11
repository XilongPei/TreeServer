#include <windows.h>
#include "sqlhdr.h"
#include "sqlproto.h"
extern _LPSQCURSOR WINAPI _iqnprep(LPSTR, LPSTR, short);
#line 1 "idbase.ec"









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

/* 
 * $include sqlca;
 */
#line 24 "idbase.ec"
#line 1 "sqlca.h"

























#pragma pack(push, 4)

#ifndef SQLCA_INCL
#define SQLCA_INCL

struct sqlca_s
	{
	long sqlcode;
	char sqlerrm[72];
	char sqlerrp[8];
	long sqlerrd[6];
			
			
			
			
			
			
	struct sqlcaw_s
	{
	char sqlwarn0;
	char sqlwarn1;

	char sqlwarn2;

	char sqlwarn3;

	char sqlwarn4;

	char sqlwarn5;
	char sqlwarn6;
	char sqlwarn7;
	} sqlwarn;
	};









#ifdef VMS
noshare
#endif


extern struct sqlca_s sqlca;

extern long SQLCODE;


#define SQLNOTFOUND 100
#endif

#pragma pack(pop, 4)

/* 
 * $include login.h;
 */
#line 25 "idbase.ec"
#line 1 "login.h"

#pragma pack(push, 4)

#ifndef LOGIN_INCL
#define LOGIN_INCL

#define	NAME_LEN  19
#define PASS_LEN  19
#define ENVI_LEN  129
#define DATE_LEN  6	
#define TEMP_LEN  81
#define ANSI_LEN  1
#define NL_LEN     3
#define LC_LEN	  9

typedef struct
	{
	char Host[NAME_LEN];
	char cHost;
	char User[NAME_LEN];
	char cUser;
	char Pass[PASS_LEN];
	char cPass;
	char Service[NAME_LEN];
	char Protocol[NAME_LEN];
	char InetType;	
	char DbPath[ENVI_LEN];
	char DbDate[DATE_LEN];
	char DbMoney[NAME_LEN];
	char DbTime[TEMP_LEN];
	char DbTemp[TEMP_LEN];
	char DbLang[NAME_LEN];
	char DbAnsiWarn[ANSI_LEN];
	char InformixDir[TEMP_LEN];
	char DbNls[NL_LEN];
	char CollChar[NL_LEN];
	char Lc_Ctype[LC_LEN];
	char Lc_Collate[LC_LEN];
	} LoginInfoStruct;

extern LoginInfoStruct InetLogin;

#endif

#pragma pack(pop, 4)

/* 
 * $include sqltypes.h;
 */
#line 26 "idbase.ec"
#line 1 "sqltypes.h"

























#pragma pack(push, 4)

#ifndef CCHARTYPE



















#define CCHARTYPE	100
#define CSHORTTYPE	101
#define CINTTYPE	102
#define CLONGTYPE	103
#define CFLOATTYPE	104
#define CDOUBLETYPE	105
#define CDECIMALTYPE	107
#define CFIXCHARTYPE	108
#define CSTRINGTYPE	109
#define CDATETYPE	110
#define CMONEYTYPE	111
#define CDTIMETYPE	112
#define CLOCATORTYPE    113
#define CVCHARTYPE	114
#define CINVTYPE	115
#define CFILETYPE	116


#define USERCOLL(x)	((x))







#define SQLCHAR		0
#define SQLSMINT	1
#define SQLINT		2
#define SQLFLOAT	3
#define SQLSMFLOAT	4
#define SQLDECIMAL	5
#define SQLSERIAL	6
#define SQLDATE		7
#define SQLMONEY	8
#define SQLNULL		9
#define SQLDTIME	10
#define SQLBYTES	11
#define SQLTEXT		12
#define SQLVCHAR	13
#define SQLINTERVAL	14
#define SQLTYPE		0xF	
#define SQLNONULL	0x100	
#define SQLMAXTYPES	15




#define SQLHOST		01000
#define SQLNETFLT	02000	

#define SIZCHAR		1
#define SIZSMINT	2
#define SIZINT		4
#define SIZFLOAT	(sizeof(double))
#define SIZSMFLOAT	(sizeof(float))
#define SIZDECIMAL	17	
#define SIZSERIAL	4
#define SIZDATE		4
#define SIZMONEY	17	
#define SIZDTIME	7	
#define SIZVCHAR	1

#define MASKNONULL(t)	((t) & ~(SQLNONULL|SQLHOST|SQLNETFLT))
#define ISSQLTYPE(t)	(MASKNONULL(t) >= SQLCHAR && MASKNONULL(t) < SQLMAXTYPES)




#define ISDECTYPE(t)		(MASKNONULL(t) == SQLDECIMAL || \
				 MASKNONULL(t) == SQLMONEY || \
				 MASKNONULL(t) == SQLDTIME || \
				 MASKNONULL(t) == SQLINTERVAL)

#define ISBLOBTYPE(type)	(ISBYTESTYPE (type) || ISTEXTTYPE(type))
#define ISBYTESTYPE(type)	(MASKNONULL(type) == SQLBYTES)
#define ISTEXTTYPE(type)	(MASKNONULL(type) == SQLTEXT)
#define ISVCTYPE(t)		(MASKNONULL(t) == SQLVCHAR)




#define ISBLOBCTYPE(type)	(ISLOCTYPE(type) || ISFILETYPE(type))
#define ISLOCTYPE(type)		(MASKNONULL(type) == CLOCATORTYPE)
#define ISFILETYPE(type)	(MASKNONULL(type) == CFILETYPE)

#define ISOPTICALCOL(type)	(type == 'O')

#define DEFDECIMAL	9	
#define DEFMONEY	9	

#define SYSPUBLIC	"public"


#endif

#pragma pack(pop, 4)

#line 27 "idbase.ec"

static long	i_ErrCode = SQL_OK;
static int sqlIdTable[MAX_OPEN_TABLE];

_declspec(dllexport) long   TOpenDataBase ( LPSTR DataBase, LPSTR UserName, LPSTR Password )
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

_declspec(dllexport) BOOL   TCloseDataBase ( void )
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

_declspec(dllexport) dFILE *  TsqlOpen ( sqlName, sqlStatement )
LPSTR sqlName;
/*
 * $char *sqlStatement;
 */
#line 90 "idbase.ec"
char *sqlStatement;
{
	struct sqlda far *Desc = (struct sqlda *)NULL;
	struct sqlvar_struct far	*col;
	int 	*FieldPos;
	dFILE	*Table;
	int  	i, size, sqlIdNumber, DataBufSize, sqlDataBufSize;
/*
 * 	$char   Query_Id[IDLEN+1], sqlCursor[CURSORNAMELEN+1];
 */
#line 97 "idbase.ec"
char Query_Id[IDLEN+1], sqlCursor[CURSORNAMELEN+1];

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

/*
 * 	$PREPARE $Query_Id FROM $sqlStatement;
 */
#line 124 "idbase.ec"
  {
#line 124 "idbase.ec"
#line 124 "idbase.ec"
  _iqnprep((LPSTR)Query_Id, (LPSTR)sqlStatement, (short)0);
#line 124 "idbase.ec"
  }
	i_ErrCode = (long)sqlca.sqlcode;
	if( sqlca.sqlcode != 0 )
		return( NULL );

/*
 * 	$DESCRIBE $Query_Id INTO Desc;
 */
#line 129 "idbase.ec"
  {
#line 129 "idbase.ec"
#line 129 "idbase.ec"
  _iqdescribe((_SQCURSOR far *)_iqlocate_cursor((LPSTR)Query_Id, 1, 1,(short)0), (struct sqlda far * far *)&Desc, (LPSTR)0);
#line 129 "idbase.ec"
  }
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

/*
 * 	$DECLARE $sqlCursor SCROLL CURSOR FOR $Query_Id;
 */
#line 237 "idbase.ec"
  {
#line 237 "idbase.ec"
#line 237 "idbase.ec"
  _iqcddcl((_SQCURSOR far *)_iqlocate_cursor((LPSTR)sqlCursor, 0, 0,(short)0), (LPSTR)sqlCursor, (_SQCURSOR far *)_iqlocate_cursor((LPSTR)Query_Id, 1, 1,(short)0), 32);
#line 237 "idbase.ec"
  }

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

/*
 * 	$OPEN $sqlCursor;
 */
#line 250 "idbase.ec"
  {
#line 250 "idbase.ec"
#line 250 "idbase.ec"
  _iqdcopen((_SQCURSOR far *)_iqlocate_cursor((LPSTR)sqlCursor, 0, 1,(short)0), (struct sqlda far *)0, (LPSTR)0, (struct value far *)0, 0);
#line 250 "idbase.ec"
  }
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

_declspec(dllexport) BOOL   TsqlClose ( dFILE *Table )
{
/*
 * 	$char	sqlCursor[CURSORNAMELEN+1];
 */
#line 267 "idbase.ec"
char sqlCursor[CURSORNAMELEN+1];

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
/*
 * 	$CLOSE $sqlCursor;
 */
#line 280 "idbase.ec"
  {
#line 280 "idbase.ec"
#line 280 "idbase.ec"
  _iqclose((_SQCURSOR far *)_iqlocate_cursor((LPSTR)sqlCursor, 0, 1,(short)0));
#line 280 "idbase.ec"
  }

	free( Table->lpSqlDataBuf );
	free( Table->rec_buf );
	free( Table->field );
	free( Table );

	FreeSql_ID( atoi( &sqlCursor[2]) );

	return TRUE;
}

_declspec(dllexport) BOOL   TsqlSeek ( dFILE *Table, long pos, int FromWhere )
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

_declspec(dllexport) BOOL   TsqlEOF ( dFILE *Table )
{
/*
 * 	$char	sqlCursor[CURSORNAMELEN+1];
 */
#line 318 "idbase.ec"
char sqlCursor[CURSORNAMELEN+1];
	struct  sqlda far *Desc;
	BOOL	EOF_Status = FALSE;

	if( Table == NULL )
		return TRUE;

	lstrcpy( (LPSTR)sqlCursor, (LPCSTR)Table->sqlCursor );
	Desc = Table->sqlDesc;

/*
 * 	$FETCH NEXT $sqlCursor USING DESCRIPTOR Desc;
 */
#line 328 "idbase.ec"
  {
#line 328 "idbase.ec"
  static _FetchSpec _FS0 = { 0, 1, 0 };
#line 328 "idbase.ec"
  _iqcftch((_SQCURSOR far *)_iqlocate_cursor((LPSTR)sqlCursor, 0, 1,(short)0), (struct sqlda far *)0, (struct sqlda far *)Desc, (LPSTR)0, (_FetchSpec far *)&_FS0);
#line 328 "idbase.ec"
  }

	i_ErrCode = (long)sqlca.sqlcode;
	if( sqlca.sqlcode != 0 )
	{
		if( sqlca.sqlcode == 100 )
			EOF_Status = TRUE;
	}

/*
 * 	$FETCH PRIOR $sqlCursor USING DESCRIPTOR Desc;
 */
#line 337 "idbase.ec"
  {
#line 337 "idbase.ec"
  static _FetchSpec _FS0 = { 0, 2, 0 };
#line 337 "idbase.ec"
  _iqcftch((_SQCURSOR far *)_iqlocate_cursor((LPSTR)sqlCursor, 0, 1,(short)0), (struct sqlda far *)0, (struct sqlda far *)Desc, (LPSTR)0, (_FetchSpec far *)&_FS0);
#line 337 "idbase.ec"
  }
	i_ErrCode = (long)sqlca.sqlcode;

	return EOF_Status;
}

_declspec(dllexport) LPSTR   TsqlGetRow ( dFILE *Table )
{
/*
 * 	$char	sqlCursor[CURSORNAMELEN+1];
 */
#line 345 "idbase.ec"
char sqlCursor[CURSORNAMELEN+1];
	struct	sqlvar_struct far 	*col;
	struct  sqlda far *Desc;
	char	DecStr[STRSZ], DateStr[DATELEN+1], TmpBuf[256];
	long 	i;
/*
 * 	$long int pos;
 */
#line 350 "idbase.ec"
long int pos;
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

/*
 * 	$FETCH ABSOLUTE $pos $sqlCursor USING DESCRIPTOR Desc;
 */
#line 375 "idbase.ec"
  {
#line 375 "idbase.ec"
  static struct sqlvar_struct _sqibind[] = 
    {
      { 103, sizeof(pos), 0, 0, 0, 0, 0, 0, 0 },
#line 375 "idbase.ec"
    };
  static struct sqlda _SD0 = { 1, _sqibind, 0, 1, 0 };
  static _FetchSpec _FS1 = { 0, 6, 0 };
#line 375 "idbase.ec"
  _sqibind[0].sqldata = (char far *) &pos;
#line 375 "idbase.ec"
  _iqcftch((_SQCURSOR far *)_iqlocate_cursor((LPSTR)sqlCursor, 0, 1,(short)0), (struct sqlda far *)&_SD0, (struct sqlda far *)Desc, (LPSTR)0, (_FetchSpec far *)&_FS1);
#line 375 "idbase.ec"
  }

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

LPSTR   sqlGetField ( dFILE *Table,	int Field_ID, LPSTR FieldDataBuf )
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



BOOL   ReType( struct sqlda far *Desc, dFIELD far *Field, int *FieldPos,
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

int   GetSql_ID ( void )
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

void   FreeSql_ID ( int IDNumber )
{
	sqlIdTable[IDNumber] = 0;
}


_declspec(dllexport) long TgetSqlErrCode( void )
{
    return  i_ErrCode;
}

_declspec(dllexport) void TresetSqlErrCode( void )
{
    i_ErrCode = SQL_OK;
}



























































