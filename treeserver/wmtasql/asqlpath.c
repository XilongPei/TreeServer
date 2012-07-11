/****************
 * asqlpath.c
 *
 * Copyright (c) CRSC 1998
 *
 ****************************************************************************/

#define __ASQLPATH_C__

#include "asqlana.h"
#include "strutl.h"
#include "asqlpath.h"

char *szAsqlPath( char *oPath, char *szAsqlPath )
{
    /*
    if( oPath == NULL || szAsqlPath == NULL )
	return  NULL;
    */

    if( oPath[1] == ':' || oPath[0] == '\\' ) {
	strZcpy(szAsqlPath, oPath, MAXPATH);
    } else {
	if( *oPath != '^' ) {
		strcpy(szAsqlPath, asqlEnv.szAsqlFromPath);
		strncat(szAsqlPath, oPath, MAXPATH-strlen(szAsqlPath));
	} else {
		strcpy(szAsqlPath, asqlEnv.szAsqlResultPath);
		strncat(szAsqlPath, oPath+1, MAXPATH-strlen(szAsqlPath));
	}
    }

    return  szAsqlPath;

} //end of szAsqlPath()



////////////////////////// end of this file /////////////////////////////////
