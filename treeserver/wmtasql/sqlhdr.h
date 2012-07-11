/***************************************************************************
 *
 *                         INFORMIX SOFTWARE, INC.
 *
 *                            PROPRIETARY DATA
 *
 *      THIS DOCUMENT CONTAINS TRADE SECRET DATA WHICH IS THE PROPERTY OF 
 *      INFORMIX SOFTWARE, INC.  THIS DOCUMENT IS SUBMITTED TO RECIPIENT IN
 *      CONFIDENCE.  INFORMATION CONTAINED HEREIN MAY NOT BE USED, COPIED OR 
 *      DISCLOSED IN WHOLE OR IN PART EXCEPT AS PERMITTED BY WRITTEN AGREEMENT 
 *      SIGNED BY AN OFFICER OF INFORMIX SOFTWARE, INC.
 *
 *      THIS MATERIAL IS ALSO COPYRIGHTED AS AN UNPUBLISHED WORK UNDER
 *      SECTIONS 104 AND 408 OF TITLE 17 OF THE UNITED STATES CODE. 
 *      UNAUTHORIZED USE, COPYING OR OTHER REPRODUCTION IS PROHIBITED BY LAW.
 *
 *
 *  Title:      sqlhdr.h
 *  Sccsid:     @(#)sqlhdr.h    8.6     10/10/91        17:37:50
 *  Description:
 *              header file for all embedded sql programs
 *
 ***************************************************************************
 */

#pragma pack(push, 4)

#ifndef _SQLHDR
#define _SQLHDR

#include "sqlda.h"
 
typedef struct _squlist
	{
	char far * far *_SQUulist;                   /* columns to be updated */
	struct _squlist far *_SQUnext;          /* next update list */
	int _SQUid;                         /* update list id */
	} _SQULIST;

typedef struct 
	{
	short _SQCstmttype;                 /* SQ_SELECT, SQ_INSERT, etc. */
	long _SQCsqlid;                     /* SQL's id for this cursor */
	short _SQCflags;                    /* CROPEN, CREOF, CRSINGLE ,CRPREPARE*/
	short _SQCnfields;                  /* number of result fields
										 * (number supplied by SQL)
										 */
	short _SQCnibind;                   /* number of input args */
	short _SQCnobind;                   /* number of output args */
#ifdef MSWIN3
	long _SQCtcount;                    /* tuples remaining in buffer */
#else
	short _SQCtcount;                   /* tuples remaining in buffer */
#endif
	short _SQCtsize;                    /* length of data expected from
										 * SQL
										 */
	short _SQCtbsize;                       /* tuple buffer size */
	struct sqlvar_struct far *_SQCibind;    /* pointer to first in array of 
											 * binding structures for arguments
											 * to be taken from the user
											 * program and sent to SQL;
											 */
	struct sqlvar_struct far *_SQCobind;    /* pointer to first in array of
											 * binding structures for values
											 * to be received from SQL and
											 * supplied to the user program;
											 */
	char far * far *_SQCcommand;                 /* pointer to ptrs to pieces of
											 * the command 
											 */
	struct sqlvar_struct far *_SQCfields;   /* pointer to first in array of
											 * structures describing the data
											 * to be received from SQL;
											 * (fields described by SQL)
											 */
	char far *_SQCstrtab;                   /* pointer to table of strings - the
											 * names of the attributes to be
											 * received from SQL
											 * (table supplied by SQL)
											 */
	char far *_SQCtbuf;                     /* tuple buffer */
	char far *_SQCtuple;                    /* pointer to current tuple within
											 * buffer
											 */
	char far *_SQCname;                     /* cursor name */
	struct sqlda  far *_SQCsqlda;           /* pointer to sqlda */

		/* used for scroll cursors */
	long _SQCfirst;                     /* rowid of 1st tuple in buffer */
	long _SQClast;                      /* rowid of last tuple in buffer */


	/* used by POWER CURSORS */
	char far *_SQCtable;                    /* table name */
	char far * far *_SQCselect_list;             /* selection/insert list */
	char far * far *_SQCorderby_list;            /* order by list */
	_SQULIST _SQCulist;                     /* the first power update list */
	/* end of used by POWER CURSORS */

	} _SQCURSOR;


typedef struct 
	{
	short _SQSstmttype;                 /* SQ_SELECT, SQ_INSERT, etc. */
	long _SQSsqlid;                     /* SQL's id for this cursor */
	} _SQSTMT;

struct hostvar_struct {
	char far *hostaddr;                     /* address of host variable */
	short fieldtype;                    /* field entry requested by GET */
	short hosttype;                     /* host type */
	short hostlen;                      /* length of field type */
	short qualifier;                    /* qualifier for DATETIME/INTERVAL */
	};

/*
 * SQL field type codes
 */
#define XSQLD           0
#define XSQLTYPE        1
#define XSQLLEN         2
#define XSQLPRECISION   3
#define XSQLNULLABLE    4
#define XSQLIND         5
#define XSQLDATA        6
#define XSQLNAME        7
#define XSQLSCALE       8
#define XSQLILEN        9
#define XSQLITYPE       10
#define XSQLIDATA       11

/*
 * Specifications for FETCH
 */
typedef struct
	{
	long fval;                  /* scroll quantity */
	int fdir;                   /* direction of FETCH (NEXT, PREVIOUS..) */
	int findchk;                /* check for indicator? */
	} _FetchSpec;

/*
 * Modes used during cursor searching (dynamic cursor name)
 */
#define LC_CREATE       0       /* create cursor if does not exist */
#define LC_SEARCH       1       /* search only */
/*
 * Types stored in csblock_t (iqutil.c)
 */
#define IQ_CURSOR       0
#define IQ_STMT         1
#define IQ_ALL          2

/* defines for SqlFreeMem, FreeType */
#define CURSOR_FREE 1
#define STRING_FREE 2
#define SQLDA_FREE  3
#define CONN_FREE   4

#endif  /* _SQLHDR */

#pragma pack(pop, 4)

