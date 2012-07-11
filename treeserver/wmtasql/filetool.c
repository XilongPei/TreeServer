/*************
 *
 * file tools
 *
 * Writen by Xilong Pei   1994-2000
 * copyright (c) Shanghai Withub Vision Software Co., Ltd 1999-2000
 *
 * 2000.5.21: chsize() in fileBytesMove() use len, but len is set error, correct this
 *
 **************************************************************************/

//#define  USE_FOR_UV

#include <dos.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <share.h>
#include <IO.H>
#include <string.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <memory.h>
#else
#include <mem.h>
#endif

#include <LIMITS.H>
#include <time.h>
#include <sys\stat.h>

#include "dir.h"
#include "filetool.h"
#include "mistring.h"
#include "memutl.h"

#ifdef USE_FOR_UV
	#include "view.h";
	#include "util.h";
#endif

/****************
*	return copy length
****************************************************************************/
long dupfile(char *sfile, char *tfile)
{
    long l, copybytes;
    int  shandle, thandle;
    unsigned short buflen = 32000;
    unsigned char *buf;
    int readlen;
    char nameBuf[MAXPATH];
    char drive[MAXDRIVE];
    char dir[MAXDIR];
    char file[MAXFILE];
    char ext[MAXEXT];

    if( sfile == NULL || tfile == NULL ) 	return  LONG_MIN;

    if( (shandle = sopen( sfile, O_RDONLY|O_BINARY, SH_DENYNO, \
				 S_IREAD|S_IWRITE) ) < 0 ) {
	return  -1L;
    }

    _splitpath(tfile, drive, dir, file, ext);
    strcpy(nameBuf, lrtrim( tfile ));
    if( file[0] == '\0' ) {
	// it is a path, make file filename with sfile
	makeTrimFilename( nameBuf, tfile, sfile );
    } else {
	strcpy( nameBuf, tfile );
    }

    if( (thandle = open( nameBuf, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, \
				  S_IWRITE) ) < 0 ) {
	close( shandle );
	return  -2L;
    }
    copybytes = l = filelength( shandle );

    // alloc memory for buf
    do {
	if( (buf = (unsigned char *)malloc( buflen ) ) == NULL ) {
		buflen /= 2;
	} else	break;

    } while( buflen != 0  && buf == NULL );

    if( buflen == 0 ) 	return  -3L;

    while( l > 0 ) {
	readlen = read(shandle, buf, buflen);
	if( readlen == -1 || readlen == 0 ) 	break;

	if( write(thandle, buf, readlen) != readlen ) break;
	l -= readlen;
    }

    free( buf );
    close( shandle );
    close( thandle );

    if( l >= 0 )    return  copybytes;
    return  -4L;

} // end of function dupfile()



/****************
*	return copy length
* sfile receive from tfile, tfile counld just be path
****************************************************************************/
long recvfile(char *sfile, char *tfile)
{
    long 	l, copybytes;
    int  	shandle, thandle;
    unsigned short buflen = 32000;
    unsigned char *buf;
    int 	readlen;
    char 	nameBuf[MAXPATH];
    char 	drive[MAXDRIVE];
    char 	dir[MAXDIR];
    char 	file[MAXFILE];
    char 	ext[MAXEXT];

    if( sfile == NULL || tfile == NULL ) 	return  LONG_MIN;

    if( (thandle = sopen( sfile, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, \
				 S_IREAD|S_IWRITE) ) < 0 ) {
	return  -1L;
    }

    _splitpath(tfile, drive, dir, file, ext);
    strcpy(nameBuf, lrtrim( tfile ));
    if( file[0] == '\0' ) {
	// it is a path, make file filename with sfile
	makeTrimFilename( nameBuf, tfile, sfile );
    } else {
	strcpy( nameBuf, tfile );
    }

    if( (shandle = open( nameBuf, O_RDONLY|O_BINARY, SH_DENYNO, \
				  S_IREAD|S_IWRITE) ) < 0 ) {
	close( shandle );
	return  -2L;
    }
    copybytes = l = filelength( shandle );

    // alloc memory for buf
    do {
	if( (buf = (unsigned char *)malloc( buflen ) ) == NULL ) {
		buflen /= 2;
	} else	break;

    } while( buflen != 0  && buf == NULL );

    if( buflen == 0 ) 	return  -3L;


    while( l > 0 ) {
	readlen = read(shandle, buf, buflen);
	if( readlen == -1 || readlen == 0 ) 	break;
	write(thandle, buf, readlen);
	l -= readlen;
    }

    free( buf );
    close( shandle );
    close( thandle );

    return  copybytes;

} // end of function dupfile()


/****************
*				fFileNewcmp
*  return > 0 h1 new than h2
****************************************************************************/
int fnewcmp( int h1, int h2 )
{
/*  struct stat {
  short  st_dev,   st_ino;
  short  st_mode,  st_nlink;
  int    st_uid,   st_gid;
  short  st_rdev;
  long   st_size,  st_atime;
  long   st_mtime, st_ctime;
};  */

    struct stat statbuf1, statbuf2;
    int    	i;

    /* get information about the file */
    if( fstat(h1, &statbuf1) < 0 )	return  INT_MIN;
    if( fstat(h2, &statbuf2) < 0 )	return  INT_MIN;

    i = statbuf1.st_ctime - statbuf2.st_ctime;

    return i;

} // end of function fnewcmp()



