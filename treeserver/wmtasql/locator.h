/***************************************************************************
 *
 *             INFORMIX SOFTWARE, INC.
 *
 *                PROPRIETARY DATA
 *
 *  THIS DOCUMENT CONTAINS TRADE SECRET DATA WHICH IS THE PROPERTY OF 
 *  INFORMIX SOFTWARE, INC.  THIS DOCUMENT IS SUBMITTED TO RECIPIENT IN
 *  CONFIDENCE.  INFORMATION CONTAINED HEREIN MAY NOT BE USED, COPIED OR 
 *  DISCLOSED IN WHOLE OR IN PART EXCEPT AS PERMITTED BY WRITTEN AGREEMENT 
 *  SIGNED BY AN OFFICER OF INFORMIX SOFTWARE, INC.
 *
 *  THIS MATERIAL IS ALSO COPYRIGHTED AS AN UNPUBLISHED WORK UNDER
 *  SECTIONS 104 AND 408 OF TITLE 17 OF THE UNITED STATES CODE. 
 *  UNAUTHORIZED USE, COPYING OR OTHER REPRODUCTION IS PROHIBITED BY LAW.
 *
 *  Title:  locator.h
 *  Sccsid: @(#)locator.h   9.1.1.1 1/11/92  15:52:49
 *
 *  Description:
 *              'locator.h' defines 'loc_t' the locator struct.
 *
 ***************************************************************************
 */

#pragma pack(push, 4)

#ifndef LOCATOR_INCL        /* avoid multiple includes */
#define LOCATOR_INCL

/* 
Locators are used to store TEXT or BYTE fields (blobs) in ESQL
programs.  The "loc_t" structure is described below.  Fields denoted
USER should be set by the user program and will be examined by the DBMS
system.  Those denoted SYSTEM are set by the system and may be examined
by the user program.  Those denoted INTERNAL contain data only the
system manupilates and examines.

If "loc_loctype" is set to LOCMEMORY, then the blob is stored in
primary memory.  The memory buffer is pointed to by the variant
"loc_buffer".  The field "loc_bufsize" gives the size of "loc_buffer".
If the "loc_bufsize" is set to "-1" and "loc_mflags" is set to "0"
and the locator is used for a fetch, memory is obtained using "malloc"
and "loc_buffer" and "loc_bufsize" are set.

If "loc_loctype" is set to LOCFILE, then the blob is stored in a file.
The file descriptor of an open operating system file is specified in
"loc_fd".

If "loc_loctype" is set to LOCFNAME, the the blob is stored in a file
and the name of the file is given.  The DBMS will open or created the
file at the correct time and in the correct mode.

If the "loc_loctype" is set to LOCUSER, "loc_(open/close/read/write)"
are called.  If the blob is an input to a SQL statement, "loc_open" is
called with the parameter "LOC_RONLY".  If the blob is an output target
for an SQL statement, "loc_open" is called with the parameter
"LOC_WONLY".

"loc_size" specifies the maximum number of bytes to use when the
locator is an input target to an SQL statement. It specifies the number
of bytes returned if the locator is an output target.  If "loc_loctype"
is LOCFILE or LOCUSER, it can be set to -1 to indicate transfer until
end-of-file.

"loc_indicator" is set by the user to -1 to indicate a NULL blob.  It
will be  set to -1 if a NULL blob is retrieved.  If the blob to be
retrieved will not fit in the space provided, the indicator contains
the size of the blob.

"loc_status" is the status return of locator operations.

"loc_type" is the "blob" type (SQLTEXT, SQLBYTES, ...).

"loc_user_env" is a pointer for the user's private use. It is neither
set nor examined by the system.  "loc_user_env" as well as the
"loc_union" fieds may be used by user supplied routines to store and
communicate information.
*/

typedef struct 
	{
	short loc_loctype;      /* USER: type of locator - see below    */
	union           /* variant on 'loc'                     */
	{
	struct          /* case LOCMEMORY                       */
		{
		long  lc_bufsize;   /* USER: buffer size */
		char far *lc_buffer;    /* USER: memory buffer to use       */
		char far *lc_currdata_p;/* INTERNAL: current memory buffer  */
		int   lc_mflags;    /* USER/INTERNAL: memory flags      */
				/*          (see below) */
		} lc_mem;

	struct          /* cases L0CFNAME & LOCFILE     */
		{
		char far *lc_fname; /* USER: file name          */
		int   lc_mode;  /* USER: perm. bits used if creating    */
		int   lc_fd;    /* USER: os file descriptior        */
		long  lc_position;  /* INTERNAL: seek position      */
		} lc_file;
	} lc_union;

	long  loc_indicator;    /* USER SYSTEM: indicator       */
	long  loc_type;     /* SYSTEM: type of blob         */
	long  loc_size;     /* USER SYSTEM: num bytes in blob or -1 */
	int   loc_status;       /* SYSTEM: status return of locator ops */
	char far *loc_user_env;     /* USER: for the user's PRIVATE use */
	long  loc_xfercount;    /* INTERNAL/SYSTEM: Transfer count  */

	int (*loc_open)();      /* USER: open function          */
	int (*loc_close)();     /* USER: close function         */
	int (*loc_read)();      /* USER: read function          */
	int (*loc_write)();     /* USER: write function         */

	int   loc_oflags;       /* USER/INTERNAL: see flag definitions below */
	} loc_t;

#define loc_fname   lc_union.lc_file.lc_fname
#define loc_fd      lc_union.lc_file.lc_fd
#define loc_position    lc_union.lc_file.lc_position    
#define loc_bufsize lc_union.lc_mem.lc_bufsize
#define loc_buffer  lc_union.lc_mem.lc_buffer
#define loc_currdata_p  lc_union.lc_mem.lc_currdata_p
#define loc_mflags  lc_union.lc_mem.lc_mflags

/* Enumeration literals for loc_loctype */

#define LOCMEMORY   1       /* memory storage */
#define LOCFNAME    2       /* File storage with file name */
#define LOCFILE     3       /* File storage with fd */
#define LOCUSER     4       /* User define functions */

/* passed to loc_open and stored in loc_oflags */
#define LOC_RONLY   0x1     /* read only */
#define LOC_WONLY   0x2     /* write only */

/* LOC_APPEND can be set when the locator is created
 * if the file is to be appended to instead of created
 */
#define LOC_APPEND  0x4     /* write with append */
#define LOC_TEMPFILE    0x8     /* 4GL tempfile blob */

/* LOC_USEALL can be set to force the maximum size of the blob to always be 
 * used when the blob is an input source.  This is the same as setting the 
 * loc_size field to -1.  Good for LOCFILE or LOCFNAME blobs only.
 */
#define LOC_USEALL  0x10        /* ignore loc_size field */
#define LOC_DESCRIPTOR  0x20        /* BLOB is optical descriptor */


/* passed to loc_open and stored in loc_mflags */
#define LOC_ALLOC   0x1     /* free and alloc memory */

#endif  /* LOCATOR_INCL */

#pragma pack(pop, 4)

