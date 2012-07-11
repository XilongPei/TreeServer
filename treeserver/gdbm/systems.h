/* systems.h - Most of the system dependant code and defines are here. */

/*  This file is part of GDBM, the GNU data base manager, by Philip A. Nelson.
    Copyright (C) 1990, 1991, 1993  Free Software Foundation, Inc.

    GDBM is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2, or (at your option)
    any later version.

    GDBM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with GDBM; see the file COPYING.  If not, write to
    the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

    You may contact the author by:
       e-mail:  phil@wwu.edu
      us-mail:  Philip A. Nelson
                Computer Science Department
                Western Washington University
                Bellingham, WA 98226

    Revision:
        Port to WIN32 platform          Jingyu Niu, 1999.11

*************************************************************************/

#ifdef __GNUC__
#define alloca  __builtin_alloca
#else /* not __GNUC__ */
#ifdef _MSC_VER
#include <malloc.h>
#else /* not _MSC_VER */
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif  /* not HAVE_ALLOCA_H */
#endif  /* _MSC_VER */
#endif  /* not __GNUC__ */

/* Include all system headers first. */
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <stdio.h>
#if HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#include <sys/stat.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef _WIN32
#include <io.h>
#endif

#ifndef SEEK_SET
#define SEEK_SET        0
#endif

#ifndef L_SET
#define L_SET SEEK_SET
#endif

/* Do we have flock?  (BSD...) */

#if HAVE_FLOCK

#ifndef LOCK_SH
#define LOCK_SH 1
#endif

#ifndef LOCK_EX
#define LOCK_EX 2
#endif

#ifndef LOCK_NB
#define LOCK_NB 4
#endif

#ifndef LOCK_UN
#define LOCK_UN 8
#endif

#define UNLOCK_FILE(dbf) flock (dbf->desc, LOCK_UN)
#define READLOCK_FILE(dbf) lock_val = flock (dbf->desc, LOCK_SH + LOCK_NB)
#define WRITELOCK_FILE(dbf) lock_val = flock (dbf->desc, LOCK_EX + LOCK_NB)

#else

#ifdef _WIN32

#define link(f1,f2)     (!CopyFileA(f1,f2,0))

#include <sys\locking.h>

#define UNLOCK_FILE(dbf) \
    {                   \
        long offset;    \
        offset = tell( dbf->desc ); \
        lseek( dbf->desc, 0, SEEK_SET );    \
        locking (dbf->desc, LK_UNLCK, 1 );  \
        lseek( dbf->desc, offset, SEEK_SET );   \
    }
#define READLOCK_FILE(dbf)  \
    {                   \
        long offset;    \
        offset = tell( dbf->desc ); \
        lseek( dbf->desc, 0, SEEK_SET );    \
        lock_val = locking (dbf->desc, LK_NBLCK, 1 );   \
        lseek( dbf->desc, offset, SEEK_SET );   \
    }
#define WRITELOCK_FILE(dbf)     \
    {                   \
        long offset;    \
        offset = tell( dbf->desc ); \
        lseek( dbf->desc, 0, SEEK_SET );    \
        lock_val = locking (dbf->desc, LK_NBLCK, 1 );   \
        lseek( dbf->desc, offset, SEEK_SET );   \
    }

#else /* not _WIN32, on Unix like systems */
/* Assume it is done like System V. */

#define UNLOCK_FILE(dbf) \
    {                   \
      struct flock flock;           \
      flock.l_type = F_UNLCK;       \
      flock.l_whence = SEEK_SET;        \
      flock.l_start = flock.l_len = 0L; \
      fcntl (dbf->desc, F_SETLK, &flock);   \
    }
#define READLOCK_FILE(dbf) \
    {                   \
      struct flock flock;           \
      flock.l_type = F_RDLCK;       \
      flock.l_whence = SEEK_SET;            \
      flock.l_start = flock.l_len = 0L; \
      lock_val = fcntl (dbf->desc, F_SETLK, &flock);    \
    }
#define WRITELOCK_FILE(dbf) \
    {                   \
      struct flock flock;           \
      flock.l_type = F_WRLCK;       \
      flock.l_whence = SEEK_SET;            \
      flock.l_start = flock.l_len = 0L; \
      lock_val = fcntl (dbf->desc, F_SETLK, &flock);    \
    }
#endif /* _WIN32 */
#endif

/* Do we have bcopy?  */
#if !HAVE_BCOPY
#if HAVE_MEMORY_H
#include <memory.h>
#endif
#define bcmp(d1, d2, n) memcmp(d1, d2, n)
#define bcopy(d1, d2, n) memcpy(d2, d1, n)
#endif

/* Do we have fsync? */
#if !HAVE_FSYNC
#ifdef _WIN32
#define fsync(f) _commit(f)
#else /* other systems */
#define fsync(f) {sync(); sync();}
#endif /* _WIN32 */
#endif

/* Default block size.  Some systems do not have blocksize in their
   stat record. This code uses the BSD blocksize from stat. */

#if HAVE_ST_BLKSIZE
#define STATBLKSIZE file_stat.st_blksize
#else
#define STATBLKSIZE 1024
#endif

/* Do we have ftruncate? */
#if HAVE_FTRUNCATE
#define TRUNCATE(dbf) ftruncate (dbf->desc, 0)
#else
#define TRUNCATE(dbf) close( open (dbf->name, O_RDWR|O_TRUNC, mode));
#endif

/* Do we have 32bit or 64bit longs? */
#if LONG_64_BITS || !INT_16_BITS
typedef int word_t;
#else
typedef long word_t;
#endif

