/* By Niu Jingyu 1998.03 */

#include <windows.h>
#include <stdio.h>
#include <lm.h>

PSID GetUserSid ( void )
{
    HANDLE hToken;
    BOOL fSuccess;
    BOOL bSuccess = FALSE;				// assume this function will fail
    DWORD cbBuffer, cbRequired;
    PTOKEN_USER pUserInfo = NULL;
    PSID pUserSid;

    __try {
		// The User's SID can be obtained from the process token
		fSuccess = OpenProcessToken(
				GetCurrentProcess(),
				TOKEN_QUERY,
				&hToken );
		if( FALSE == fSuccess )
			__leave;

		// Set buffer size to 0 for first call to determine
		// the size of buffer we need.
		cbBuffer = 0;
		fSuccess = GetTokenInformation(
				hToken,
				TokenUser,
				NULL,
				cbBuffer,
				&cbRequired );
		if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
			__leave;

		// Allocate a buffer for our token user data
		cbBuffer = cbRequired;
		pUserInfo = (PTOKEN_USER)HeapAlloc( GetProcessHeap(), 0, cbBuffer );
		if( NULL == pUserInfo )
			__leave;

	    // Make the "real" call
	    fSuccess = GetTokenInformation(
				hToken,
				TokenUser,
				pUserInfo,
				cbBuffer,
				&cbRequired );
	    if( FALSE == fSuccess )
			__leave;

	    // Make another copy of the SID for the return value
	    cbBuffer = GetLengthSid( pUserInfo->User.Sid );

		pUserSid = (PSID)HeapAlloc( GetProcessHeap(), 0, cbBuffer );
		if( NULL == pUserSid )
			__leave;

	    fSuccess = CopySid(cbBuffer, pUserSid, pUserInfo->User.Sid);
	    if( FALSE == fSuccess )
			__leave;

		HeapFree( GetProcessHeap(), 0, pUserInfo );
		pUserInfo = NULL;

		bSuccess = TRUE;
	}
	__finally {
	    // Cleanup and indicate failure, if appropriate.
		if( !bSuccess ) {
			if( NULL != pUserInfo )
				HeapFree( GetProcessHeap(), 0, pUserInfo );
			pUserSid = NULL;
		}
	}

    return pUserSid;
}

PSID CreateWorldSid ( void )
{
    SID_IDENTIFIER_AUTHORITY authWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID pSid = NULL, psidWorld = NULL;
    BOOL bRes;
    DWORD cbSid;
	
	__try {
		bRes = AllocateAndInitializeSid( &authWorld,
			    1,
			    SECURITY_WORLD_RID,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    &psidWorld);

		if( FALSE == bRes )
			__leave;

	    // Make a copy of the SID using a HeapAlloc'd block for return
	    cbSid = GetLengthSid( psidWorld );

		pSid = (PSID) HeapAlloc( GetProcessHeap(), 0, cbSid );
		if( NULL == pSid )
			__leave;

		bRes = CopySid( cbSid, pSid, psidWorld );
		if( FALSE == bRes )
			__leave;
	}
	__finally {
		if( psidWorld )
			FreeSid( psidWorld );
	}

    return pSid;
}

PSID CreateSystemSid ( void )
{
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PSID pSid = NULL, psidSystem = NULL;
    BOOL bRes;
    DWORD cbSid;
	
	__try {
		bRes = AllocateAndInitializeSid( &authNT,
			    1,
			    SECURITY_LOCAL_SYSTEM_RID,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    &psidSystem);

		if( FALSE == bRes )
			__leave;

	    // Make a copy of the SID using a HeapAlloc'd block for return
	    cbSid = GetLengthSid( psidSystem );

		pSid = (PSID) HeapAlloc( GetProcessHeap(), 0, cbSid );
		if( NULL == pSid )
			__leave;

		bRes = CopySid( cbSid, pSid, psidSystem );
		if( FALSE == bRes )
			__leave;
	}
	__finally {
		if( psidSystem )
			FreeSid( psidSystem );
	}

    return pSid;
}

PSID CreateAnonymousSid ( void )
{
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PSID pSid = NULL, psidAnonymous = NULL;
    BOOL bRes;
    DWORD cbSid;
	
	__try {
		bRes = AllocateAndInitializeSid( &authNT,
			    1,
			    SECURITY_ANONYMOUS_LOGON_RID,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    &psidAnonymous);

		if( FALSE == bRes )
			__leave;

	    // Make a copy of the SID using a HeapAlloc'd block for return
	    cbSid = GetLengthSid( psidAnonymous );

		pSid = (PSID) HeapAlloc( GetProcessHeap(), 0, cbSid );
		if( NULL == pSid )
			__leave;

		bRes = CopySid( cbSid, pSid, psidAnonymous );
		if( FALSE == bRes )
			__leave;
	}
	__finally {
		if( psidAnonymous )
			FreeSid( psidAnonymous );
	}

    return pSid;
}

PSID CreateInteractiveSid ( void )
{
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PSID pSid = NULL, psidInteractive = NULL;
    BOOL bRes;
    DWORD cbSid;
	
	__try {
		bRes = AllocateAndInitializeSid( &authNT,
			    1,
			    SECURITY_INTERACTIVE_RID,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    &psidInteractive);

		if( FALSE == bRes )
			__leave;

	    // Make a copy of the SID using a HeapAlloc'd block for return
	    cbSid = GetLengthSid( psidInteractive );

		pSid = (PSID) HeapAlloc( GetProcessHeap(), 0, cbSid );
		if( NULL == pSid )
			__leave;

		bRes = CopySid( cbSid, pSid, psidInteractive );
		if( FALSE == bRes )
			__leave;
	}
	__finally {
		if( psidInteractive )
			FreeSid( psidInteractive );
	}

    return pSid;
}

