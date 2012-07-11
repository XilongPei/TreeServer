// Command.cpp : Implementation of CCommand
#include "stdafx.h"
#include <comdef.h>
#include "TSADO.h"
#include "Command.h"
#include "AboutBox.h"

#include "tscapi.h"

/////////////////////////////////////////////////////////////////////////////
// CCommand

STDMETHODIMP CCommand::Logon(BSTR szServer, BSTR szUser, BSTR szPasswd, VARIANT_BOOL *bSuccess)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	if( bLogoned )
		Logoff();

	dwUserId = TSCAPI::tsLogon ( (char *)(_bstr_t)szServer, (char *)(_bstr_t)szUser, (char *)(_bstr_t)szPasswd, &hConnect );
	if( 0xFFFFFFFF == dwUserId ) {
		*bSuccess = FALSE;
		bLogoned = FALSE;
		dwErrorCode = GetLastError();
		dwErrorCode = dwUserId;
	}
	else {
		*bSuccess = TRUE;
		bLogoned = TRUE;
		dwErrorCode = 0;
	}

	return S_OK;
}

STDMETHODIMP CCommand::Logoff()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	if( bLogoned ) {
		TableClose();

		if( !TSCAPI::tsLogoff( hConnect ) )
			dwErrorCode = GetLastError();
		else
			dwErrorCode = 0;

		bLogoned = FALSE;
	}

	return S_OK;
}

STDMETHODIMP CCommand::ASQLClear()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	ASQLString.Empty();
	dwErrorCode = 0;

	return S_OK;
}

STDMETHODIMP CCommand::ASQLAdd(BSTR szASQL)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	_bstr_t s( szASQL );

	ASQLString += (LPTSTR)s;

	dwErrorCode = 0;

	return S_OK;
}

STDMETHODIMP CCommand::ASQLExecute(VARIANT_BOOL *bSuccess)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	DWORD dwRetCode;
	LPSTR lpErrorMsg;
	DWORD dwcbErrorMsg = 1024;
	LPVOID lpResult;
	DWORD dwcbResult = 1024;

	_bstr_t s( (LPCTSTR)ASQLString );

	if( !bLogoned ) {
		*bSuccess = FALSE;
		dwErrorCode = 8001;
		return E_FAIL;
	}

	//lpErrorMsg = (LPSTR)::HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwcbErrorMsg ); 
	lpErrorMsg = (LPSTR)::CoTaskMemAlloc( dwcbErrorMsg ); 
	if( !lpErrorMsg ) {
		dwErrorCode = ERROR_OUTOFMEMORY;
		return E_OUTOFMEMORY;
	}

	//lpResult = (LPSTR)::HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwcbResult ); 
	lpResult = (LPSTR)::CoTaskMemAlloc( dwcbResult ); 
	if( !lpResult ) {
		dwErrorCode = ERROR_OUTOFMEMORY;
		CoTaskMemFree( lpErrorMsg );
		return E_OUTOFMEMORY;
	}

	dwRetCode = TSCAPI::tsCallAsqlSvrM( hConnect, (char *)s, 
							lpResult, &dwcbResult, lpErrorMsg, dwcbErrorMsg );

	if( 3 == dwRetCode ) {
		*bSuccess = FALSE;
		dwErrorCode = 8002;
		bASQLError = FALSE;
	}
	else {
		LPCSTR szNoError = "ERR:0";

		if( strncmp( lpErrorMsg, szNoError, lstrlenA( szNoError ) )==0 ) {
			dwErrorCode = 0;
			*bSuccess = TRUE;
		}
		else {
			*bSuccess = FALSE;
			dwErrorCode = 8002;
			bASQLError = TRUE;
		}
	}

	if( bASQLError )
		ASQLErrorMsg = (LPTSTR)((_bstr_t)lpErrorMsg);
	else
		ASQLErrorMsg.Empty();

	//::HeapFree( GetProcessHeap(), 0, lpResult );
	//::HeapFree( GetProcessHeap(), 0, lpErrorMsg );
	::CoTaskMemFree( lpResult );
	::CoTaskMemFree( lpErrorMsg );

	return S_OK;
}

