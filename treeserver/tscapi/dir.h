/***
*	dir.h - declarations for const values.
*
*   Copy by NiuJingyu 1997/07/28
*
*	define WIN32 to use long file name system.
*
****/

#if !defined(__DIR_H)
#define __DIR_H

#include <direct.h>

#ifdef WIN32

#if !defined(_M_MPPC) && !defined(_M_M68K)
#define MAXPATH		260 /* max. length of full pathname */
#define MAXDRIVE	3   /* max. length of drive component */
#define MAXDIR		256 /* max. length of path component */
#define MAXFILE		256 /* max. length of file name component */
#define MAXEXT		256 /* max. length of extension component */
#else   /* defined(_M_M68K) || defined(_M_MPPC) */
#define MAXPATH  	256 /* max. length of full pathname */
#define MAXDIR   	32  /* max. length of path component */
#define MAXFILE		64  /* max. length of file name component */
#endif  /* defined(_M_M68K) || defined(_M_MPPC) */

#else  /* !defined(WIN32) */

#define MAXPATH   80
#define MAXDRIVE  3
#define MAXDIR    66
#define MAXFILE   9
#define MAXEXT    5

#endif  /* ifdef WIN32 */

#endif // __DIR_H