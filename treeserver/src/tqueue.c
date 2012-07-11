/******************************
*	Module Name:
*		TQUEUE.C
*
*	Abstract:
*		Tree Server kernel request&reponse queue management
*	module. 
*
*	Author:
*		NiuJingyu.
*
*	Copyright (c) NiuJingyu 1997  China Railway Software Corporation
*
*	Revision History:
*		Write by NiuJingyu, 1997.10
*			队列由两部分构成：请求队列和响应队列。用户的请求由请求队列递交，
*			从响应队列取回结果。该方式只适用在一次连接中完成一次会话。
*		Rewrite by NiuJingyu, 1998.01
*			原来的队列的操作方式不能成在一次连接中完成多次会话，而客户断的这
*			种正常要求不能实现，则对队列进行了改写。
*		Update by NiuJingyu, 1998.03
*           添加了用户的管理。
********************************************************************************/

#define STRICT
#include <windows.h>
#include <tchar.h>

#include "terror.h"
#include "tqueue.h"
#include "shell.h"
#include "schedule.h"
#include "tuser.h"

LPQUEUE g_lpSharedQueue = NULL;
extern TCHAR szAuthServer[CNLEN+1];

LPQUEUE InitQueue ( DWORD dwSize, LPDWORD lpdwRetCode )
{
	UINT uQueueByteSize;
	DWORD dwErrorCode;
	HLOCAL hQueue;
	LPQUEUE lpQueue;
	DWORD i;
	BOOL fExist = FALSE;

	if( lpdwRetCode == NULL ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return NULL;
	}

	if( g_lpSharedQueue != NULL ) {
		//queue has been already initialized
		SetLastError( ERROR_INVALID_PARAMETER );
		return  NULL;
	}
	
	*lpdwRetCode = TERR_SUCCESS;

	// Check the queue size parameter, if invalid, use the default minimize size.
	if( dwSize < (DWORD)MIN_QUEUE )	{
		dwSize = MIN_QUEUE;
		*lpdwRetCode = TERR_WARN_QUEUE_TOO_SMALL;
	}

	// Calculate the total size of the memory block. 
	uQueueByteSize = sizeof(TQUEUE) + ( sizeof(HANDLE) + sizeof(REQUEST) ) * dwSize;
	
	// Alloc the memory.
	hQueue = LocalAlloc( LPTR, uQueueByteSize );
	if( hQueue == NULL ) {
		*lpdwRetCode = GetLastError();
		return NULL;
	}

	lpQueue = (LPQUEUE)LocalLock( hQueue );
	if( lpQueue == NULL ) {
		*lpdwRetCode = GetLastError();
		LocalFree( hQueue );
		return NULL;
	}

	// Create the read/write synchronization object.
	// Initially no thread owns the mutex.
	lpQueue->hQueueMutex = CreateMutex( 
			NULL,
			FALSE,
			_TEXT( "$TS_QueueMutex$" ) );
	dwErrorCode = GetLastError();
	if( dwErrorCode == ERROR_ALREADY_EXISTS )
		fExist = TRUE;

	// Initially the queue resource available, maximize resource count
	// use the user specified value.
	lpQueue->hResSemaphore = CreateSemaphore( 
			NULL, 
			dwSize,
			(LONG)dwSize,
			_TEXT( "$TS_ResSemaphore$" ) );
	dwErrorCode = GetLastError();
	if( dwErrorCode == ERROR_ALREADY_EXISTS )
		fExist = TRUE;

	// Initially no user use the queue, maximize resource count use the user
	// specified value.
	lpQueue->hUserSemaphore = CreateSemaphore( 
			NULL, 
			0, 
			(LONG)dwSize, 
			_TEXT( "$TS_UserSemaphore$" ) );
	dwErrorCode = GetLastError();
	if( dwErrorCode == ERROR_ALREADY_EXISTS )
		fExist = TRUE;
	
	// One of the synchronization object create failure.
	if( lpQueue->hQueueMutex == NULL || 
			lpQueue->hUserSemaphore == NULL ||
			lpQueue->hResSemaphore == NULL ||
			fExist ) {
		
		if( fExist )
			*lpdwRetCode = TERR_SYNC_OBJECT_EXIST;
		else
			*lpdwRetCode = GetLastError();

		ReleaseQueue();
		return NULL;
	}

	// Initialize the queue structure.
	lpQueue->lphEvent = (LPHANDLE)( (LPSTR)lpQueue + sizeof(TQUEUE) );
	lpQueue->lpQueue = (LPREQUEST)( (LPSTR)lpQueue + sizeof(TQUEUE) 
									+ sizeof(HANDLE) * dwSize );
	lpQueue->dwTotal = dwSize;

	// Create the synchronization event, default status is nonsignaled.
	for( i = 0; i < dwSize; i++ ) {
		TCHAR szEventName[80];

		wsprintf( szEventName, _TEXT( "$TS_QueueEvent%X$" ), i );
		lpQueue->lphEvent[i] = CreateEvent( NULL, FALSE, FALSE, szEventName );
		
		dwErrorCode = GetLastError();
		if( dwErrorCode == ERROR_ALREADY_EXISTS )
			fExist = TRUE;

		if( lpQueue->lphEvent == NULL || fExist ) {
			if( fExist )
				*lpdwRetCode = TERR_SYNC_OBJECT_EXIST;
			else
				*lpdwRetCode = GetLastError();

			ReleaseQueue();
			return NULL;
		}
	}

	return lpQueue;
}
	
