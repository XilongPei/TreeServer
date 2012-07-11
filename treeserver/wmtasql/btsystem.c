/*
 *  system.c    This file contains all functions that are specific
 *              to certain operating systems, compilers or hardware
 *              None of the functions do anything really tricky; all
 *              MUST be made to work correctly as described or weirdnesses
 *              will almost certainly result! This works out of the box for
 *              most compilers.
 *
 *              Currently the following functions live here:
 *
 *              System Functions
 *                  exist()     check for a given files' existence
 *                  rename()    rename an existing file
 *                  unlink()    erase an existing file
 *                * memcpy()    copy memory from place to place
 *                * memset()    set memory to a given value
 *                * ltoc()      convert a long to a three byte char string
 *                * ctol()      convert a three byte char to a long
 *                  rmsign()    convert a (possibly) signed char to an short
 *
 *              Significant performance gains can be realized by wrenching
 *              the *'d functions. See the notes in the comments surrounding
 *              them.
 *
 *              Jon Simkins, 85.03.06
 *              
 */
#include <stdio.h>
#include <io.h>

#include "ctsystem.h"
#include "ctree.h"

/*
 *  If your compiler supports these functions, leave the defines set. If
 *  your compiler does not support the named functions, comment them out.
 *  (commenting them out negates the statement 'HAS_Whatever' and
 *  includes code here to supply that function.)
 *
 *  BTree makes heavy use of memset(), memcpy(), ctol(), and ltoc(). In
 *  quick test here, driver with TIMES set to 300 ran for 6.75 minutes using
 *  the portable versions of these functions and only 5.75 minutes using
 *  the optimized versions.
 *
 *  rename() and unlink() are not needed by the BTree Library (although
 *  the ISAM Driver needs them). They cannot be implemented in a
 *  portable fashion; fortunately, all compilers we've seen support them.
 *
 */

#define HAS_RENAME 
#define HAS_UNLINK
#define HAS_MEMCPY
#define HAS_MEMSET
/* #define FAST_LTOC  */

#ifdef MSC
#define HAS_MEMCPY
#define HAS_MEMSET
#define FAST_LTOC
#endif

/*
 *  exist   check to see if a file by a given name (path) exists.
 *          This call must return TRUE if the file exists and FALSE if it
 *          doesn't. It must not create a file nor must it alter an existing
 *          file.
 *          The version given SHOULD be portable; your compiler may
 *          provide a more effecient method of checking (such as
 *          access() in UNIX)
 */
short exist(char *fName)
{
    short fd;
    if ((fd = open(fName, 0)) < 0) return FALSE;
        close(fd);
    return TRUE;
}

#ifndef HAS_RENAME
/*
 *  rename      rename <oldfile> to <newfile>. Both files are assumed to
 *              be closed (inactive) at the time rename is called.
 *              This is only used by the ISAM driver at this time.
 *              There is no 'standard' way to write this; fortunately
 *              most compilers support it!
 */
short rename(char *oldfile, char *newfile)
{
    /* very compiler dependant code goes here! */
}
#endif

#ifndef HAS_UNLINK
/*
 *  unlink      erase the given file. It is assumed to be closed
 *              (inactive).
 *              This is commonly supported.
 */
short unlink(char *fname)
{
    /* highly compiler dependant code goes here! */
}
#endif

#ifndef HAS_MEMCPY
/*
 *  memcpy      copy <num> bytes from <source> to <dest>. A pointer
 *              to the destination string is returned.
 *              Most compilers support some form of this.
 *              If yours does not, the code given below is very
 *              portable. Generally, compiler writers support some
 *              assembler version just to speed things up.
 *              
 *              The faster the better!
 *
 */
char *memcpy(char *dest, char *source, short num)
{
    register j;
    if (source < dest)
        for(j = num-1; j >= 0; j--)
            dest[j] = source[j];
    else
        for(j = 0; j < num; j++)
            dest[j] = source[j];
   return dest;
}
#endif

#ifndef HAS_MEMSET
/*
 *  memset      set <num> bytes beginning at <addr> to <c>.
 *              This is commonly supported.
 *
 *              The faster, the better!
 */
char *memset(char *ptr, short c, short num)
{
   register j;

   for(j = 0; j < num; j++)
      ptr[j] = c;
   return ptr;
}
#endif

/*
 *
 *  ltoc() and ctol(): Convert a long to a string and back.
 *
 *  The following functions should be coded in the speediest manner possible
 *  taking advantage of your particular hardware/compiler. The first version
 *  will work in all situations, but it can be worth the ten minutes or so that
 *  it takes to adapt the direct byte copy method.
 *
 *  Try #defining FAST_LTOC and running the little program:
 *
 *  main()
 *  {
 *     long j;
 *     extern long ctol();
 *     char buff[3];
 *     for(j=START; j<END; j++)
 *       ltoc(j, buff);
 *       if (ctol(buff) != j) {
 *         puts("Routines aren't working!\n");
 *         exit(0);
 *       }
 *     }
 *  }
 *
 *  with START and END values of 250 to 260 and 65530 to 65560 or so.
 */

#ifndef FAST_LTOC
/*
 * ltoc  converts a long integer (less than 16,581,375) into a
 *       three byte character pointed to by *buff.
 */
void ltoc(long j, char *buff)
{
    buff[0] = ( j >> 16 ) & 0xff;
    buff[1] = ( j >> 8 ) & 0xff;
    buff[2] = j & 0xff;
}

/*
 *   ctol   the complementary of the above function; convert a three
 *        byte value back into a long.
 */
long ctol(char *buff)
{
   long k;
#ifdef SIGN_EXT
   k = (long) rmsign(buff[0]) << 16;
   k += (long) rmsign(buff[1]) << 8;
   k += (long) rmsign(buff[2]);
#else
   k = (long) buff[0] << 16;
   k += (long) buff[1] << 8;
   k += (long) buff[2];
#endif
   return k;
}

#else
/*
 *  faster, non-portable versions of ltoc() and ctol(). Should work with
 *  all MS-DOS compilers and can easily be made to work with almost any
 *  compiler/hardware setup.
 */
void
ltoc(j, buff)
long j;
char *buff;
{
    memcpy(buff, &j, 3);
}

long
ctol(buff)
char *buff;
{
    long j = 0;
    memcpy(&j, buff, 3);
    return j;
}
#endif

/*
 *  rmsign  ensure that char values are positive.
 *          This routine is portable.
 *          It's only in this module because either or both of BTree and
 *          vlen might need it.
 */
short rmsign(short c)
{
    return (c+256) % 256;
}

