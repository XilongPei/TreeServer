#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys\stat.h>

#include "trialck.h"

int trialbuild ( int days, char *path )
{
	time_t t;
	int fd;
	long left, used;
	long tmp;
	char trialfile[255];

	if( path ) {
		strcpy( trialfile, path );
		strcat( trialfile, "\\" );
		strcat( trialfile,  TERMFILE );
	}
	else
		sprintf( trialfile, "%s", TERMFILE );

	t = time( &t );
	
	if( (fd = open( trialfile, O_BINARY | O_CREAT | O_TRUNC | O_RDWR, S_IREAD | S_IWRITE ) ) < 0 )
		return -1;

	used = 0;
	left = days * 24 * 60 * 60;

	/* build time */
	write( fd, &t, sizeof( time_t ) );
	/* Last use time */
	write( fd, &t, sizeof( time_t ) );
	/* Used time */
	tmp = ~used;
	write( fd, &tmp, sizeof( long ) );
	/* left time */
	tmp = ~left;
	write( fd, &tmp, sizeof( long ) );
	/* total time */
	tmp = ~left;
	write( fd, &tmp, sizeof( long ) );

	/* write the verify series */
	tmp = t ^ used;
	write( fd, &tmp, sizeof( long ) );

	tmp = t ^ left;
	write( fd, &tmp, sizeof( long ) );

	tmp = 0 ^ left;
	write( fd, &tmp, sizeof( long ) );

	close( fd );

	return 0;
}

int trialck ( void )
{
	time_t t1, t2, t;
	int fd;
	long days, used, left, total;
	long tmp;
	char f[255];
	char *p;

	GetModuleFileName( NULL, f, 255 );
	for( p = f + strlen( f ) - 1; p >= f; p-- ) {
		if( *p == '\\' ) {
			p++;
			break;
		}
	}
	*p = '\x0';

	strcat( f, TERMFILE );
	if( (fd = open( f, O_BINARY | O_RDWR, S_IREAD | S_IWRITE ) ) < 0 )
		return -1;

	read( fd, &t1, sizeof( time_t ) );
	read( fd, &t2, sizeof( time_t ) );

	read( fd, &tmp, sizeof( long ) );
	used = ~tmp;

	read( fd, &tmp, sizeof( long ) );
	left = ~tmp;

	read( fd, &tmp, sizeof( long ) );
	total = ~tmp;

	if( used + left != total ) {
		close( fd );
		return -1;
	}

	read( fd, &tmp, sizeof( long ) );
	if( tmp != (t1 ^ used) ) {
		close( fd );
		return -1;
	}
	
	read( fd, &tmp, sizeof( long ) );
	if( tmp != (t2 ^ left) ) {
		close( fd );
		return -1;
	}

	read( fd, &tmp, sizeof( long ) );
	if( tmp != ((t2-t1) ^ total) ) {
		close( fd );
		return -1;
	}

	t = time( &t );
	if( t - t2 < 0 ) {
		close( fd );
		return -1;
	}

	used += t - t2;
	left -= t - t2;

	if( left <= 0 )
		days = 0;
	else
		days = (left+12*60*60) / (24 * 60 * 60 );

	lseek( fd, sizeof( size_t ), SEEK_SET );

	/* Last use time */
	write( fd, &t, sizeof( time_t ) );
	/* Used time */
	tmp = ~used;
	write( fd, &tmp, sizeof( long ) );
	/* left time */
	tmp = ~left;
	write( fd, &tmp, sizeof( long ) );
	/* total time */
	tmp = ~total;
	write( fd, &tmp, sizeof( long ) );

	/* write the verify series */
	tmp = t1 ^ used;
	write( fd, &tmp, sizeof( long ) );

	tmp = t ^ left;
	write( fd, &tmp, sizeof( long ) );

	tmp = (t-t1) ^ total;
	write( fd, &tmp, sizeof( long ) );

	close( fd );

	return days;
}
