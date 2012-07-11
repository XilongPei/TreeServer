/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue Feb 13 10:12:48 2001
 */
/* Compiler settings for tsado.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __tsado_h__
#define __tsado_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __Command_FWD_DEFINED__
#define __Command_FWD_DEFINED__

#ifdef __cplusplus
typedef class Command Command;
#else
typedef struct Command Command;
#endif /* __cplusplus */

#endif 	/* __Command_FWD_DEFINED__ */


#ifndef __ICommand_FWD_DEFINED__
#define __ICommand_FWD_DEFINED__
typedef interface ICommand ICommand;
#endif 	/* __ICommand_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __TSADOLib_LIBRARY_DEFINED__
#define __TSADOLib_LIBRARY_DEFINED__

/* library TSADOLib */
/* [helpstring][version][uuid] */ 

typedef /* [public] */ struct  tagColInfo
    {
    BSTR szColName;
    unsigned char iType;
    int iSize;
    int iPrecision;
    }	COL_INFO;

typedef /* [public] */ COL_INFO __RPC_FAR *PCOL_INFO;

typedef /* [public] */ COL_INFO __RPC_FAR *LPCOL_INFO;


EXTERN_C const IID LIBID_TSADOLib;

EXTERN_C const CLSID CLSID_Command;

#ifdef __cplusplus

class DECLSPEC_UUID("CD238373-4F88-11D3-BFE1-0000E8E7CE21")
Command;
#endif
#endif /* __TSADOLib_LIBRARY_DEFINED__ */

#ifndef __ICommand_INTERFACE_DEFINED__
#define __ICommand_INTERFACE_DEFINED__

