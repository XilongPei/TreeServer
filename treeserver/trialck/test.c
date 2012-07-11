#include <stdio.h>
#include <windows.h>

void main( void )
{
	DWORD o = 102;
	HMODULE hLib;
	FARPROC p;

	hLib = LoadLibrary( "TERMCK.DLL" );
	p = GetProcAddress( hLib, o );
	o = (*p)();
	FreeLibrary( hLib );
	printf( "%d", o );
}