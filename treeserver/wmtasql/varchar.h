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
 *  Title:	varchar.h
 *  Sccsid:	@(#)varchar.h	9.1.1.1	1/11/92  15:57:30
 *  Description:
 *		header file for varying length character data type
 *
 ***************************************************************************
 */

#pragma pack(push, 4)

/*
 * VARCHAR macros
 */

#define MAXVCLEN		(255)
#define VCLENGTH(len)		(VCMAX(len)+1)
#define VCMIN(size)		(((size) >> 8) & 0x00ff)
#define VCMAX(size)		((size) & 0x00ff)
#define VCSIZ(max, min)		((((min) << 8) & 0xff00) + ((max) & 0x00ff))

#pragma pack(pop, 4)

