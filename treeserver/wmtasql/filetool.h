#ifndef __FILETOOL__
#define __FILETOOL__

long dupfile(char *sfile, char *tfile);
	/*
	* return copy length
	*/
long recvfile(char *sfile, char *tfile);

int fnewcmp( int h1, int h2 );

int nFileNewcmp( const char *file1, const char *file2 );

char *beSurePath( char *path );

long fileBytesMove( FILE *fp, long from_pos, long to_pos );

#ifdef DOS_COMPATABLE
unsigned short batchCopy( void *mesDisp, char *spath, char *files, char *dpath );
#endif

int mkdirs(const char *path);

#endif