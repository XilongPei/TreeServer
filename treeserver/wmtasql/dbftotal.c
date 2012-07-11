/****************
* dbftotal.c
* 1998.12.24
*****************************************************************************/

#define __DBFTOTAL_C__


#include "dio.h"
#include "mistools.h"
#include "dbftotal.h"

/*
**-------------------------------------------------------------------------
** dbftotal()
** add table stored in dfs into table stored in dft. 
**------------------------------------------------------------------------*/
_declspec(dllexport) int dbftotal(char *szDft, char *szDfs)
{
    dFILE *dft, *dfs;

    dfs = dAwake( szDfs, DOPENPARA );
    if( dfs == NULL ) {
        return  -1;
    }

    dft = dAwake( szDft, DOPENPARA );
    if( dft == NULL ) {
        dSleep( dfs );
        return  -2;
    }

    dTableSum(dft, dfs);

    dSleep( dfs );
    dSleep( dft );

    return  1;

} //end of dbftotal()