/* interface ICommand */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICommand;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CD238372-4F88-11D3-BFE1-0000E8E7CE21")
    ICommand : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Logon( 
            /* [in] */ BSTR szServer,
            /* [in] */ BSTR szUser,
            /* [in] */ BSTR szPasswd,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bSuccess) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Logoff( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ASQLClear( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ASQLAdd( 
            /* [in] */ BSTR szASQL) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ASQLExecute( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bSuccess) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TableOpen( 
            /* [in] */ BSTR szDatabase,
            /* [in] */ BSTR szTable,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bSuccess) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TableClose( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE First( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Next( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Prev( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Last( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RowCount( 
            /* [retval][out] */ long __RPC_FAR *lRowCnt) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ColumnCount( 
            /* [retval][out] */ int __RPC_FAR *iColCnt) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ColumnById( 
            /* [in] */ int iColId,
            /* [retval][out] */ BSTR __RPC_FAR *pValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ColumnByName( 
            /* [in] */ BSTR szColName,
            /* [retval][out] */ BSTR __RPC_FAR *pValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AboutBox( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_szASQLString( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_szASQLString( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EOF( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BOF( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ErrorCode( 
            /* [retval][out] */ unsigned long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_szASQLErrorMsg( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ASQLError( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColumnName( 
            /* [in] */ int i,
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColumnSize( 
            /* [in] */ int i,
            /* [retval][out] */ int __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColumnPrecision( 
            /* [in] */ int i,
            /* [retval][out] */ int __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColumnType( 
            /* [in] */ int i,
            /* [retval][out] */ unsigned char __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICommandVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICommand __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICommand __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICommand __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICommand __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICommand __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICommand __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICommand __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Logon )( 
            ICommand __RPC_FAR * This,
            /* [in] */ BSTR szServer,
            /* [in] */ BSTR szUser,
            /* [in] */ BSTR szPasswd,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bSuccess);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Logoff )( 
            ICommand __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ASQLClear )( 
            ICommand __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ASQLAdd )( 
            ICommand __RPC_FAR * This,
            /* [in] */ BSTR szASQL);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ASQLExecute )( 
            ICommand __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bSuccess);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TableOpen )( 
            ICommand __RPC_FAR * This,
            /* [in] */ BSTR szDatabase,
            /* [in] */ BSTR szTable,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bSuccess);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TableClose )( 
            ICommand __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *First )( 
            ICommand __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            ICommand __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Prev )( 
            ICommand __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Last )( 
            ICommand __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RowCount )( 
            ICommand __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *lRowCnt);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ColumnCount )( 
            ICommand __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *iColCnt);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ColumnById )( 
            ICommand __RPC_FAR * This,
            /* [in] */ int iColId,
            /* [retval][out] */ BSTR __RPC_FAR *pValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ColumnByName )( 
            ICommand __RPC_FAR * This,
            /* [in] */ BSTR szColName,
            /* [retval][out] */ BSTR __RPC_FAR *pValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AboutBox )( 
            ICommand __RPC_FAR * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_szASQLString )( 
            ICommand __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_szASQLString )( 
            ICommand __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EOF )( 
            ICommand __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BOF )( 
            ICommand __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ErrorCode )( 
            ICommand __RPC_FAR * This,
            /* [retval][out] */ unsigned long __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_szASQLErrorMsg )( 
            ICommand __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ASQLError )( 
            ICommand __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColumnName )( 
            ICommand __RPC_FAR * This,
            /* [in] */ int i,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColumnSize )( 
            ICommand __RPC_FAR * This,
            /* [in] */ int i,
            /* [retval][out] */ int __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColumnPrecision )( 
            ICommand __RPC_FAR * This,
            /* [in] */ int i,
            /* [retval][out] */ int __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColumnType )( 
            ICommand __RPC_FAR * This,
            /* [in] */ int i,
            /* [retval][out] */ unsigned char __RPC_FAR *pVal);
        
        END_INTERFACE
    } ICommandVtbl;

    interface ICommand
    {
        CONST_VTBL struct ICommandVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICommand_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICommand_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICommand_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICommand_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICommand_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICommand_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICommand_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICommand_Logon(This,szServer,szUser,szPasswd,bSuccess)	\
    (This)->lpVtbl -> Logon(This,szServer,szUser,szPasswd,bSuccess)

#define ICommand_Logoff(This)	\
    (This)->lpVtbl -> Logoff(This)

#define ICommand_ASQLClear(This)	\
    (This)->lpVtbl -> ASQLClear(This)

#define ICommand_ASQLAdd(This,szASQL)	\
    (This)->lpVtbl -> ASQLAdd(This,szASQL)

#define ICommand_ASQLExecute(This,bSuccess)	\
    (This)->lpVtbl -> ASQLExecute(This,bSuccess)

#define ICommand_TableOpen(This,szDatabase,szTable,bSuccess)	\
    (This)->lpVtbl -> TableOpen(This,szDatabase,szTable,bSuccess)

#define ICommand_TableClose(This)	\
    (This)->lpVtbl -> TableClose(This)

#define ICommand_First(This)	\
    (This)->lpVtbl -> First(This)

#define ICommand_Next(This)	\
    (This)->lpVtbl -> Next(This)

#define ICommand_Prev(This)	\
    (This)->lpVtbl -> Prev(This)

#define ICommand_Last(This)	\
    (This)->lpVtbl -> Last(This)

#define ICommand_RowCount(This,lRowCnt)	\
    (This)->lpVtbl -> RowCount(This,lRowCnt)

#define ICommand_ColumnCount(This,iColCnt)	\
    (This)->lpVtbl -> ColumnCount(This,iColCnt)

#define ICommand_ColumnById(This,iColId,pValue)	\
    (This)->lpVtbl -> ColumnById(This,iColId,pValue)

#define ICommand_ColumnByName(This,szColName,pValue)	\
    (This)->lpVtbl -> ColumnByName(This,szColName,pValue)

#define ICommand_AboutBox(This)	\
    (This)->lpVtbl -> AboutBox(This)

#define ICommand_get_szASQLString(This,pVal)	\
    (This)->lpVtbl -> get_szASQLString(This,pVal)

#define ICommand_put_szASQLString(This,newVal)	\
    (This)->lpVtbl -> put_szASQLString(This,newVal)

#define ICommand_get_EOF(This,pVal)	\
    (This)->lpVtbl -> get_EOF(This,pVal)

#define ICommand_get_BOF(This,pVal)	\
    (This)->lpVtbl -> get_BOF(This,pVal)

#define ICommand_get_ErrorCode(This,pVal)	\
    (This)->lpVtbl -> get_ErrorCode(This,pVal)

#define ICommand_get_szASQLErrorMsg(This,pVal)	\
    (This)->lpVtbl -> get_szASQLErrorMsg(This,pVal)

#define ICommand_get_ASQLError(This,pVal)	\
    (This)->lpVtbl -> get_ASQLError(This,pVal)

#define ICommand_get_ColumnName(This,i,pVal)	\
    (This)->lpVtbl -> get_ColumnName(This,i,pVal)

#define ICommand_get_ColumnSize(This,i,pVal)	\
    (This)->lpVtbl -> get_ColumnSize(This,i,pVal)

#define ICommand_get_ColumnPrecision(This,i,pVal)	\
    (This)->lpVtbl -> get_ColumnPrecision(This,i,pVal)

#define ICommand_get_ColumnType(This,i,pVal)	\
    (This)->lpVtbl -> get_ColumnType(This,i,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_Logon_Proxy( 
    ICommand __RPC_FAR * This,
    /* [in] */ BSTR szServer,
    /* [in] */ BSTR szUser,
    /* [in] */ BSTR szPasswd,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bSuccess);


void __RPC_STUB ICommand_Logon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_Logoff_Proxy( 
    ICommand __RPC_FAR * This);


void __RPC_STUB ICommand_Logoff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_ASQLClear_Proxy( 
    ICommand __RPC_FAR * This);


void __RPC_STUB ICommand_ASQLClear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_ASQLAdd_Proxy( 
    ICommand __RPC_FAR * This,
    /* [in] */ BSTR szASQL);


void __RPC_STUB ICommand_ASQLAdd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_ASQLExecute_Proxy( 
    ICommand __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bSuccess);


void __RPC_STUB ICommand_ASQLExecute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_TableOpen_Proxy( 
    ICommand __RPC_FAR * This,
    /* [in] */ BSTR szDatabase,
    /* [in] */ BSTR szTable,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bSuccess);


void __RPC_STUB ICommand_TableOpen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_TableClose_Proxy( 
    ICommand __RPC_FAR * This);


void __RPC_STUB ICommand_TableClose_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_First_Proxy( 
    ICommand __RPC_FAR * This);


void __RPC_STUB ICommand_First_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_Next_Proxy( 
    ICommand __RPC_FAR * This);


void __RPC_STUB ICommand_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_Prev_Proxy( 
    ICommand __RPC_FAR * This);


void __RPC_STUB ICommand_Prev_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_Last_Proxy( 
    ICommand __RPC_FAR * This);


void __RPC_STUB ICommand_Last_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_RowCount_Proxy( 
    ICommand __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *lRowCnt);


void __RPC_STUB ICommand_RowCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_ColumnCount_Proxy( 
    ICommand __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *iColCnt);


void __RPC_STUB ICommand_ColumnCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_ColumnById_Proxy( 
    ICommand __RPC_FAR * This,
    /* [in] */ int iColId,
    /* [retval][out] */ BSTR __RPC_FAR *pValue);


void __RPC_STUB ICommand_ColumnById_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_ColumnByName_Proxy( 
    ICommand __RPC_FAR * This,
    /* [in] */ BSTR szColName,
    /* [retval][out] */ BSTR __RPC_FAR *pValue);


void __RPC_STUB ICommand_ColumnByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICommand_AboutBox_Proxy( 
    ICommand __RPC_FAR * This);


void __RPC_STUB ICommand_AboutBox_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICommand_get_szASQLString_Proxy( 
    ICommand __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB ICommand_get_szASQLString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ICommand_put_szASQLString_Proxy( 
    ICommand __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ICommand_put_szASQLString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICommand_get_EOF_Proxy( 
    ICommand __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB ICommand_get_EOF_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICommand_get_BOF_Proxy( 
    ICommand __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB ICommand_get_BOF_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICommand_get_ErrorCode_Proxy( 
    ICommand __RPC_FAR * This,
    /* [retval][out] */ unsigned long __RPC_FAR *pVal);


void __RPC_STUB ICommand_get_ErrorCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICommand_get_szASQLErrorMsg_Proxy( 
    ICommand __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB ICommand_get_szASQLErrorMsg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICommand_get_ASQLError_Proxy( 
    ICommand __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB ICommand_get_ASQLError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICommand_get_ColumnName_Proxy( 
    ICommand __RPC_FAR * This,
    /* [in] */ int i,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB ICommand_get_ColumnName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICommand_get_ColumnSize_Proxy( 
    ICommand __RPC_FAR * This,
    /* [in] */ int i,
    /* [retval][out] */ int __RPC_FAR *pVal);


void __RPC_STUB ICommand_get_ColumnSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICommand_get_ColumnPrecision_Proxy( 
    ICommand __RPC_FAR * This,
    /* [in] */ int i,
    /* [retval][out] */ int __RPC_FAR *pVal);


void __RPC_STUB ICommand_get_ColumnPrecision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICommand_get_ColumnType_Proxy( 
    ICommand __RPC_FAR * This,
    /* [in] */ int i,
    /* [retval][out] */ unsigned char __RPC_FAR *pVal);


void __RPC_STUB ICommand_get_ColumnType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICommand_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
