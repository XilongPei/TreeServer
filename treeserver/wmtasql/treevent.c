/*****************
* treevent.c
* 1997.12.16 CRSC
***********************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "treevent.h"
#include "wst2mt.h"

BOOL treesvrInfoReg(
    HANDLE  hEventLog,	// handle returned by RegisterEventSource
    WORD  wType,	// event type to log
    WORD  wCategory,	// event category
    DWORD  dwEventID,	// event identifier
    PSID  lpUserSid,	// user security identifier (optional)
    DWORD  dwDataSize,	// size of binary data, in bytes
    LPSTR lpStrings,	// array of strings to merge with message
    LPVOID  lpRawData 	// address of binary data
) {

    char *buf[2];

    if( lServerAsRunning == 0 ) {
        FILE *hd;
	time_t timer;
	struct tm *tblock;

	/* gets time of day */
	timer = time(NULL);

	/* converts date/time to a structure */
	tblock = localtime(&timer);

	hd = fopen("TREEEVNT.LOG", "a+t");
        if( hd == NULL )
            return  FALSE;

        fseek(hd, 0, SEEK_END);
    fprintf(hd, "\n%s%s\n", asctime(tblock), lpStrings);
        fclose(hd);
        return  TRUE;
    }

    buf[0] = lpStrings;
    buf[1] = NULL;

    if( hEventLog == NULL ) {
	hEventLog = RegisterEventSource(NULL, "TreeServer");
	
	if( hEventLog == NULL )
	    return  FALSE;

	ReportEvent(
	    hEventLog,	// handle returned by RegisterEventSource
	    wType,	// event type to log
	    wCategory,	// event category
	    dwEventID,	// event identifier 
	    lpUserSid,	// user security identifier (optional) 
	    1,// number of strings to merge with message  
	    dwDataSize,	// size of binary data, in bytes
	    buf,	// array of strings to merge with message 
	    lpRawData); // address of binary data 
	return  DeregisterEventSource(hEventLog);
    } 
    return  ReportEvent(
	    hEventLog,	// handle returned by RegisterEventSource 
	    wType,	// event type to log 
	    wCategory,	// event category 
	    dwEventID,	// event identifier 
	    lpUserSid,	// user security identifier (optional) 
	    1,
	    dwDataSize,	// size of binary data, in bytes
	    buf,
	    lpRawData); // address of binary data 

} //end of treesvrInfoReg()


#ifdef _TEST_

#include <stdio.h>

void main( void )
{
	BOOL fSuccess;
	char *buf[8] = {" \0\0", "testaaadfdsgfdgfdsgfdsgdfsgfffffffffffffffffffffffffffffffffffffffffffffffffffffaaaaaaaaaa\x0\x0", NULL};

	fSuccess = treesvrInfoReg(
		NULL,	
		EVENTLOG_WARNING_TYPE,	
		0,	
		0,	
		NULL,	
		0,	
		"test 1234 \ttest ok",
		NULL);
	
}

#endif