DWORD ReleaseQueue ( VOID )
{
	HLOCAL hQueue;
	DWORD i;

	// Check the pointer parameter if available.
	if( g_lpSharedQueue == NULL )
		return ERROR_INVALID_PARAMETER;
	
	// Retrieves the handle associated with the specified pointer 
	// to a local memory object.
	hQueue = LocalHandle( (LPCVOID)g_lpSharedQueue );
	if( hQueue == NULL )
		return GetLastError();

	// Close the synchronization object handles.
	for( i = 0; i < g_lpSharedQueue->dwTotal; i++ ) {
		if( g_lpSharedQueue->lphEvent[i] )
			CloseHandle( g_lpSharedQueue->lphEvent[i] );
	}

	if( g_lpSharedQueue->hQueueMutex )
		CloseHandle( g_lpSharedQueue->hQueueMutex );
	if(	g_lpSharedQueue->hUserSemaphore )
		CloseHandle( g_lpSharedQueue->hUserSemaphore );
	if(	g_lpSharedQueue->hResSemaphore )
		CloseHandle( g_lpSharedQueue->hResSemaphore );

	// Free the local memory object and invalidates its handle.  
	LocalUnlock( hQueue );
	LocalFree( hQueue );

	g_lpSharedQueue = NULL;

	return TERR_SUCCESS;
}

DWORD SendRequest ( LPREQUEST lpRequest )
{
	DWORD dwRetCode;
	DWORD dwResult;
	DWORD dwPointer;
	// BOOL fSuccess;
	
	if( lpRequest == NULL )
		return ERROR_INVALID_PARAMETER;

	dwRetCode = TERR_SUCCESS;

	// This moved to Function MsgToRequest
	// By Jingyu Niu, 1999.08.17
	// Use the current thread id to indicate the request(It's unique).
	//lpRequest->dwRequestId = GetCurrentThreadId();

	GetLocalTime( &(lpRequest->Time) );

	_try {		
		// Wait the resource semaphore is available, this indicate that the queue 
		// not full. Then this thread is allowed access the queue.
		dwResult = WaitForSingleObject( g_lpSharedQueue->hResSemaphore, INFINITE );

		// Wait the mutex is signaled, this indicate no other threads are 
		// accessing the queue.	Then this thread allowed access the queue.
		dwResult = WaitForSingleObject( g_lpSharedQueue->hQueueMutex, INFINITE );

		// Get the current tail pointer.
		dwPointer = g_lpSharedQueue->dwPointerIn;
		
		//forcibly to reset the event
		ResetEvent( g_lpSharedQueue->lphEvent[dwPointer] );

		// Write the request to queue.
		CopyMemory( (PVOID)&(g_lpSharedQueue->lpQueue[dwPointer]),
				(CONST VOID *)lpRequest, sizeof(REQUEST) );

		// Updata the queue control header.
		g_lpSharedQueue->dwPointerIn = (dwPointer+1)%(g_lpSharedQueue->dwTotal);

		// Increment the queue's use count.
		ReleaseSemaphore( g_lpSharedQueue->hUserSemaphore, 1, NULL );

		// Release the Mutex, allow other threads to use the synchronization object.
		ReleaseMutex( g_lpSharedQueue->hQueueMutex );
	}

	_finally {
		// Wait the event is signaled, this indicate the service thread
		// already processed the request.
		dwResult = WaitForSingleObject( g_lpSharedQueue->lphEvent[dwPointer], INFINITE );

		// Here can not lock the queue, because this solt already used by this thread,
		// not another thread can access this solt.
		// dwResult = WaitForSingleObject( lpSharedQueue->hQueueMutex, INFINITE );

		// Read from request queue to user buffer.
		CopyMemory( (PVOID)lpRequest, 
				(CONST VOID *)&(g_lpSharedQueue->lpQueue[dwPointer]), sizeof(REQUEST) );

		//////////////////////////////////////////////////////////////////////////////
		// This only for debug.
		FillMemory( (LPVOID)&(g_lpSharedQueue->lpQueue[dwPointer]), sizeof(REQUEST), 0 );
		//////////////////////////////////////////////////////////////////////////////

		ReleaseSemaphore( g_lpSharedQueue->hResSemaphore, 1, NULL );

		// Release the Mutex, allow other threads to use the synchronization object.
		//ReleaseMutex( lpSharedQueue->hQueueMutex );
	}
	
	return dwRetCode;
}

