	
// Command.h : Declaration of the CCommand

#ifndef __COMMAND_H_
#define __COMMAND_H_

#include "resource.h"       // main symbols
#include "tscapi.h"	// Added by ClassView

/////////////////////////////////////////////////////////////////////////////
// CCommand
class ATL_NO_VTABLE CCommand : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCommand, &CLSID_Command>,
	public IDispatchImpl<ICommand, &IID_ICommand, &LIBID_TSADOLib>
{
public:
	CCommand()
	{
		m_pUnkMarshaler = NULL;

		ASQLErrorMsg = TEXT("");
		ASQLString = TEXT("");
		bASQLError = FALSE;
		bLogoned = FALSE;
		dwErrorCode = 0;
		dwTableId = 0;
	}

	~CCommand()
	{
		TableClose();
		Logoff();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_COMMAND)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCommand)
	COM_INTERFACE_ENTRY(ICommand)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

// ICommand
public:
	STDMETHOD(get_ColumnType)(/*[in]*/ int i, /*[out, retval]*/ unsigned char *pVal);
	STDMETHOD(get_ColumnPrecision)(/*[in]*/ int i, /*[out, retval]*/ int *pVal);
	STDMETHOD(get_ColumnSize)(/*[in]*/ int i, /*[out, retval]*/ int *pVal);
	STDMETHOD(get_ColumnName)(/*[in]*/ int i, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_ASQLError)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_szASQLErrorMsg)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_ErrorCode)(/*[out, retval]*/ unsigned long *pVal);
	STDMETHOD(get_BOF)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_EOF)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_szASQLString)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_szASQLString)(/*[in]*/ BSTR newVal);
	STDMETHOD(AboutBox)();
	STDMETHOD(ColumnByName)( /*[in]*/ BSTR szColName, /*[out, retval]*/ BSTR* pValue );
	STDMETHOD(ColumnById)( /*[in]*/ int iColId, /*[out, retval]*/ BSTR *pValue );
	STDMETHOD(ColumnCount)( /*[out, retval]*/ int *iColCnt );
	STDMETHOD(RowCount)( /*[out, retval]*/ long *lRowCnt );
	STDMETHOD(Last)();
	STDMETHOD(Prev)();
	STDMETHOD(Next)();
	STDMETHOD(First)();
	STDMETHOD(TableClose)();
	STDMETHOD(TableOpen)( /*[in]*/ BSTR szDatabase, /*[in]*/ BSTR szTable, /*[out, retval]*/ VARIANT_BOOL *bSuccess );
	STDMETHOD(ASQLExecute)( /*[out,retval]*/ VARIANT_BOOL *bSuccess );
	STDMETHOD(ASQLAdd)( /*[in]*/ BSTR szASQL );
	STDMETHOD(ASQLClear)();
	STDMETHOD(Logoff)();
	STDMETHOD(Logon)( /*[in]*/ BSTR szServer, /*[in]*/ BSTR szUser, /*[in]*/ BSTR szPasswd, /*[out, retval]*/ VARIANT_BOOL *bSuccess );
private:
	DWORD dwUserId;
	BOOL bEOF;
	BOOL bBOF;
	TSCAPI::LPDIO_FIELD lpColInfo;
	LONG lCurrentRow;
	LPSTR lpRowData;
	DWORD dwTableId;
	TSCAPI::LPTABLE_INFO lpTableInfo;
	CString ASQLString;
	BOOL bASQLError;
	CString ASQLErrorMsg;
	DWORD dwErrorCode;
	BOOL bLogoned;
	HANDLE hConnect;
};

#endif //__COMMAND_H_
