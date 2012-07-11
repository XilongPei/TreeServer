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
 *
 *  Title:  value.h
 *  Sccsid: @(#)value.h 9.1.1.2 6/29/92  12:25:23
 *  Description:
 *      value header include file; multi-purpose value struct
 *
 ***************************************************************************
 */

#pragma pack(push, 4)

#include "decimal.h"
#include "locator.h"
#include "blob.h"

/* !null && blob type */
#define ISBLOBVALUE(val)    (val.v_ind != -1 && ISBLOBTYPE(val.v_type))

#define MAXADDR     14

typedef struct value
	{
	short v_type;       /* SQL data type        */
	short v_ind;        /* null indicator       */
	short v_prec;       /* decimal precision        */
	union           /* data value           */
	{           /*  depending on v_type     */
	struct
		{           /* SQLCHAR          */
		char far *vcp;      /* string start         */
		short vidx;     
		short vlen;     /* string length        */
		short vflgs;    /* flags - see below        */
		short vsstart;  /* substring start for 4GL-RDS  */
		short vsend;    /* substring end for 4GL-RDS    */
		} vchar;
	int vint;       /* SQLSMINT         */
	long vlng;      /* SQLINT           */
	float vflo;     /* SQLSMFLOAT           */
	double vdub;        /* SQLFLOAT         */
	dec_t vdec;     /* SQLDECIMAL           */
	short vaddr[MAXADDR];   /* 4GL address modifiers    */
	tblob_t vtblob;     /* BLOB as stored in tuple  */
	loc_t far *vlocator;    /* blobs locator        */
	} v_val;
	} value_t;

#define CASTVALP    (struct value far *)

/*
 * defines to make the union transparent
 */
#define v_charp     v_val.vchar.vcp
#define v_index     v_val.vchar.vidx
#define v_len       v_val.vchar.vlen
#define v_flags     v_val.vchar.vflgs
#define v_sstart    v_val.vchar.vsstart
#define v_send      v_val.vchar.vsend
#define v_int       v_val.vint
#define v_long      v_val.vlng
#define v_float     v_val.vflo
#define v_double    v_val.vdub
#define v_decimal   v_val.vdec
#define v_idesc     v_ind
#define v_naddr     v_prec
#define v_addr      v_val.vaddr

#define v_tblob     v_val.vtblob
#define v_blocator  v_val.vlocator
/*
 * flags for v_flags
 * used by the 4GL Debugger and Pcode Run Time
 */
#define V_BREAK     01      /* break when variable is updated */ 
#define V_SUBSTR    02      /* char value is a substring */
#define V_QUOTED    04      /* char value is from quoted string */
#define V_ASCII0    010     /* ascii 0 value */

#define FRCBOOL(x)  if (x->v_ind >= 0)\
				switch (x->v_type) \
				{\
				case SQLSMINT: break;\
				case SQLDATE:\
				case SQLSERIAL:\
				case SQLINT: if (x->v_long != 0)\
						x->v_int = 1;\
						 else\
						x->v_int = 0;\
						 x->v_type = SQLSMINT;\
						 x->v_prec = PRECMAKE(5, 0);\
						 break;\
					default:     cvtosmint(x);\
						 break;\
				} \
			else \
				{ \
				x->v_int = 0; \
				x->v_type = SQLSMINT; \
				x->v_prec = PRECMAKE(5, 0); \
				}

#pragma pack(pop, 4)

