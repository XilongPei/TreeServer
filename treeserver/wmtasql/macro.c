/****************
* macro.c
* copyright (c) EastUnion Computer Service Co., Ltd.  1995
* author: MX Group
*****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "macro.h"
#include "mistring.h"


//if use UV kernal
#include "cfg_fast.h"
#include "ts_dict.h"

//#pragma argsused
static char *loadMarcoString( char *idString )
{
    char *seeds[10];
    char buf[128];
    char *s;

    strncpy(buf, idString, 128);
    buf[127] = '\0';

    seeds[2] = NULL;
    seeds[1] = NULL;
    seperateStr(buf, '@', seeds );

    if( seeds[2] == NULL ) {
	if( seeds[1] == NULL ) {
		s = GetCfgKey(csuDataDictionary, "STRRES", NULL, seeds[0]);
	} else {
		s = GetCfgKey(csuDataDictionary, seeds[0], NULL, seeds[1]);
	}
    } else {
	s = GetCfgKey(csuDataDictionary, seeds[0], seeds[1], seeds[2]);
    }

    if( s == NULL )	return 	idString;
    return  s;

} // end of loadMarcoString()


char *macro( char *deststring, short maxlen, char *macrostring, LOAD_STR_FUN fun )
{
    short   reader, writer;
    char  idstring[80], id;
    char  *res;

    reader = writer = 0;
    if( fun == NULL ) {
	fun = loadMarcoString;
    }

    maxlen--;	//for hole '\0'
    while( (writer < maxlen) && macrostring[reader] !='\0' )
    {
       if( macrostring[reader] == '%'&& \
	  macrostring[reader+1] != '\0' && macrostring[reader+1] != '%' )
	  {
	     // load macro string
	     {
	     id = 0;
	     while( macrostring[++reader] !='%' ) { 	// copy idstring
		idstring[id++] = macrostring[reader];
                if( macrostring[reader] == '\0' )
                    break;
	     }
	     idstring[id] = '\0';
             if( macrostring[reader] != '\0' )
	         reader++;
	     }
	     // load resource by idstring
	     res = (*fun)( idstring );
	     strcpy( &deststring[writer], res );
	     writer += strlen(res);
       } else {
	  deststring[writer++] = macrostring[reader++];
       }
    }
    deststring[writer] ='\0';

    return deststring;
}

/*
main()
{
    char   string[200];
    printf("%s is %s\n", "%PATH%SUB0",      macro( string, 200, "%PATH%SUB0", loadMarcoString));
    printf("%s is %s\n", "%%PATH%SUB0",     macro( string, 200, "%%PATH%SUB0", loadMarcoString));
    printf("%s is %s\n", "%%%PATH%SUB0",    macro( string, 200, "%%%PATH%SUB0", loadMarcoString));
    printf("%s is %s\n", "%PATH%%OK%SUB0",  macro( string, 200, "%PATH%%OK%SUB0", loadMarcoString));

}
*/