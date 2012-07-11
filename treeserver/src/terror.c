/* By Niu Jingyu 1998.05 */

#include <windows.h>
#include <lm.h>
#include <tchar.h>
#include "terror.h"

struct tagTerrorNoTable {
	DWORD terrNo;
	char  *errstr;
} TerrorNoTable[] = {
	 {TERR_WARN_QUEUE_TOO_SMALL, "TERR_WARN_QUEUE_TOO_SMALL"},
	 {TERR_QUEUE_ERROR, "TERR_QUEUE_ERROR"},
	 {TERR_QUEUE_ERROR, "TERR_QUEUE_ERROR"},
	 {TERR_QUEUE_FULL, "TERR_QUEUE_FULL"},
	 {TERR_QUEUE_EMPTY, "TERR_QUEUE_EMPTY"},
	 {TERR_QUEUE_NO_RESPONSE, "TERR_QUEUE_NO_RESPONSE"},
	 {TERR_INVALID_USER, "TERR_INVALID_USER"},
	 {TERR_INVALID_PARAMETER, "TERR_INVALID_PARAMETER"},
	 {TERR_INVALID_COMMAND, "TERR_INVALID_COMMAND"},
	 {TERR_SYNC_OBJECT_EXIST, "TERR_SYNC_OBJECT_EXIST"},
	 {TERR_INVALID_MESSAGE, "TERR_INVALID_MESSAGE"},
	 {TERR_INVALID_COMPUTER, "TERR_INVALID_COMPUTER"},
	 {TERR_NO_INVALID_TED, "TERR_NO_INVALID_TED"},
	 {TERR_INVALID_TED_LIBRARY, "TERR_INVALID_TED_LIBRARY"},
	 {TERR_INVALID_TED_COMMAND, "TERR_INVALID_TED_COMMAND"},
	 {TERR_INVALID_TED_ENTRY, "TERR_INVALID_TED_ENTRY"},
	 {TERR_INVALID_TED_CAPABILITY, "TERR_INVALID_TED_CAPABILITY"},
	 {TERR_INVALID_TED_TIMER, "TERR_INVALID_TED_TIMER"},
	 {TERR_INVALID_TED_IDLE, "TERR_INVALID_TED_IDLE"},
	 {TERR_INVALID_TED_TIMEOUT, "TERR_INVALID_TED_TIMEOUT"},
	 {TERR_INVALID_TED_ENABLE, "TERR_INVALID_TED_ENABLE"},
	 {TERR_TED_DISABLE, "TERR_TED_DISABLE"},
	 {TERR_TED_LAOD_FAILURE, "TERR_TED_LAOD_FAILURE"},
	 {TERR_TED_ERROR_ENTRY, "TERR_TED_ERROR_ENTRY"},
	 {TERR_INVALID_REQUEST, "TERR_INVALID_REQUEST"},
	 {TERR_NO_TEINFO_ENTRY, "TERR_NO_TEINFO_ENTRY"},
	 {TERR_INVALID_FLAG, "TERR_INVALID_FLAG"},
	 {TERR_INVALID_TASKTABLE, "TERR_INVALID_TASKTABLE"},
	 {TERR_ADJUST_TASKTABLE, "TERR_ADJUST_TASKTABLE"},
	 {TERR_INVALID_TASK, "TERR_INVALID_TASK"},
	 {TERR_NAMEDPIPE_TIMEOUT, "TERR_NAMEDPIPE_TIMEOUT"},
	 {TERR_INVALID_TASK_LOCATION, "TERR_INVALID_TASK_LOCATION"},
	 {TERR_ACCESS_VIOLATION, "TERR_ACCESS_VIOLATION"},
	 {TERR_INVALID_ADDRESS, "TERR_INVALID_ADDRESS"},
	 {TERR_INVALID_AUTHSERVER, "TERR_INVALID_AUTHSERVER"},
	 {TERR_ALLOC_EXCHBUF_FAILURE, "TERR_ALLOC_EXCHBUF_FAILURE"},
	 {TERR_NOT_LOGON, "TERR_NOT_LOGON"},
	 {TERR_CLIENT_CLOSE, "TERR_CLIENT_CLOSE"},
	 {TERR_SERVICE_CLOSE, "TERR_SERVICE_CLOSE"},
	 {TERR_NOT_ALL_COPYED, "TERR_NOT_ALL_COPYED"},
	 {TERR_SCRIPTS_TOO_LONG, "TERR_SCRIPTS_TOO_LONG"},
	 {TERR_DATA_TOO_LONG, "TERR_DATA_TOO_LONG"},
	 {TERR_CREATE_SP_FILE, "TERR_CREATE_SP_FILE"},
	 {TERR_OPEN_SP_FILE, "TERR_OPEN_SP_FILE"},
	 {TERR_SP_FILE_IN_USE, "TERR_SP_FILE_IN_USE"},
	 {TERR_CLOSE_SP_FILE, "TERR_CLOSE_SP_FILE"},
	 {TERR_INVALID_PROC_NAME, "TERR_INVALID_PROC_NAME"},
	 {TERR_WRITE_SP_FILE, "TERR_WRITE_SP_FILE"},
	 {TERR_READ_SP_FILE, "TERR_READ_SP_FILE"},
	 {TERR_PROC_NOT_EXIST, "TERR_PROC_NOT_EXIST"},
	 {TERR_PROC_ALREADY_EXIST, "TERR_PROC_ALREADY_EXIST"},
	 {TERR_USER_NO_PRIVILEGE, "TERR_USER_NO_PRIVILEGE"},

	 {TERR_USER_DB_INTERNAL_ERROR, "TERR_USER_DB_INTERNAL_ERROR"},
	 {TERR_USER_DB_IN_USE, "TERR_USER_DB_IN_USE"},
	 {TERR_CREATE_USER_DB, "TERR_CREATE_USER_DB"},
	 {TERR_OPEN_USER_DB, "TERR_OPEN_USER_DB"},
	 {TERR_CLOSE_USER_DB, "TERR_CLOSE_USER_DB"},
	 {TERR_READ_USER_DB, "TERR_READ_USER_DB"},
	 {TERR_WRITE_USER_DB, "TERR_WRITE_USER_DB"},
	 {TERR_NO_USER_DB, "TERR_NO_USER_DB"},
	 {TERR_USER_ALREADY_EXIST, "TERR_USER_ALREADY_EXIST"},
	 {TERR_USER_NOT_EXIST, "TERR_USER_NOT_EXIST"},
	 {TERR_INVALID_USER_NAME, "TERR_INVALID_USER_NAME"},
	 {TERR_INVALID_PASSWORD, "TERR_INVALID_PASSWORD"},
	 {TERR_INVALID_PATH, "TERR_INVALID_PATH"},
	 {TERR_INVALID_CNAME, "TERR_INVALID_CNAME"},
	 {TERR_INVALID_LOGON_HOURS, "TERR_INVALID_LOGON_HOURS"},
	 {TERR_INVALID_ACCESS_MASK, "TERR_INVALID_ACCESS_MASK"},
	 {TERR_INVALID_LOGON_COMPUTER, "TERR_INVALID_LOGON_COMPUTER"},
	 {TERR_USER_LOCKED, "TERR_USER_LOCKED"},
	 {TERR_CAN_NOT_ACCESS, "TERR_CAN_NOT_ACCESS"},
	 {TERR_TEMPLATE_NOT_EXIST, "TERR_TEMPLATE_NOT_EXIST"},
	 {0xFFFFFFFF, ""}
};