STDMETHODIMP CCommand::TableOpen(BSTR szDatabase, BSTR szTable, VARIANT_BOOL *bSuccess)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	TSCAPI::RECPROPS rp;

	if( !bLogoned ) {
		*bSuccess = FALSE;
		dwErrorCode = 8001;
		return E_FAIL;
	}

	TableClose();

	//lpTableInfo = (TSCAPI::LPTABLE_INFO)::HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, /*sizeof(TSCAPI::TABLE_INFO)*/ 4096 * sizeof(char) );
	lpTableInfo = (TSCAPI::LPTABLE_INFO)::CoTaskMemAlloc( /*sizeof(TSCAPI::TABLE_INFO)*/ 4096 * sizeof(char) );
	if( !lpTableInfo ) {
		dwErrorCode = ERROR_OUTOFMEMORY;
		*bSuccess = FALSE;
		return E_OUTOFMEMORY;
	}
		
	dwTableId = TSCAPI::tsTableOpen( hConnect, (char *)((_bstr_t)szDatabase), 
					  (char *)((_bstr_t)szTable), &lpTableInfo );
	if( 0 == dwTableId ) {
		dwErrorCode = GetLastError();
		//::HeapFree( GetProcessHeap(), 0, lpTableInfo );
		::CoTaskMemFree( lpTableInfo );
		*bSuccess = FALSE;
		return E_FAIL;
	}

	lpTableInfo->field_num = lpTableInfo->headlen / 32 - 1;
	lpTableInfo->rec_num = TSCAPI::tsTableRecNum( hConnect, dwTableId );

	DWORD dwcbColInfo = (lpTableInfo->field_num+1) * sizeof(TSCAPI::DIO_FIELD);

	//lpColInfo = (TSCAPI::LPDIO_FIELD)::HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwcbColInfo );
	lpColInfo = (TSCAPI::LPDIO_FIELD)::CoTaskMemAlloc( dwcbColInfo );
	if( !lpColInfo ) {
		dwErrorCode = ERROR_OUTOFMEMORY;
		TSCAPI::tsTableClose( hConnect, dwTableId );
		//::HeapFree( GetProcessHeap(), 0, lpTableInfo );
		::CoTaskMemFree( lpTableInfo );
		*bSuccess = FALSE;
		return E_OUTOFMEMORY;
	}

	TSCAPI::tsTableGetFieldInfo( hConnect, dwTableId, lpColInfo, dwcbColInfo );
	*(lpColInfo[lpTableInfo->field_num].field) = '\x0';

	//lpRowData = (LPSTR)::HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, lpTableInfo->rec_len * sizeof(char) );
	lpRowData = (LPSTR)::CoTaskMemAlloc( lpTableInfo->rec_len * sizeof(char) );
	if( !lpRowData ) {
		dwErrorCode = ERROR_OUTOFMEMORY;
		TSCAPI::tsTableClose( hConnect, dwTableId );
		//::HeapFree( GetProcessHeap(), 0, lpTableInfo );
		//::HeapFree( GetProcessHeap(), 0, lpColInfo );
		::CoTaskMemFree( lpTableInfo );
		::CoTaskMemFree( lpColInfo );
		*bSuccess = FALSE;
		return E_OUTOFMEMORY;
	}

	if( lpTableInfo->rec_num != 0 ) {
		lCurrentRow = 1;

		TSCAPI::tsTableGetRec( hConnect, dwTableId, (DWORD)lCurrentRow, lpRowData,
				lpTableInfo->rec_len, &rp );


		bEOF = FALSE;
		bBOF = FALSE;
	}
	else {
		lCurrentRow = 0;

		ZeroMemory( lpRowData, lpTableInfo->rec_len );

		bEOF = TRUE;
		bBOF = TRUE;
	}

	*bSuccess = TRUE;
	dwErrorCode = 0;

	return S_OK;
}

STDMETHODIMP CCommand::TableClose()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	if( dwTableId ) {
		TSCAPI::tsTableClose( hConnect, dwTableId );
		//::HeapFree( GetProcessHeap(), 0, lpRowData );
		//::HeapFree( GetProcessHeap(), 0, lpColInfo );
		//::HeapFree( GetProcessHeap(), 0, lpTableInfo );
		::CoTaskMemFree( lpRowData );
		::CoTaskMemFree( lpColInfo );
		::CoTaskMemFree( lpTableInfo );

		dwTableId = 0;
	}

	dwErrorCode = 0;

	return S_OK;
}

