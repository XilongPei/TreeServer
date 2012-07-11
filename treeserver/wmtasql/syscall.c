/****************
* syscall.c
* 1995.4.20
*****************************************************************************/
#ifndef _WINDOWS_

#include <string.h>
#include <process.H>
#include <stdlib.H>
#include "dir.h"

#include "syscall.h"
#include "strutl.h"
#ifdef  __N_C_S
#include "uscreen.h"
#endif

int systemCall(const char *command)
{
    int  i;
    char buf[256];
    char path[MAXPATH];
    int  disk;

    getcwd(path, MAXPATH);

#ifdef DOS_COMPATABLE
    _dos_getdrive( &disk );
#else
    disk = _getdrive();
#endif
//    BackupWholeScreen();
    if( command == NULL || *command == '\0' ) {
	i = system("");
    } else {
	strZcpy(buf, command, 255);
	if( strchr(command, '>') == NULL ) {
		strcat(buf, ">NUL");
	}
	i = system( buf );
    }
    if( i == 0 ) {
	// get the exit() result
//      RestoreScreen();
#ifdef  __N_C_S
	FlushScreen();
#endif        
	return  0;
    }
//    RestoreScreen();
#ifdef  __N_C_S
    FlushScreen();
#endif
#ifdef DOS_COMPATABLE
    _dos_setdrive(disk, &i);
#else
    _chdrive(disk);
#endif
    chdir(path);

    return  -errno;

}
#endif