BOOL GetAccountSid ( LPTSTR SystemName, LPTSTR AccountName, PSID *Sid )
{
    LPTSTR ReferencedDomain = NULL;
    DWORD cbReferencedDomain = DNLEN+1;	// initial allocation size
    DWORD cbSid=128;					// initial allocation attempt
    BOOL bSuccess = FALSE;				// assume this function will fail
    SID_NAME_USE peUse;

    __try {
		// initial memory allocations
	    if( (*Sid=HeapAlloc(
                    GetProcessHeap(),
                    0,
                    cbSid
                    ) ) == NULL) 
			__leave;

	    if( (ReferencedDomain=HeapAlloc(
                    GetProcessHeap(),
                    0,
                    cbReferencedDomain
                    )) == NULL ) 
			__leave;

		// Obtain the SID of the specified account on the specified system.
		while( !LookupAccountName(
                    SystemName,         // machine to lookup account on
                    AccountName,        // account to lookup
                    *Sid,               // SID of interest
                    &cbSid,             // size of SID
                    ReferencedDomain,   // domain account was found on
                    &cbReferencedDomain,
                    &peUse
                    ) ) {
			if( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
			    // reallocate memory
			    if( (*Sid=HeapReAlloc(
			                GetProcessHeap(),
				            0,
				            *Sid,
				            cbSid
				            )) == NULL ) 
					__leave;

				if( (ReferencedDomain=HeapReAlloc(
				            GetProcessHeap(),
					        0,
						    ReferencedDomain,
						    cbReferencedDomain
						    )) == NULL ) 
					__leave;
			}
			else 
				__leave;
		}
	
	    bSuccess=TRUE;

	} // finally
    __finally {
	    // Cleanup and indicate failure, if appropriate.
		HeapFree( GetProcessHeap(), 0, ReferencedDomain );

		if( !bSuccess ) {
		    if( *Sid != NULL ) {
		        HeapFree( GetProcessHeap(), 0, *Sid );
		        *Sid = NULL;
		    }
		}
    } // finally

    return bSuccess;
}
/*
#define PERR(s) fprintf(stderr, "%s(%d) %s : Error %ld\n", \
		    __FILE__, __LINE__, (s), GetLastError() )

void main ( void )
{
	PSID pOwnerSid, pGroupSid;
    BOOL fSuccess;
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
    PACL pAcl;
    DWORD cbAcl;
	HANDLE hFile;
    DWORD cbMsg, dwBytesWritten;
    char szMsg[] = "TreeServer Security test application.\n";

    pOwnerSid = GetUserSid();
	fSuccess = GetAccountSid( NULL, "TreeServer Users", &pGroupSid );
    if ( !fSuccess ) {
		PERR( "GetAccountSid" );
		ExitProcess(EXIT_FAILURE);
    }

    cbAcl = GetLengthSid( pOwnerSid ) + GetLengthSid( pGroupSid ) +
	    sizeof(ACL) + (2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));

    pAcl = (PACL) HeapAlloc(GetProcessHeap(), 0, cbAcl);
    if (NULL == pAcl) {
		PERR( "HeapAlloc" );
		ExitProcess(EXIT_FAILURE);
    }

    fSuccess = InitializeAcl(pAcl,
		    cbAcl,
		    ACL_REVISION);
    if (FALSE == fSuccess) {
		PERR("InitializeAcl");
		ExitProcess(EXIT_FAILURE);
    }

	fSuccess = AddAccessAllowedAce(pAcl,
		    ACL_REVISION,
		    GENERIC_ALL,
		    pOwnerSid);
    if (FALSE == fSuccess) {
		PERR("AddAccessAllowedAce");
		ExitProcess(EXIT_FAILURE);
    }

    fSuccess = AddAccessAllowedAce(pAcl,
		    ACL_REVISION,
		    GENERIC_READ|GENERIC_WRITE,
		    pGroupSid);
    if (FALSE == fSuccess) {
		PERR("AddAccessAllowedAce");
		ExitProcess(EXIT_FAILURE);
    }

    InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );

    fSuccess = SetSecurityDescriptorDacl(&sd,
			TRUE,
			pAcl,
			FALSE);
    if (FALSE == fSuccess) {
		PERR("SetSecurityDescriptorDacl");
		ExitProcess(EXIT_FAILURE);
    }

	fSuccess =  SetSecurityDescriptorOwner(
			&sd,
			pOwnerSid,
			FALSE );  

    if ( !fSuccess ) {
		PERR( "SetSecurityDescriptorOwner" );
		ExitProcess(EXIT_FAILURE);
    }

	fSuccess =  SetSecurityDescriptorGroup(
			&sd,
			pGroupSid,
			FALSE );  

    if ( !fSuccess ) {
		PERR( "SetSecurityDescriptorGroup" );
		ExitProcess(EXIT_FAILURE);
    }

	sa.nLength = sizeof( SECURITY_ATTRIBUTES );
	sa.lpSecurityDescriptor = (LPVOID)&sd;
	sa.bInheritHandle = FALSE;

	hFile = CreateFile( 
			"Hello.TXT",
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			&sa,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL );
	if( hFile == INVALID_HANDLE_VALUE ) {
		PERR( "CreateFile" );
		ExitProcess(EXIT_FAILURE);
    }
	
    cbMsg = lstrlen(szMsg);

    fSuccess = WriteFile(hFile,
		     szMsg,
		     cbMsg,
		     &dwBytesWritten,
		     NULL);

    if (FALSE == fSuccess) {
		PERR("WriteFile");
		ExitProcess(EXIT_FAILURE);
    }

	FlushFileBuffers( hFile );
	CloseHandle( hFile );

	ExitProcess( 0 );
}
*/	