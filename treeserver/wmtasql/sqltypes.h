/***************************************************************************
 *
 *			   INFORMIX SOFTWARE, INC.
 *
 *			      PROPRIETARY DATA
 *
 *	THIS DOCUMENT CONTAINS TRADE SECRET DATA WHICH IS THE PROPERTY OF 
 *	INFORMIX SOFTWARE, INC.  THIS DOCUMENT IS SUBMITTED TO RECIPIENT IN
 *	CONFIDENCE.  INFORMATION CONTAINED HEREIN MAY NOT BE USED, COPIED OR 
 *	DISCLOSED IN WHOLE OR IN PART EXCEPT AS PERMITTED BY WRITTEN AGREEMENT 
 *	SIGNED BY AN OFFICER OF INFORMIX SOFTWARE, INC.
 *
 *	THIS MATERIAL IS ALSO COPYRIGHTED AS AN UNPUBLISHED WORK UNDER
 *	SECTIONS 104 AND 408 OF TITLE 17 OF THE UNITED STATES CODE. 
 *	UNAUTHORIZED USE, COPYING OR OTHER REPRODUCTION IS PROHIBITED BY LAW.
 *
 *
 *  Title:	sqltypes.h
 *  Sccsid:	@(#)sqltypes.h	9.1.1.1	1/11/92  15:56:48
 *  Description:
 *		type definition
 *
 ***************************************************************************
 */

#pragma pack(push, 4)

#ifndef CCHARTYPE

/***********************
 * ++++ CAUTION ++++
 * Any new type to be added to the following lists should not
 * have the following bit pattern (binary short):
 *
 *	xxxxx111xxxxxxxx
 *
 * where x can be either 0 or 1.
 *
 * This is due to the use of the bits as SQLNONULL, SQLHOST and SQLNETFLT
 * (see below).
 *
 * FAILURE TO DO SO WOULD RESULT IN POSSIBLE ERRORS DURING CONVERSIONS.
 *
 ***********************/

 /* C language types */

#define CCHARTYPE	100
#define CSHORTTYPE	101
#define CINTTYPE	102
#define CLONGTYPE	103
#define CFLOATTYPE	104
#define CDOUBLETYPE	105
#define CDECIMALTYPE	107
#define CFIXCHARTYPE	108
#define CSTRINGTYPE	109
#define CDATETYPE	110
#define CMONEYTYPE	111
#define CDTIMETYPE	112
#define CLOCATORTYPE    113
#define CVCHARTYPE	114
#define CINVTYPE	115
#define CFILETYPE	116


#define USERCOLL(x)	((x))


/*
 * Define all possible database types
 *   include C-ISAM types here as well as in isam.h
 */

#define SQLCHAR		0
#define SQLSMINT	1
#define SQLINT		2
#define SQLFLOAT	3
#define SQLSMFLOAT	4
#define SQLDECIMAL	5
#define SQLSERIAL	6
#define SQLDATE		7
#define SQLMONEY	8
#define SQLNULL		9
#define SQLDTIME	10
#define SQLBYTES	11
#define SQLTEXT		12
#define SQLVCHAR	13
#define SQLINTERVAL	14
#define SQLTYPE		0xF	/* type mask		*/
#define SQLNONULL	0x100	/* disallow nulls	*/
#define SQLMAXTYPES	15

/* this is not a real type but a flag to show that the
 * value is from a host variable
 */
#define SQLHOST		01000
#define SQLNETFLT	02000	/* float-to-decimal for networked backend */

#define SIZCHAR		1
#define SIZSMINT	2
#define SIZINT		4
#define SIZFLOAT	(sizeof(double))
#define SIZSMFLOAT	(sizeof(float))
#define SIZDECIMAL	17	/* decimal(32) */
#define SIZSERIAL	4
#define SIZDATE		4
#define SIZMONEY	17	/* decimal(32) */
#define SIZDTIME	7	/* decimal(12,0) */
#define SIZVCHAR	1

#define MASKNONULL(t)	((t) & ~(SQLNONULL|SQLHOST|SQLNETFLT))
#define ISSQLTYPE(t)	(MASKNONULL(t) >= SQLCHAR && MASKNONULL(t) < SQLMAXTYPES)

/*
 * SQL types macros
 */
#define ISDECTYPE(t)		(MASKNONULL(t) == SQLDECIMAL || \
				 MASKNONULL(t) == SQLMONEY || \
				 MASKNONULL(t) == SQLDTIME || \
				 MASKNONULL(t) == SQLINTERVAL)

#define ISBLOBTYPE(type)	(ISBYTESTYPE (type) || ISTEXTTYPE(type))
#define ISBYTESTYPE(type)	(MASKNONULL(type) == SQLBYTES)
#define ISTEXTTYPE(type)	(MASKNONULL(type) == SQLTEXT)
#define ISVCTYPE(t)		(MASKNONULL(t) == SQLVCHAR)

/*
 * C types macros
 */
#define ISBLOBCTYPE(type)	(ISLOCTYPE(type) || ISFILETYPE(type)) 
#define ISLOCTYPE(type)		(MASKNONULL(type) == CLOCATORTYPE) 
#define ISFILETYPE(type)	(MASKNONULL(type) == CFILETYPE) 

#define ISOPTICALCOL(type)	(type == 'O')

#define DEFDECIMAL	9	/* default decimal(16) size */
#define DEFMONEY	9	/* default decimal(16) size */

#define SYSPUBLIC	"public"


#endif /* CCHARTYPE */

#pragma pack(pop, 4)