DWORD ProcessRequest ( LPREQUEST lpRequest )
{
	DWORD dwRetCode;
	DWORD dwResult;
	DWORD dwPointer;
	HANDLE hThread;
	DWORD hToken;
	BOOL fSuccess;
	
	if( lpRequest == NULL )
		return ERROR_INVALID_PARAMETER;

	dwRetCode = TERR_SUCCESS;

	_try {
		// Wait for the queue havs available job to process.
		dwResult = WaitForSingleObject( g_lpSharedQueue->hUserSemaphore, INFINITE );

		// Wait the mutex is signaled, this indicate no other threads are 
		// accessing the queue.	Then this thread allowed access the queue.
		dwResult = WaitForSingleObject( g_lpSharedQueue->hQueueMutex, INFINITE );

		// Get the current tail pointer.
		dwPointer = g_lpSharedQueue->dwPointerOut;

		// Read from request queue to user buffer.
		CopyMemory( (PVOID)lpRequest, 
				(CONST VOID *)&(g_lpSharedQueue->lpQueue[dwPointer]), sizeof(REQUEST) );

		// Updata the queue control header.
		g_lpSharedQueue->dwPointerOut = (dwPointer+1)%(g_lpSharedQueue->dwTotal);
		
		// Release the Mutex, allow other threads to use the synchronization object.
		ReleaseMutex( g_lpSharedQueue->hQueueMutex );
	}

	_finally {
		dwResult = TERR_SUCCESS;
		
		// Audit user.
		fSuccess = TUserLogon( 
  				lpRequest->szUser,
				lpRequest->szPasswd,
				lpRequest->szComputer,
				&hToken	);
		if( !fSuccess ) {
			lpRequest->dwRequest = GetLastError();
			dwRetCode = lpRequest->dwRequest;
			goto _Response;
		}

		ZeroMemory( lpRequest->szPasswd, MAX_PASSWD+1 );
		
		// Alloc the exchange buffer.
		lpRequest->lpExchangeBuf = AllocExchngBuf();
		if( lpRequest->lpExchangeBuf == NULL ) {
		// If alloc exchange buffer falure.
			lpRequest->dwRequest = TERR_ALLOC_EXCHBUF_FAILURE;
			dwRetCode = lpRequest->dwRequest;
			goto _Response;
		}

		lpRequest->lpExchangeBuf->dwCliThreadId = lpRequest->dwRequestId;
		//lpRequest->dwRequestId = (DWORD)hToken;

		// Create Service Thread.
		hThread = CreateThread ( 
				NULL, 
				0,
				(LPTHREAD_START_ROUTINE)ServiceShell,
				(LPVOID)lpRequest,
				0,
				&(lpRequest->lpExchangeBuf->dwSrvThreadId) );
		if( hThread == NULL ) {
		// If alloc exchange buffer falure.
			lpRequest->dwRequest = GetLastError();
			dwRetCode = lpRequest->dwRequest;
			goto _Response;
		}

		TSRegisterUser( lpRequest );
	
		// Wait the mutex is signaled, this indicate no other threads are 
		// accessing the queue.	Then this thread allowed access the queue.
		//dwResult = WaitForSingleObject( lpSharedQueue->hQueueMutex, INFINITE );
		CloseHandle( hThread );
		lpRequest->dwRequest = TERR_SUCCESS;

_Response:
		GetLocalTime( &(lpRequest->Time) );

		// Write the response to the queue.
		CopyMemory( (PVOID)&(g_lpSharedQueue->lpQueue[dwPointer]), 
				(CONST VOID *)lpRequest, sizeof(REQUEST) );

		// Set the state of the synchronization event to signaled. 
		// Indicate the request has already processed.
		SetEvent( g_lpSharedQueue->lphEvent[dwPointer] );

		// Release the Mutex, allow other threads to use the synchronization object.
		//ReleaseMutex( lpSharedQueue->hQueueMutex );
	}
	
	return dwRetCode;
}
