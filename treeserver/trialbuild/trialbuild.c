#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

typedef int (*TFUN)( int, char * );

void main( int argc, char *argv[] )
{
	HMODULE hLib;
	int days;
	TFUN p;
	char *path = NULL;

	if( argc < 2 ) {
		printf( "\nUSAGE trialbuild [days]\n" );
		exit( 0 );
	}
	if( argc = 3 )
		path = argv[2];

	days = atoi( argv[1] );
	
	hLib = LoadLibrary( "TRIALCK.DLL" );
	if( hLib == NULL ) {
		printf( "error build trial term.\n" );
		exit( 0 );
	}

	p = (TFUN)GetProcAddress( hLib, (LPSTR)101 );
	if( p == NULL ) {
		printf( "error build trial term.\n" );
		FreeLibrary( hLib );
		exit( 0 );
	}

	if( (*p)( days, path ) == 0 )
		printf( "build trial term success.\n" ); 
	else
		printf( "error build trial term.\n" );

	FreeLibrary( hLib );
}