STDMETHODIMP CCommand::First()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	TSCAPI::RECPROPS rp;

	if( 0 == dwTableId ) {
		dwErrorCode = 8003;
		return E_FAIL;
	}

	if( 0 == lpTableInfo->rec_num ) {
		return S_OK;
	}	

	bBOF = FALSE;
	bEOF = FALSE;

	lCurrentRow = 1;

	TSCAPI::tsTableGetRec( hConnect, dwTableId, (DWORD)lCurrentRow, lpRowData,
			lpTableInfo->rec_len, &rp );

	dwErrorCode = 0;
	return S_OK;
}

STDMETHODIMP CCommand::Next()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	TSCAPI::RECPROPS rp;

	if( 0 == dwTableId ) {
		dwErrorCode = 8003;
		return E_FAIL;
	}

	if( 0 == lpTableInfo->rec_num ) {
		return S_OK;
	}	

	if( bEOF )
		return S_OK;
	else if( bBOF )
		bBOF = FALSE;

	lCurrentRow++;
	if( lCurrentRow > lpTableInfo->rec_num ) {
		bEOF = TRUE;
		return S_OK;
	}

	TSCAPI::tsTableGetRec( hConnect, dwTableId, (DWORD)lCurrentRow, lpRowData,
			lpTableInfo->rec_len, &rp );

	return S_OK;
}

STDMETHODIMP CCommand::Prev()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	TSCAPI::RECPROPS rp;

	if( 0 == dwTableId ) {
		dwErrorCode = 8003;
		return E_FAIL;
	}

	if( 0 == lpTableInfo->rec_num ) {
		return S_OK;
	}	

	if( bBOF )
		return S_OK;
	else if( bEOF )
		bEOF = FALSE;

	lCurrentRow--;
	if( lCurrentRow < 1 )
		bBOF = TRUE;

	TSCAPI::tsTableGetRec( hConnect, dwTableId, (DWORD)lCurrentRow, lpRowData,
			lpTableInfo->rec_len, &rp );

	return S_OK;
}

STDMETHODIMP CCommand::Last()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	TSCAPI::RECPROPS rp;

	if( 0 == dwTableId ) {
		dwErrorCode = 8003;
		return E_FAIL;
	}

	if( 0 == lpTableInfo->rec_num ) {
		return S_OK;
	}	

	bBOF = FALSE;
	bEOF = FALSE;

	lCurrentRow = lpTableInfo->rec_num;

	TSCAPI::tsTableGetRec( hConnect, dwTableId, (DWORD)lCurrentRow, lpRowData,
			lpTableInfo->rec_len, &rp );

	return S_OK;
}

STDMETHODIMP CCommand::RowCount(long *lRowCnt)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	if( 0 == dwTableId ) {
		dwErrorCode = 8003;
		return E_FAIL;
	}

	*lRowCnt = lpTableInfo->rec_num;

	return S_OK;
}

STDMETHODIMP CCommand::ColumnCount(int *iColCnt)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	if( 0 == dwTableId ) {
		dwErrorCode = 8003;
		return E_FAIL;
	}

	*iColCnt = lpTableInfo->field_num;

	return S_OK;
}

STDMETHODIMP CCommand::ColumnById(int iColId, BSTR *pValue)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	if( 0 == dwTableId ) {
		dwErrorCode = 8003;
		return E_FAIL;
	}

	if( iColId < 1 || iColId > lpTableInfo->field_num ) {
		dwErrorCode = 8004;
		return E_FAIL;
	}

	LPSTR s = ( LPSTR )( lpRowData + lpColInfo[iColId-1].fieldstart );
	LPSTR c = (LPSTR)new char[lpColInfo[iColId-1].fieldlen+1];
	lstrcpynA( c, s, lpColInfo[iColId-1].fieldlen+1 );
	*pValue = SysAllocString( (wchar_t *)((_bstr_t)c) );
	delete c;

	if( !(*pValue ) ) {
		dwErrorCode = ERROR_OUTOFMEMORY;
		return E_OUTOFMEMORY;
	}

	dwErrorCode = 0;
	return S_OK;
}

