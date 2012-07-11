/******************************
*	Module Name:
*		TRunInfo.H
*
*	Abstract:
*		Tree Server Runtime Info Map File and mutex
*       for Control Console use them 
*
*	Author:
*		Xilong Pei.
*
*	Copyright (c) 1999  China Railway Software Corporation.
*
*	Revision History:
*		Write by Xilong Pei, 1999.3
********************************************************************************/

#include <windows.h>
#include "TRunInfo.h"

HANDLE	hTreeRunInfoMapFile;
LPSTR	lpTreeRunInfoMapAddress;
HANDLE	hTreeRunInfoConnectionMutex;

int CreateRunInfoMapForTreeSVR( void )
{

	__try
	{
		// Create file mapping for connection buffer, which is used for 
		// exchanging connection information between client and server
		// sides.
		hTreeRunInfoMapFile = CreateFileMapping( ( HANDLE )0xFFFFFFFF,
			(LPSECURITY_ATTRIBUTES)NULL,
			PAGE_READWRITE,
			0,
			4096,
			"TreeRunInfo" );
		if( hTreeRunInfoMapFile == NULL )
		{
			// dwRetCode = TERR_FILE_MAPPING_ERROR;
			return  1;
		}

		// This makes the specified portion of the file ( In this case,
		// our file-mapping object is backed by the operating-system paging 
		// file ) visible in the address space of the calling process. 
		lpTreeRunInfoMapAddress = MapViewOfFile( hTreeRunInfoMapFile,
			FILE_MAP_ALL_ACCESS,
			0, 
			0,
			0 );
		if( lpTreeRunInfoMapAddress == NULL )
		{
			// dwRetCode = TERR_MAP_VIEW_ERROR;
			CloseHandle( hTreeRunInfoMapFile );
			return  2;
		}

		// Create hTreeRunInfoConnectionMutex event. This object is used for 
		// mutual-exclusion access to establishing connection.
		hTreeRunInfoConnectionMutex = CreateMutex( NULL, FALSE, "TRUNINFO" );
		if( hTreeRunInfoConnectionMutex == NULL )
		{
			// dwRetCode = TERR_CREATE_CONNECTION_MUTEX_ERROR;
			UnmapViewOfFile( lpTreeRunInfoMapAddress );
			CloseHandle( hTreeRunInfoMapFile );
			return  3;
		}
	}

	_finally
	{
		return  0;
	}

} //end of CreateRunInfoMapForTreeSVR()


///////////////////////////////////////////////////////////////////////
int freeRunInfoMapForTreeSVR( void )
{
		if( hTreeRunInfoConnectionMutex != NULL )
			CloseHandle( hTreeRunInfoConnectionMutex );

		if( lpTreeRunInfoMapAddress != NULL )
			UnmapViewOfFile( lpTreeRunInfoMapAddress );

		if( hTreeRunInfoMapFile != NULL )
			CloseHandle( hTreeRunInfoMapFile );

		return  0;

} //end of freeRunInfoMapForTreeSVR()


/////////////////////////////////////////////////////////////////////////
int OpenRunInfoMapForTreeSVR( void )
{

	__try
	{
		// open file mapping for connection buffer, which is used for 
		// exchanging connection information between client and server
		// sides.
		hTreeRunInfoMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS,
			FALSE,
			"TreeRunInfo" );
		if( hTreeRunInfoMapFile == NULL )
		{
			// dwRetCode = TERR_FILE_MAPPING_ERROR;
			return  1;
		}

		// This makes the specified portion of the file ( In this case,
		// our file-mapping object is backed by the operating-system paging 
		// file ) visible in the address space of the calling process. 
		lpTreeRunInfoMapAddress = MapViewOfFile( hTreeRunInfoMapFile,
			FILE_MAP_ALL_ACCESS,
			0, 
			0,
			0 );
		if( lpTreeRunInfoMapAddress == NULL )
		{
			// dwRetCode = TERR_MAP_VIEW_ERROR;
			CloseHandle( hTreeRunInfoMapFile );
			return  2;
		}

		// Create hTreeRunInfoConnectionMutex event. This object is used for 
		// mutual-exclusion access to establishing connection.
		hTreeRunInfoConnectionMutex = CreateMutex( NULL, FALSE, "TRUNINFO" );
		if( hTreeRunInfoConnectionMutex == NULL )
		{
			// dwRetCode = TERR_CREATE_CONNECTION_MUTEX_ERROR;
			UnmapViewOfFile( lpTreeRunInfoMapAddress );
			CloseHandle( hTreeRunInfoMapFile );
			return  3;
		}
	}

	_finally
	{
		return  0;
	}

} //end of CreateRunInfoMapForTreeSVR()