/****************
*				nFileNewcmp
*  return > 0 h1 new than h2
****************************************************************************/
int nFileNewcmp( const char *file1, const char *file2 )
{
/*  struct stat {
  short  st_dev,   st_ino;
  short  st_mode,  st_nlink;
  int    st_uid,   st_gid;
  short  st_rdev;
  long   st_size,  st_atime;
  long   st_mtime, st_ctime;
};  */

    struct stat statbuf1, statbuf2;
    int    	i;

    /* get information about the file */
    if( stat(file1, &statbuf1) < 0 )	return INT_MIN;
    if( stat(file2, &statbuf2) < 0 )	return INT_MIN;

    i = statbuf1.st_ctime - statbuf2.st_ctime;

    return i;

} // end of function fnewcmp()


/****************
*				beSurePath
****************************************************************************/
char *beSurePath( char *path )
{
    /*char drive[MAXDRIVE];
    char dir[MAXDIR];
    char file[MAXFILE];
    char ext[MAXEXT];
    */
	short len;
	
    if( path == NULL )		return( NULL );

    trim(path);
/*    _splitpath(path, drive, dir, file, ext);
    _makepath(path, drive, dir, "", "");
*/
    len = strlen( path );
    if( path[len-1] == ':' )
	strcat( path, ".\\" );
    else if( path[len-1] != '\\' )
	strcat( path, "\\" );

    return  path;

} // end of beSurePath()


/****************
*				fileBytesMove()
****************************************************************************/
long fileBytesMove( FILE *fp, long from_pos, long to_pos )
{
    char *p;
    int bufsize;
    int bytes;
    int fno, len;

    if( from_pos == to_pos )   return 0L;

    fno = fileno(fp);
    
    bufsize = 64000;
    p = tryMalloc( &bufsize, 16 );
    if( p == NULL )   return  -1L;

    if( from_pos > to_pos ) {
	bytes = bufsize;
	while( bytes >= bufsize ) {
		fseek( fp, from_pos, SEEK_SET );
		bytes = fread( p, 1, bufsize, fp);
		fseek( fp, to_pos, SEEK_SET );
		fwrite( p, 1, bytes, fp);
		from_pos += bufsize;
		to_pos += bufsize;
	}

	len = filelength( fno )-(from_pos-to_pos);
	//chsize( fno, len );
    }
    else {
	long  beg_pos, end_pos;

	beg_pos = filelength( fno );
	end_pos = filelength( fno ) + (to_pos-from_pos);
	len = end_pos;
	chsize( fno,  end_pos );

	bytes = bufsize;
	while( bytes >= bufsize ) {
		end_pos = ((end_pos - to_pos) > bufsize ) ? \
			   (end_pos-bufsize) : (to_pos);

		if( (beg_pos - from_pos) > bufsize ) {
			beg_pos = beg_pos - bufsize;
		} else {
			bytes = beg_pos - from_pos;
			beg_pos = from_pos;
		}

		fseek( fp, beg_pos, SEEK_SET );
		//bytes = min(bytes, len-beg_pos);
		bytes = fread( p, 1, bytes, fp);
		fseek( fp, end_pos, SEEK_SET );
		fwrite( p, 1, bytes, fp);
	} // end of while
    } // end of else

    free( p );

    //this is importand, or it's error in Windows
    //bytes = filelength( fno );
    fflush(fp);
    chsize( fno,  len );

    //bytes = filelength( fno );
    //return  filelength( fno );
    return  len;

} // end of function fileBytesMove()


/****************
*				batchCopy()
****************************************************************************/
#ifdef DOS_COMPATABLE
unsigned short batchCopy( void *mesDisp, char *spath, char *files, char *dpath )
{
    char temp[MAXPATH];
    char temp1[MAXPATH];
    struct 	find_t fblk;
    unsigned short count = 0;
					   
    strcpy( temp, spath );
    strcat( temp, files );
    if( !_dos_findfirst(temp, _A_ARCH, &fblk) )
    {
	do {

	    count++;

#ifdef USE_FOR_UV
	    sprintf( temp, "Copying %s\n", fblk.name );
	    if( mesDisp != NULL )
	    {
		 sendMessage( (View *)&mesDisp, evBroadcast, \
							cmChangeText, temp );
	    }
#endif
	    strcpy( temp, spath );
	    strcat( temp, fblk.name );
	    strcpy( temp1, dpath );
	    strcat( temp1, fblk.name );
	    dupfile( temp, temp1 );
	} while( !_dos_findnext(&fblk) );
    } else {
#ifdef USE_FOR_UV
	sendMessage( (View *)&mesDisp, evBroadcast, \
			 cmChangeText, "No file copied" );
#endif
    }

    return  count;

} // end of batchCopy()
#endif


/****************
*				mkdirs()
****************************************************************************/
int mkdirs(const char *path)
{
    char   *s;
    char   dir[MAXPATH];

    strcpy(dir, path);
    s = dir;
    while( (s = strchr(s, '\\')) != NULL ) {
	*s = '\0';
	mkdir(dir);
//here we cannot stop mkdir, for the next lever subdir
//	if( stat )	return  stat;
	*s = '\\';
	s++;
	if( *s == '\0' )	return  0;
    }

    return  mkdir(dir);

} // end of mkdirs()


/*
void main( void )
{
	int i;                self->dRuningText
	i = nFileNewcmp( "\\tg\\data\\sub0.ndx", "\\tg\\data\\sub0.dbf" );

	printf("%d\n", i);
}
*/


/*********************** end of module filetool.c **************************/