STDMETHODIMP CCommand::ColumnByName(BSTR szColName, BSTR *pValue)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	TSCAPI::LPDIO_FIELD p;

	if( 0 == dwTableId ) {
		dwErrorCode = 8003;
		return E_FAIL;
	}

	for( p = lpColInfo; *(p->field) != '\0'; p++ ) {
		if( !stricmp( (const char *)(p->field), (char *)((_bstr_t)szColName) ) ) {
			LPSTR s = ( LPSTR )( lpRowData + p->fieldstart );
			LPSTR c = (LPSTR)new char[p->fieldlen+1];
			lstrcpynA( c, s, p->fieldlen+1 );
			*pValue = SysAllocString( (wchar_t *)((_bstr_t)c) );
			delete c;

			if( !(*pValue ) ) {
				dwErrorCode = ERROR_OUTOFMEMORY;
				return E_OUTOFMEMORY;
			}

			dwErrorCode = 0;
			return S_OK;
		}
	}

	dwErrorCode = 8004;
	return E_FAIL;
}

STDMETHODIMP CCommand::AboutBox()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	CAboutBox myAbout;

	myAbout.DoModal();

	return S_OK;
}

STDMETHODIMP CCommand::get_szASQLString(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	_bstr_t s( (LPCTSTR)ASQLString );

	*pVal = SysAllocString( (wchar_t *)s );
	if( !(*pVal) ) {
		dwErrorCode = ERROR_OUTOFMEMORY;
		return E_OUTOFMEMORY;
	}

	dwErrorCode = 0;
	return S_OK;
}

STDMETHODIMP CCommand::put_szASQLString(BSTR newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	_bstr_t s( newVal );

	ASQLString = (LPTSTR)s;

	dwErrorCode = 0;
	return S_OK;
}

STDMETHODIMP CCommand::get_ErrorCode(unsigned long *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	*pVal = dwErrorCode;

	return S_OK;
}

STDMETHODIMP CCommand::get_szASQLErrorMsg(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	_bstr_t s( (LPCTSTR)ASQLErrorMsg );

	*pVal = SysAllocString( (wchar_t *)s );
	if( !(*pVal) ) {
		dwErrorCode = ERROR_OUTOFMEMORY;
		return E_OUTOFMEMORY;
	}

	dwErrorCode = 0;

	return S_OK;
}

STDMETHODIMP CCommand::get_ASQLError(VARIANT_BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	*pVal = bASQLError;

	return S_OK;
}

STDMETHODIMP CCommand::get_BOF(VARIANT_BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	*pVal = bBOF;

	return S_OK;
}

STDMETHODIMP CCommand::get_EOF(VARIANT_BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	*pVal = bEOF;

	return S_OK;
}

STDMETHODIMP CCommand::get_ColumnName(int i, BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	if( 0 == dwTableId ) {
		dwErrorCode = 8003;
		return E_FAIL;
	}

	if( i < 1 || i > lpTableInfo->field_num ) {
		dwErrorCode = 8004;
		return E_FAIL;
	}

	*pVal = ::SysAllocString( (wchar_t *)((_bstr_t)(char *)lpColInfo[i-1].field) );
	if( !(*pVal) ) {
		dwErrorCode = ERROR_OUTOFMEMORY;
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

STDMETHODIMP CCommand::get_ColumnSize(int i, int *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	if( 0 == dwTableId ) {
		dwErrorCode = 8003;
		return E_FAIL;
	}

	if( i < 1 || i > lpTableInfo->field_num ) {
		dwErrorCode = 8004;
		return E_FAIL;
	}

	*pVal = lpColInfo[i-1].fieldlen;

	return S_OK;
}

STDMETHODIMP CCommand::get_ColumnPrecision(int i, int *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	if( 0 == dwTableId ) {
		dwErrorCode = 8003;
		return E_FAIL;
	}

	if( i < 1 || i > lpTableInfo->field_num ) {
		dwErrorCode = 8004;
		return E_FAIL;
	}

	*pVal = lpColInfo[i-1].fielddec;

	return S_OK;
}

STDMETHODIMP CCommand::get_ColumnType(int i, unsigned char *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	if( 0 == dwTableId ) {
		dwErrorCode = 8003;
		return E_FAIL;
	}

	if( i < 1 || i > lpTableInfo->field_num ) {
		dwErrorCode = 8004;
		return E_FAIL;
	}

	*pVal = lpColInfo[i-1].fieldtype;

	return S_OK;
}
