#include <windows.h>
#include "asqlana.h"
/*
int FAR PASCAL LibMain ( HANDLE hModule, WORD wDataSeg, WORD wHeapSize, LPSTR lpszCmdLine )
{
   if( wHeapSize > 0 )
		  UnlockData( 0 );

   return( 1 );
}

int FAR PASCAL WEP ( int bSystemExit )
{
   return( 1 );
}
*/
void  WINAPI AsqlSetEnv ( LPCSTR lpDestPath, LPCSTR lpSrcPath )
{
	lstrcpy( (LPSTR) asqlEnv.szAsqlResultPath, lpDestPath );
	lstrcpy( (LPSTR) asqlEnv.szAsqlFromPath, lpSrcPath );
}

BOOL  WINAPI AsqlExecFromFile ( LPCSTR AsqlStatFileName )
{
	if ( AskQS ( (char *)AsqlStatFileName, AsqlExprInFile, (dFILE *)NULL,
			(dFILE *) NULL, (SysVarOFunType **) NULL, (short *) NULL ) == 0 )
		return FALSE;

	return TRUE;
}
