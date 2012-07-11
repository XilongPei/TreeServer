/*********
 * dbs_util.c
 * author:  Xilong Pei
 ****************************************************************************/

#include "dir.h"
#include "cfg_fast.h"
#include "strutl.h"
#include "ts_dict.h"
#include "filetool.h"

int buildDatabase(char *parameter)
{
    char *s;
    char  buf[MAXPATH];

    s = GetCfgKey(csuDataDictionary, "SYSTEM", "DBROOT", "PATH");
    if( s == NULL ) {
	return  1;
    }

    //get [SYSTEM]
    //    #DBROOT
    //	   PATH=x:\
    //	   PATH=c:\asd\
    //	   PATH=c:abc\		//this is error path setting
    strZcpy(buf, s, MAXPATH);
    s = strrchr(buf, '\\');
    if( s != NULL ) {
	*s = '\0';
	s = strrchr(buf, '\\');
	if( s != NULL )
	    *s = '\0';
    }
    strcat(buf, "\\");
    strcat(buf, parameter);
    strcat(buf, "\\");

    WriteCfgKey( csuDataDictionary, "DATABASE", \
				    parameter, \
				    "PATH", \
				    buf );
    mkdirs(buf);

    WriteCfgKey( csuDataDictionary, parameter, \
				    "", \
				    "REM", \
				    "");
    return  0;

} //end of buildDatabase()


int dropDatabase(char *parameter)
{
    return  0;
}