DWORD GetErrorMessage ( DWORD dwLastError, LPTSTR lpBuffer, DWORD cbBuffer )
{
    HMODULE hModule = NULL;
    DWORD dwBufferLength;
    DWORD dwFormatFlags;
    int   i;

    if( lpBuffer == NULL || cbBuffer == 0 )
		return ERROR_INVALID_PARAMETER;

    for( i = 0;  TerrorNoTable[i].terrNo != 0xFFFFFFFF;  i++ ) {
		if( TerrorNoTable[i].terrNo == dwLastError ) {
			wsprintf(lpBuffer, "#%d %s", dwLastError, TerrorNoTable[i].errstr);
			return ERROR_SUCCESS;
		}
    }

	dwFormatFlags = FORMAT_MESSAGE_IGNORE_INSERTS |
		    FORMAT_MESSAGE_FROM_SYSTEM ;

    // if dwLastError is in the network range, load the message source
    if(dwLastError >= NERR_BASE && dwLastError <= MAX_NERR) {
	hModule = LoadLibraryEx(
			_TEXT("netmsg.dll"),
	    NULL,
	    LOAD_LIBRARY_AS_DATAFILE );

	if(hModule != NULL)
            dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    // call FormatMessage() to allow for message text to be acquired
    // from the system or the supplied module handle
    dwBufferLength = FormatMessage(
        dwFormatFlags,
        hModule, // module to get message from (NULL == system)
        dwLastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
        lpBuffer,
        cbBuffer,
        NULL );

	if(hModule != NULL)
        FreeLibrary(hModule);

	if( dwBufferLength == 0 )
		return GetLastError();
	return ERROR_SUCCESS;
}

/*
DWORD GetErrorMessage ( DWORD dwLastError, LPTSTR lpBuffer, DWORD cbBuffer )
{
    HMODULE hModule = NULL;
    DWORD dwBufferLength;
    DWORD dwFormatFlags;

    if( lpBuffer == NULL || cbBuffer == 0 )
		return ERROR_INVALID_PARAMETER;

	if( dwLastError > TACCOUNT_ERROR_BASE ) {
		if( LoadString( GetModuleHandle( NULL ), dwLastError, lpBuffer, cbBuffer ) == 0 )
			lstrcpy( lpBuffer, "Unknow error code." );
		return 0;
	}

	dwFormatFlags = FORMAT_MESSAGE_IGNORE_INSERTS |
                    FORMAT_MESSAGE_FROM_SYSTEM ;

    // if dwLastError is in the network range, load the message source
    if(dwLastError >= NERR_BASE && dwLastError <= MAX_NERR) {
        hModule = LoadLibraryEx( 
			_TEXT("netmsg.dll"),
            NULL,
            LOAD_LIBRARY_AS_DATAFILE );

        if(hModule != NULL)
            dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    // call FormatMessage() to allow for message text to be acquired
    // from the system or the supplied module handle
    dwBufferLength = FormatMessage(
        dwFormatFlags,
        hModule, // module to get message from (NULL == system)
        dwLastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
        lpBuffer,
        cbBuffer,
        NULL );

	if(hModule != NULL)
        FreeLibrary(hModule);

	if( dwBufferLength == 0 )
		return GetLastError();
	return ERROR_SUCCESS;
}
*/
