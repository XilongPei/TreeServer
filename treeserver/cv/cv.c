#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

int main( int argc, char *argv[] )
{
	int i;

	if( argc < 2 )
		return 0;

	for( i = 1; i < argc; i++ ) {
		char buffer[4096];

		int fd = open( argv[i], O_BINARY | O_RDWR );
		if( fd < 0 ) {
			printf( "Process file '%s' error : %s\n", argv[i], strerror( errno ) );
			continue;
		}

		while( !eof( fd ) ) {
			char *p;
			int bytes = read( fd, buffer, 4096 );
			if( !bytes )
				continue;

			for( p = buffer; p < buffer+4096; p++ ) {
				if( *p == 0x0A || *p == 0x0D )
					continue;

				if( *p < 0x20 || *p > 0x7E )
					if( *(p+1) == 0x0A || *(p+1) == 0x0D )
						*p = 0x20;
			}

			lseek( fd, -bytes, SEEK_CUR );
			write( fd, buffer, bytes );
		}

		close( fd );
		printf( "Process file '%s' success.\n", argv[i] );
	}

	return 0;
}
