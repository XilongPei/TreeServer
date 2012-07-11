/*****************
* treevent.c
* 1997.12.16 CRSC
***********************************************************************************/

#ifndef __treevent_h_
#define __treevent_h_

#include <windows.h>

BOOL treesvrInfoReg(
    HANDLE  hEventLog,	// handle returned by RegisterEventSource 
    WORD  wType,	// event type to log 
    WORD  wCategory,	// event category 
    DWORD  dwEventID,	// event identifier 
    PSID  lpUserSid,	// user security identifier (optional) 
    DWORD  dwDataSize,	// size of binary data, in bytes
    LPSTR lpStrings,	// array of strings to merge with message 
    LPVOID  lpRawData); 	// address of binary data 

#endif
